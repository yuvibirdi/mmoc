#include "sema/TypeChecker.h"
#include "utils/Error.h"
#include <iostream>

namespace sema {

bool TypeChecker::checkTypes(ast::TranslationUnit *tu) {
    hasErrors_ = false;
    symbolTable_.enterScope(); // Global scope
    
    for (const auto &decl : tu->declarations) {
        if (auto *funcDecl = dynamic_cast<ast::FunctionDecl*>(decl.get())) {
            checkFunctionDecl(funcDecl);
        } else if (auto *varDecl = dynamic_cast<ast::VarDecl*>(decl.get())) {
            checkVarDecl(varDecl);
        }
    }
    
    symbolTable_.exitScope();
    return !hasErrors_;
}

bool TypeChecker::checkFunctionDecl(ast::FunctionDecl *func) {
    // Add function to symbol table
    if (!symbolTable_.addSymbol(func->name, func->returnType, true)) {
        error("Function '" + func->name + "' redefined");
        return false;
    }
    
    if (func->isDefinition()) {
        symbolTable_.enterScope(); // Function scope
        
        // Add parameters to symbol table
        for (const auto &param : func->parameters) {
            if (!symbolTable_.addSymbol(param.second, param.first)) {
                error("Parameter '" + param.second + "' redefined");
            }
        }
        
        // Check function body
        if (func->body) {
            for (const auto &stmt : func->body->statements) {
                checkStmt(stmt.get());
            }
        }
        
        symbolTable_.exitScope();
    }
    
    return true;
}

bool TypeChecker::checkVarDecl(ast::VarDecl *var) {
    // Check if variable already exists in current scope
    if (symbolTable_.existsInCurrentScope(var->name)) {
        error("Variable '" + var->name + "' redefined");
        return false;
    }
    
    // Add to symbol table
    symbolTable_.addSymbol(var->name, var->type);
    
    // Check initializer if present
    if (var->initializer) {
        std::string initType = inferType(var->initializer.get());
        if (!areTypesCompatible(var->type, initType)) {
            error("Type mismatch in variable '" + var->name + "' initialization");
            return false;
        }
    }
    
    return true;
}

bool TypeChecker::checkStmt(ast::Stmt *stmt) {
    if (auto *varDecl = dynamic_cast<ast::VarDecl*>(stmt)) {
        return checkVarDecl(varDecl);
    } else if (auto *exprStmt = dynamic_cast<ast::ExprStmt*>(stmt)) {
        if (exprStmt->expression) {
            return checkExpr(exprStmt->expression.get());
        }
    } else if (auto *returnStmt = dynamic_cast<ast::ReturnStmt*>(stmt)) {
        if (returnStmt->expression) {
            return checkExpr(returnStmt->expression.get());
        }
    } else if (auto *compoundStmt = dynamic_cast<ast::CompoundStmt*>(stmt)) {
        symbolTable_.enterScope();
        bool result = true;
        for (const auto &s : compoundStmt->statements) {
            result &= checkStmt(s.get());
        }
        symbolTable_.exitScope();
        return result;
    } else if (auto *ifStmt = dynamic_cast<ast::IfStmt*>(stmt)) {
        bool result = checkExpr(ifStmt->condition.get());
        result &= checkStmt(ifStmt->thenStmt.get());
        if (ifStmt->elseStmt) {
            result &= checkStmt(ifStmt->elseStmt.get());
        }
        return result;
    } else if (auto *whileStmt = dynamic_cast<ast::WhileStmt*>(stmt)) {
        bool result = checkExpr(whileStmt->condition.get());
        result &= checkStmt(whileStmt->body.get());
        return result;
    }
    
    return true;
}

bool TypeChecker::checkExpr(ast::Expr *expr) {
    if (auto *id = dynamic_cast<ast::Identifier*>(expr)) {
        Symbol *symbol = symbolTable_.lookupSymbol(id->name);
        if (!symbol) {
            error("Undefined variable '" + id->name + "'");
            return false;
        }
    } else if (auto *binExpr = dynamic_cast<ast::BinaryExpr*>(expr)) {
        bool result = checkExpr(binExpr->left.get());
        result &= checkExpr(binExpr->right.get());
        
        // TODO: Check type compatibility for binary operations
        return result;
    } else if (auto *unaryExpr = dynamic_cast<ast::UnaryExpr*>(expr)) {
        return checkExpr(unaryExpr->operand.get());
    } else if (auto *callExpr = dynamic_cast<ast::CallExpr*>(expr)) {
        // TODO: Check function calls
        (void)callExpr; // Suppress unused warning
        return true;
    }
    
    return true;
}

std::string TypeChecker::inferType(ast::Expr *expr) {
    if (dynamic_cast<ast::IntegerLiteral*>(expr)) {
        return "int";
    } else if (dynamic_cast<ast::FloatingLiteral*>(expr)) {
        return "double";
    } else if (dynamic_cast<ast::CharacterLiteral*>(expr)) {
        return "char";
    } else if (dynamic_cast<ast::StringLiteral*>(expr)) {
        return "char*";
    } else if (auto *id = dynamic_cast<ast::Identifier*>(expr)) {
        Symbol *symbol = symbolTable_.lookupSymbol(id->name);
        return symbol ? symbol->type : "unknown";
    } else if (auto *binExpr = dynamic_cast<ast::BinaryExpr*>(expr)) {
        // For now, assume binary expressions return int
        (void)binExpr; // Suppress unused warning
        return "int";
    }
    
    return "unknown";
}

bool TypeChecker::areTypesCompatible(const std::string &type1, const std::string &type2) {
    // Simplified type compatibility
    if (type1 == type2) {
        return true;
    }
    
    // Allow implicit conversions between numeric types
    std::vector<std::string> numericTypes = {"int", "char", "float", "double"};
    auto isNumeric = [&](const std::string &type) {
        return std::find(numericTypes.begin(), numericTypes.end(), type) != numericTypes.end();
    };
    
    return isNumeric(type1) && isNumeric(type2);
}

void TypeChecker::error(const std::string &message) {
    std::cerr << "Type error: " << message << std::endl;
    hasErrors_ = true;
}

} // namespace sema
