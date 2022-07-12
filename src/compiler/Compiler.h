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

#pragma once

#include "Common.h"
#include "Error.h"
#include "ast/Expression.h"
#include "ast/Repeat.h"
#include "ast/Statement.h"
#include "compiler/Bytecode.h"
#include "runtime/objects/Function.h"

SIF_NAMESPACE_BEGIN

class Compiler : public Statement::Visitor, public Expression::Visitor {
  public:
    Compiler();

    void addExtern(const std::string &name);
    Strong<Bytecode> compile(const Statement &statement);

    const std::vector<CompileError> &errors() const;

  private:
    void error(const Node &node, const std::string &message);

    struct Local {
        std::string name;
        int scopeDepth;
    };

    struct Frame {
        Strong<Bytecode> bytecode;
        std::vector<Local> locals;
        std::vector<Function::Capture> captures;
    };

    Bytecode &bytecode();
    std::vector<Local> &locals();
    std::vector<Function::Capture> &captures();

    int findLocal(const Frame &frame, const std::string &name);
    int findCapture(const std::string &name);
    bool findGlobal(const std::string &name);
    int addCapture(Frame &frame, int index, bool isLocal);

    void assign(Location location, const std::string &name);
    void resolve(Location location, const std::string &name);
    void addReturn();
    void beginScope();
    void endScope(const Location &location);

#pragma mark - Statement::Visitor

    void visit(const Block &) override;
    void visit(const FunctionDecl &) override;
    void visit(const If &) override;
    void visit(const Assignment &) override;
    void visit(const Return &) override;
    void visit(const ExpressionStatement &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCondition &) override;
    void visit(const RepeatForEach &) override;
    void visit(const ExitRepeat &) override;
    void visit(const NextRepeat &) override;

#pragma mark - Expression::VoidVisitor

    void visit(const Call &) override;
    void visit(const Binary &) override;
    void visit(const Unary &) override;
    void visit(const Grouping &) override;
    void visit(const Variable &) override;
    void visit(const RangeLiteral &) override;
    void visit(const ListLiteral &) override;
    void visit(const DictionaryLiteral &) override;
    void visit(const Literal &) override;

    int _scopeDepth;
    std::vector<Frame> _frames;
    Mapping<std::string, uint16_t> _globals;
    std::vector<CompileError> _errors;
    uint16_t _nextRepeat;
    uint16_t _exitRepeat;
};

SIF_NAMESPACE_END
