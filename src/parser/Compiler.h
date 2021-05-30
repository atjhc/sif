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
#include "runtime/Error.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "ast/Repeat.h"
#include "parser/Bytecode.h"

CH_NAMESPACE_BEGIN

class Compiler: public Statement::Visitor, public Expression::Visitor {
public:

    Compiler(Owned<Statement> statement);

    Strong<Bytecode> compile();

    const std::vector<CompileError> &errors() const;

private:

    Bytecode &bytecode();
    void error(const Node &node, const std::string &message);

    struct Local {
        std::string name;
        int depth; 
    };

    int findLocal(const std::string &name) const;
    
    void assign(Location location, const std::string &name);
    void resolve(Location location, const std::string &name);
    
#pragma mark - Statement::Visitor

    void visit(const Block &) override;
    void visit(const FunctionDecl &) override;
    void visit(const If &) override;
    void visit(const Assignment &) override;
    void visit(const Return &) override;
    void visit(const ExpressionStatement &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCondition &) override;
    void visit(const ExitRepeat &) override;
    void visit(const NextRepeat &) override;

#pragma mark - Expression::VoidVisitor

    void visit(const Call &) override;
    void visit(const Binary &) override;
    void visit(const Unary &) override;
    void visit(const Grouping &) override;
    void visit(const Variable &) override;
    void visit(const ListLiteral &) override;
    void visit(const Literal &) override;

    int _depth;
    Strong<Bytecode> _bytecode;
    Owned<Statement> _statement;
    Owned<std::vector<Local>> _locals;
    Map<std::string, uint16_t> _globals;
    std::vector<CompileError> _errors;
    uint16_t _nextRepeat;
    uint16_t _exitRepeat;
};

CH_NAMESPACE_END
