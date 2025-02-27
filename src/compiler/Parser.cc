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
    _failed = false;
    _index = 0;
    _parsingDepth = 0;
    _scopes.push_back(Scope());
    _grammar.argument = MakeOwned<Grammar>();
    _variables.insert("it");
    _variables.insert("empty");
}

Parser::~Parser() {}

Strong<Statement> Parser::statement() {
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

void Parser::declare(const std::string &variable) {
    _scopes.back().variables.insert(variable);
    _variables.insert(variable);
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

void Parser::beginScope(const Parser::Scope &scope) {
    _scopes.push_back(scope);
    for (auto &&signature : scope.signatures) {
        _grammar.insert(signature, signature.terms.begin());
    }
}

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
            Signature::Choice choice;
            do {
                if (auto word = consumeWord()) {
                    choice.tokens.push_back(word.value());
                } else {
                    return emitError(Error(peek().location, "expected a word"));
                }
            } while (match({Token::Type::Slash}));
            if (!consume(Token::Type::RightParen)) {
                return emitError(Error(peek().location, Concat("expected ", Quoted(")"))));
            }
            signature.terms.push_back(Signature::Option{choice});
        } else {
            std::vector<Signature::Argument::Target> targets;
            do {
                Optional<Token> name;
                Optional<Token> typeName;
                if ((name = consumeWord())) {
                    if (argumentNames.find(name.value().text) != argumentNames.end()) {
                        return emitError(Error(name.value().location,
                                               "duplicate argument names in function declaration"));
                    }
                    argumentNames.insert(name.value().text);
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
                targets.push_back(Signature::Argument::Target{name, typeName});
            } while (match({Token::Type::Comma}));
            if (!consume(Token::Type::RightBrace)) {
                return emitError(Error(peek().location, Concat("expected ", Quoted("}"))));
            }
            signature.terms.push_back(Signature::Argument{targets});
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

Strong<Statement> Parser::parseBlock(const Parser::TokenTypes &endTypes) {
    std::vector<Strong<Statement>> statements;
    auto start = peek().location;
    while (!isAtEnd()) {
        if (match({Token::Type::NewLine})) {
            continue;
        }
        if (check(endTypes)) {
            break;
        }
        if (auto statement = parseStatement()) {
            statements.push_back(statement);
        }
    }
    auto block = MakeStrong<Block>(statements);
    block->range = SourceRange{start, peek().end()};
    return block;
}

Strong<Statement> Parser::parseStatement() {
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
    return statement.value();
}

Strong<Statement> Parser::parseFunction() {
    auto start = previous().location;

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
    if (signature) {
        for (auto &&argument : signature->arguments()) {
            for (auto &&target : argument.targets) {
                auto token = target.name;
                if (token) {
                    auto name = lowercase(token->text);
                    declare(name);
                }
            }
        }
    }

    auto statement = parseBlock({Token::Type::End});
    _parsingDepth--;
    if (!consumeEnd(Token::Type::Function)) {
        emitError(Error(peek().location, Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    endScope();

    if (signature) {
        auto decl = MakeStrong<FunctionDecl>(signature.value(), statement);
        decl->range = SourceRange{start, previous().end()};
        return decl;
    }
    return nullptr;
}

Strong<Statement> Parser::parseIf() {
    auto start = previous().location;
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
    Strong<Statement> ifClause = nullptr;
    Strong<Statement> elseClause = nullptr;

    if (consumeNewLine()) {
        if (auto statement = parseBlock({Token::Type::End, Token::Type::Else})) {
            ifClause = statement;
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
            ifClause = statement.value();
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
                elseClause = statement;
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
                    ifClause = statement;
                } else {
                    return statement;
                }
            } else {
                if (auto statement = parseSimpleStatement()) {
                    elseClause = statement.value();
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
        MakeStrong<If>(condition.value(), ifClause, elseClause);
    ifStatement->range = SourceRange{start, previous().end()};
    return ifStatement;
}

Strong<Statement> Parser::parseUse() {
    auto start = previous().location;
    Optional<Token> token = match({Token::Type::StringLiteral, Token::Type::Word});
    if (!token) {
        emitError(Error(peek().range(), "expected a string literal or word"));
        synchronize();
        return nullptr;
    }
    consumeNewLine();

    std::vector<Error> outErrors;
    auto source = token.value().encodedStringOrWord();
    auto module = _config.moduleProvider.module(source);
    if (module && module.value()) {
        Append(_scopes.back().signatures, module.value()->signatures());
        for (auto &&signature : module.value()->signatures()) {
            _grammar.insert(signature);
        }
    } else if (!module) {
        emitError(Error(start, module.error().what()));
    }

    auto useStatement = MakeStrong<Use>(token.value());
    useStatement->range = SourceRange{start, previous().end()};
    return useStatement;
}

Strong<Statement> Parser::parseUsing() {
    auto start = previous().location;
    Optional<Token> token = match({Token::Type::StringLiteral, Token::Type::Word});
    if (!token) {
        emitError(Error(peek().range(), "expected a string literal"));
        synchronize();
        return nullptr;
    }

    std::vector<Error> outErrors;
    auto source = token.value().encodedStringOrWord();
    auto module = _config.moduleProvider.module(source);
    if (module && module.value()) {
        beginScope(Scope{module.value()->signatures()});
    } else if (!module) {
        emitError(Error(start, module.error().what()));
    }

    Strong<Statement> statement;
    if (consumeNewLine()) {
        statement = parseBlock({Token::Type::End});
        if (!consumeEnd(Token::Type::Using)) {
            emitError(Error(peek().range(), Concat("expected ", Quoted("end"))));
            return nullptr;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            statement = result.value();
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

    auto usingStatement = MakeStrong<Using>(token.value(), statement);
    usingStatement->range = SourceRange{start, previous().end()};
    return usingStatement;
}

Strong<Statement> Parser::parseTry() {
    auto start = previous().location;

    Strong<Statement> statement;
    if (consumeNewLine()) {
        statement = parseBlock({Token::Type::End});
        if (!consumeEnd(Token::Type::Try)) {
            emitError(Error(peek().range(), Concat("expected ", Quoted("end"))));
            return nullptr;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            statement = result.value();
            consumeNewLine();
        } else {
            synchronize();
            emitError(result.error());
            return nullptr;
        }
    }

    auto tryStatement = MakeStrong<Try>(statement);
    tryStatement->range = SourceRange{start, previous().end()};
    return tryStatement;
}

Strong<Statement> Parser::parseRepeat() {
    auto start = previous().location;

    _parsingDepth++;
    Optional<Token> token;
    if (consumeNewLine() || (token = match({Token::Type::Forever}))) {
        if (token.has_value() && !consumeNewLine()) {
            emitError(Error(peek().range(), "expected a new line"));
            synchronize();
        }
        auto statement = parseRepeatForever();
        if (statement) {
            statement->range = SourceRange{start, previous().end()};
        }
        return statement;
    }
    if ((token = match({Token::Type::While, Token::Type::Until}))) {
        auto statement = parseRepeatConditional();
        if (statement) {
            statement->range = SourceRange{start, previous().end()};
        }
        return statement;
    }
    if (match({Token::Type::For})) {
        auto statement = parseRepeatFor();
        if (statement) {
            statement->range = SourceRange{start, previous().end()};
        }
        return statement;
    }

    emitError(
        Error(peek().range(), Concat("expected ", Quoted("forever"), ", ", Quoted("while"), ", ",
                                     Quoted("until"), ", ", Quoted("for"), ", or a new line")));
    synchronize();
    return parseRepeatForever();
}

Strong<Statement> Parser::parseRepeatForever() {
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().range(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    auto repeat = MakeStrong<Repeat>(statement);
    return repeat;
}

Strong<Statement> Parser::parseRepeatConditional() {
    bool conditionValue = (previous().type == Token::Type::While ? true : false);
    auto condition = parseExpression();
    if (!condition) {
        emitError(condition.error());
        synchronize();
    } else if (!consumeNewLine()) {
        emitError(Error(peek().range(), "expected a new line"));
        synchronize();
    }
    auto statement = parseBlock({Token::Type::End});
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().range(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    if (condition) {
        auto repeat = MakeStrong<RepeatCondition>(statement, condition.value(),
                                                 conditionValue);
        return repeat;
    }
    return nullptr;
}

Strong<Statement> Parser::parseRepeatFor() {
    std::vector<Strong<Variable>> variables;
    Strong<Expression> expression;

    do {
        auto token = consumeWord();
        if (!token) {
            emitError(Error(peek().range(), "expected a word"));
            synchronize();
        } else {
            auto variable = MakeStrong<Variable>(token.value());
            variable->range = SourceRange{token.value().location, token.value().end()};
            auto name = lowercase(variable->name.text);
            declare(name);
            variables.push_back(variable);
        }
    } while (match({Token::Type::Comma}));

    if (!variables.empty()) {
        if (!consume(Token::Type::In)) {
            emitError(Error(peek().range(), Concat("expected ", Quoted("in"))));
            synchronize();
        }

        auto list = parseList();
        if (!list) {
            emitError(list.error());
            synchronize();
        } else {
            expression = list.value();
            if (!consumeNewLine()) {
                emitError(Error(peek().range(), "expected a new line"));
                synchronize();
            }
        }
    }
    auto statement = parseBlock({Token::Type::End});
    if (!statement) {
        return statement;
    }
    if (!consumeEnd(Token::Type::Repeat)) {
        emitError(Error(peek().range(), Concat("expected ", Quoted("end"))));
        return nullptr;
    }
    _parsingDepth--;
    auto repeat =
        MakeStrong<RepeatFor>(statement, variables, expression);
    return repeat;
}

Result<Strong<Statement>, Error> Parser::parseSimpleStatement() {
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

Result<Strong<Statement>, Error> Parser::parseAssignment() {
    auto start = previous().location;
    std::vector<Strong<AssignmentTarget>> variableDecls;
    do {
        Optional<Variable::Scope> scope = None;
        Optional<Token> typeName = None;
        std::vector<Strong<Expression>> subscripts;

        if (match({Token::Type::Global})) {
            scope = Variable::Scope::Global;
        } else if (match({Token::Type::Local})) {
            scope = Variable::Scope::Local;
        }

        auto token = consumeWord();
        if (!token) {
            return Fail(Error(peek().range(), "expected a variable name"));
        }
        if (match({Token::Type::Colon})) {
            if (auto word = consumeWord()) {
                typeName = word.value();
            } else {
                emitError(Error(peek().range(), "expected a type name"));
                synchronize();
                return nullptr;
            }
        } else {
            while (match({Token::Type::LeftBracket})) {
                auto subscript = parseExpression();
                if (!subscript) {
                    return Fail(subscript.error());
                }
                subscripts.push_back(subscript.value());
                if (!consume(Token::Type::RightBracket)) {
                    return Fail(Error(peek().range(), Concat("expected ", Quoted("]"))));
                }
            }
        }
        if (subscripts.size() == 0) {
            auto name = lowercase(token.value().text);
            declare(name);
        }
        auto variable = MakeStrong<Variable>(token.value(), scope);
        variable->range = SourceRange{token.value().location, token.value().end()};
        auto assignmentTarget =
            MakeStrong<AssignmentTarget>(variable, typeName, subscripts);
        assignmentTarget->range =
            SourceRange{assignmentTarget->variable->range.start, previous().end()};
        variableDecls.push_back(assignmentTarget);
    } while (match({Token::Type::Comma}));

    if (!consume(Token::Type::To)) {
        return Fail(Error(peek().range(), Concat("expected ", Quoted("to"))));
    }

    auto expression = parseExpression();
    if (!expression) {
        return Fail(expression.error());
    }

    auto assignment =
        MakeStrong<Assignment>(variableDecls, expression.value());
    assignment->range = SourceRange{start, previous().end()};
    return assignment;
}

Result<Strong<Statement>, Error> Parser::parseExit() {
    auto start = previous().location;
    if (!_parsingRepeat) {
        return Fail(
            Error(peek().range(), Concat("unexpected ", Quoted("exit"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Fail(Error(peek().range(), Concat("expected ", Quoted("repeat"))));
    }
    auto exitRepeat = MakeStrong<ExitRepeat>();
    exitRepeat->range = SourceRange{start, previous().end()};
    return exitRepeat;
}

Result<Strong<Statement>, Error> Parser::parseNext() {
    auto start = previous().location;
    if (!_parsingRepeat) {
        return Fail(
            Error(peek().range(), Concat("unexpected ", Quoted("next"), " outside repeat block")));
    }
    if (!consume(Token::Type::Repeat)) {
        return Fail(Error(peek().location, Concat("expected ", Quoted("repeat"))));
    }
    auto nextRepeat = MakeStrong<NextRepeat>();
    nextRepeat->range = SourceRange{start, previous().end()};
    return nextRepeat;
}

Result<Strong<Statement>, Error> Parser::parseReturn() {
    auto start = previous().location;
    Strong<Expression> expression;
    if (!isAtEnd() && !check({Token::Type::NewLine})) {
        if (auto returnExpression = parseExpression()) {
            expression = returnExpression.value();
        } else {
            return Fail(returnExpression.error());
        }
    }
    auto returnStatement = MakeStrong<Return>(expression);
    returnStatement->range = SourceRange{start, previous().end()};
    return returnStatement;
}

Result<Strong<Statement>, Error> Parser::parseExpressionStatement() {
    auto expression = parseExpression();
    if (!expression) {
        return Fail(expression.error());
    }
    auto statement = MakeStrong<ExpressionStatement>(expression.value());
    statement->range = statement->expression->range;
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

Result<Strong<Expression>, Error> Parser::parseExpression() {
    auto token = peek();
    if (token.isEndOfStatement()) {
        return Fail(Error(peek().range(), "expected expression"));
    }
    return parseClause();
}

Result<Strong<Expression>, Error> Parser::parseClause() {
    auto expression = parseEquality();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken = match({Token::Type::And, Token::Type::Or})) {
        auto equality = parseEquality();
        if (!equality) {
            return equality;
        }
        auto end = equality.value()->range.end;
        expression =
            MakeStrong<Binary>(expression.value(), binaryOp(operatorToken.value().type),
                              equality.value());
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseEquality() {
    auto expression = parseComparison();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    SourceLocation end;
    while (auto operatorToken =
               match({Token::Type::Equal, Token::Type::NotEqual, Token::Type::Is})) {
        if (operatorToken.value().type == Token::Type::Is && match({Token::Type::Not})) {
            auto comparison = parseComparison();
            if (!comparison) {
                return comparison;
            }
            end = comparison.value()->range.end;
            expression =
                MakeStrong<Binary>(expression.value(), Binary::Operator::NotEqual,
                                  comparison.value());
        } else {
            auto comparison = parseComparison();
            if (!comparison) {
                return comparison;
            }
            end = comparison.value()->range.end;
            expression = MakeStrong<Binary>(expression.value(),
                                           binaryOp(operatorToken.value().type),
                                           comparison.value());
        }
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseComparison() {
    auto expression = parseList();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken =
               match({Token::Type::LessThan, Token::Type::GreaterThan, Token::Type::LessThanOrEqual,
                      Token::Type::GreaterThanOrEqual})) {
        auto list = parseList();
        if (!list) {
            return list;
        }
        auto end = list.value()->range.end;
        expression =
            MakeStrong<Binary>(expression.value(), binaryOp(operatorToken.value().type),
                              list.value());
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseList() {
    auto expression = parseRange();
    if (!expression) {
        return expression;
    }
    if (check({Token::Type::Comma})) {
        auto start = expression.value()->range.start;

        std::vector<Strong<Expression>> expressions;
        expressions.push_back(expression.value());
        while (match({Token::Type::Comma})) {
            auto range = parseRange();
            if (!range) {
                return range;
            }
            expressions.push_back(range.value());
        }

        expression = MakeStrong<ListLiteral>(expressions);
        expression.value()->range = SourceRange{start, previous().end()};
    }

    return expression;
}

Result<Strong<Expression>, Error> Parser::parseRange() {
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto rangeOperator = match({Token::Type::ThreeDots, Token::Type::ClosedRange})) {
        bool closed = rangeOperator.value().type == Token::Type::ThreeDots ? true : false;
        auto term = parseTerm();
        if (!term) {
            return term;
        }
        auto end = term.value()->range.end;
        expression =
            MakeStrong<RangeLiteral>(expression.value(), term.value(), closed);
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseTerm() {
    auto expression = parseFactor();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken = match({Token::Type::Plus, Token::Type::Minus})) {
        auto factor = parseFactor();
        if (!factor) {
            return factor;
        }
        auto end = factor.value()->range.end;
        expression =
            MakeStrong<Binary>(expression.value(), binaryOp(operatorToken.value().type),
                              factor.value());
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseFactor() {
    auto expression = parseExponent();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken =
               match({Token::Type::Star, Token::Type::Slash, Token::Type::Percent})) {
        auto exponent = parseExponent();
        if (!exponent) {
            return exponent;
        }
        auto end = exponent.value()->range.end;
        expression =
            MakeStrong<Binary>(expression.value(), binaryOp(operatorToken.value().type),
                              exponent.value());
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseExponent() {
    auto expression = parseUnary();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken = match({Token::Type::Carrot})) {
        auto unary = parseUnary();
        if (!unary) {
            return unary;
        }
        auto end = unary.value()->range.end;
        expression =
            MakeStrong<Binary>(expression.value(), binaryOp(operatorToken.value().type),
                              unary.value());
        expression.value()->range = SourceRange{start, end};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parseUnary() {
    if (auto operatorToken = match({Token::Type::Minus, Token::Type::Not})) {
        auto unary = parseUnary();
        if (!unary) {
            return unary;
        }
        auto end = unary.value()->range.end;
        auto expression =
            MakeStrong<Unary>(unaryOp(operatorToken.value().type), unary.value());
        expression->range = SourceRange{operatorToken.value().location, end};
        return expression;
    }
    return parseCall();
}

static std::string computeErrorString(const Signature &matchingSignature,
                                      const std::vector<Signature> &signatures) {
    auto errorString = Concat("no matching function ", Quoted(matchingSignature.name()));
    if (signatures.size() == 1) {
        errorString = Concat(errorString, ". Did you mean ", Quoted(signatures[0].name()), "?");
    } else if (signatures.size() <= 5) {
        auto signaturesMatches =
            Join(signatures, "\n  ", [](Signature signature) { return signature.name(); });
        errorString = Concat(errorString, "\nPossible matches:\n  ", signaturesMatches, "\n");
    }
    return errorString;
}

Result<Strong<Expression>, Error> Parser::parseCall() {
    std::vector<Strong<Expression>> arguments;
    Signature matchingSignature;

    auto grammar = &_grammar;
    auto token = peek();
    auto start = token.location;

    while (token.isPrimary()) {
        if (token.isWord()) {
            auto word = lowercase(token.text);
            auto variable = _variables.find(word);
            if (variable != _variables.end() && grammar->argument) {
                auto argument =
                    (grammar->argument->isLeaf() && grammar != &_grammar ? parseList()
                                                                         : parseSubscript());
                if (!argument) {
                    return argument;
                }
                arguments.push_back(argument.value());
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
            auto argument =
                (grammar->argument->isLeaf() && grammar != &_grammar ? parseList()
                                                                     : parseSubscript());
            if (!argument) {
                return argument;
            }
            arguments.push_back(argument.value());
            matchingSignature.terms.emplace_back(Signature::Argument());
            grammar = grammar->argument.get();
            token = peek();
            continue;
        }
        break;
    }
    if (matchingSignature.terms.size() == 1) {
        if (arguments.size() == 1) {
            return arguments[0];
        }
    }
    auto signature = grammar->signature;
    if (!signature) {
        if (matchingSignature.terms.size() == 0) {
            return Fail(Error(start, Concat("unexpected ", token.description())));
        }

        auto errorString = computeErrorString(matchingSignature, grammar->allSignatures());
        return Fail(Error(SourceRange{start, previous().end()}, errorString));
    }
    auto call =
        MakeStrong<Call>(signature.value(), std::vector<Optional<Token>>(), arguments);
    call->range = SourceRange{start, previous().end()};
    return call;
}

Result<Strong<Expression>, Error> Parser::parseSubscript() {
    auto expression = parsePrimary();
    if (!expression) {
        return expression;
    }
    auto start = expression.value()->range.start;
    while (auto operatorToken = match({Token::Type::LeftBracket})) {
        auto subscript = parseExpression();
        if (!subscript) {
            return subscript;
        }
        if (!consume(Token::Type::RightBracket)) {
            return Fail(Error(peek().range(), Concat("expected ", Quoted("]"))));
        }
        expression = MakeStrong<Binary>(expression.value(), Binary::Subscript,
                                       subscript.value());
        expression.value()->range = SourceRange{start, previous().end()};
    }
    return expression;
}

Result<Strong<Expression>, Error> Parser::parsePrimary() {
    if (auto token = match({
            Token::Type::BoolLiteral,
            Token::Type::IntLiteral,
            Token::Type::FloatLiteral,
            Token::Type::StringLiteral,
            Token::Type::Empty,
        })) {
        auto literal = MakeStrong<Literal>(token.value());
        literal->range = SourceRange{token.value().location, token.value().end()};
        return literal;
    }

    if (match({Token::Type::LeftParen})) {
        return parseGrouping();
    }

    if (match({Token::Type::LeftBrace})) {
        return parseContainerLiteral();
    }

    if (peek().isWord() || peek().type == Token::Type::Global ||
        peek().type == Token::Type::Local) {
        return parseVariable();
    }

    return Fail(Error(peek().range(), Concat("unexpected ", peek().description())));
}

Result<Strong<Expression>, Error> Parser::parseVariable() {
    Optional<Variable::Scope> scope = None;
    SourceLocation start;
    if (peek().type == Token::Type::Global || peek().type == Token::Type::Local) {
        start = peek().location;
        scope =
            (peek().type == Token::Type::Global ? Variable::Scope::Global : Variable::Scope::Local);
        advance();
    }
    auto token = consumeWord();
    if (!token) {
        return Fail(Error(peek().range(), "expected a variable name"));
    }
    if (!scope) {
        start = token.value().location;
    }
    auto variable = MakeStrong<Variable>(token.value(), scope);
    variable->range = SourceRange{start, token.value().end()};
    return variable;
}

Result<Strong<Expression>, Error> Parser::parseGrouping() {
    auto start = previous().location;
    auto expression = parseExpression();
    if (!expression) {
        return expression;
    }
    if (!consume(Token::Type::RightParen)) {
        return Fail(Error(peek().range(), Concat("expected ", Quoted(")"))));
    }
    auto grouping = MakeStrong<Grouping>(expression.value());
    grouping->range = SourceRange{start, previous().end()};
    return grouping;
}

Result<Strong<Expression>, Error> Parser::parseContainerLiteral() {
    auto start = previous().location;
    if (match({Token::Type::RightBrace})) {
        auto container = MakeStrong<ListLiteral>();
        container->range = SourceRange{start, previous().end()};
        return container;
    }
    if (match({Token::Type::Colon})) {
        if (!consume(Token::Type::RightBrace)) {
            return Fail(Error(peek().range(), Concat("expected ", Quoted("}"))));
        }
        auto container = MakeStrong<DictionaryLiteral>();
        container->range = SourceRange{start, previous().end()};
        return container;
    }

    Strong<Expression> containerExpression;
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }

    if (match({Token::Type::Colon})) {
        Mapping<Strong<Expression>, Strong<Expression>> values;
        auto valueExpression = parseTerm();
        if (!valueExpression) {
            return valueExpression;
        }
        values[expression.value()] = valueExpression.value();
        if (consume(Token::Type::Comma)) {
            do {
                auto keyExpression = parseTerm();
                if (!keyExpression) {
                    return keyExpression;
                }
                if (!consume(Token::Type::Colon)) {
                    return Fail(Error(peek().range(), Concat("expected ", Quoted(":"))));
                }
                auto valueExpression = parseTerm();
                if (!valueExpression) {
                    return valueExpression;
                }
                values[keyExpression.value()] = valueExpression.value();
            } while (match({Token::Type::Comma}));
        }
        containerExpression = MakeStrong<DictionaryLiteral>(values);
    } else if (match({Token::Type::Comma})) {
        std::vector<Strong<Expression>> values;
        values.push_back(expression.value());
        do {
            auto expression = parseTerm();
            if (!expression) {
                return expression;
            }
            values.push_back(expression.value());
        } while (match({Token::Type::Comma}));

        containerExpression = MakeStrong<ListLiteral>(values);
    } else if (check({Token::Type::RightBrace})) {
        std::vector<Strong<Expression>> values;
        values.push_back(expression.value());
        containerExpression = MakeStrong<ListLiteral>(values);
    } else {
        return Fail(Error(peek().range(), Concat("expected ", Quoted(":"), " or ", Quoted(","))));
    }

    if (!consume(Token::Type::RightBrace)) {
        return Fail(Error(peek().range(), Concat("expected ", Quoted("}"))));
    }
    containerExpression->range = SourceRange{start, previous().end()};
    return containerExpression;
}

SIF_NAMESPACE_END
