#pragma once

#include "ast/Node.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace codegen {

/**
 * LLVM IR generator for AST nodes.
 */
class IRGenerator {
public:
    IRGenerator();
    ~IRGenerator() = default;
    
    /**
     * Generate LLVM IR for a translation unit.
     */
    std::string generateIR(ast::TranslationUnit *tu);
    
private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    
    // Symbol table for variables
    std::unordered_map<std::string, llvm::Value*> namedValues_;
    
    // Current function being generated
    llvm::Function *currentFunction_ = nullptr;
    
    // Visit methods for different AST node types
    void visitTranslationUnit(ast::TranslationUnit *tu);
    void visitFunctionDecl(ast::FunctionDecl *func);
    void visitVarDecl(ast::VarDecl *var);
    
    llvm::Value* visitStmt(ast::Stmt *stmt);
    llvm::Value* visitCompoundStmt(ast::CompoundStmt *stmt);
    llvm::Value* visitExprStmt(ast::ExprStmt *stmt);
    llvm::Value* visitReturnStmt(ast::ReturnStmt *stmt);
    llvm::Value* visitIfStmt(ast::IfStmt *stmt);
    llvm::Value* visitWhileStmt(ast::WhileStmt *stmt);
    llvm::Value* visitForStmt(ast::ForStmt *stmt);
    
    llvm::Value* visitExpr(ast::Expr *expr);
    llvm::Value* visitIntegerLiteral(ast::IntegerLiteral *lit);
    llvm::Value* visitFloatingLiteral(ast::FloatingLiteral *lit);
    llvm::Value* visitCharacterLiteral(ast::CharacterLiteral *lit);
    llvm::Value* visitStringLiteral(ast::StringLiteral *lit);
    llvm::Value* visitIdentifier(ast::Identifier *id);
    llvm::Value* visitBinaryExpr(ast::BinaryExpr *expr);
    llvm::Value* visitUnaryExpr(ast::UnaryExpr *expr);
    llvm::Value* visitCallExpr(ast::CallExpr *expr);
    
    // Helper methods
    llvm::Type* getLLVMType(const std::string &cType);
    llvm::Function* createFunction(const std::string &name, const std::string &returnType,
                                   const std::vector<std::pair<std::string, std::string>> &params);
    
    // Error handling
    void error(const std::string &message);
};

} // namespace codegen
