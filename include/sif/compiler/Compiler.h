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

#include <sif/Common.h>
#include <sif/Error.h>
#include <sif/ast/Expression.h>
#include <sif/ast/Repeat.h>
#include <sif/ast/Statement.h>
#include <sif/compiler/Bytecode.h>
#include <sif/compiler/Module.h>
#include <sif/compiler/Reporter.h>
#include <sif/runtime/objects/Function.h>

SIF_NAMESPACE_BEGIN

struct CompilerConfig {
    ModuleProvider &moduleProvider;
    Reporter &errorReporter;
    bool interactive = false;
    bool enableDebugInfo = true;
};

class Compiler : public Statement::Visitor, public Expression::Visitor {
  public:
    Compiler(const CompilerConfig &config);

    Strong<Bytecode> compile(const Statement &statement);

    const Set<std::string> &globals() const;
    const std::vector<Error> &errors() const;

  private:
    void error(const SourceRange &range, const std::string &message);
    void error(const SourceLocation &location, const std::string &message) {
        error(SourceRange{location, location}, message);
    }
    void error(const Node &node, const std::string &message) { error(node.range, message); }

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
    int addCapture(Frame &frame, int index, bool isLocal);

    void assignLocal(const SourceLocation &location, const std::string &name);
    void assignGlobal(const SourceLocation &location, const std::string &name);
    void assignVariable(const SourceLocation &location, const std::string &name,
                        Optional<Variable::Scope> scope);
    void assignFunction(const SourceLocation &location, const std::string &name);

    void resolve(const Call &call, const std::string &name);
    void resolve(const Variable &variable, const std::string &name);

    void addImplicitReturnIfNeeded();
    void addLocal(const std::string &name = "");

    void beginScope();
    void endScope(const SourceLocation &location);

#pragma mark - Statement::Visitor

    void visit(const Block &) override;
    void visit(const FunctionDecl &) override;
    void visit(const If &) override;
    void visit(const Try &) override;
    void visit(const Use &) override;
    void visit(const Using &) override;
    void visit(const Assignment &) override;
    void visit(const Return &) override;
    void visit(const ExpressionStatement &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCondition &) override;
    void visit(const RepeatFor &) override;
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
    void visit(const StringInterpolation &) override;

    CompilerConfig _config;

    int _scopeDepth;
    std::vector<Frame> _frames;
    Set<std::string> _globals;
    uint16_t _nextRepeat;
    std::stack<std::vector<uint16_t>> _exitPatches;
    bool _failed = false;
};

SIF_NAMESPACE_END
