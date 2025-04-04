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

    return parseBlock({});
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

bool Parser::failed() const { return _failed; }

void Parser::declare(const Signature &signature) {
    _scopes.back().signatures.push_back(signature);
    _grammar.insert(signature);
}

void Parser::declare(const std::vector<Signature> &signatures) {
    for (const auto &signature : signatures) {
        declare(signature);
    }
}

void Parser::declare(const std::string &variable) {
    if (variable == "_") {
        return;
    }

    _scopes.back().variables.insert(variable);
    _variables.insert(variable);
}

void Parser::declare(const Set<std::string> &variables) {
    for (const auto &variable : variables) {
        declare(variable);
    }
}

const std::vector<Signature> &Parser::declarations() const { return _exportedDeclarations; }

const std::vector<Signature> &Parser::signatures() const { return _scopes.back().signatures; }
const Set<std::string> &Parser::variables() const { return _scopes.back().variables; }

const std::vector<SourceRange> &Parser::commentRanges() const { return _commentRanges; }

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
    while (token.type == Token::Type::Comment) {
        _commentRanges.push_back(token.range);
        token = _config.scanner.scan();
    }

    if (token.type == Token::Type::Error) {
        emitError(Error(token.range, token.text));
    }

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
    if (isAtEnd()) {
        return peek();
    }
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
    _grammar.insert(scope.signatures.begin(), scope.signatures.end());
}

void Parser::endScope() {
    _scopes.pop_back();

    _variables.clear();
    _variables.insert("it");
    _variables.insert("empty");

    _grammar = Grammar();
    _grammar.argument = MakeOwned<Grammar>();

    for (auto &&scope : _scopes) {
        _grammar.insert(scope.signatures.begin(), scope.signatures.end());
        _variables.insert(scope.variables.begin(), scope.variables.end());
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

Signature Parser::parseSignature() {
    Signature signature;
    Set<std::string> argumentNames;

    _config.scanner.disableMultilineMode();
    while (peek().isWord() || peek().type == Token::Type::LeftParen ||
           peek().type == Token::Type::LeftBrace) {
        auto token = advance();
        if (token.isWord()) {
            std::vector<Token> tokens({token});
            while (match({Token::Type::Slash})) {
                auto word = consumeWord();
                if (word) {
                    tokens.push_back(word.value());
                } else {
                    emitError(Error(peek().range.start, "expected a word"));
                }
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
                    emitError(Error(peek().range.start, "expected a word"));
                }
            } while (match({Token::Type::Slash}));
            if (!consume(Token::Type::RightParen)) {
                emitError(Error(peek().range.start, Concat("expected ", Quoted(")"))));
            }
            signature.terms.push_back(Signature::Option{choice});
        } else {
            std::vector<Signature::Argument::Target> targets;
            do {
                Optional<Token> name = None;
                Optional<Token> typeName = None;
                if ((name = consumeWord())) {
                    if (argumentNames.find(name.value().text) != argumentNames.end()) {
                        emitError(Error(name.value().range,
                                        "duplicate argument names in function declaration"));
                    }
                    if (name.value().text != "_") {
                        argumentNames.insert(name.value().text);
                    }
                    if (match({Token::Type::Colon})) {
                        if (auto result = consumeWord()) {
                            typeName = result.value();
                        } else {
                            emitError(Error(peek().range.start, "expected a type name"));
                        }
                    }
                } else if (match({Token::Type::Colon})) {
                    if (auto result = consumeWord()) {
                        typeName = result.value();
                    } else {
                        emitError(Error(peek().range.start, "expected a type name"));
                    }
                }
                targets.push_back(Signature::Argument::Target{name, typeName});
            } while (match({Token::Type::Comma}));
            if (!consume(Token::Type::RightBrace)) {
                emitError(Error(peek().range.start, Concat("expected ", Quoted("}"))));
            }
            signature.terms.push_back(Signature::Argument{targets});
        }
    }
    if (signature.terms.size() == 0) {
        emitError(Error(peek().range.start,
                        Concat("expected a word, ", Quoted("("), ", or ", Quoted("{"))));
    }
    if (match({Token::Type::Arrow})) {
        if (auto word = consumeWord()) {
            signature.typeName = word.value();
        } else {
            emitError(Error(peek().range.start, "expected a type name"));
        }
    }
    _config.scanner.enableMultilineMode();
    return signature;
}

Strong<Statement> Parser::parseBlock(const Parser::TokenTypes &endTypes) {
    std::vector<Strong<Statement>> statements;
    auto start = peek().range.start;
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
    block->range = SourceRange{start, peek().range.end};
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
        emitError(Error(peek().range.start, "expected a new line"));
        synchronize();
        return nullptr;
    }
    return statement.value();
}

Strong<Statement> Parser::parseFunction() {
    _parsingDepth++;

    auto decl = MakeStrong<FunctionDecl>();
    decl->ranges.function = previous().range;
    decl->range.start = previous().range.start;

    auto signature = parseSignature();
    if (!consumeNewLine()) {
        emitError(Error(peek().range.start, "expected a new line"));
        synchronize();
    }

    decl->signature = signature;
    _scopes.back().signatures.push_back(signature);
    if (_scopes.size() == 1) {
        _exportedDeclarations.push_back(signature);
    }
    _grammar.insert(signature);

    // Overwrite any existing variable declarations.
    auto name = signature.name();
    _variables.erase(name);
    _scopes.back().variables.erase(name);

    beginScope();
    for (auto &&argument : signature.arguments()) {
        for (auto &&target : argument.targets) {
            auto token = target.name;
            if (token) {
                auto name = lowercase(token->text);
                declare(name);
            }
        }
    }

    auto statement = parseBlock({Token::Type::End});
    decl->statement = statement;

    if (peek().type == Token::Type::End) {
        decl->ranges.end = peek().range;
        decl->range.end = peek().range.end;
    }
    if (consumeEnd(Token::Type::Function)) {
        if (previous().type == Token::Type::Function) {
            decl->ranges.endFunction = previous().range;
            decl->range.end = previous().range.end;
        }
    } else {
        emitError(Error(peek().range.start, Concat("expected ", Quoted("end"))));
    }

    _parsingDepth--;
    endScope();
    return decl;
}

Strong<Statement> Parser::parseIf() {
    _parsingDepth++;

    auto ifStatement = MakeStrong<If>();
    ifStatement->range.start = previous().range.start;
    ifStatement->ranges.if_ = previous().range;

    auto condition = parseExpression();
    if (condition) {
        ifStatement->condition = condition;
    } else {
        auto token = synchronize({Token::Type::Then, Token::Type::NewLine});
        if (token.isEndOfStatement()) {
            return ifStatement;
        }
        ifStatement->ranges.then = token.range;
    }

    consumeNewLine();
    if (!consume(Token::Type::Then)) {
        emitError(Error(peek().range.start, Concat("expected ", Quoted("then"))));
        auto token = synchronize({Token::Type::Then, Token::Type::NewLine});
        if (token.isEndOfStatement()) {
            return ifStatement;
        }
    }
    ifStatement->ranges.then = previous().range;

    Optional<Token> token;
    if (consumeNewLine()) {
        if (auto statement = parseBlock({Token::Type::End, Token::Type::Else})) {
            ifStatement->ifStatement = statement;
        } else {
            return ifStatement;
        }
        token = match({Token::Type::End, Token::Type::Else});
        if (!token) {
            emitError(Error(peek().range.start,
                            Concat("expected ", Quoted("end"), " or ", Quoted("else"))));
        } else if (token.value().type == Token::Type::End) {
            ifStatement->ranges.end = token.value().range;
            if (match({Token::Type::If})) {
                ifStatement->ranges.endIf = previous().range;
            }
            _parsingDepth--;
            consumeNewLine();
        }
    } else {
        _parsingDepth--;
        if (auto statement = parseSimpleStatement()) {
            ifStatement->ifStatement = statement.value();
            consumeNewLine();
        } else {
            synchronize();
            emitError(statement.error());
        }
    }

    if ((token.has_value() && token.value().type == Token::Type::Else) ||
        (!token.has_value() && match({Token::Type::Else}))) {
        if (previous().type == Token::Type::Else) {
            ifStatement->ranges.else_ = previous().range;
        } else {
            ifStatement->ranges.else_ = token.value().range;
        }

        if (consumeNewLine()) {
            if (auto statement = parseBlock({Token::Type::End})) {
                ifStatement->elseStatement = statement;
            }

            if (peek().type == Token::Type::End) {
                ifStatement->ranges.end = peek().range;
            }

            if (consumeEnd(Token::Type::If)) {
                if (previous().type == Token::Type::If) {
                    ifStatement->ranges.endIf = previous().range;
                }
            } else {
                emitError(Error(peek().range.start, Concat("expected ", Quoted("end"))));
            }

            _parsingDepth--;
            if (!consumeNewLine()) {
                emitError(Error(peek().range.start, "expected new line or end of script"));
                synchronize();
                return ifStatement;
            }
        } else {
            _parsingDepth--;
            if (match({Token::Type::If})) {
                if (auto statement = parseIf()) {
                    ifStatement->ifStatement = statement;
                } else {
                    return ifStatement;
                }
            } else {
                if (auto statement = parseSimpleStatement()) {
                    ifStatement->elseStatement = statement.value();
                    consumeNewLine();
                } else {
                    emitError(statement.error());
                    synchronize();
                    return ifStatement;
                }
            }
        }
    }

    return ifStatement;
}

Strong<Statement> Parser::parseUse() {
    auto useStatement = MakeStrong<Use>();
    useStatement->range.start = previous().range.start;
    useStatement->ranges.use = previous().range;
    Optional<Token> token = match({Token::Type::StringLiteral, Token::Type::Word});
    if (!token) {
        emitError(Error(peek().range, "expected a string or word"));
        synchronize();
        return useStatement;
    }
    useStatement->target = token.value();
    useStatement->range.end = token.value().range.end;
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
        emitError(Error(useStatement->range, module.error().what()));
    }
    return useStatement;
}

Strong<Statement> Parser::parseUsing() {
    auto usingStatement = MakeStrong<Using>();
    usingStatement->range.start = previous().range.start;
    usingStatement->ranges.using_ = previous().range;
    Optional<Token> token = match({Token::Type::StringLiteral, Token::Type::Word});
    if (!token) {
        emitError(Error(peek().range, "expected a string literal or word"));
        synchronize();
        return usingStatement;
    }
    usingStatement->target = token.value();

    std::vector<Error> outErrors;
    auto source = token.value().encodedStringOrWord();
    auto module = _config.moduleProvider.module(source);
    if (module) {
        beginScope(Scope{module.value()->signatures()});
    } else {
        emitError(Error(token.value().range, module.error().what()));
        beginScope(Scope{});
    }

    if (consumeNewLine()) {
        auto statement = parseBlock({Token::Type::End});
        if (statement) {
            usingStatement->statement = statement;
            usingStatement->ranges.end = peek().range;
            usingStatement->range.end = peek().range.end;
        }
        if (!consumeEnd(Token::Type::Using)) {
            emitError(Error(peek().range, Concat("expected ", Quoted("end"))));
        }
        if (previous().type == Token::Type::Using) {
            usingStatement->ranges.endUsing = previous().range;
            usingStatement->range.end = previous().range.end;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            usingStatement->statement = result.value();
            usingStatement->range.end = previous().range.end;
            consumeNewLine();
        } else {
            synchronize();
        }
    }
    endScope();
    return usingStatement;
}

Strong<Statement> Parser::parseTry() {
    auto tryStatement = MakeStrong<Try>();
    tryStatement->range.start = previous().range.start;
    tryStatement->ranges.try_ = previous().range;

    if (consumeNewLine()) {
        auto statement = parseBlock({Token::Type::End});
        if (statement) {
            tryStatement->statement = statement;
            tryStatement->ranges.end = peek().range;
            tryStatement->range.end = peek().range.end;
        }
        if (!consumeEnd(Token::Type::Try)) {
            emitError(Error(peek().range, Concat("expected ", Quoted("end"))));
        }
        if (previous().type == Token::Type::Try) {
            tryStatement->ranges.endTry = previous().range;
            tryStatement->range.end = previous().range.end;
        }
    } else {
        _parsingDepth--;
        if (auto result = parseSimpleStatement()) {
            tryStatement->statement = result.value();
            tryStatement->range.end = previous().range.end;
            consumeNewLine();
        } else {
            synchronize();
        }
    }
    return tryStatement;
}

Strong<Statement> Parser::parseRepeat() {
    auto repeatRange = previous().range;

    _parsingDepth++;
    Optional<Token> token;
    if (consumeNewLine() || (token = match({Token::Type::Forever}))) {
        Optional<SourceRange> foreverRange;
        if (previous().type == Token::Type::Forever) {
            foreverRange = previous().range;
        }
        if (token.has_value() && !consumeNewLine()) {
            emitError(Error(peek().range, "expected a new line"));
            synchronize();
        }
        auto statement = parseRepeatForever();
        if (statement) {
            statement->range = SourceRange{repeatRange.start, previous().range.end};
            statement->Repeat::ranges.repeat = repeatRange;
            statement->Repeat::ranges.forever = foreverRange;
        }
        return statement;
    }
    if ((token = match({Token::Type::While, Token::Type::Until}))) {
        auto statement = parseRepeatCondition();
        if (statement) {
            statement->range = SourceRange{repeatRange.start, previous().range.end};
            statement->Repeat::ranges.repeat = repeatRange;
        }
        return statement;
    }
    if (match({Token::Type::For})) {
        auto statement = parseRepeatFor();
        if (statement) {
            statement->range = SourceRange{repeatRange.start, previous().range.end};
            statement->Repeat::ranges.repeat = repeatRange;
        }
        return statement;
    }

    emitError(
        Error(peek().range, Concat("expected ", Quoted("forever"), ", ", Quoted("while"), ", ",
                                   Quoted("until"), ", ", Quoted("for"), ", or a new line")));
    synchronize();
    auto repeat = parseRepeatForever();
    repeat->ranges.repeat = repeatRange;
    repeat->range.start = repeatRange.start;
    return repeat;
}

Strong<Repeat> Parser::parseRepeatForever() {
    auto repeat = MakeStrong<Repeat>();
    repeat->ranges.repeat = previous().range;
    repeat->range.start = previous().range.start;

    repeat->statement = parseBlock({Token::Type::End});
    if (peek().type == Token::Type::End) {
        repeat->ranges.end = peek().range;
        repeat->range.end = peek().range.end;
    }
    if (consumeEnd(Token::Type::Repeat)) {
        if (previous().type == Token::Type::Repeat) {
            repeat->ranges.endRepeat = previous().range;
            repeat->range.end = previous().range.end;
        }
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted("end"))));
    }

    _parsingDepth--;
    return repeat;
}

Strong<RepeatCondition> Parser::parseRepeatCondition() {
    auto repeat = MakeStrong<RepeatCondition>();
    repeat->ranges.conjunction = previous().range;

    repeat->conjunction =
        (previous().type == Token::Type::While ? RepeatCondition::Conjunction::While
                                               : RepeatCondition::Conjunction::Until);
    repeat->condition = parseExpression();

    if (!consumeNewLine()) {
        emitError(Error(peek().range, "expected a new line"));
        synchronize();
    }

    repeat->statement = parseBlock({Token::Type::End});
    if (peek().type == Token::Type::End) {
        repeat->Repeat::ranges.end = peek().range;
        repeat->range.end = peek().range.end;
    }
    if (consumeEnd(Token::Type::Repeat)) {
        if (previous().type == Token::Type::Repeat) {
            repeat->Repeat::ranges.endRepeat = previous().range;
            repeat->range.end = previous().range.end;
        }
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted("end"))));
    }

    _parsingDepth--;
    return repeat;
}

Strong<RepeatFor> Parser::parseRepeatFor() {
    auto repeat = MakeStrong<RepeatFor>();
    repeat->ranges.for_ = previous().range;

    do {
        if (auto token = consumeWord()) {
            auto variable = MakeStrong<Variable>(token.value());
            variable->range = SourceRange{token.value().range.start, token.value().range.end};
            auto name = lowercase(variable->name.text);
            declare(name);
            repeat->variables.push_back(variable);
        } else {
            emitError(Error(peek().range, "expected a variable name"));
            break;
        }
    } while (match({Token::Type::Comma}));

    if (consume(Token::Type::In)) {
        repeat->ranges.in = previous().range;
        repeat->expression = parseExpression();

        if (!consumeNewLine()) {
            emitError(Error(peek().range, "expected a new line"));
            synchronize();
        }
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted("in"))));
        synchronize({Token::Type::In, Token::Type::NewLine});
    }

    repeat->statement = parseBlock({Token::Type::End});
    if (peek().type == Token::Type::End) {
        repeat->Repeat::ranges.end = peek().range;
        repeat->range.end = peek().range.end;
    }
    if (consumeEnd(Token::Type::Repeat)) {
        if (previous().type == Token::Type::Repeat) {
            repeat->Repeat::ranges.endRepeat = previous().range;
            repeat->range.end = previous().range.end;
        }
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted("end"))));
    }

    _parsingDepth--;
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
    auto assignment = MakeStrong<Assignment>();
    assignment->ranges.set = previous().range;
    assignment->range.start = previous().range.start;

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
            emitError(Error(peek().range, "expected a variable name"));
            synchronize();
            return assignment;
        }
        if (match({Token::Type::Colon})) {
            if (auto word = consumeWord()) {
                typeName = word.value();
            } else {
                emitError(Error(peek().range, "expected a type name"));
                synchronize();
                return assignment;
            }
        } else {
            while (match({Token::Type::LeftBracket})) {
                auto subscript = parseExpression();
                if (!subscript) {
                    synchronize();
                    return assignment;
                }
                subscripts.push_back(subscript);
                if (!consume(Token::Type::RightBracket)) {
                    emitError(Error(peek().range, Concat("expected ", Quoted("]"))));
                    return assignment;
                }
            }
        }
        if (subscripts.size() == 0) {
            auto name = lowercase(token.value().text);
            declare(name);
        }
        auto variable = MakeStrong<Variable>(token.value(), scope);
        variable->range = SourceRange{token.value().range.start, token.value().range.end};
        auto assignmentTarget = MakeStrong<AssignmentTarget>(variable, typeName, subscripts);
        assignmentTarget->range =
            SourceRange{assignmentTarget->variable->range.start, previous().range.end};
        variableDecls.push_back(assignmentTarget);
    } while (match({Token::Type::Comma}));
    assignment->targets = variableDecls;

    if (!consume(Token::Type::To)) {
        emitError(Error(peek().range, Concat("expected ", Quoted("to"))));
        synchronize();
        return assignment;
    }
    assignment->ranges.to = previous().range;
    assignment->expression = parseExpression();
    assignment->range.end = previous().range.end;
    return assignment;
}

Result<Strong<Statement>, Error> Parser::parseExit() {
    auto exitRepeat = MakeStrong<ExitRepeat>();
    exitRepeat->ranges.exit = previous().range;
    if (!_parsingRepeat) {
        emitError(
            Error(peek().range, Concat("unexpected ", Quoted("exit"), " outside repeat block")));
    }
    if (consume(Token::Type::Repeat)) {
        exitRepeat->ranges.repeat = previous().range;
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted("repeat"))));
    }
    exitRepeat->range = SourceRange{exitRepeat->ranges.exit.start, previous().range.end};
    return exitRepeat;
}

Result<Strong<Statement>, Error> Parser::parseNext() {
    auto nextRepeat = MakeStrong<NextRepeat>();
    nextRepeat->ranges.next = previous().range;
    if (!_parsingRepeat) {
        emitError(
            Error(peek().range, Concat("unexpected ", Quoted("next"), " outside repeat block")));
    }
    if (consume(Token::Type::Repeat)) {
        nextRepeat->ranges.repeat = previous().range;
    } else {
        emitError(Error(peek().range.start, Concat("expected ", Quoted("repeat"))));
    }
    nextRepeat->range = SourceRange{nextRepeat->ranges.next.start, previous().range.end};
    return nextRepeat;
}

Result<Strong<Statement>, Error> Parser::parseReturn() {
    auto returnStatement = MakeStrong<Return>();
    returnStatement->range = previous().range;
    returnStatement->ranges.return_ = previous().range;
    if (!isAtEnd() && !check({Token::Type::NewLine})) {
        if (auto returnExpression = parseExpression()) {
            returnStatement->expression = returnExpression;
            returnStatement->range.end = returnExpression->range.end;
        }
    }
    return returnStatement;
}

Result<Strong<Statement>, Error> Parser::parseExpressionStatement() {
    auto statement = MakeStrong<ExpressionStatement>();
    auto expression = parseExpression();
    if (!expression) {
        return statement;
    }
    statement->expression = expression;
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

Strong<Expression> Parser::parseExpression() { return parseClause(); }

Strong<Expression> Parser::parseClause() {
    auto expression = parseEquality();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken = match({Token::Type::And, Token::Type::Or})) {
        auto binaryExpression =
            MakeStrong<Binary>(expression, binaryOp(operatorToken.value().type), nullptr);
        binaryExpression->range.start = start;
        binaryExpression->ranges.operator_ = previous().range;

        auto equality = parseEquality();
        if (!equality) {
            return binaryExpression;
        }
        binaryExpression->rightExpression = equality;
        binaryExpression->range.end = equality->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseEquality() {
    auto expression = parseComparison();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    expression->range.start = start;
    while (auto operatorToken =
               match({Token::Type::Equal, Token::Type::NotEqual, Token::Type::Is})) {
        auto binaryExpression = MakeStrong<Binary>();
        binaryExpression->leftExpression = expression;
        binaryExpression->range.start = start;
        if (operatorToken.value().type == Token::Type::Is && match({Token::Type::Not})) {
            binaryExpression->binaryOperator = Binary::Operator::NotEqual;
        } else {
            binaryExpression->binaryOperator = binaryOp(operatorToken.value().type);
        }
        auto comparison = parseComparison();
        if (!comparison) {
            return binaryExpression;
        }
        binaryExpression->rightExpression = comparison;
        binaryExpression->range.end = comparison->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseComparison() {
    auto expression = parseList();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken =
               match({Token::Type::LessThan, Token::Type::GreaterThan, Token::Type::LessThanOrEqual,
                      Token::Type::GreaterThanOrEqual})) {
        auto binaryExpression = MakeStrong<Binary>();
        binaryExpression->leftExpression = expression;
        binaryExpression->binaryOperator = binaryOp(operatorToken.value().type);
        binaryExpression->range.start = start;

        auto list = parseList();
        if (!list) {
            return binaryExpression;
        }
        binaryExpression->rightExpression = list;
        binaryExpression->range.end = list->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseList() {
    auto expression = parseRange();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    if (check({Token::Type::Comma})) {
        auto listExpression = MakeStrong<ListLiteral>();
        listExpression->range.start = start;
        listExpression->expressions.push_back(expression);
        while (match({Token::Type::Comma})) {
            auto range = parseRange();
            if (!range) {
                return listExpression;
            }
            listExpression->expressions.push_back(range);
        }

        listExpression->range.end = previous().range.end;
        expression = listExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseRange() {
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto rangeOperator = match({Token::Type::ThreeDots, Token::Type::ClosedRange})) {
        auto rangeExpression = MakeStrong<RangeLiteral>();
        rangeExpression->start = expression;
        rangeExpression->range.start = start;
        rangeExpression->closed =
            rangeOperator.value().type == Token::Type::ThreeDots ? true : false;
        auto term = parseTerm();
        if (!term) {
            return rangeExpression;
        }
        rangeExpression->end = term;
        rangeExpression->range.end = term->range.end;
        expression = rangeExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseTerm() {
    auto expression = parseFactor();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken = match({Token::Type::Plus, Token::Type::Minus})) {
        auto binaryExpression = MakeStrong<Binary>();
        binaryExpression->leftExpression = expression;
        binaryExpression->binaryOperator = binaryOp(operatorToken.value().type);
        binaryExpression->range.start = start;
        auto factor = parseFactor();
        if (!factor) {
            return binaryExpression;
        }
        binaryExpression->rightExpression = factor;
        binaryExpression->range.end = factor->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseFactor() {
    auto expression = parseExponent();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken =
               match({Token::Type::Star, Token::Type::Slash, Token::Type::Percent})) {
        auto binaryExpression = MakeStrong<Binary>();
        binaryExpression->leftExpression = expression;
        binaryExpression->binaryOperator = binaryOp(operatorToken.value().type);
        binaryExpression->range.start = start;
        auto exponent = parseExponent();
        if (!exponent) {
            return exponent;
        }
        binaryExpression->rightExpression = exponent;
        binaryExpression->range.end = exponent->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseExponent() {
    auto expression = parseUnary();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken = match({Token::Type::Carrot})) {
        auto binaryExpression = MakeStrong<Binary>();
        binaryExpression->binaryOperator = binaryOp(operatorToken.value().type);
        binaryExpression->leftExpression = expression;
        binaryExpression->range.start = start;
        auto unary = parseUnary();
        if (!unary) {
            return binaryExpression;
        }
        binaryExpression->rightExpression = unary;
        binaryExpression->range.end = unary->range.end;
        expression = binaryExpression;
    }
    return expression;
}

Strong<Expression> Parser::parseUnary() {
    if (auto operatorToken = match({Token::Type::Minus, Token::Type::Not})) {
        auto unaryExpression = MakeStrong<Unary>(unaryOp(operatorToken.value().type));
        unaryExpression->range.start = operatorToken.value().range.start;
        auto unary = parseUnary();
        if (!unary) {
            return unaryExpression;
        }
        unaryExpression->expression = unary;
        unaryExpression->range.start = unary->range.end;
        return unaryExpression;
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

Strong<Expression> Parser::parseCall() {
    std::vector<Strong<Expression>> arguments;
    std::vector<SourceRange> ranges;
    Signature matchingSignature;

    auto grammar = &_grammar;
    auto token = peek();
    auto start = token.range.start;

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
                arguments.push_back(argument);
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
                ranges.push_back(token.range);
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
            arguments.push_back(argument);
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
            emitError(Error(start, "expected an expression"));
            return nullptr;
        }

        auto errorString = computeErrorString(matchingSignature, grammar->allSignatures());
        emitError(Error(SourceRange{start, previous().range.end}, errorString));
        return nullptr;
    }
    auto call = MakeStrong<Call>(signature.value(), arguments);
    call->range = SourceRange{start, previous().range.end};
    call->ranges = ranges;
    return call;
}

Strong<Expression> Parser::parseSubscript() {
    auto expression = parsePrimary();
    if (!expression) {
        return expression;
    }
    auto start = expression->range.start;
    while (auto operatorToken = match({Token::Type::LeftBracket})) {
        auto subscript = parseExpression();
        if (!subscript) {
            return subscript;
        }
        if (!consume(Token::Type::RightBracket)) {
            emitError(Error(peek().range, Concat("expected ", Quoted("]"))));
            return nullptr;
        }
        expression = MakeStrong<Binary>(expression, Binary::Subscript, subscript);
        expression->range = SourceRange{start, previous().range.end};
    }
    return expression;
}

Strong<Expression> Parser::parsePrimary() {
    if (auto token = match({
            Token::Type::BoolLiteral,
            Token::Type::IntLiteral,
            Token::Type::FloatLiteral,
            Token::Type::StringLiteral,
            Token::Type::Empty,
        })) {
        auto literal = MakeStrong<Literal>(token.value());
        literal->range = SourceRange{token.value().range.start, token.value().range.end};
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

    emitError(Error(peek().range, Concat("unexpected ", peek().description())));
    return nullptr;
}

Strong<Expression> Parser::parseVariable() {
    auto variable = MakeStrong<Variable>();
    variable->range.start = peek().range.start;
    if (peek().type == Token::Type::Global || peek().type == Token::Type::Local) {
        variable->ranges.scope = peek().range;
        variable->scope =
            (peek().type == Token::Type::Global ? Variable::Scope::Global : Variable::Scope::Local);
        advance();
    }
    auto token = consumeWord();
    if (!token) {
        emitError(Error(peek().range, "expected a variable name"));
        return variable;
    }
    variable->name = token.value();
    if (token.value().text == "_") {
        emitError(
            Error(token.value().range, Concat(Quoted("_"), " may not be used as a variable name")));
    }
    variable->range.end = token.value().range.end;
    return variable;
}

Strong<Expression> Parser::parseGrouping() {
    auto grouping = MakeStrong<Grouping>();
    grouping->ranges.leftGrouping = previous().range;
    grouping->range.start = previous().range.start;

    grouping->expression = parseExpression();
    if (consume(Token::Type::RightParen)) {
        grouping->ranges.rightGrouping = previous().range;
        grouping->range.end = peek().range.end;
    } else {
        emitError(Error(peek().range, Concat("expected ", Quoted(")"))));
    }
    return grouping;
}

Strong<Expression> Parser::parseContainerLiteral() {
    auto start = previous().range.start;
    if (match({Token::Type::RightBrace})) {
        auto container = MakeStrong<ListLiteral>();
        container->range = SourceRange{start, previous().range.end};
        return container;
    }
    if (match({Token::Type::Colon})) {
        auto container = MakeStrong<DictionaryLiteral>();
        container->range.start = start;
        if (!consume(Token::Type::RightBrace)) {
            emitError(Error(peek().range, Concat("expected ", Quoted("}"))));
            return container;
        }
        container->range.end = previous().range.end;
        return container;
    }

    Strong<Expression> containerExpression;
    auto expression = parseTerm();
    if (!expression) {
        return expression;
    }

    if (match({Token::Type::Colon})) {
        auto dictionaryExpression = MakeStrong<DictionaryLiteral>();
        dictionaryExpression->range.start = start;

        auto valueExpression = parseTerm();
        if (!valueExpression) {
            return dictionaryExpression;
        }
        dictionaryExpression->values[expression] = valueExpression;
        if (consume(Token::Type::Comma)) {
            do {
                auto keyExpression = parseTerm();
                if (!keyExpression) {
                    return dictionaryExpression;
                }
                if (!consume(Token::Type::Colon)) {
                    emitError(Error(peek().range, Concat("expected ", Quoted(":"))));
                    return dictionaryExpression;
                }
                auto valueExpression = parseTerm();
                if (!valueExpression) {
                    return dictionaryExpression;
                }
                dictionaryExpression->values[keyExpression] = valueExpression;
            } while (match({Token::Type::Comma}));
        }
        containerExpression = dictionaryExpression;
    } else if (match({Token::Type::Comma})) {
        auto listExpression = MakeStrong<ListLiteral>();
        listExpression->range.start = start;

        listExpression->expressions.push_back(expression);
        do {
            auto expression = parseTerm();
            if (!expression) {
                return listExpression;
            }
            listExpression->expressions.push_back(expression);
        } while (match({Token::Type::Comma}));
        containerExpression = listExpression;
    } else if (check({Token::Type::RightBrace})) {
        std::vector<Strong<Expression>> values;
        values.push_back(expression);
        containerExpression = MakeStrong<ListLiteral>(values);
    } else {
        emitError(Error(peek().range,
                        Concat("expected ", Quoted(":"), ", ", Quoted(","), " or ", Quoted("}"))));

        auto listExpression = MakeStrong<ListLiteral>();
        listExpression->range.start = start;
        listExpression->expressions.push_back(expression);
        return listExpression;
    }

    if (!consume(Token::Type::RightBrace)) {
        emitError(Error(peek().range, Concat("expected ", Quoted("}"))));
    }
    containerExpression->range = SourceRange{start, previous().range.end};
    return containerExpression;
}

SIF_NAMESPACE_END
