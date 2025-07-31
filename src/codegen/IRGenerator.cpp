#include "codegen/IRGenerator.h"
#include "utils/Error.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

#include <iostream>
#include <sstream>

namespace codegen {

IRGenerator::IRGenerator() {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("main", *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
    
    // Set target triple to avoid warnings during compilation
    module_->setTargetTriple(llvm::sys::getDefaultTargetTriple());
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
    // First pass: Create function declarations (signatures only)
    for (const auto &decl : tu->declarations) {
        if (auto *funcDecl = dynamic_cast<ast::FunctionDecl*>(decl.get())) {
            createFunction(funcDecl->name, funcDecl->returnType, funcDecl->parameters);
        }
    }
    
    // Second pass: Generate function bodies and variable declarations
    for (const auto &decl : tu->declarations) {
        if (auto *funcDecl = dynamic_cast<ast::FunctionDecl*>(decl.get())) {
            visitFunctionDecl(funcDecl);
        } else if (auto *varDecl = dynamic_cast<ast::VarDecl*>(decl.get())) {
            visitVarDecl(varDecl);
        }
    }
}

void IRGenerator::visitFunctionDecl(ast::FunctionDecl *func) {
    // Get the existing function declaration from first pass
    llvm::Function *function = module_->getFunction(func->name);
    if (!function) {
        error("Function declaration not found: " + func->name);
        return;
    }
    
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
    } else if (auto *breakStmt = dynamic_cast<ast::BreakStmt*>(stmt)) {
        return visitBreakStmt(breakStmt);
    } else if (auto *continueStmt = dynamic_cast<ast::ContinueStmt*>(stmt)) {
        return visitContinueStmt(continueStmt);
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
    // Only add branch if the block is not already terminated
    if (!thenBlock->getTerminator()) {
        builder_->CreateBr(mergeBlock);
    }
    
    // Generate else block if present
    if (elseBlock) {
        function->insert(function->end(), elseBlock);
        builder_->SetInsertPoint(elseBlock);
        visitStmt(stmt->elseStmt.get());
        // Only add branch if the block is not already terminated
        if (!elseBlock->getTerminator()) {
            builder_->CreateBr(mergeBlock);
        }
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
    
    // Push loop context for break/continue
    loopStack_.push_back({loopBlock, afterBlock}); // continue goes to loop condition, break goes to after
    
    builder_->CreateBr(loopBlock);
    builder_->SetInsertPoint(loopBlock);
    
    llvm::Value *condValue = visitExpr(stmt->condition.get());
    condValue = builder_->CreateICmpNE(condValue, 
        llvm::ConstantInt::get(*context_, llvm::APInt(32, 0)), "loopcond");
    
    builder_->CreateCondBr(condValue, bodyBlock, afterBlock);
    
    builder_->SetInsertPoint(bodyBlock);
    visitStmt(stmt->body.get());
    // Only add branch if the block is not already terminated (in case of break/continue)
    if (!bodyBlock->getTerminator()) {
        builder_->CreateBr(loopBlock);
    }
    
    // Pop loop context
    loopStack_.pop_back();
    
    builder_->SetInsertPoint(afterBlock);
    
    return nullptr;
}

llvm::Value* IRGenerator::visitForStmt(ast::ForStmt *stmt) {
    // A for loop has four parts:
    // 1. Initialization (executed once before the loop)
    // 2. Condition (checked before each iteration)
    // 3. Increment (executed after each iteration)
    // 4. Body (the loop body)
    
    // Create basic blocks
    llvm::Function *function = builder_->GetInsertBlock()->getParent();
    llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context_, "for.loop", function);
    llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(*context_, "for.body", function);
    llvm::BasicBlock *incrementBlock = llvm::BasicBlock::Create(*context_, "for.inc", function);
    llvm::BasicBlock *afterBlock = llvm::BasicBlock::Create(*context_, "for.end", function);
    
    // Generate initialization code
    if (stmt->init) {
        visitStmt(stmt->init.get());
    }
    
    // Push loop context for break/continue (continue goes to increment, break goes to after)
    loopStack_.push_back({incrementBlock, afterBlock});
    
    // Jump to loop condition check
    builder_->CreateBr(loopBlock);
    
    // Generate loop condition block
    builder_->SetInsertPoint(loopBlock);
    if (stmt->condition) {
        llvm::Value *condValue = visitExpr(stmt->condition.get());
        // Convert to boolean if needed
        if (condValue->getType()->isIntegerTy() && condValue->getType()->getIntegerBitWidth() != 1) {
            condValue = builder_->CreateICmpNE(condValue, 
                llvm::ConstantInt::get(condValue->getType(), 0), "for.cond");
        }
        builder_->CreateCondBr(condValue, bodyBlock, afterBlock);
    } else {
        // No condition means infinite loop (until break)
        builder_->CreateBr(bodyBlock);
    }
    
    // Generate loop body
    builder_->SetInsertPoint(bodyBlock);
    if (stmt->body) {
        visitStmt(stmt->body.get());
    }
    
    // Only jump to increment if not already terminated (break/continue)
    if (!bodyBlock->getTerminator()) {
        builder_->CreateBr(incrementBlock);
    }
    
    // Generate increment block
    builder_->SetInsertPoint(incrementBlock);
    if (stmt->increment) {
        visitExpr(stmt->increment.get());
    }
    
    // Jump back to condition check
    builder_->CreateBr(loopBlock);
    
    // Pop loop context
    loopStack_.pop_back();
    
    // Continue with code after the loop
    builder_->SetInsertPoint(afterBlock);
    
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
        case ast::BinaryExpr::OpKind::LT: {
            llvm::Value *cmp = builder_->CreateICmpSLT(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
        case ast::BinaryExpr::OpKind::GT: {
            llvm::Value *cmp = builder_->CreateICmpSGT(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
        case ast::BinaryExpr::OpKind::LE: {
            llvm::Value *cmp = builder_->CreateICmpSLE(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
        case ast::BinaryExpr::OpKind::GE: {
            llvm::Value *cmp = builder_->CreateICmpSGE(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
        case ast::BinaryExpr::OpKind::EQ: {
            llvm::Value *cmp = builder_->CreateICmpEQ(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
        case ast::BinaryExpr::OpKind::NE: {
            llvm::Value *cmp = builder_->CreateICmpNE(left, right, "cmptmp");
            return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_), "booltmp");
        }
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
        case ast::BinaryExpr::OpKind::AddAssign:
        case ast::BinaryExpr::OpKind::SubAssign:
        case ast::BinaryExpr::OpKind::MulAssign:
        case ast::BinaryExpr::OpKind::DivAssign:
        case ast::BinaryExpr::OpKind::ModAssign:
            // TODO: Implement compound assignment operators
            error("Compound assignment operators not yet implemented");
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
        case ast::UnaryExpr::OpKind::AddressOf: {
            // For address-of, we need the address of the operand
            // This should only work on lvalue expressions (variables)
            if (auto *id = dynamic_cast<ast::Identifier*>(expr->operand.get())) {
                auto it = namedValues_.find(id->name);
                if (it != namedValues_.end()) {
                    // Return the alloca instruction itself (the address)
                    return it->second;
                }
                error("Variable not found: " + id->name);
                return nullptr;
            }
            error("Address-of operator requires an lvalue");
            return nullptr;
        }
        case ast::UnaryExpr::OpKind::Dereference: {
            // For dereference, operand should be a pointer, load from it
            // In modern LLVM, we need to determine the element type from context
            // For now, assume int pointers (most common case)
            llvm::Type *elementType = llvm::Type::getInt32Ty(*context_);
            return builder_->CreateLoad(elementType, operand, "deref");
        }
        default:
            error("Unsupported unary operator");
            return nullptr;
    }
}

llvm::Value* IRGenerator::visitCallExpr(ast::CallExpr *expr) {
    // Get the function being called
    auto *funcExpr = expr->function.get();
    
    // For now, only handle direct function calls (identifier)
    auto *identifier = dynamic_cast<ast::Identifier*>(funcExpr);
    if (!identifier) {
        error("Only direct function calls are supported");
        return nullptr;
    }
    
    std::string funcName = identifier->name;
    
    // Look up the function in the module
    llvm::Function *function = module_->getFunction(funcName);
    if (!function) {
        error("Unknown function name: " + funcName);
        return nullptr;
    }
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    for (auto &arg : expr->arguments) {
        llvm::Value *argValue = visitExpr(arg.get());
        if (!argValue) {
            error("Failed to generate argument");
            return nullptr;
        }
        args.push_back(argValue);
    }
    
    // Check argument count
    if (args.size() != function->arg_size()) {
        error("Function call argument count mismatch");
        return nullptr;
    }
    
    // Create the function call
    return builder_->CreateCall(function, args, "calltmp");
}

llvm::Type* IRGenerator::getLLVMType(const std::string &cType) {
    // Handle pointer types
    if (cType.back() == '*') {
        // Modern LLVM uses opaque pointers, so we just return a pointer type
        return llvm::PointerType::get(*context_, 0);
    }
    
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

llvm::Value* IRGenerator::visitBreakStmt(ast::BreakStmt *stmt) {
    (void)stmt; // Suppress unused parameter warning
    if (loopStack_.empty()) {
        error("Break statement not within a loop");
        return nullptr;
    }
    
    // Jump to the break block of the innermost loop
    builder_->CreateBr(loopStack_.back().breakBlock);
    
    return nullptr;
}

llvm::Value* IRGenerator::visitContinueStmt(ast::ContinueStmt *stmt) {
    (void)stmt; // Suppress unused parameter warning
    if (loopStack_.empty()) {
        error("Continue statement not within a loop");
        return nullptr;
    }
    
    // Jump to the continue block of the innermost loop
    builder_->CreateBr(loopStack_.back().continueBlock);
    
    return nullptr;
}

void IRGenerator::error(const std::string &message) {
    throw std::runtime_error("IR Generation error: " + message);
}

} // namespace codegen
