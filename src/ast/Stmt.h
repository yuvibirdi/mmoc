#pragma once

#include "ast/Node.h"
#include "ast/Expr.h"
#include <memory>
#include <vector>

namespace ast {

/**
 * Base class for all statements.
 */
struct Stmt : public Node {
    virtual ~Stmt() = default;
};

/**
 * Expression statement (e.g., "x = 5;").
 */
struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expression;
    
    explicit ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
    
    std::string toString() const override {
        return expression ? expression->toString() + ";" : ";";
    }
};

/**
 * Return statement.
 */
struct ReturnStmt : public Stmt {
    std::unique_ptr<Expr> expression;
    
    explicit ReturnStmt(std::unique_ptr<Expr> expr = nullptr) 
        : expression(std::move(expr)) {}
    
    std::string toString() const override {
        return "return" + (expression ? " " + expression->toString() : "") + ";";
    }
};

/**
 * If statement.
 */
struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt;
    
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, 
           std::unique_ptr<Stmt> elseS = nullptr)
        : condition(std::move(cond)), thenStmt(std::move(then)), elseStmt(std::move(elseS)) {}
    
    std::string toString() const override;
};

/**
 * While statement.
 */
struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
    
    std::string toString() const override {
        return "while (" + condition->toString() + ") " + body->toString();
    }
};

/**
 * For statement.
 */
struct ForStmt : public Stmt {
    std::unique_ptr<Stmt> init;     // Can be declaration or expression statement
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> increment;
    std::unique_ptr<Stmt> body;
    
    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond,
            std::unique_ptr<Expr> inc, std::unique_ptr<Stmt> body)
        : init(std::move(init)), condition(std::move(cond)), 
          increment(std::move(inc)), body(std::move(body)) {}
    
    std::string toString() const override;
};

/**
 * Break statement.
 */
struct BreakStmt : public Stmt {
    std::string toString() const override {
        return "break;";
    }
};

/**
 * Continue statement.
 */
struct ContinueStmt : public Stmt {
    std::string toString() const override {
        return "continue;";
    }
};

/**
 * Compound statement (block).
 */
struct CompoundStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    explicit CompoundStmt(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}
    
    std::string toString() const override;
};

/**
 * Variable declaration statement.
 */
struct VarDecl : public Stmt {
    std::string name;
    std::string type;
    std::unique_ptr<Expr> initializer;
    
    VarDecl(std::string n, std::string t, std::unique_ptr<Expr> init = nullptr)
        : name(std::move(n)), type(std::move(t)), initializer(std::move(init)) {}
    
    std::string toString() const override;
};

/**
 * Function declaration/definition.
 */
struct FunctionDecl : public Node {
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> parameters; // (type, name) pairs
    std::unique_ptr<CompoundStmt> body; // nullptr for declarations
    
    FunctionDecl(std::string n, std::string retType, 
                 std::vector<std::pair<std::string, std::string>> params,
                 std::unique_ptr<CompoundStmt> body = nullptr)
        : name(std::move(n)), returnType(std::move(retType)), 
          parameters(std::move(params)), body(std::move(body)) {}
    
    std::string toString() const override;
    
    bool isDefinition() const { return body != nullptr; }
};

/**
 * Translation unit (top-level AST node).
 */
struct TranslationUnit : public Node {
    std::vector<std::unique_ptr<Node>> declarations;
    
    explicit TranslationUnit(std::vector<std::unique_ptr<Node>> decls)
        : declarations(std::move(decls)) {}
    
    std::string toString() const override;
};

} // namespace ast
