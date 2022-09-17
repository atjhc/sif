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

#if defined(DEBUG)
static inline std::string Describe(const Token &token) { return token.debugDescription(); }

static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Describe(token);
}
#endif

Parser::Parser(const ParserConfig &config, Strong<Scanner> scanner, Strong<Reader> reader)
    : _config(config), _scanner(scanner), _reader(reader) {
    _parsingRepeat = false;
    _recording = false;
    _index = 0;
    _depth = 0;
    _parsingDepth = 0;
}

Parser::~Parser() {}

Owned<Statement> Parser::statement() {
    auto error = _reader->read(0);
    if (error) {
        _errors.push_back(ParseError(Token(), error.value().what()));
        return nullptr;
    }
    _scanner->reset(_reader->contents());

    auto block = parseBlock({});
    if (_errors.size()) {
        return nullptr;
    }
    return block;
}

Optional<Signature> Parser::signature() {
    auto error = _reader->read(0);
    if (error) {
        emitError(ParseError(Token(), error.value().what()));
        return None;
    }
    _scanner->reset(_reader->contents());

    auto signature = parseSignature();
    if (_errors.size()) {
        return None;
    }
    return signature;
}

void Parser::declare(const Signature &signature) { _signatureDecls.push_back({signature, _depth}); }

const std::vector<ParseError> &Parser::errors() { return _errors; }

#pragma mark - Utilities

Optional<Token> Parser::match(const Parser::TokenList &types) {
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

Optional<Token> Parser::consumeWord() {
    if (peek().isWord()) {
        return advance();
    }
    return None;
}

Optional<Token> Parser::consume(Token::Type type) {
    if (check({type}))
        return advance();
    return None;
}

Optional<Token> Parser::consumeEnd(Token::Type type) {
    auto endToken = consume(Token::Type::End);
    if (endToken) {
        if (auto matchToken = match({type})) {
            return matchToken.value();
        }
    }
    return endToken;
}

bool Parser::consumeNewLine() {
    if (isAtEnd() && _parsingDepth > 0 && _reader->readable()) {
        auto error = _reader->read(_parsingDepth);
        if (error) {
            emitError(ParseError(Token(), error.value().what()));
            return false;
        }
        _scanner->reset(_reader->contents());
        _tokens[_index].type = Token::Type::NewLine;
        advance();
        return true;
    }
    if (isAtEnd()) {
        return true;
    }
    if (check({Token::Type::NewLine})) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(const Parser::TokenList &types) {
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
    auto token = _scanner->scan();
    _tokens.push_back(token);
    trace(Concat("Scanned: ", Describe(token)));
#if defined(DEBUG)
    if (_config.enableTracing) {
        _trace(_traceTokens());
    }
#endif
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

Token Parser::synchronize(const Parser::TokenList &tokenTypes) {
    trace("Synchronizing");
    _recording = false;
    while (!isAtEnd()) {
        if (match(tokenTypes)) {
            break;
        }
        advance();
    }
    return previous();
}

NoneType Parser::emitError(const ParseError &error) {
    _errors.push_back(error);
    return None;
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
    trace(Concat("Commit (", previous().debugDescription(), ", ", peek().debugDescription(), ")"));
}

void Parser::beginScope() { _depth++; }

void Parser::endScope() {
    _depth--;
    while (_signatureDecls.size() > 0 && _signatureDecls.back().depth > _depth) {
        _signatureDecls.pop_back();
    }
}

#if defined(DEBUG)

void Parser::_trace(const std::string &message) const {
    if (_config.enableTracing) {
        std::cout << message << std::endl;
    }
}

std::string Parser::_traceTokens() const {
    std::ostringstream str;
    for (auto i = _tokens.begin(); i < _tokens.end(); i++) {
        str << (i - _tokens.begin() == _index ? " . " : " ") << *i;
    }
    return str.str();
}

#endif

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

#pragma mark - Grammar

Optional<Signature> Parser::parseSignature() {
    Signature signature;
    Set<std::string> argumentNames;

    while (peek().isWord() || peek().type == Token::Type::LeftParen ||
           peek().type == Token::Type::LeftBrace) {
        auto token = advance();
        if (token.isWord()) {
            std::vector<Token> tokens({token});
            while (match({Token::Type::Slash})) {
                auto word = consumeWord();
                if (!word) {
                    return emitError(ParseError(peek(), "expected a word"));
                }
                tokens.push_back(word.value());
            }
            if (tokens.size() > 1) {
                std::sort(tokens.begin(), tokens.end(),
                          [](Token lhs, Token rhs) { return lhs.text < rhs.text; });
                signature.terms.push_back(Signature::Choice{tokens});
            } else {
                signature.terms.push_back(Signature::Term{token});
            }
        } else if (token.type == Token::Type::LeftParen) {
            if (auto word = consumeWord()) {
                signature.terms.push_back(Signature::Option{word.value()});
                if (!consume(Token::Type::RightParen)) {
                    return emitError(ParseError(peek(), Concat("expected ", Quoted(")"))));
                }
            } else {
                return emitError(ParseError(peek(), "expected a word"));
            }
        } else {
            Optional<Token> word;
            Optional<Token> typeName;
            if ((word = matchWord())) {
                if (argumentNames.find(word.value().text) != argumentNames.end()) {
                    return emitError(ParseError(
                        word.value(), "duplicate argument names in function declaration"));
                }
                argumentNames.insert(word.value().text);
                if (match({Token::Type::Colon})) {
                    if (auto result = consumeWord()) {
                        typeName = result.value();
                    } else {
                        return emitError(ParseError(peek(), "expected a type name"));
                    }
                }
            } else if (match({Token::Type::Colon})) {
                if (auto result = consumeWord()) {
                    typeName = result.value();
                } else {
                    return emitError(ParseError(peek(), "expected a type name"));
                }
            }
            if (!consume(Token::Type::RightBrace)) {
                return emitError(ParseError(peek(), Concat("expected ", Quoted("}"))));
            }
            signature.terms.push_back(Signature::Argument{word, typeName});
        }
    }
    if (signature.terms.size() == 0) {
        return emitError(
            ParseError(peek(), Concat("expected a word, ", Quoted("("), ", or ", Quoted("{"))));
    }
    if (match({Token::Type::Arrow})) {
        if (auto word = consumeWord()) {
            signature.typeName = word.value();
        } else {
            return emitError(ParseError(peek(), "expected a type name"));
        }
    }
    return signature;
}

Owned<Statement> Parser::parseBlock(const Parser::TokenList &endTypes) {
    std::vector<Owned<Statement>> statements;
    while (!isAtEnd()) {
        if (match({Token::Type::NewLine})) {
            continue;
        }
        if (check(endTypes)) {
            break;
        }
        if (auto statement = parseStatement()) {
            statements.push_back(std::move(statement));
        }
    }
    return MakeOwned<Block>(std::move(statements));
}

Owned<Statement> Parser::parseStatement() {
    if (match({Token::Type::Function})) {
        bool wasParsingRepeat = _parsingRepeat;
        _parsingRepeat = false;
        auto statement = parseFunction();
        _parsingRepeat = wasParsingRepeat;
        return statement;
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
    if (!statement) {
        emitError(statement.error());
        synchronize();
        return nullptr;
    }
    if (!consumeNewLine()) {
        emitError(ParseError(peek(), "expected a new line"));
        synchronize();
        return nullptr;
    }
    return std::move(statement.value());
}

Owned<Statement> Parser::parseFunction() {
    _parsingDepth++;
    auto signature = parseSignature();
    if (!signature) {
        synchronize();
    } else if (!consumeNewLine()) {
        emitError(ParseError(peek(), "expected a new line"));
        synchronize();
    }

    if (signature) {
        declare(signature.value());
    }

    beginScope();
    auto statement = parseBlock({Token::Type::End});
    _parsingDepth--;
    if (!consumeEnd(Token::Type::Function)) {
        emitError(ParseError(peek(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    endScope();

    if (signature) {
        return MakeOwned<FunctionDecl>(signature.value(), std::move(statement));
    }
    return nullptr;
}

Owned<Statement> Parser::parseIf() {
    auto location = previous().location;
    auto condition = parseExpression();

    _parsingDepth++;
    if (!condition) {
        emitError(condition.error());
        synchronize({Token::Type::Then, Token::Type::NewLine});
        return nullptr;
    } else {
        consumeNewLine();
        if (!consume(Token::Type::Then)) {
            emitError(ParseError(peek(), Concat("expected ", Quoted("then"))));
            auto token = synchronize({Token::Type::Then, Token::Type::NewLine});
            if (token.isEndOfStatement()) {
                return nullptr;
            }
        }
    }

    Optional<Token> token;
    Owned<Statement> ifClause = nullptr;
    Owned<Statement> elseClause = nullptr;

    if (consumeNewLine()) {
        if (auto statement = parseBlock({Token::Type::End, Token::Type::Else})) {
            ifClause = std::move(statement);
        } else {
            return statement;
        }
        token = match({Token::Type::End, Token::Type::Else});
        if (!token) {
            emitError(
                ParseError(peek(), Concat("expected ", Quoted("end"), " or ", Quoted("else"))));
        } else if (token.value().type == Token::Type::End) {
            match({Token::Type::If});
            _parsingDepth--;
            consumeNewLine();
        }
    } else {
        _parsingDepth--;
        if (auto statement = parseSimpleStatement()) {
            ifClause = std::move(statement.value());
            consumeNewLine();
        } else {
            synchronize();
            emitError(statement.error());
            return nullptr;
        }
    }

    if ((token.has_value() && token.value().type == Token::Type::Else) ||
        (!token.has_value() && match({Token::Type::Else}))) {
        if (consumeNewLine()) {
            if (auto statement = parseBlock({Token::Type::End})) {
                elseClause = std::move(statement);
            } else {
                return statement;
            }

            if (!consumeEnd(Token::Type::If)) {
                emitError(ParseError(peek(), Concat("expected ", Quoted("end"))));
                return nullptr;
            }
            _parsingDepth--;
            if (!consumeNewLine()) {
                emitError(ParseError(peek(), "expected new line or end of script"));
                synchronize();
                return nullptr;
            }
        } else {
            _parsingDepth--;
            if (match({Token::Type::If})) {
                if (auto statement = parseIf()) {
                    ifClause = std::move(statement);
                } else {
                    return statement;
                }
            } else {
                if (auto statement = parseSimpleStatement()) {
                    elseClause = std::move(statement.value());
                    consumeNewLine();
                } else {
                    emitError(statement.error());
                    synchronize();
                    return nullptr;
                }
            }
        }
    }

    auto ifStatement =
        MakeOwned<If>(std::move(condition.value()), std::move(ifClause), std::move(elseClause));
    ifStatement->location = location;
    return ifStatement;
}

Owned<Statement> Parser::parseRepeat() {
    auto location = previous().location;

    _parsingDepth++;
    Optional<Token> token;
    if (consumeNewLine() || (token = match({Token::Type::Forever}))) {
        if (token.has_value() && !consumeNewLine()) {
            emitError(ParseError(peek(), "expected a new line"));
            synchronize();
        }
        auto statement = parseRepeatForever();
        if (statement) {
            statement->location = location;
        }
        return statement;
    }
    if ((token = match({Token::Type::While, Token::Type::Until}))) {
        auto statement = parseRepeatConditional();
        if (statement) {
            statement->location = location;
        }
        return statement;
    }
    if (match({Token::Type::For})) {
        auto statement = parseRepeatFor();
        if (statement) {
            statement->location = location;
        }
        return statement;
    }

    emitError(ParseError(peek(), Concat("expected ", Quoted("forever"), ", ", Quoted("while"), ", ",
                                        Quoted("until"), ", ", Quoted("for"), ", or a new line")));
    synchronize();
    return parseRepeatForever();
}

Owned<Statement> Parser::parseRepeatForever() {
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(ParseError(peek(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    auto repeat = MakeOwned<Repeat>(std::move(statement));
    return repeat;
}

Owned<Statement> Parser::parseRepeatConditional() {
    bool conditionValue = (previous().type == Token::Type::While ? true : false);
    auto condition = parseExpression();
    if (!condition) {
        emitError(condition.error());
        synchronize();
    } else if (!consumeNewLine()) {
        emitError(ParseError(peek(), "expected a new line"));
        synchronize();
    }
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(ParseError(peek(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    if (condition) {
        auto repeat = MakeOwned<RepeatCondition>(std::move(statement), std::move(condition.value()),
                                                 conditionValue);
        return repeat;
    }
    return nullptr;
}

Owned<Statement> Parser::parseRepeatFor() {
    Owned<Variable> variable;
    Owned<Expression> expression;
    auto token = consumeWord();
    if (!token) {
        emitError(ParseError(peek(), "expected a word"));
        synchronize();
    } else {
        variable = MakeOwned<Variable>(token.value());
        variable->location = token.value().location;
        if (!consume(Token::Type::In)) {
            emitError(ParseError(peek(), Concat("expected ", Quoted("in"))));
            synchronize();
        }
    }
    if (variable) {
        auto list = parseList();
        if (!list) {
            emitError(list.error());
            synchronize();
        } else {
            expression = std::move(list.value());
            if (!consumeNewLine()) {
                emitError(ParseError(peek(), "expected a new line"));
                synchronize();
            }
        }
    }
    auto statement = parseBlock({Token::Type::End});
    if (!statement) {
        return statement;
    }
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(ParseError(peek(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    auto repeat =
        MakeOwned<RepeatFor>(std::move(statement), std::move(variable), std::move(expression));
    return repeat;
}

Result<Owned<Statement>, ParseError> Parser::parseSimpleStatement() {
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

Result<Owned<Statement>, ParseError> Parser::parseAssignment() {
    auto location = previous().location;
    Variable::Scope scope = Variable::Scope::Unspecified;
    if (match({Token::Type::Global})) {
        scope = Variable::Scope::Global;
    } else if (match({Token::Type::Local})) {
        scope = Variable::Scope::Local;
    }
    auto token = consumeWord();
    if (!token) {
        return Error(ParseError(peek(), "expected a variable name"));
    }
    Optional<Token> typeName;
    std::vector<Owned<Expression>> subscripts;
    if (match({Token::Type::Colon})) {
        if (auto word = consumeWord()) {
            typeName = word.value();
        } else {
            emitError(ParseError(peek(), "expected a type name"));
            synchronize();
            return nullptr;
        }
    } else {
        while (match({Token::Type::LeftBracket})) {
            auto subscript = parseExpression();
            if (!subscript) {
                return Error(subscript.error());
            }
            subscripts.push_back(std::move(subscript.value()));
            if (!consume(Token::Type::RightBracket)) {
                return Error(ParseError(peek(), Concat("expected ", Quoted("]"))));
            }
        }
    }
    if (!consume(Token::Type::To)) {
        return Error(ParseError(peek(), Concat("expected ", Quoted("to"))));
    }
    auto expression = parseExpression();
    if (!expression) {
        return Error(expression.error());
    }
    auto variable = MakeOwned<Variable>(token.value(), typeName, scope);
    variable->location = token.value().location;
    auto assignment = MakeOwned<Assignment>(std::move(variable), std::move(subscripts),
                                            std::move(expression.value()));
    assignment->location = location;
    return assignment;
}

Result<Owned<Statement>, ParseError> Parser::parseExit() {
    auto location = previous().location;
    if (!_parsingRepeat) {
        return Error(
            ParseError(previous(), Concat("unexpected ", Quoted("exit"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Error(ParseError(peek(), Concat("expected ", Quoted("repeat"))));
    }
    auto exitRepeat = MakeOwned<ExitRepeat>();
    exitRepeat->location = location;
    return exitRepeat;
}

Result<Owned<Statement>, ParseError> Parser::parseNext() {
    auto location = previous().location;
    if (!_parsingRepeat) {
        return Error(
            ParseError(previous(), Concat("unexpected ", Quoted("next"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Error(ParseError(peek(), Concat("expected ", Quoted("repeat"))));
    }
    auto nextRepeat = MakeOwned<NextRepeat>();
    nextRepeat->location = location;
    return nextRepeat;
}

Result<Owned<Statement>, ParseError> Parser::parseReturn() {
    auto location = previous().location;
    Owned<Expression> expression;
    if (!isAtEnd() && !check({Token::Type::NewLine})) {
        if (auto returnExpression = parseExpression()) {
            expression = std::move(returnExpression.value());
        } else {
            return Error(returnExpression.error());
        }
    }
    auto returnStatement = MakeOwned<Return>(std::move(expression));
    returnStatement->location = location;
    return returnStatement;
}

Result<Owned<Statement>, ParseError> Parser::parseExpressionStatement() {
    auto expression = parseExpression();
    if (!expression) {
        return Error(expression.error());
    }
    auto statement = MakeOwned<ExpressionStatement>(std::move(expression.value()));
    statement->location = statement->expression->location;
    return statement;
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

Result<Owned<Expression>, ParseError> Parser::parseExpression() { return parseClause(); }

Result<Owned<Expression>, ParseError> Parser::parseClause() {
    auto expression = parseEquality();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken = match({Token::Type::And, Token::Type::Or})) {
        auto equality = parseEquality();
        if (!equality) {
            return equality;
        }
        expression =
            MakeOwned<Binary>(std::move(expression.value()), binaryOp(operatorToken.value().type),
                              std::move(equality.value()));
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseEquality() {
    auto expression = parseComparison();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken =
               match({Token::Type::Equal, Token::Type::NotEqual, Token::Type::Is})) {
        if (operatorToken.value().type == Token::Type::Is && match({Token::Type::Not})) {
            auto comparison = parseComparison();
            if (!comparison) {
                return comparison;
            }
            expression =
                MakeOwned<Binary>(std::move(expression.value()), Binary::Operator::NotEqual,
                                  std::move(comparison.value()));
        } else {
            auto comparison = parseComparison();
            if (!comparison) {
                return comparison;
            }
            expression = MakeOwned<Binary>(std::move(expression.value()),
                                           binaryOp(operatorToken.value().type),
                                           std::move(comparison.value()));
        }
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseComparison() {
    auto expression = parseList();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken =
               match({Token::Type::LessThan, Token::Type::GreaterThan, Token::Type::LessThanOrEqual,
                      Token::Type::GreaterThanOrEqual})) {
        auto list = parseList();
        if (!list) {
            return list;
        }
        expression =
            MakeOwned<Binary>(std::move(expression.value()), binaryOp(operatorToken.value().type),
                              std::move(list.value()));
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseList() {
    auto expression = parseRange();
    if (!expression) {
        return expression;
    }
    if (check({Token::Type::Comma})) {
        auto location = expression.value()->location;

        std::vector<Owned<Expression>> expressions;
        expressions.push_back(std::move(expression.value()));
        while (match({Token::Type::Comma})) {
            auto range = parseRange();
            if (!range) {
                return range;
            }
            expressions.push_back(std::move(range.value()));
        }

        expression = MakeOwned<ListLiteral>(std::move(expressions));
        expression.value()->location = location;
    }

    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseRange() {
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto rangeOperator = match({Token::Type::ThreeDots, Token::Type::ClosedRange})) {
        bool closed = rangeOperator.value().type == Token::Type::ThreeDots ? true : false;
        auto term = parseTerm();
        if (!term) {
            return term;
        }
        expression =
            MakeOwned<RangeLiteral>(std::move(expression.value()), std::move(term.value()), closed);
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseTerm() {
    auto expression = parseFactor();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken = match({Token::Type::Plus, Token::Type::Minus})) {
        auto factor = parseFactor();
        if (!factor) {
            return factor;
        }
        expression =
            MakeOwned<Binary>(std::move(expression.value()), binaryOp(operatorToken.value().type),
                              std::move(factor.value()));
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseFactor() {
    auto expression = parseExponent();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken =
               match({Token::Type::Star, Token::Type::Slash, Token::Type::Percent})) {
        auto exponent = parseExponent();
        if (!exponent) {
            return exponent;
        }
        expression =
            MakeOwned<Binary>(std::move(expression.value()), binaryOp(operatorToken.value().type),
                              std::move(exponent.value()));
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseExponent() {
    auto expression = parseUnary();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken = match({Token::Type::Carrot})) {
        auto unary = parseUnary();
        if (!unary) {
            return unary;
        }
        expression =
            MakeOwned<Binary>(std::move(expression.value()), binaryOp(operatorToken.value().type),
                              std::move(unary.value()));
        expression.value()->location = location;
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parseUnary() {
    if (auto operatorToken = match({Token::Type::Minus, Token::Type::Not})) {
        auto unary = parseUnary();
        if (!unary) {
            return unary;
        }
        auto expression =
            MakeOwned<Unary>(unaryOp(operatorToken.value().type), std::move(unary.value()));
        expression->location = operatorToken.value().location;
        return expression;
    }
    return parseCall();
}

Result<Owned<Expression>, ParseError> Parser::parseCall() {
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
        candidates = Filter(candidates, [&](Candidate &candidate) {
            return checkTerm(peek(), primaries.size(), candidate);
        });
#if defined(DEBUG)
        if (_config.enableTracing && candidates.size() > 1) {
            _trace("Candidates: ");
            std::vector<std::string> candidateNames;
            for (const auto &candidate : candidates) {
                candidateNames.push_back(Concat("  ", candidate.signature.name()));
                if (candidateNames.size() == 5) {
                    candidateNames.push_back(
                        Concat("  ... (", candidates.size() - candidateNames.size(), " more)"));
                    break;
                }
            }
            _trace(Join(candidateNames, "\n"));
        }
#endif
        if (candidates.size() == 1 && candidates[0].isComplete()) {
            // Special case lower parsing precedence for trailing arguments.
            // This allows chaining calls and right associativity.
            if (candidates[0].signature.endsWithArgument()) {
                auto list = parseList();
                if (!list) {
                    return list;
                }
                primaries.push_back(std::move(list.value()));
            } else {
                auto subscript = parseSubscript();
                if (!subscript) {
                    return subscript;
                }
                primaries.push_back(std::move(subscript.value()));
            }
            break;
        } else if (candidates.size() > 0) {
            auto subscript = parseSubscript();
            if (!subscript) {
                return subscript;
            }
            primaries.push_back(std::move(subscript.value()));
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
        return Error(ParseError(startToken, "no matching function for expression"));
    }

    // Partial match of multiple signatures.
    if (candidates.size() > 1) {
        std::vector<std::string> ambiguousList;
        for (const auto &candidate : candidates) {
            ambiguousList.push_back(Concat("  ", candidate.signature.name()));
        }
        return Error(ParseError(startToken, Concat("ambiguous expression. Possible candidates:\n",
                                                   Join(ambiguousList, "\n"))));
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

Result<Owned<Expression>, ParseError> Parser::parseSubscript() {
    auto expression = parsePrimary();
    if (!expression) {
        return expression;
    }
    auto location = expression.value()->location;
    while (auto operatorToken = match({Token::Type::LeftBracket})) {
        auto subscript = parseExpression();
        if (!subscript) {
            return subscript;
        }
        expression = MakeOwned<Binary>(std::move(expression.value()), Binary::Subscript,
                                       std::move(subscript.value()));
        expression.value()->location = location;
        if (!consume(Token::Type::RightBracket)) {
            return Error(ParseError(peek(), Concat("expected ", Quoted("]"))));
        }
    }
    return expression;
}

Result<Owned<Expression>, ParseError> Parser::parsePrimary() {
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
        return parseContainerLiteral();
    }

    if (match({Token::Type::Global})) {
        auto token = consumeWord();
        if (!token) {
            return Error(ParseError(peek(), "expected a variable name"));
        }
        auto variable = MakeOwned<Variable>(token.value(), None, Variable::Scope::Global);
        variable->location = token.value().location;
        return variable;
    }

    if (match({Token::Type::Local})) {
        auto token = consumeWord();
        if (!token) {
            return Error(ParseError(peek(), "expected a variable name"));
        }
        auto variable = MakeOwned<Variable>(token.value(), None, Variable::Scope::Local);
        variable->location = token.value().location;
        return variable;
    }

    if (peek().isWord()) {
        auto token = advance();
        auto variable = MakeOwned<Variable>(token);
        variable->location = token.location;
        return variable;
    }

    return Error(ParseError(peek(), Concat("unexpected ", peek().description())));
}

Result<Owned<Expression>, ParseError> Parser::parseGrouping() {
    auto expression = parseExpression();
    if (!expression) {
        return expression;
    }
    if (!consume(Token::Type::RightParen)) {
        return Error(ParseError(peek(), Concat("expected ", Quoted(")"))));
    }
    auto grouping = MakeOwned<Grouping>(std::move(expression.value()));
    grouping->location = grouping->expression->location;
    return grouping;
}

Result<Owned<Expression>, ParseError> Parser::parseContainerLiteral() {
    if (match({Token::Type::RightBrace})) {
        return MakeOwned<ListLiteral>();
    }
    if (match({Token::Type::Colon})) {
        if (!consume(Token::Type::RightBrace)) {
            return Error(ParseError(peek(), Concat("expected ", Quoted("}"))));
        }
        return MakeOwned<DictionaryLiteral>();
    }

    Owned<Expression> containerExpression;
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }

    if (match({Token::Type::Colon})) {
        Mapping<Owned<Expression>, Owned<Expression>> values;
        auto valueExpression = parseTerm();
        if (!valueExpression) {
            return valueExpression;
        }
        values[std::move(expression.value())] = std::move(valueExpression.value());
        if (consume(Token::Type::Comma)) {
            do {
                auto keyExpression = parseTerm();
                if (!keyExpression) {
                    return keyExpression;
                }
                if (!consume(Token::Type::Colon)) {
                    return Error(ParseError(peek(), Concat("expected ", Quoted(":"))));
                }
                auto valueExpression = parseTerm();
                if (!valueExpression) {
                    return valueExpression;
                }
                values[std::move(keyExpression.value())] = std::move(valueExpression.value());
            } while (match({Token::Type::Comma}));
        }
        containerExpression = MakeOwned<DictionaryLiteral>(std::move(values));
    } else if (match({Token::Type::Comma})) {
        std::vector<Owned<Expression>> values;
        values.push_back(std::move(expression.value()));
        do {
            auto expression = parseTerm();
            if (!expression) {
                return expression;
            }
            values.push_back(std::move(expression.value()));
        } while (match({Token::Type::Comma}));

        containerExpression = MakeOwned<ListLiteral>(std::move(values));
    } else if (check({Token::Type::RightBrace})) {
        std::vector<Owned<Expression>> values;
        values.push_back(std::move(expression.value()));
        containerExpression = MakeOwned<ListLiteral>(std::move(values));
    } else {
        return Error(ParseError(peek(), Concat("expected ", Quoted(":"), " or ", Quoted(","))));
    }

    if (!consume(Token::Type::RightBrace)) {
        return Error(ParseError(peek(), Concat("expected ", Quoted("}"))));
    }
    return containerExpression;
}

SIF_NAMESPACE_END
