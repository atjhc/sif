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
#include "utilities/strings.h"

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

Parser::Parser(const ParserConfig &config) : _config(config) {
    _parsingRepeat = false;
    _recording = false;
    _index = 0;
    _parsingDepth = 0;
    _scopes.push_back(Scope());
    _grammar.argument = MakeOwned<Grammar>();
    _variables.insert("it");
    _variables.insert("empty");
}

Parser::~Parser() {}

Owned<Statement> Parser::statement() {
    auto error = _config.reader.read(0);
    if (error) {
        emitError(error.value());
        return nullptr;
    }
    _config.scanner.reset(_config.reader.contents());

    auto block = parseBlock({});
    if (_failed) {
        return nullptr;
    }
    return block;
}

Optional<Signature> Parser::signature() {
    auto error = _config.reader.read(0);
    if (error) {
        emitError(error.value());
        return None;
    }
    _config.scanner.reset(_config.reader.contents());

    auto signature = parseSignature();
    if (_failed) {
        return None;
    }
    return signature;
}

void Parser::declare(const Signature &signature) {
    _scopes.back().signatures.push_back(signature);
    _grammar.insert(signature);
}

const std::vector<Signature> &Parser::declarations() const { return _exportedDeclarations; }

#pragma mark - Utilities

Optional<Token> Parser::match(const Parser::TokenTypes &types) {
    if (check(types)) {
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
    if (isAtEnd() && _parsingDepth > 0 && _config.reader.readable()) {
        auto error = _config.reader.read(_parsingDepth);
        if (error) {
            emitError(error.value());
            return false;
        }
        _config.scanner.reset(_config.reader.contents());
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

bool Parser::check(const Parser::TokenTypes &types) {
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
    auto token = _config.scanner.scan();
    _tokens.push_back(token);
    trace(Concat("Scanned: ", Describe(token)));
#if defined(DEBUG)
    if (_config.enableTracing) {
        if (token.type == Token::Type::Error) {
            _trace(Concat("Error: ", token.text));
        }
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

Token Parser::synchronize(const Parser::TokenTypes &tokenTypes) {
    trace("Synchronizing");
    _recording = false;
    while (!isAtEnd()) {
        if (match(tokenTypes)) {
            break;
        }
        advance();
    }
    trace("end Synchronizing");
    return previous();
}

NoneType Parser::emitError(const Error &error) {
    _failed = true;
    _config.reporter.report(error);
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

void Parser::beginScope(const Parser::Scope &scope) { _scopes.push_back(scope); }

void Parser::endScope() {
    _scopes.pop_back();

    _variables.clear();
    _variables.insert("it");
    _variables.insert("empty");

    _grammar = Grammar();
    _grammar.argument = MakeOwned<Grammar>();

    for (auto &&scope : _scopes) {
        for (auto &&signature : scope.signatures) {
            _grammar.insert(signature, signature.terms.begin());
        }
        for (auto &&variable : scope.variables) {
            _variables.insert(variable);
        }
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
                    return emitError(Error(peek().location, "expected a word"));
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
                    return emitError(Error(peek().location, Concat("expected ", Quoted(")"))));
                }
            } else {
                return emitError(Error(peek().location, "expected a word"));
            }
        } else {
            Optional<Token> word;
            Optional<Token> typeName;
            if ((word = consumeWord())) {
                if (argumentNames.find(word.value().text) != argumentNames.end()) {
                    return emitError(Error(word.value().location,
                                           "duplicate argument names in function declaration"));
                }
                argumentNames.insert(word.value().text);
                if (match({Token::Type::Colon})) {
                    if (auto result = consumeWord()) {
                        typeName = result.value();
                    } else {
                        return emitError(Error(peek().location, "expected a type name"));
                    }
                }
            } else if (match({Token::Type::Colon})) {
                if (auto result = consumeWord()) {
                    typeName = result.value();
                } else {
                    return emitError(Error(peek().location, "expected a type name"));
                }
            }
            if (!consume(Token::Type::RightBrace)) {
                return emitError(Error(peek().location, Concat("expected ", Quoted("}"))));
            }
            signature.terms.push_back(Signature::Argument{word, typeName});
        }
    }
    if (signature.terms.size() == 0) {
        return emitError(
            Error(peek().location, Concat("expected a word, ", Quoted("("), ", or ", Quoted("{"))));
    }
    if (match({Token::Type::Arrow})) {
        if (auto word = consumeWord()) {
            signature.typeName = word.value();
        } else {
            return emitError(Error(peek().location, "expected a type name"));
        }
    }
    return signature;
}

Owned<Statement> Parser::parseBlock(const Parser::TokenTypes &endTypes) {
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
    if (match({Token::Type::Try})) {
        return parseTry();
    }
    if (match({Token::Type::Repeat})) {
        bool wasParsingRepeat = _parsingRepeat;
        _parsingRepeat = true;
        auto statement = parseRepeat();
        _parsingRepeat = wasParsingRepeat;
        return statement;
    }
    if (match({Token::Type::Use})) {
        return parseUse();
    }
    if (match({Token::Type::Using})) {
        return parseUsing();
    }

    auto statement = parseSimpleStatement();
    if (!statement) {
        emitError(statement.error());
        synchronize();
        return nullptr;
    }
    if (!consumeNewLine()) {
        emitError(Error(peek().location, "expected a new line"));
        synchronize();
        return nullptr;
    }
    return std::move(statement.value());
}

Owned<Statement> Parser::parseFunction() {
    auto location = previous().location;

    _parsingDepth++;
    auto signature = parseSignature();
    if (!signature) {
        synchronize();
    } else if (!consumeNewLine()) {
        emitError(Error(peek().location, "expected a new line"));
        synchronize();
    }

    if (signature) {
        _scopes.back().signatures.push_back(signature.value());
        if (_scopes.size() == 1) {
            _exportedDeclarations.push_back(signature.value());
        }
        // TODO: How should we handle ambiguous redefinitions?
        _grammar.insert(signature.value());
    }

    beginScope();
    auto statement = parseBlock({Token::Type::End});
    _parsingDepth--;
    if (!consumeEnd(Token::Type::Function)) {
        emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    endScope();

    if (signature) {
        auto decl = MakeOwned<FunctionDecl>(signature.value(), std::move(statement));
        decl->location = location;
        return decl;
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
            emitError(Error(peek().location, Concat("expected ", Quoted("then"))));
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
                Error(peek().location, Concat("expected ", Quoted("end"), " or ", Quoted("else"))));
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
                emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
                return nullptr;
            }
            _parsingDepth--;
            if (!consumeNewLine()) {
                emitError(Error(peek().location, "expected new line or end of script"));
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

Owned<Statement> Parser::parseUse() {
    auto location = previous().location;
    Optional<Token> token = consume(Token::Type::StringLiteral);
    if (!token) {
        emitError(Error(peek().location, "expected a string literal"));
        synchronize();
        return nullptr;
    }
    consumeNewLine();

    std::vector<Error> outErrors;
    auto source = token.value().encodedString();
    auto module = _config.moduleProvider.module(source);
    if (module && module.value()) {
        Append(_scopes.back().signatures, module.value()->signatures());
    } else if (!module) {
        emitError(Error(location, module.error().what()));
    }

    auto useStatement = MakeOwned<Use>(token.value());
    useStatement->location = location;
    return useStatement;
}

Owned<Statement> Parser::parseUsing() {
    auto location = previous().location;
    Optional<Token> token = consume(Token::Type::StringLiteral);
    if (!token) {
        emitError(Error(peek().location, "Expected a string literal"));
        synchronize();
        return nullptr;
    }

    std::vector<Error> outErrors;
    auto source = token.value().encodedString();
    auto module = _config.moduleProvider.module(source);
    if (module && module.value()) {
        beginScope(Scope{module.value()->signatures()});
    } else if (!module) {
        emitError(Error(location, module.error().what()));
    }

    Owned<Statement> statement;
    if (consumeNewLine()) {
        statement = parseBlock({Token::Type::End});
        if (!consumeEnd(Token::Type::Using)) {
            emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
            return nullptr;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            statement = std::move(result.value());
            consumeNewLine();
        } else {
            synchronize();
            emitError(result.error());
            return nullptr;
        }
    }

    if (module) {
        endScope();
    }

    auto usingStatement = MakeOwned<Using>(token.value(), std::move(statement));
    usingStatement->location = location;
    return usingStatement;
}

Owned<Statement> Parser::parseTry() {
    auto location = previous().location;

    Owned<Statement> statement;
    if (consumeNewLine()) {
        statement = parseBlock({Token::Type::End});
        if (!consumeEnd(Token::Type::Try)) {
            emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
            return nullptr;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            statement = std::move(result.value());
            consumeNewLine();
        } else {
            synchronize();
            emitError(result.error());
            return nullptr;
        }
    }

    auto tryStatement = MakeOwned<Try>(std::move(statement));
    tryStatement->location = location;
    return tryStatement;
}

Owned<Statement> Parser::parseRepeat() {
    auto location = previous().location;

    _parsingDepth++;
    Optional<Token> token;
    if (consumeNewLine() || (token = match({Token::Type::Forever}))) {
        if (token.has_value() && !consumeNewLine()) {
            emitError(Error(peek().location, "expected a new line"));
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

    emitError(
        Error(peek().location, Concat("expected ", Quoted("forever"), ", ", Quoted("while"), ", ",
                                      Quoted("until"), ", ", Quoted("for"), ", or a new line")));
    synchronize();
    return parseRepeatForever();
}

Owned<Statement> Parser::parseRepeatForever() {
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
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
        emitError(Error(peek().location, "expected a new line"));
        synchronize();
    }
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
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
        emitError(Error(peek().location, "expected a word"));
        synchronize();
    } else {
        variable = MakeOwned<Variable>(token.value());
        variable->location = token.value().location;
        if (!consume(Token::Type::In)) {
            emitError(Error(peek().location, Concat("expected ", Quoted("in"))));
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
                emitError(Error(peek().location, "expected a new line"));
                synchronize();
            }
        }
    }
    auto statement = parseBlock({Token::Type::End});
    if (!statement) {
        return statement;
    }
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    auto repeat =
        MakeOwned<RepeatFor>(std::move(statement), std::move(variable), std::move(expression));
    return repeat;
}

Result<Owned<Statement>, Error> Parser::parseSimpleStatement() {
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

Result<Owned<Statement>, Error> Parser::parseAssignment() {
    auto location = previous().location;
    Variable::Scope scope = Variable::Scope::Unspecified;
    if (match({Token::Type::Global})) {
        scope = Variable::Scope::Global;
    } else if (match({Token::Type::Local})) {
        scope = Variable::Scope::Local;
    }
    auto token = consumeWord();
    if (!token) {
        return Fail(Error(peek().location, "expected a variable name"));
    }
    Optional<Token> typeName;
    std::vector<Owned<Expression>> subscripts;
    if (match({Token::Type::Colon})) {
        if (auto word = consumeWord()) {
            typeName = word.value();
        } else {
            emitError(Error(peek().location, "expected a type name"));
            synchronize();
            return nullptr;
        }
    } else {
        while (match({Token::Type::LeftBracket})) {
            auto subscript = parseExpression();
            if (!subscript) {
                return Fail(subscript.error());
            }
            subscripts.push_back(std::move(subscript.value()));
            if (!consume(Token::Type::RightBracket)) {
                return Fail(Error(peek().location, Concat("expected ", Quoted("]"))));
            }
        }
    }
    if (!consume(Token::Type::To)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted("to"))));
    }
    if (subscripts.size() == 0) {
        auto name = lowercase(token.value().text);
        _scopes.back().variables.insert(name);
        _variables.insert(name);
    }
    auto expression = parseExpression();
    if (!expression) {
        return Fail(expression.error());
    }
    auto variable = MakeOwned<Variable>(token.value(), typeName, scope);
    variable->location = token.value().location;
    auto assignment = MakeOwned<Assignment>(std::move(variable), std::move(subscripts),
                                            std::move(expression.value()));
    assignment->location = location;
    return assignment;
}

Result<Owned<Statement>, Error> Parser::parseExit() {
    auto location = previous().location;
    if (!_parsingRepeat) {
        return Fail(Error(previous().location,
                          Concat("unexpected ", Quoted("exit"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted("repeat"))));
    }
    auto exitRepeat = MakeOwned<ExitRepeat>();
    exitRepeat->location = location;
    return exitRepeat;
}

Result<Owned<Statement>, Error> Parser::parseNext() {
    auto location = previous().location;
    if (!_parsingRepeat) {
        return Fail(Error(previous().location,
                          Concat("unexpected ", Quoted("next"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted("repeat"))));
    }
    auto nextRepeat = MakeOwned<NextRepeat>();
    nextRepeat->location = location;
    return nextRepeat;
}

Result<Owned<Statement>, Error> Parser::parseReturn() {
    auto location = previous().location;
    Owned<Expression> expression;
    if (!isAtEnd() && !check({Token::Type::NewLine})) {
        if (auto returnExpression = parseExpression()) {
            expression = std::move(returnExpression.value());
        } else {
            return Fail(returnExpression.error());
        }
    }
    auto returnStatement = MakeOwned<Return>(std::move(expression));
    returnStatement->location = location;
    return returnStatement;
}

Result<Owned<Statement>, Error> Parser::parseExpressionStatement() {
    auto expression = parseExpression();
    if (!expression) {
        return Fail(expression.error());
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

Result<Owned<Expression>, Error> Parser::parseExpression() { return parseClause(); }

Result<Owned<Expression>, Error> Parser::parseClause() {
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

Result<Owned<Expression>, Error> Parser::parseEquality() {
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

Result<Owned<Expression>, Error> Parser::parseComparison() {
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

Result<Owned<Expression>, Error> Parser::parseList() {
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

Result<Owned<Expression>, Error> Parser::parseRange() {
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

Result<Owned<Expression>, Error> Parser::parseTerm() {
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

Result<Owned<Expression>, Error> Parser::parseFactor() {
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

Result<Owned<Expression>, Error> Parser::parseExponent() {
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

Result<Owned<Expression>, Error> Parser::parseUnary() {
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

bool Parser::Grammar::insert(const Signature &signature, std::vector<Signature::Term>::const_iterator term) {
    if (term == signature.terms.end()) {
        if (this->signature) {
            return false;
        }
        this->signature = signature;
        return true;
    }

    bool result = true;
    auto insertToken = [&](Token token) {
        auto word = lowercase(token.text);
        auto it = terms.find(word);
        if (it == terms.end()) {
            auto grammar = MakeOwned<Grammar>();
            grammar->insert(signature, std::next(term));
            terms[word] = std::move(grammar);
            return;
        }
        if(!it->second->insert(signature, std::next(term))) {
            result = false;
        }
    };
    auto insertArgument = [&](Signature::Argument) {
        if (argument) {
            if (!argument->insert(signature, std::next(term))) {
                result = false;
            }
            return;
        }
        argument = MakeOwned<Grammar>();
        argument->insert(signature, std::next(term));
    };
    std::visit(Overload{
                   insertToken,
                   insertArgument,
                   [&](Signature::Choice choice) {
                       for (auto &token : choice.tokens) {
                           insertToken(token);
                       }
                   },
                   [&](Signature::Option argument) {
                        insertToken(argument.token);
                        if(!insert(signature, std::next(term))) {
                            result = false;
                        }
                   },
               },
               *term);
    return result;
}

bool Parser::Grammar::isLeaf() const {
    return argument == nullptr && terms.size() == 0;
}

Result<Owned<Expression>, Error> Parser::parseCall() {
    std::vector<Owned<Expression>> arguments;
    Signature matchingSignature;

    auto grammar = &_grammar;
    auto token = peek();
    auto location = token.location;

    while (token.isPrimary()) {
        if (token.isWord()) {
            auto word = lowercase(token.text);
            auto variable = _variables.find(word);
            if (variable != _variables.end() && grammar->argument) {
                auto argument = (grammar->argument->isLeaf() && grammar != &_grammar ? parseList() : parseSubscript());
                if (!argument) {
                    return argument;
                }
                arguments.push_back(std::move(argument.value()));
                matchingSignature.terms.emplace_back(Signature::Argument());
                grammar = grammar->argument.get();
                token = peek();
                continue;
            }
            auto term = grammar->terms.find(word);
            if (term != grammar->terms.end()) {
                matchingSignature.terms.emplace_back(token);
                grammar = term->second.get();
                advance();
                token = peek();
                continue;
            }
        }
        if (grammar->argument) {
            auto argument = (grammar->argument->isLeaf() && grammar != &_grammar ? parseList() : parseSubscript());
            if (!argument) {
                return argument;
            }
            arguments.push_back(std::move(argument.value()));
            matchingSignature.terms.emplace_back(Signature::Argument());
            grammar = grammar->argument.get();
            token = peek();
            continue;
        }
        break;
    }
    if (matchingSignature.terms.size() == 1 ) {
        if (arguments.size() == 1) {
            return std::move(arguments[0]);
        }
    }
    auto signature = grammar->signature;
    if (!signature) {
        return Fail(Error(location, Concat("No matching function ", matchingSignature.description())));
    }
    auto call =
        MakeOwned<Call>(signature.value(), std::vector<Optional<Token>>(), std::move(arguments));
    call->location = location;
    return call;
}

Result<Owned<Expression>, Error> Parser::parseSubscript() {
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
            return Fail(Error(peek().location, Concat("expected ", Quoted("]"))));
        }
    }
    return expression;
}

Result<Owned<Expression>, Error> Parser::parsePrimary() {
    if (auto token = match({
            Token::Type::BoolLiteral,
            Token::Type::IntLiteral,
            Token::Type::FloatLiteral,
            Token::Type::StringLiteral,
            Token::Type::Empty,
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

    if (peek().isWord() || peek().type == Token::Type::Global || peek().type == Token::Type::Local) {
        return parseVariable();
    }

    return Fail(Error(peek().location, Concat("unexpected ", peek().description())));
}

Result<Owned<Expression>, Error> Parser::parseVariable() {
    auto scope = Variable::Scope::Unspecified;
    if (peek().type == Token::Type::Global || peek().type == Token::Type::Local) {
        scope = (peek().type == Token::Type::Global ? Variable::Scope::Global : Variable::Scope::Local);
        advance();
    }
    auto token = consumeWord();
    if (!token) {
        return Fail(Error(peek().location, "expected a variable name"));
    }
    auto variable = MakeOwned<Variable>(token.value(), None, scope);
    variable->location = token.value().location;
    return variable;
}

Result<Owned<Expression>, Error> Parser::parseGrouping() {
    auto expression = parseExpression();
    if (!expression) {
        return expression;
    }
    if (!consume(Token::Type::RightParen)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted(")"))));
    }
    auto grouping = MakeOwned<Grouping>(std::move(expression.value()));
    grouping->location = grouping->expression->location;
    return grouping;
}

Result<Owned<Expression>, Error> Parser::parseContainerLiteral() {
    if (match({Token::Type::RightBrace})) {
        return MakeOwned<ListLiteral>();
    }
    if (match({Token::Type::Colon})) {
        if (!consume(Token::Type::RightBrace)) {
            return Fail(Error(peek().location, Concat("expected ", Quoted("}"))));
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
                    return Fail(Error(peek().location, Concat("expected ", Quoted(":"))));
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
        return Fail(Error(peek().location, Concat("expected ", Quoted(":"), " or ", Quoted(","))));
    }

    if (!consume(Token::Type::RightBrace)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted("}"))));
    }
    return containerExpression;
}

SIF_NAMESPACE_END
