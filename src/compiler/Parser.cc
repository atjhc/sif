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
#include "ast/Repeat.h"
#include "Utilities.h"

#include <vector>

CH_NAMESPACE_BEGIN

#if defined(DEBUG)
#define trace(msg) _trace(msg)
#else
#define trace(msg)
#endif

static inline std::string Describe(const Token &token) {
    return token.description();
}

static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Describe(token);
}

Parser::Parser(const ParserConfig &config, Scanner &scanner) 
    : _config(config), _scanner(scanner) {
    _parsingRepeat = false;
    _recording = false;
    _index = 0;
}

Owned<Statement> Parser::parse() {
    auto result = _parseBlock();
    if (_errors.size() > 0) {
        return nullptr;
    }
    return result;
}

FunctionSignature Parser::parseFunctionSignature() {
    return _parseFunctionSignature();
}

void Parser::declare(const FunctionSignature &signature) {
    _functionDecls.insert(signature);
}

const std::vector<SyntaxError> &Parser::errors() {
    return _errors;
}

#pragma mark - Utilities

Optional<Token> Parser::_match(const std::initializer_list<Token::Type> &types) {
    if (_check(types)) {
        return _advance();
    }
    return None;
}

Token Parser::_consumeWord() {
    if (_peek().isWord()) return _advance();
    throw SyntaxError(_peek(), "expected a word");
}

Token Parser::_consume(Token::Type type, const std::string &errorMessage) {
    if (_check({type})) return _advance();
    throw SyntaxError(_peek(), errorMessage);
}

Token Parser::_consumeNewLine() {
    if (_isAtEnd()) return _peek();
    if (_check({Token::Type::NewLine})) return _advance();
    throw SyntaxError(_peek(), "expected new line or end of script");
}

Token Parser::_consumeEnd(Token::Type type) {
    auto endToken = _consume(Token::Type::End, Concat("expected ", Quoted("end")));
    if (auto matchToken = _match({type})) {
        return matchToken.value();
    }
    return endToken;
}

bool Parser::_check(const std::initializer_list<Token::Type> &types) {
    if (_isAtEnd()) {
        return false;
    }
    for (auto type : types) {
        if (_peek().type == type) {
            return true;
        }
    }
    return false;
}

bool Parser::_isAtEnd() {
    return _peek().type == Token::Type::EndOfFile;
}

Token& Parser::_scan() {
    auto token = _scanner.scan();
    _tokens.push_back(token);
    trace(Concat("Scanned ", Describe(token)));
    trace(Concat("Tokens [", Join(_tokens, ", "), "] ", _index));
    return _tokens.back();
}

Token& Parser::_advance() {
    if (!_isAtEnd()) {
        if (!_recording && _tokens.size() > 1) {
            _tokens.erase(_tokens.begin());
        }
        if (_recording || _index == 0) {
            _index++;
        }
        if (_index == _tokens.size()) {
            _scan();
        }
    }
    return _previous();
}

Token& Parser::_peek() {
    if (_tokens.size() == 0) {
        return _scan();
    }
    return _tokens[_index];
}

Token& Parser::_previous() {
    if (_tokens.size() > 1) {
        return _tokens[_index - 1];
    }
    throw std::runtime_error("scanner underflow");
}

void Parser::_synchronize() {
    trace("Synchronizing");
    _recording = false;
    auto token = _advance();
    while (!_isAtEnd()) {
        if (token.type == Token::Type::NewLine) {
            return;
        }
        token = _advance();
    }
}

void Parser::_checkpoint() {
    _recording = true;
    _saved.push(_index);
    trace(Concat("Checkpoint (", _index, ")"));
}

void Parser::_rewind() {
    _index = _saved.top();
    _saved.pop();
    if (_saved.size() == 0) {
        _recording = false;
    }
    trace(Concat("Rewind (", _index, ")"));
}

void Parser::_commit() {
    _saved.pop();
    if (_saved.size() == 0) {
        _recording = false;
        _tokens.erase(_tokens.begin(), _tokens.begin() + _index - 1);
        _index = 1;
    }
    trace(Concat("Commit (", _previous().description(), ", ", _peek().description(), ")"));
}

#if defined(DEBUG)
void Parser::_trace(const std::string &message) {
    if (_config.enableTracing) {
        std::cout << message << std::endl;
    }
}
#endif

#pragma mark - Grammar

bool Parser::_matchTerm(const FunctionSignature &signature, int index, std::vector<Optional<Token>> &tokens, std::vector<Owned<Expression>> &arguments) {
    const auto &term = signature.terms[index];
    if (auto token = std::get_if<Token>(&term)) {
        if (_peek().type == token->type && lowercase(_peek().text) == lowercase(token->text)) {
            _advance();
            return true;
        }
        return false;
    }
    if (auto option = std::get_if<FunctionSignature::Option>(&term)) {
        if (_peek().type == option->token.type && lowercase(_peek().text) == lowercase(option->token.text)) {
            tokens.push_back(_advance());
        } else {
            tokens.push_back(None);
        }
        return true;
    }
    if (auto argument = std::get_if<FunctionSignature::Argument>(&term)) {
        if (index == signature.terms.size() - 1) {
            if (auto matchedExpression = _parseList()) {
                arguments.push_back(std::move(matchedExpression));
                return true;
            }
        } else {
           if (auto matchedExpression = _parseSubscript()) {
                arguments.push_back(std::move(matchedExpression));
                return true;
            }
        }
        return false;
    }
    if (auto choice = std::get_if<FunctionSignature::Choice>(&term)) {
        for (const auto &token : choice->tokens) {
            if (_peek().isWord() && lowercase(_peek().text) == lowercase(token.text)) {
                tokens.push_back(_advance());
                return true;
            }
        }
        return false;
    }
    return false;
}

bool Parser::_matchSignature(const FunctionSignature &signature, std::vector<Optional<Token>> &tokens, std::vector<Owned<Expression>> &arguments) {
    trace(Concat("Checking function signature ", Quoted(signature.name())));
    for (int i = 0; i < signature.terms.size(); i++) {
        if (!_matchTerm(signature, i, tokens, arguments)) {
            return false;
        }
    }
    trace(Concat("Matched function signature ", Quoted(signature.name())));
    return true;
}

Owned<Statement> Parser::_parseBlock(const std::initializer_list<Token::Type> &endTypes) {
    std::vector<Owned<Statement>> statements;
    while (!_isAtEnd()) {
        if (_match({Token::Type::NewLine})) {
            continue;
        }
        if (_check(endTypes)) {
            break;
        }
        
        try {
            statements.push_back(_parseStatement());
        } catch (const SyntaxError &error) {
            _errors.push_back(error);
            _synchronize();
        }
    }
    return MakeOwned<Block>(std::move(statements));
}

Owned<Statement> Parser::_parseStatement() {
    if (_match({Token::Type::Function})) {
        return _parseFunction();
    }
    if (_match({Token::Type::If})) {
        return _parseIf();
    }
    if (_match({Token::Type::Repeat})) {
        bool wasParsingRepeat = _parsingRepeat;
        _parsingRepeat = true;
        auto statement = _parseRepeat();
        _parsingRepeat = wasParsingRepeat;
        return statement;
    }

    auto statement = _parseSimpleStatement();
    _consumeNewLine();

    return statement;
}

FunctionSignature Parser::_parseFunctionSignature() {
    FunctionSignature signature;
    while (_peek().isWord() || _peek().type == Token::Type::LeftParen) {
        auto token = _advance();
        if (token.isWord()) {
            std::vector<Token> tokens({token});
            while (_match({Token::Type::Slash})) {
               tokens.push_back(_consumeWord());
            }
            if (tokens.size() > 1) {
                std::sort(tokens.begin(), tokens.end(), [](Token lhs, Token rhs) {
                    return lhs.text < rhs.text;
                });
                signature.terms.push_back(FunctionSignature::Choice{tokens});
            } else {
                signature.terms.push_back(FunctionSignature::Term{token});
            }
        } else {
            if (_peek().isWord()) {
                auto word = _advance();
                if (_match({Token::Type::Colon})) {
                    auto typeName = _match({Token::Type::Word});
                    _consume(Token::Type::RightParen, Concat("expected ", Quoted(")")));
                    signature.terms.push_back(FunctionSignature::Argument{word, typeName});
                } else if (_match({Token::Type::RightParen})) {
                    signature.terms.push_back(FunctionSignature::Option{word});
                }
            } else if (_match({Token::Type::Colon})) {
                auto typeName = _match({Token::Type::Word});
                signature.terms.push_back(FunctionSignature::Term{FunctionSignature::Argument{None, typeName}});
                _consume(Token::Type::RightParen, "expected right parenthesis");
            } else {
                throw SyntaxError(_peek(), Concat("expected a word, ", Quoted(":"), ", or ", Quoted("/")));
            }
        }
    }
    if (signature.terms.size() == 0) {
        throw SyntaxError(_peek(), Concat("expected a word or ", Quoted("(")));
    }
    if (_match({Token::Type::Arrow})) {
        signature.typeName = _match({Token::Type::Word});
    }

    return signature;
}

Owned<Statement> Parser::_parseFunction() {
    auto signature = _parseFunctionSignature();
    _consumeNewLine();
    declare(signature);
    auto statement = _parseBlock({Token::Type::End});
    _consumeEnd(Token::Type::Function);

    return MakeOwned<FunctionDecl>(signature, std::move(statement));
}

Owned<Statement> Parser::_parseSimpleStatement() {
    if (_match({Token::Type::Set})) {
        return _parseAssignment();
    }
    if (_match({Token::Type::Exit})) {
        return _parseExit();
    }
    if (_match({Token::Type::Next})) {
        return _parseNext();
    }
    if (_match({Token::Type::Return})) {
        return _parseReturn();
    }
    return _parseExpressionStatement();
}

Owned<Statement> Parser::_parseIf() {
    auto condition = _parseExpression();
    _match({Token::Type::NewLine});
    _consume(Token::Type::Then, Concat("expected ", Quoted("then")));

    Optional<Token> token;
    Owned<Statement> ifClause = nullptr;
    Owned<Statement> elseClause = nullptr;

    if (_match({Token::Type::NewLine})) {
        ifClause = _parseBlock({Token::Type::End, Token::Type::Else});
        if (!(token = _match({Token::Type::End, Token::Type::Else}))) {
            throw SyntaxError(_peek(), Concat("expected ", Quoted("end"), " or ", Quoted("else")));
        }
        if (token.value().type == Token::Type::End) {
            _match({Token::Type::If});
            _consumeNewLine();
        }
    } else {
        ifClause = _parseSimpleStatement();
        _match({Token::Type::NewLine});
    }

    if ((token.has_value() && token.value().type == Token::Type::Else) || 
        (!token.has_value() && _match({Token::Type::Else}))) {
        if (_match({Token::Type::NewLine})) {
            elseClause = _parseBlock({Token::Type::End});
            _consumeEnd(Token::Type::If);
            _consumeNewLine();
        } else {
            if (_match({Token::Type::If})) {
                elseClause = _parseIf();
            } else {
                elseClause = _parseSimpleStatement();
                _consumeNewLine();
            }
        }
    }

    auto ifStatement = MakeOwned<If>(std::move(condition), std::move(ifClause), std::move(elseClause));
    ifStatement->location = ifStatement->condition->location;
    return ifStatement;
}

Owned<Statement> Parser::_parseRepeat() {
    if (auto token = _match({Token::Type::Forever, Token::Type::NewLine})) {
        if (token.value().type == Token::Type::Forever) {
            _consumeNewLine();
        }
        auto statement = _parseBlock({Token::Type::End});
        _consumeEnd(Token::Type::Repeat);
        _consumeNewLine();
        return MakeOwned<Repeat>(std::move(statement));
    }
    if (auto token = _match({Token::Type::While, Token::Type::Until})) {
        bool conditionValue = (token.value().type == Token::Type::While ? true : false);
        auto condition = _parseExpression();
        _consumeNewLine();
        auto statement = _parseBlock({Token::Type::End});
        _consumeEnd(Token::Type::Repeat);
        return MakeOwned<RepeatCondition>(std::move(statement), std::move(condition), conditionValue);
    }
    throw SyntaxError(_peek(), "unexpected expression");
}

Owned<Statement> Parser::_parseAssignment() {
    auto token = _consume(Token::Type::Word, Concat("expected variable name"));
    Optional<Token> typeName;
    if (_match({Token::Type::Colon})) {
        typeName = _consumeWord();
    }
    _consume(Token::Type::To, Concat("expected ", Quoted("to")));
    auto expression = _parseExpression();
    auto variable = MakeOwned<Variable>(token, typeName);
    return MakeOwned<Assignment>(std::move(variable), std::move(expression));
}

Owned<Statement> Parser::_parseExit() {
    if (!_parsingRepeat) {
        throw SyntaxError(_previous(), Concat("unexpected ", Quoted("exit"), " outside repeat block"));
    }
    _consume(Token::Type::Repeat, Concat("expected ", Quoted("repeat")));
    return MakeOwned<ExitRepeat>();
}

Owned<Statement> Parser::_parseNext() {
    if (!_parsingRepeat) {
        throw SyntaxError(_previous(), Concat("unexpected ", Quoted("next"), " outside repeat block"));
    }
    _consume(Token::Type::Repeat, Concat("expected ", Quoted("repeat")));
    return MakeOwned<NextRepeat>();
}

Owned<Statement> Parser::_parseReturn() {
    Owned<Expression> expression;
    auto location = _previous().location;
    if (!_check({Token::Type::NewLine})) {
        expression = _parseExpression();
    }
    auto returnStatement = MakeOwned<Return>(std::move(expression));
    returnStatement->location = location;
    return returnStatement;
}

Owned<Statement> Parser::_parseExpressionStatement() {
    return MakeOwned<ExpressionStatement>(_parseExpression());
}

Binary::Operator binaryOp(Token::Type tokenType) {
    switch (tokenType) {
    case Token::Type::And: return Binary::Operator::And;
    case Token::Type::Or: return Binary::Operator::Or;
    case Token::Type::Plus: return Binary::Operator::Plus;
    case Token::Type::Minus: return Binary::Operator::Minus;
    case Token::Type::Star: return Binary::Operator::Multiply;
    case Token::Type::Slash: return Binary::Operator::Divide;
    case Token::Type::Percent: return Binary::Operator::Modulo;
    case Token::Type::Carrot: return Binary::Operator::Exponent;
    case Token::Type::Is: return Binary::Operator::Equal;
    case Token::Type::Equal: return Binary::Operator::Equal;
    case Token::Type::NotEqual: return Binary::Operator::NotEqual;
    case Token::Type::LessThan: return Binary::Operator::LessThan;
    case Token::Type::LessThanOrEqual: return Binary::Operator::LessThanOrEqual;
    case Token::Type::GreaterThan: return Binary::Operator::GreaterThan;
    case Token::Type::GreaterThanOrEqual: return Binary::Operator::GreaterThanOrEqual;
    default: throw std::range_error("unknown token type");
    }
}

Unary::Operator unaryOp(Token::Type tokenType) {
    switch (tokenType) {
    case Token::Type::Minus: return Unary::Operator::Minus;
    case Token::Type::Not: return Unary::Operator::Not;
    default: throw std::range_error("unknown token type");
    }
}

Owned<Expression> Parser::_parseExpression() {
    return _parseClause();
}

Owned<Expression> Parser::_parseClause() {
    auto expression = _parseEquality();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::And, Token::Type::Or})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseEquality());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseEquality() {
    auto expression = _parseComparison();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::Equal, Token::Type::NotEqual, Token::Type::Is})) {
        if (operatorToken.value().type == Token::Type::Is && _match({Token::Type::Not})) {
            expression = MakeOwned<Binary>(std::move(expression), Binary::Operator::NotEqual, _parseComparison());
        } else {
            expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseComparison());
        }
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseComparison() {
    auto expression = _parseList();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::LessThan, Token::Type::GreaterThan, Token::Type::LessThanOrEqual, Token::Type::GreaterThanOrEqual})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseList());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseList() {
    auto expression = _parseRange();

    if (_check({Token::Type::Comma})) {
        auto location = expression->location;
        
        std::vector<Owned<Expression>> expressions;
        expressions.push_back(std::move(expression));
        while (_match({Token::Type::Comma})) {
            expressions.push_back(_parseRange());
        }

        expression = MakeOwned<ListLiteral>(std::move(expressions));
        expression->location = location;
    }

    return expression;
}

Owned<Expression> Parser::_parseRange() {
    auto expression = _parseTerm();
    auto location = expression->location;
    while (auto rangeOperator = _match({Token::Type::ThreeDots, Token::Type::ClosedRange})) {
        bool closed = rangeOperator.value().type == Token::Type::ThreeDots ? true : false;
        expression = MakeOwned<RangeLiteral>(std::move(expression), _parseTerm(), closed);
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseTerm() {
    auto expression = _parseFactor();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::Plus, Token::Type::Minus})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseFactor());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseFactor() {
    auto expression = _parseExponent();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::Star, Token::Type::Slash, Token::Type::Percent})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseExponent());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseExponent() {
    auto expression = _parseUnary();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::Carrot})) {
        expression = MakeOwned<Binary>(std::move(expression), binaryOp(operatorToken.value().type), _parseUnary());
        expression->location = location;
    }
    return expression;
}

Owned<Expression> Parser::_parseUnary() {
    if (auto operatorToken = _match({Token::Type::Minus, Token::Type::Not})){
        auto expression = MakeOwned<Unary>(unaryOp(operatorToken.value().type), _parseUnary());
        expression->location = operatorToken.value().location;
        return expression;
    }
    return _parseCall();
}

Owned<Expression> Parser::_parseCall() {
    for (const auto &signature : _functionDecls) {
        Location location = _peek().location;
        std::vector<Optional<Token>> tokens;
        std::vector<Owned<Expression>> arguments;
        _checkpoint();
        if (_matchSignature(signature, tokens, arguments)) {
            _commit();
            auto call = MakeOwned<Call>(signature, std::move(tokens), std::move(arguments));
            call->location = location;
            return call;
        }
        _rewind();
    }
    return _parseSubscript();
}

Owned<Expression> Parser::_parseSubscript() {
    auto expression = _parsePrimary();
    auto location = expression->location;
    while (auto operatorToken = _match({Token::Type::LeftBracket})) {
        expression = MakeOwned<Binary>(std::move(expression), Binary::Subscript, _parseExpression());
        expression->location = location;
        _consume(Token::Type::RightBracket, Concat("expected ", Quoted("]")));
    }
    return expression;
}

Owned<Expression> Parser::_parsePrimary() {
    if (auto token = _match({
        Token::Type::IntLiteral,
        Token::Type::FloatLiteral, 
        Token::Type::StringLiteral,
        Token::Type::BoolLiteral,
    })) {
        auto literal = MakeOwned<Literal>(token.value());
        literal->location = token.value().location;
        return literal;
    }
    
    if (_match({Token::Type::LeftParen})) {
        return _parseGrouping();
    }

    if (_match({Token::Type::LeftBrace})) {
        return _parseDictionaryLiteral();
    }

    if (_match({Token::Type::LeftBracket})) {
        return _parseListLiteral();
    }

    if (_peek().isWord()) {
        auto token = _advance();
        auto variable = MakeOwned<Variable>(token);
        variable->location = token.location;
        return variable;
    }

    throw SyntaxError(_peek(), Concat("unexpected ", Quoted(_peek().description())));
}

Owned<Expression> Parser::_parseGrouping() {
    auto expression = _parseExpression();
    _consume(Token::Type::RightParen, Concat("expected ", Quoted(")")));
    auto grouping = MakeOwned<Grouping>(std::move(expression));
    grouping->location = grouping->expression->location;
    return grouping;
}

Owned<Expression> Parser::_parseListLiteral() {
    std::vector<Owned<Expression>> values;

    if (!_match({Token::Type::RightBracket})) {
        do {
            auto expression = _parseTerm();
            values.push_back(std::move(expression));
        } while (_match({Token::Type::Comma}));
        _consume(Token::Type::RightBracket, Concat("expected ", Quoted("]")));
    }

    return MakeOwned<ListLiteral>(std::move(values));
}

Owned<Expression> Parser::_parseDictionaryLiteral() {
    Map<Owned<Expression>, Owned<Expression>> values;

    if (!_match({Token::Type::RightBrace})) {
        do {
            auto keyExpression = _parseTerm();
            _consume(Token::Type::Colon, Concat("expected ", Quoted(":")));
            auto valueExpression = _parseTerm();

            values[std::move(keyExpression)] = std::move(valueExpression);
        } while (_match({Token::Type::Comma}));
        _consume(Token::Type::RightBrace, Concat("expected ", Quoted("}")));
    }

    return MakeOwned<DictionaryLiteral>(std::move(values));
}

CH_NAMESPACE_END
