#pragma once

#include "sema/SymbolTable.h"
#include "ast/Node.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

namespace sema {

/**
 * Type checker for semantic analysis.
 */
class TypeChecker {
public:
    TypeChecker() = default;
    
    /**
     * Check types for a translation unit.
     */
    bool checkTypes(ast::TranslationUnit *tu);
    
private:
    SymbolTable symbolTable_;
    
    // Type checking methods
    bool checkFunctionDecl(ast::FunctionDecl *func);
    bool checkVarDecl(ast::VarDecl *var);
    bool checkStmt(ast::Stmt *stmt);
    bool checkExpr(ast::Expr *expr);
    
    // Type inference
    std::string inferType(ast::Expr *expr);
    
    // Type compatibility
    bool areTypesCompatible(const std::string &type1, const std::string &type2);
    
    // Error reporting
    void error(const std::string &message);
    
    bool hasErrors_ = false;
};

} // namespace sema
