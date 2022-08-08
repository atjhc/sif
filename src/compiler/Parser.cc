//
//  Copyright (c) 2021 James Callender
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "compiler/Parser.h"
#include "Error.h"
#include "Utilities.h"
#include "ast/Repeat.h"

#include <vector>

SIF_NAMESPACE_BEGIN

#if defined(DEBUG)
#define trace(msg) _trace(msg)
#else
#define trace(msg)
#endif

static inline std::string Describe(const Token &token) { return token.description(); }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Describe(token);
}
#pragma clang diagnostic pop

Parser::Parser(const ParserConfig &config, Scanner &scanner) : _config(config), _scanner(scanner) {
    _parsingRepeat = false;
    _recording = false;
    _index = 0;
    _depth = 0;
}

Owned<Statement> Parser::parse() {
    auto result = parseBlock();
    if (_errors.size() > 0) {
        return nullptr;
    }
    return result;
}

void Parser::declare(const Signature &signature) { _signatureDecls.push_back({signature, _depth}); }

const std::vector<SyntaxError> &Parser::errors() { return _errors; }

#pragma mark - Utilities

Optional<Token> Parser::match(const std::initializer_list<Token::Type> &types) {
    if (check(types)) {
        return advance();
    }
    return None;
}

Optional<Token> Parser::matchWord() {
    if (peek().isWord()) {
        return advance();
    }
    return None;
}

Token Parser::consumeWord(const std::string &message) {
    if (peek().isWord()) {
        return advance();
    }
    throw SyntaxError(peek(), message);
}

Token Parser::consume(Token::Type type, const std::string &errorMessage) {
    if (check({type}))
        return advance();
    throw SyntaxError(peek(), errorMessage);
}

Token Parser::consumeNewLine() {
    if (isAtEnd())
        return peek();
    if (check({Token::Type::NewLine}))
        return advance();
    throw SyntaxError(peek(), "expected new line or end of script");
}

Token Parser::consumeEnd(Token::Type type) {
    auto endToken = consume(Token::Type::End, Concat("expected ", Quoted("end")));
    if (auto matchToken = match({type})) {
        return matchToken.value();
    }
    return endToken;
}

bool Parser::check(const std::initializer_list<Token::Type> &types) {
    if (isAtEnd()) {
        return false;
    }
    for (auto type : types) {
        if (peek().type == type) {
            return true;
        }
    }
    return false;
}

bool Parser::isAtEnd() { return peek().type == Token::Type::EndOfFile; }

Token Parser::scan() {
    auto token = _scanner.scan();
    _tokens.push_back(token);
    trace(Concat("Scanned ", Describe(token)));
    trace(Concat("Tokens [", Join(_tokens, ", "), "] ", _index));
    return _tokens.back();
}

Token Parser::advance() {
    if (!isAtEnd()) {
        if (!_recording && _tokens.size() > 1) {
            _tokens.erase(_tokens.begin());
        }
        if (_recording || _index == 0) {
            _index++;
        }
        if (_index == _tokens.size()) {
            scan();
        }
    }
    return previous();
}

Token Parser::peek() {
    if (_tokens.size() == 0) {
        return scan();
    }
    return _tokens[_index];
}

Token Parser::previous() {
    if (_tokens.size() > 1) {
        return _tokens[_index - 1];
    }
    throw std::runtime_error("scanner underflow");
}

void Parser::synchronize() {
    trace("Synchronizing");
    _recording = false;
    auto token = advance();
    while (!isAtEnd()) {
        if (token.type == Token::Type::NewLine) {
            return;
        }
        token = advance();
    }
}

void Parser::checkpoint() {
    _recording = true;
    _saved.push(_index);
    trace(Concat("Checkpoint (", _index, ")"));
}

void Parser::rewind() {
    _index = _saved.top();
    _saved.pop();
    if (_saved.size() == 0) {
        _recording = false;
    }
    trace(Concat("Rewind (", _index, ")"));
}

void Parser::commit() {
    _saved.pop();
    if (_saved.size() == 0) {
        _recording = false;
        _tokens.erase(_tokens.begin(), _tokens.begin() + _index - 1);
        _index = 1;
    }
    trace(Concat("Commit (", previous().description(), ", ", peek().description(), ")"));
}

void Parser::beginScope() { _depth++; }

void Parser::endScope() {
    _depth--;
    while (_signatureDecls.size() > 0 && _signatureDecls.back().depth > _depth) {
        _signatureDecls.pop_back();
    }
}

#if defined(DEBUG)
void Parser::_trace(const std::string &message) {
    if (_config.enableTracing) {
        std::cout << message << std::endl;
    }
}
#endif

#pragma mark - Grammar

struct Candidate {
    Signature signature;
    size_t offset;
    std::vector<size_t> arguments;

    bool isComplete() const { return offset == signature.terms.size(); }
};

bool isPrimary(const Token &token) {
    if (token.isWord())
        return true;
    switch (token.type) {
    case Token::Type::LeftBrace:
    case Token::Type::LeftBracket:
    case Token::Type::LeftParen:
    case Token::Type::IntLiteral:
    case Token::Type::FloatLiteral:
    case Token::Type::BoolLiteral:
    case Token::Type::StringLiteral:
        return true;
    default:
        return false;
    }
}

bool checkTerm(const Token &token, size_t offset, Candidate &candidate) {
    if (candidate.isComplete()) {
        return false;
    }
    const auto &term = candidate.signature.terms[candidate.offset++];
    if (auto option = std::get_if<Signature::Option>(&term)) {
        if (offset == candidate.signature.terms.size()) {
            return true;
        }
        if (token.type == option->token.type &&
            lowercase(token.text) == lowercase(option->token.text)) {
            return true;
        }
        return checkTerm(token, offset, candidate);
    }
    if (auto argument = std::get_if<Signature::Argument>(&term)) {
        if (!isPrimary(token)) {
            return false;
        }
        candidate.arguments.push_back(offset);
        return true;
    }
    if (!token.isWord()) {
        return false;
    }
    if (auto matchToken = std::get_if<Token>(&term)) {
        if (lowercase(token.text) == lowercase(matchToken->text)) {
            return true;
        }
        return false;
    }
    if (auto choice = std::get_if<Signature::Choice>(&term)) {
        auto text = lowercase(token.text);
        for (const auto &alternate : choice->tokens) {
            if (lowercase(alternate.text) == text) {
                return true;
            }
        }
        return false;
    }
    return false;
}

Owned<Statement> Parser::parseBlock(const std::initializer_list<Token::Type> &endTypes) {
    std::vector<Owned<Statement>> statements;
    while (!isAtEnd()) {
        if (match({Token::Type::NewLine})) {
            continue;
        }
        if (check(endTypes)) {
            break;
        }
        try {
            statements.push_back(parseStatement());
        } catch (const SyntaxError &error) {
            _errors.push_back(error);
            synchronize();
        }
    }
    return MakeOwned<Block>(std::move(statements));
}

Owned<Statement> Parser::parseStatement() {
    if (match({Token::Type::Function})) {
        return parseFunction();
    }
    if (match({Token::Type::If})) {
        return parseIf();
    }
    if (match({Token::Type::Repeat})) {
        bool wasParsingRepeat = _parsingRepeat;
        _parsingRepeat = true;
        auto statement = parseRepeat();
        _parsingRepeat = wasParsingRepeat;
        return statement;
    }

    auto statement = parseSimpleStatement();
    consumeNewLine();

    return statement;
}

Signature Parser::parseSignature() {
    Signature signature;
    while (peek().isWord() || peek().type == Token::Type::LeftParen ||
           peek().type == Token::Type::LeftBrace) {
        auto token = advance();
        if (token.isWord()) {
            std::vector<Token> tokens({token});
            while (match({Token::Type::Slash})) {
                tokens.push_back(consumeWord());
            }
            if (tokens.size() > 1) {
                std::sort(tokens.begin(), tokens.end(),
                          [](Token lhs, Token rhs) { return lhs.text < rhs.text; });
                signature.terms.push_back(Signature::Choice{tokens});
            } else {
                signature.terms.push_back(Signature::Term{token});
            }
        } else if (token.type == Token::Type::LeftParen) {
            auto word = consumeWord();
            signature.terms.push_back(Signature::Option{word});
            consume(Token::Type::RightParen, "expected right parenthesis");
        } else {
            Optional<Token> word;
            Optional<Token> typeName;
            if ((word = matchWord())) {
                if (match({Token::Type::Colon})) {
                    typeName = consumeWord("expected a type name");
                }
            } else if (match({Token::Type::Colon})) {
                typeName = consumeWord("expected a type name");
            }
            consume(Token::Type::RightBrace, Concat("expected ", Quoted("}")));
            signature.terms.push_back(Signature::Argument{word, typeName});
        }
    }
    if (signature.terms.size() == 0) {
        throw SyntaxError(peek(), Concat("expected a word, ", Quoted("("), ", or ", Quoted("{")));
    }
    if (match({Token::Type::Arrow})) {
        signature.typeName = consumeWord("expected a type name");
    }
    return signature;
}

Owned<Statement> Parser::parseFunction() {
    auto signature = parseSignature();
    consumeNewLine();
    declare(signature);

    beginScope();
    auto statement = parseBlock({Token::Type::End});
    consumeEnd(Token::Type::Function);
    endScope();

    return MakeOwned<FunctionDecl>(signature, std::move(statement));
}

Owned<Statement> Parser::parseSimpleStatement() {
    if (match({Token::Type::Set})) {
        return parseAssignment();
    }
    if (match({Token::Type::Exit})) {
        return parseExit();
    }
    if (match({Token::Type::Next})) {
        return parseNext();
    }
    if (match({Token::Type::Return})) {
        return parseReturn();
    }
    return parseExpressionStatement();
}

Owned<Statement> Parser::parseIf() {
    auto condition = parseExpression();
    match({Token::Type::NewLine});
    consume(Token::Type::Then, Concat("expected ", Quoted("then")));

    Optional<Token> token;
    Owned<Statement> ifClause = nullptr;
    Owned<Statement> elseClause = nullptr;

    if (match({Token::Type::NewLine})) {
        ifClause = parseBlock({Token::Type::End, Token::Type::Else});
        if (!(token = match({Token::Type::End, Token::Type::Else}))) {
            throw SyntaxError(peek(), Concat("expected ", Quoted("end"), " or ", Quoted("else")));
        }
        if (token.value().type == Token::Type::End) {
            match({Token::Type::If});
            consumeNewLine();
        }
    } else {
        ifClause = parseSimpleStatement();
        match({Token::Type::NewLine});
    }

    if ((token.has_value() && token.value().type == Token::Type::Else) ||
        (!token.has_value() && match({Token::Type::Else}))) {
        if (match({Token::Type::NewLine})) {
            elseClause = parseBlock({Token::Type::End});
            consumeEnd(Token::Type::If);
            consumeNewLine();
        } else {
            if (match({Token::Type::If})) {
                elseClause = parseIf();
            } else {
                elseClause = parseSimpleStatement();
                consumeNewLine();
            }
        }
    }

    auto ifStatement =
        MakeOwned<If>(std::move(condition), std::move(ifClause), std::move(elseClause));
    ifStatement->location = ifStatement->condition->location;
    return ifStatement;
}

Owned<Statement> Parser::parseRepeat() {
    if (auto token = match({Token::Type::Forever, Token::Type::NewLine})) {
        if (token.value().type == Token::Type::Forever) {
            consumeNewLine();
        }
        auto statement = parseBlock({Token::Type::End});
        consumeEnd(Token::Type::Repeat);
        consumeNewLine();
        return MakeOwned<Repeat>(std::move(statement));
    }
    if (auto token = match({Token::Type::While, Token::Type::Until})) {
        bool conditionValue = (token.value().type == Token::Type::While ? true : false);
        auto condition = parseExpression();
        consumeNewLine();
        auto statement = parseBlock({Token::Type::End});
        consumeEnd(Token::Type::Repeat);
        return MakeOwned<RepeatCondition>(std::move(statement), std::move(condition),
                                          conditionValue);
    }
    if (match({Token::Type::For})) {
        auto token = consumeWord();
        auto variable = MakeOwned<Variable>(token);
        variable->location = token.location;
        consume(Token::Type::In, Concat("expected ", Quoted("in")));
        auto expression = parseList();
        consumeNewLine();
        auto statement = parseBlock({Token::Type::End});
        consumeEnd(Token::Type::Repeat);
        return MakeOwned<RepeatFor>(std::move(statement), std::move(variable),
                                    std::move(expression));
    }
    throw SyntaxError(peek(), "unexpected expression");
}

Owned<Statement> Parser::parseAssignment() {
    Variable::Scope scope = Variable::Scope::Unspecified;
    if (match({Token::Type::Global})) {
        scope = Variable::Scope::Global;
    } else if (match({Token::Type::Local})) {
        scope = Variable::Scope::Local;
    }
    auto token = consumeWord("expected variable name");
    Optional<Token> typeName;
    if (match({Token::Type::Colon})) {
        typeName = consumeWord();
    }
    consume(Token::Type::To, Concat("expected ", Quoted("to")));
    auto expression = parseExpression();
    auto variable = MakeOwned<Variable>(token, typeName, scope);
    return MakeOwned<Assignment>(std::move(variable), std::move(expression));
}

Owned<Statement> Parser::parseExit() {
    if (!_parsingRepeat) {
        throw SyntaxError(previous(),
                          Concat("unexpected ", Quoted("exit"), " outside repeat block"));
    }
    consume(Token::Type::Repeat, Concat("expected ", Quoted("repeat")));
    return MakeOwned<ExitRepeat>();
}

Owned<Statement> Parser::parseNext() {
    if (!_parsingRepeat) {
        throw SyntaxError(previous(),
                          Concat("unexpected ", Quoted("next"), " outside repeat block"));
    }
    consume(Token::Type::Repeat, Concat("expected ", Quoted("repeat")));
    return MakeOwned<NextRepeat>();
}

Owned<Statement> Parser::parseReturn() {
    Owned<Expression> expression;
    auto location = previous().location;
    if (!check({Token::Type::NewLine})) {
        expression = parseExpression();
    }
    auto returnStatement = MakeOwned<Return>(std::move(expression));
    returnStatement->location = location;
    return returnStatement;
}

Owned<Statement> Parser::parseExpressionStatement() {
    return MakeOwned<ExpressionStatement>(parseExpression());
}

Binary::Operator binaryOp(Token::Type tokenType) {
    switch (tokenType) {
    case Token::Type::And:
        return Binary::Operator::And;
    case Token::Type::Or:
        return Binary::Operator::Or;
    case Token::Type::Plus:
        return Binary::Operator::Plus;
    case Token::Type::Minus:
        return Binary::Operator::Minus;
    case Token::Type::Star:
        return Binary::Operator::Multiply;
    case Token::Type::Slash:
        return Binary::Operator::Divide;
    case Token::Type::Percent:
        return Binary::Operator::Modulo;
    case Token::Type::Carrot:
        return Binary::Operator::Exponent;
    case Token::Type::Is:
        return Binary::Operator::Equal;
    case Token::Type::Equal:
        return Binary::Operator::Equal;
    case Token::Type::NotEqual:
        return Binary::Operator::NotEqual;
    case Token::Type::LessThan:
        return Binary::Operator::LessThan;
    case Token::Type::LessThanOrEqual:
        return Binary::Operator::LessThanOrEqual;
    case Token::Type::GreaterThan:
        return Binary::Operator::GreaterThan;
    case Token::Type::GreaterThanOrEqual:
        return Binary::Operator::GreaterThanOrEqual;
    default:
        throw std::range_error("unknown token type");
    }
}

Unary::Operator unaryOp(Token::Type tokenType) {
    switch (tokenType) {
    case Token::Type::Minus:
        return Unary::Operator::Minus;
    case Token::Type::Not:
        return Unary::Operator::Not;
    default:
        throw std::range_error("unknown token type");
    }
}

Owned<Expression> Parser::parseExpression() { return parseClause(); }

Owned<Expression> Parser::parseClause() {
    auto expression = parseEquality();
    auto location = expression->location;
    while (auto operatorToken = match({Token::Type::And, Token::Type::Or})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type),
                                       parseEquality());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseEquality() {
    auto expression = parseComparison();
    auto location = expression->location;
    while (auto operatorToken =
               match({Token::Type::Equal, Token::Type::NotEqual, Token::Type::Is})) {
        if (operatorToken.value().type == Token::Type::Is && match({Token::Type::Not})) {
            expression = MakeOwned<Binary>(std::move(expression), Binary::Operator::NotEqual,
                                           parseComparison());
        } else {
            expression = MakeOwned<Binary>(std::move(expression),
                                           binaryOp(operatorToken.value().type), parseComparison());
        }
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseComparison() {
    auto expression = parseList();
    auto location = expression->location;
    while (auto operatorToken =
               match({Token::Type::LessThan, Token::Type::GreaterThan, Token::Type::LessThanOrEqual,
                      Token::Type::GreaterThanOrEqual})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type),
                                       parseList());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseList() {
    auto expression = parseRange();

    if (check({Token::Type::Comma})) {
        auto location = expression->location;

        std::vector<Owned<Expression>> expressions;
        expressions.push_back(std::move(expression));
        while (match({Token::Type::Comma})) {
            expressions.push_back(parseRange());
        }

        expression = MakeOwned<ListLiteral>(std::move(expressions));
        expression->location = location;
    }

    return expression;
}

Owned<Expression> Parser::parseRange() {
    auto expression = parseTerm();
    auto location = expression->location;
    while (auto rangeOperator = match({Token::Type::ThreeDots, Token::Type::ClosedRange})) {
        bool closed = rangeOperator.value().type == Token::Type::ThreeDots ? true : false;
        expression = MakeOwned<RangeLiteral>(std::move(expression), parseTerm(), closed);
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseTerm() {
    auto expression = parseFactor();
    auto location = expression->location;
    while (auto operatorToken = match({Token::Type::Plus, Token::Type::Minus})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type),
                                       parseFactor());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseFactor() {
    auto expression = parseExponent();
    auto location = expression->location;
    while (auto operatorToken =
               match({Token::Type::Star, Token::Type::Slash, Token::Type::Percent})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type),
                                       parseExponent());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseExponent() {
    auto expression = parseUnary();
    auto location = expression->location;
    while (auto operatorToken = match({Token::Type::Carrot})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type),
                                       parseUnary());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::parseUnary() {
    if (auto operatorToken = match({Token::Type::Minus, Token::Type::Not})) {
        auto expression = MakeOwned<Unary>(unaryOp(operatorToken.value().type), parseUnary());
        expression->location = operatorToken.value().location;
        return expression;
    }
    return parseCall();
}

Owned<Expression> Parser::parseCall() {
    std::vector<Candidate> candidates;
    std::set<Signature> signatures;
    for (auto it = _signatureDecls.rbegin(); it != _signatureDecls.rend(); it++) {
        if (signatures.find(it->signature) == signatures.end()) {
            candidates.push_back({it->signature, 0});
            signatures.insert(it->signature);
        }
    }
    auto startToken = peek();
    std::vector<Owned<Expression>> primaries;
    while (candidates.size() > 0 && isPrimary(peek())) {
        trace(Concat("Checking ", Quoted(peek().text)));
        candidates = Filter(candidates, [&](Candidate &candidate) {
            return checkTerm(peek(), primaries.size(), candidate);
        });
        trace(Concat("Candidates: ", candidates.size()));
        if (candidates.size() == 1 && candidates[0].isComplete()) {
            // Special case lower parsing precedence for trailing arguments.
            // This allows chaining calls and right associativity.
            if (candidates[0].signature.endsWithArgument()) {
                primaries.push_back(parseList());
            } else {
                primaries.push_back(parseSubscript());
            }
            break;
        } else if (candidates.size() > 0) {
            primaries.push_back(parseSubscript());
        }
    }

    // Filter incomplete matches.
    candidates = Filter(candidates, [&](Candidate &candidate) { return candidate.isComplete(); });

    // No matching signatures.
    if (candidates.size() == 0) {
        // Special case single primaries.
        if (primaries.size() == 1) {
            return std::move(primaries[0]);
        } else if (primaries.size() == 0) {
            return parseSubscript();
        }
        throw SyntaxError(startToken, "no matching function for expression");
    }

    // Partial match of multiple signatures.
    if (candidates.size() > 1) {
        auto ambiguousList = Map(
            candidates, [](auto &&candidate) { return Concat("  ", candidate.signature.name()); });
        throw SyntaxError(startToken, Concat("ambiguous expression. Possible candidates:\n",
                                             Join(ambiguousList, "\n")));
    }

    const auto &candidate = candidates.back();
    trace(Concat("Matched ", candidate.signature.name()));
    std::vector<Owned<Expression>> arguments;
    for (size_t i = 0; i < candidate.arguments.size(); i++) {
        arguments.push_back(std::move(primaries[candidate.arguments[i]]));
    }
    auto call =
        MakeOwned<Call>(candidate.signature, std::vector<Optional<Token>>(), std::move(arguments));
    call->location = peek().location;
    return call;
}

Owned<Expression> Parser::parseSubscript() {
    auto expression = parsePrimary();
    auto location = expression->location;
    while (auto operatorToken = match({Token::Type::LeftBracket})) {
        expression = MakeOwned<Binary>(std::move(expression), Binary::Subscript, parseExpression());
        expression->location = location;
        consume(Token::Type::RightBracket, Concat("expected ", Quoted("]")));
    }
    return expression;
}

Owned<Expression> Parser::parsePrimary() {
    if (auto token = match({
            Token::Type::BoolLiteral,
            Token::Type::IntLiteral,
            Token::Type::FloatLiteral,
            Token::Type::StringLiteral,
            Token::Type::EmptyLiteral,
        })) {
        auto literal = MakeOwned<Literal>(token.value());
        literal->location = token.value().location;
        return literal;
    }

    if (match({Token::Type::LeftParen})) {
        return parseGrouping();
    }

    if (match({Token::Type::LeftBrace})) {
        return parseDictionaryLiteral();
    }

    if (match({Token::Type::LeftBracket})) {
        return parseListLiteral();
    }

    if (match({Token::Type::Global})) {
        auto token = consumeWord("expected variable name");
        auto variable = MakeOwned<Variable>(token, None, Variable::Scope::Global);
        variable->location = token.location;
        return variable;
    }

    if (match({Token::Type::Local})) {
        auto token = consumeWord("expected variable name");
        auto variable = MakeOwned<Variable>(token, None, Variable::Scope::Local);
        variable->location = token.location;
        return variable;
    }

    if (peek().isWord()) {
        auto token = advance();
        auto variable = MakeOwned<Variable>(token);
        variable->location = token.location;
        return variable;
    }

    throw SyntaxError(peek(), Concat("unexpected ", Quoted(peek().description())));
}

Owned<Expression> Parser::parseGrouping() {
    auto expression = parseExpression();
    consume(Token::Type::RightParen, Concat("expected ", Quoted(")")));
    auto grouping = MakeOwned<Grouping>(std::move(expression));
    grouping->location = grouping->expression->location;
    return grouping;
}

Owned<Expression> Parser::parseListLiteral() {
    std::vector<Owned<Expression>> values;

    if (!match({Token::Type::RightBracket})) {
        do {
            auto expression = parseTerm();
            values.push_back(std::move(expression));
        } while (match({Token::Type::Comma}));
        consume(Token::Type::RightBracket, Concat("expected ", Quoted("]")));
    }

    return MakeOwned<ListLiteral>(std::move(values));
}

Owned<Expression> Parser::parseDictionaryLiteral() {
    Mapping<Owned<Expression>, Owned<Expression>> values;

    if (!match({Token::Type::RightBrace})) {
        do {
            auto keyExpression = parseTerm();
            consume(Token::Type::Colon, Concat("expected ", Quoted(":")));
            auto valueExpression = parseTerm();

            values[std::move(keyExpression)] = std::move(valueExpression);
        } while (match({Token::Type::Comma}));
        consume(Token::Type::RightBrace, Concat("expected ", Quoted("}")));
    }

    return MakeOwned<DictionaryLiteral>(std::move(values));
}

SIF_NAMESPACE_END
