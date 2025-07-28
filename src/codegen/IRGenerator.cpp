#include "codegen/IRGenerator.h"
#include "utils/Error.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <sstream>

namespace codegen {

IRGenerator::IRGenerator() {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("main", *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

std::string IRGenerator::generateIR(ast::TranslationUnit *tu) {
    visitTranslationUnit(tu);
    
    // Verify the module
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyModule(*module_, &errorStream)) {
        throw std::runtime_error("Module verification failed: " + error);
    }
    
    // Convert module to string
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    module_->print(irStream, nullptr);
    
    return ir;
}

void IRGenerator::visitTranslationUnit(ast::TranslationUnit *tu) {
    for (const auto &decl : tu->declarations) {
        if (auto *funcDecl = dynamic_cast<ast::FunctionDecl*>(decl.get())) {
            visitFunctionDecl(funcDecl);
        } else if (auto *varDecl = dynamic_cast<ast::VarDecl*>(decl.get())) {
            visitVarDecl(varDecl);
        }
    }
}

void IRGenerator::visitFunctionDecl(ast::FunctionDecl *func) {
    llvm::Function *function = createFunction(func->name, func->returnType, func->parameters);
    
    if (func->isDefinition()) {
        currentFunction_ = function;
        
        // Create entry basic block
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(*context_, "entry", function);
        builder_->SetInsertPoint(entry);
        
        // Clear previous function's named values
        namedValues_.clear();
        
        // Add function parameters to symbol table
        auto paramIt = func->parameters.begin();
        for (auto &arg : function->args()) {
            if (paramIt != func->parameters.end()) {
                arg.setName(paramIt->second);
                namedValues_[paramIt->second] = &arg;
                ++paramIt;
            }
        }
        
        // Generate function body
        visitCompoundStmt(func->body.get());
        
        // Verify function
        std::string error;
        llvm::raw_string_ostream errorStream(error);
        if (llvm::verifyFunction(*function, &errorStream)) {
            throw std::runtime_error("Function verification failed: " + error);
        }
        
        currentFunction_ = nullptr;
    }
}

void IRGenerator::visitVarDecl(ast::VarDecl *var) {
    llvm::Type *type = getLLVMType(var->type);
    
    if (currentFunction_) {
        // Local variable
        llvm::AllocaInst *alloca = builder_->CreateAlloca(type, nullptr, var->name);
        namedValues_[var->name] = alloca;
        
        if (var->initializer) {
            llvm::Value *initValue = visitExpr(var->initializer.get());
            builder_->CreateStore(initValue, alloca);
        }
    } else {
        // Global variable
        llvm::Constant *initializer = nullptr;
        if (var->initializer) {
            // For now, only support constant initializers for globals
            if (auto *intLit = dynamic_cast<ast::IntegerLiteral*>(var->initializer.get())) {
                initializer = llvm::ConstantInt::get(type, intLit->value);
            } else {
                initializer = llvm::Constant::getNullValue(type);
            }
        } else {
            initializer = llvm::Constant::getNullValue(type);
        }
        
        auto *globalVar = new llvm::GlobalVariable(
            *module_, type, false, llvm::GlobalValue::ExternalLinkage,
            initializer, var->name
        );
        namedValues_[var->name] = globalVar;
    }
}

llvm::Value* IRGenerator::visitStmt(ast::Stmt *stmt) {
    if (auto *compoundStmt = dynamic_cast<ast::CompoundStmt*>(stmt)) {
        return visitCompoundStmt(compoundStmt);
    } else if (auto *exprStmt = dynamic_cast<ast::ExprStmt*>(stmt)) {
        return visitExprStmt(exprStmt);
    } else if (auto *returnStmt = dynamic_cast<ast::ReturnStmt*>(stmt)) {
        return visitReturnStmt(returnStmt);
    } else if (auto *ifStmt = dynamic_cast<ast::IfStmt*>(stmt)) {
        return visitIfStmt(ifStmt);
    } else if (auto *whileStmt = dynamic_cast<ast::WhileStmt*>(stmt)) {
        return visitWhileStmt(whileStmt);
    } else if (auto *forStmt = dynamic_cast<ast::ForStmt*>(stmt)) {
        return visitForStmt(forStmt);
    } else if (auto *varDecl = dynamic_cast<ast::VarDecl*>(stmt)) {
        visitVarDecl(varDecl);
        return nullptr;
    } else if (dynamic_cast<ast::BreakStmt*>(stmt) || dynamic_cast<ast::ContinueStmt*>(stmt)) {
        // TODO: Implement break/continue with loop context
        return nullptr;
    }
    
    error("Unsupported statement type");
    return nullptr;
}

llvm::Value* IRGenerator::visitCompoundStmt(ast::CompoundStmt *stmt) {
    llvm::Value *lastValue = nullptr;
    
    for (const auto &s : stmt->statements) {
        lastValue = visitStmt(s.get());
    }
    
    return lastValue;
}

llvm::Value* IRGenerator::visitExprStmt(ast::ExprStmt *stmt) {
    if (stmt->expression) {
        return visitExpr(stmt->expression.get());
    }
    return nullptr;
}

llvm::Value* IRGenerator::visitReturnStmt(ast::ReturnStmt *stmt) {
    if (stmt->expression) {
        llvm::Value *retValue = visitExpr(stmt->expression.get());
        return builder_->CreateRet(retValue);
    } else {
        return builder_->CreateRetVoid();
    }
}

llvm::Value* IRGenerator::visitIfStmt(ast::IfStmt *stmt) {
    llvm::Value *condValue = visitExpr(stmt->condition.get());
    
    // Convert condition to boolean
    condValue = builder_->CreateICmpNE(condValue, 
        llvm::ConstantInt::get(*context_, llvm::APInt(32, 0)), "ifcond");
    
    llvm::Function *function = builder_->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context_, "then", function);
    llvm::BasicBlock *elseBlock = stmt->elseStmt ? 
        llvm::BasicBlock::Create(*context_, "else") : nullptr;
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context_, "ifcont");
    
    if (elseBlock) {
        builder_->CreateCondBr(condValue, thenBlock, elseBlock);
    } else {
        builder_->CreateCondBr(condValue, thenBlock, mergeBlock);
    }
    
    // Generate then block
    builder_->SetInsertPoint(thenBlock);
    visitStmt(stmt->thenStmt.get());
    builder_->CreateBr(mergeBlock);
    
    // Generate else block if present
    if (elseBlock) {
        function->insert(function->end(), elseBlock);
        builder_->SetInsertPoint(elseBlock);
        visitStmt(stmt->elseStmt.get());
        builder_->CreateBr(mergeBlock);
    }
    
    // Continue with merge block
    function->insert(function->end(), mergeBlock);
    builder_->SetInsertPoint(mergeBlock);
    
    return nullptr;
}

llvm::Value* IRGenerator::visitWhileStmt(ast::WhileStmt *stmt) {
    llvm::Function *function = builder_->GetInsertBlock()->getParent();
    llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context_, "loop", function);
    llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(*context_, "body", function);
    llvm::BasicBlock *afterBlock = llvm::BasicBlock::Create(*context_, "afterloop", function);
    
    builder_->CreateBr(loopBlock);
    builder_->SetInsertPoint(loopBlock);
    
    llvm::Value *condValue = visitExpr(stmt->condition.get());
    condValue = builder_->CreateICmpNE(condValue, 
        llvm::ConstantInt::get(*context_, llvm::APInt(32, 0)), "loopcond");
    
    builder_->CreateCondBr(condValue, bodyBlock, afterBlock);
    
    builder_->SetInsertPoint(bodyBlock);
    visitStmt(stmt->body.get());
    builder_->CreateBr(loopBlock);
    
    builder_->SetInsertPoint(afterBlock);
    
    return nullptr;
}

llvm::Value* IRGenerator::visitForStmt(ast::ForStmt *stmt) {
    // TODO: Implement for loops
    error("For loops not yet implemented");
    return nullptr;
}

llvm::Value* IRGenerator::visitExpr(ast::Expr *expr) {
    if (auto *intLit = dynamic_cast<ast::IntegerLiteral*>(expr)) {
        return visitIntegerLiteral(intLit);
    } else if (auto *floatLit = dynamic_cast<ast::FloatingLiteral*>(expr)) {
        return visitFloatingLiteral(floatLit);
    } else if (auto *charLit = dynamic_cast<ast::CharacterLiteral*>(expr)) {
        return visitCharacterLiteral(charLit);
    } else if (auto *strLit = dynamic_cast<ast::StringLiteral*>(expr)) {
        return visitStringLiteral(strLit);
    } else if (auto *id = dynamic_cast<ast::Identifier*>(expr)) {
        return visitIdentifier(id);
    } else if (auto *binExpr = dynamic_cast<ast::BinaryExpr*>(expr)) {
        return visitBinaryExpr(binExpr);
    } else if (auto *unaryExpr = dynamic_cast<ast::UnaryExpr*>(expr)) {
        return visitUnaryExpr(unaryExpr);
    } else if (auto *callExpr = dynamic_cast<ast::CallExpr*>(expr)) {
        return visitCallExpr(callExpr);
    }
    
    error("Unsupported expression type");
    return nullptr;
}

llvm::Value* IRGenerator::visitIntegerLiteral(ast::IntegerLiteral *lit) {
    return llvm::ConstantInt::get(*context_, llvm::APInt(32, lit->value));
}

llvm::Value* IRGenerator::visitFloatingLiteral(ast::FloatingLiteral *lit) {
    return llvm::ConstantFP::get(*context_, llvm::APFloat(lit->value));
}

llvm::Value* IRGenerator::visitCharacterLiteral(ast::CharacterLiteral *lit) {
    return llvm::ConstantInt::get(*context_, llvm::APInt(8, lit->value));
}

llvm::Value* IRGenerator::visitStringLiteral(ast::StringLiteral *lit) {
    return builder_->CreateGlobalString(lit->value);
}

llvm::Value* IRGenerator::visitIdentifier(ast::Identifier *id) {
    llvm::Value *value = namedValues_[id->name];
    if (!value) {
        error("Unknown variable name: " + id->name);
        return nullptr;
    }
    
    // If it's an alloca (local variable), load its value
    if (llvm::isa<llvm::AllocaInst>(value)) {
        // For LLVM 20+, we need to explicitly cast to get the element type
        auto allocaInst = llvm::cast<llvm::AllocaInst>(value);
        return builder_->CreateLoad(allocaInst->getAllocatedType(), value, id->name);
    }
    
    return value;
}

llvm::Value* IRGenerator::visitBinaryExpr(ast::BinaryExpr *expr) {
    llvm::Value *left = visitExpr(expr->left.get());
    llvm::Value *right = visitExpr(expr->right.get());
    
    if (!left || !right) {
        return nullptr;
    }
    
    switch (expr->op) {
        case ast::BinaryExpr::OpKind::Add:
            return builder_->CreateAdd(left, right, "addtmp");
        case ast::BinaryExpr::OpKind::Sub:
            return builder_->CreateSub(left, right, "subtmp");
        case ast::BinaryExpr::OpKind::Mul:
            return builder_->CreateMul(left, right, "multmp");
        case ast::BinaryExpr::OpKind::Div:
            return builder_->CreateSDiv(left, right, "divtmp");
        case ast::BinaryExpr::OpKind::Mod:
            return builder_->CreateSRem(left, right, "modtmp");
        case ast::BinaryExpr::OpKind::LT:
            return builder_->CreateICmpSLT(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::GT:
            return builder_->CreateICmpSGT(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::LE:
            return builder_->CreateICmpSLE(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::GE:
            return builder_->CreateICmpSGE(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::EQ:
            return builder_->CreateICmpEQ(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::NE:
            return builder_->CreateICmpNE(left, right, "cmptmp");
        case ast::BinaryExpr::OpKind::LogicalAnd:
            return builder_->CreateAnd(left, right, "andtmp");
        case ast::BinaryExpr::OpKind::LogicalOr:
            return builder_->CreateOr(left, right, "ortmp");
        case ast::BinaryExpr::OpKind::BitwiseAnd:
            return builder_->CreateAnd(left, right, "andtmp");
        case ast::BinaryExpr::OpKind::BitwiseOr:
            return builder_->CreateOr(left, right, "ortmp");
        case ast::BinaryExpr::OpKind::BitwiseXor:
            return builder_->CreateXor(left, right, "xortmp");
        case ast::BinaryExpr::OpKind::LeftShift:
            return builder_->CreateShl(left, right, "shltmp");
        case ast::BinaryExpr::OpKind::RightShift:
            return builder_->CreateAShr(left, right, "shrtmp");
        case ast::BinaryExpr::OpKind::Assign:
            // Assignment: left should be an lvalue
            if (auto *id = dynamic_cast<ast::Identifier*>(expr->left.get())) {
                llvm::Value *var = namedValues_[id->name];
                if (var && llvm::isa<llvm::AllocaInst>(var)) {
                    builder_->CreateStore(right, var);
                    return right;
                }
            }
            error("Invalid assignment target");
            return nullptr;
        default:
            error("Unsupported binary operator");
            return nullptr;
    }
}

llvm::Value* IRGenerator::visitUnaryExpr(ast::UnaryExpr *expr) {
    llvm::Value *operand = visitExpr(expr->operand.get());
    if (!operand) {
        return nullptr;
    }
    
    switch (expr->op) {
        case ast::UnaryExpr::OpKind::Plus:
            return operand; // Unary plus is a no-op
        case ast::UnaryExpr::OpKind::Minus:
            return builder_->CreateNeg(operand, "negtmp");
        case ast::UnaryExpr::OpKind::Not:
            return builder_->CreateNot(operand, "nottmp");
        case ast::UnaryExpr::OpKind::BitwiseNot:
            return builder_->CreateNot(operand, "nottmp");
        default:
            error("Unsupported unary operator");
            return nullptr;
    }
}

llvm::Value* IRGenerator::visitCallExpr(ast::CallExpr *expr) {
    // TODO: Implement function calls
    error("Function calls not yet implemented");
    return nullptr;
}

llvm::Type* IRGenerator::getLLVMType(const std::string &cType) {
    if (cType == "int") {
        return llvm::Type::getInt32Ty(*context_);
    } else if (cType == "char") {
        return llvm::Type::getInt8Ty(*context_);
    } else if (cType == "float") {
        return llvm::Type::getFloatTy(*context_);
    } else if (cType == "double") {
        return llvm::Type::getDoubleTy(*context_);
    } else if (cType == "void") {
        return llvm::Type::getVoidTy(*context_);
    } else {
        // Default to int
        return llvm::Type::getInt32Ty(*context_);
    }
}

llvm::Function* IRGenerator::createFunction(const std::string &name, const std::string &returnType,
                                            const std::vector<std::pair<std::string, std::string>> &params) {
    std::vector<llvm::Type*> paramTypes;
    for (const auto &param : params) {
        paramTypes.push_back(getLLVMType(param.first));
    }
    
    llvm::Type *retType = getLLVMType(returnType);
    llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);
    
    llvm::Function *function = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, module_.get()
    );
    
    return function;
}

void IRGenerator::error(const std::string &message) {
    throw std::runtime_error("IR Generation error: " + message);
}

} // namespace codegen
