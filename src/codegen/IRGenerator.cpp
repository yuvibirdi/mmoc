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
        // Ensure function is properly terminated
        if (!function->getReturnType()->isVoidTy()) {
            if (!builder_->GetInsertBlock()->getTerminator()) {
                // Insert default return 0 for non-void functions
                builder_->CreateRet(llvm::ConstantInt::get(function->getReturnType(), 0));
            }
        } else {
            if (!builder_->GetInsertBlock()->getTerminator()) {
                builder_->CreateRetVoid();
            }
        }
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
    int depth=0; for(char c: var->type) if(c=='*') depth++; pointerDepth_[var->name]=depth;
    if (currentFunction_) {
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

int IRGenerator::computePointerDepth(ast::Expr *expr) {
    if (auto *id = dynamic_cast<ast::Identifier*>(expr)) {
        auto it = pointerDepth_.find(id->name); if(it!=pointerDepth_.end()) return it->second; return 0;
    } else if (auto *un = dynamic_cast<ast::UnaryExpr*>(expr)) {
        if (un->op == ast::UnaryExpr::OpKind::Dereference) {
            // Dereference reduces depth by 1
            int inner = computePointerDepth(un->operand.get());
            return inner>0? inner-1 : 0;
        } else if (un->op == ast::UnaryExpr::OpKind::AddressOf) {
            int inner = computePointerDepth(un->operand.get());
            return inner+1; // & increases depth
        }
    }
    return 0;
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
    // Normalize boolean sized integers
    if (condValue->getType()->isIntegerTy(1)) {
        // Already i1, use directly
    } else if (condValue->getType()->isIntegerTy()) {
        condValue = builder_->CreateICmpNE(condValue, llvm::ConstantInt::get(condValue->getType(), 0), "ifcondnorm");
    }
    // Convert condition to boolean (now condValue should be i1)
    llvm::Function *function = builder_->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context_, "then", function);
    llvm::BasicBlock *elseBlock = stmt->elseStmt ? llvm::BasicBlock::Create(*context_, "else") : nullptr;
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context_, "ifcont");
    if (elseBlock) {
        builder_->CreateCondBr(condValue, thenBlock, elseBlock);
    } else {
        builder_->CreateCondBr(condValue, thenBlock, mergeBlock);
    }
    builder_->SetInsertPoint(thenBlock);
    visitStmt(stmt->thenStmt.get());
    if (!thenBlock->getTerminator()) {
        builder_->CreateBr(mergeBlock);
    }
    if (elseBlock) {
        function->insert(function->end(), elseBlock);
        builder_->SetInsertPoint(elseBlock);
        visitStmt(stmt->elseStmt.get());
        if (!elseBlock->getTerminator()) {
            builder_->CreateBr(mergeBlock);
        }
    }
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
    } else if (auto *condExpr = dynamic_cast<ast::ConditionalExpr*>(expr)) {
        return visitConditionalExpr(condExpr);
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
    llvm::Value *ptr = namedValues_[id->name];
    if(!ptr) {
        // Fallback: treat identifier as function symbol
        if (auto *fn = module_->getFunction(id->name)) {
            return fn; // function pointer usable for direct call via CallExpr elsewhere
        }
        error("Unknown variable name: "+id->name); return nullptr;
    }
    if (auto *ai = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
        return builder_->CreateLoad(ai->getAllocatedType(), ptr, id->name);
    }
    return ptr;
}

llvm::Value* IRGenerator::emitAddress(ast::Expr *expr) {
    if (auto *id = dynamic_cast<ast::Identifier*>(expr)) {
        auto it = namedValues_.find(id->name);
        if (it!=namedValues_.end()) return it->second;
        error("Unknown variable name: "+id->name); return nullptr;
    } else if (auto *un = dynamic_cast<ast::UnaryExpr*>(expr)) {
        if (un->op == ast::UnaryExpr::OpKind::Dereference) {
            // Address of *E is the value of E (a pointer)
            llvm::Value *v = visitExpr(un->operand.get());
            if(!v || !v->getType()->isPointerTy()) { error("Dereference of non-pointer type"); return nullptr; }
            return v;
        }
    }
    error("Not an lvalue expression"); return nullptr;
}
llvm::Value* IRGenerator::loadIdentifier(const std::string &name) {
    auto it = namedValues_.find(name); if(it==namedValues_.end()) return nullptr;
    llvm::Value *allocaPtr = it->second;
    if (auto *ai = llvm::dyn_cast<llvm::AllocaInst>(allocaPtr)) {
        return builder_->CreateLoad(ai->getAllocatedType(), allocaPtr, name);
    }
    return allocaPtr;
}

llvm::Value* IRGenerator::visitUnaryExpr(ast::UnaryExpr *expr) {
    switch (expr->op) {
        case ast::UnaryExpr::OpKind::AddressOf: {
            return emitAddress(expr->operand.get());
        }
        case ast::UnaryExpr::OpKind::Dereference: {
            int operandDepth = computePointerDepth(expr->operand.get());
            if (operandDepth == 0) { error("Dereference of non-pointer type"); return nullptr; }
            int resultDepth = operandDepth - 1;
            llvm::Value *operandVal = emitAddress(expr->operand.get());
            if (!operandVal || !operandVal->getType()->isPointerTy()) { error("Dereference of non-pointer type"); return nullptr; }
            if (resultDepth > 0) {
                llvm::Type *ptrTy = llvm::PointerType::get(*context_, 0);
                return builder_->CreateLoad(ptrTy, operandVal, "derefptr");
            } else {
                llvm::Type *intTy = llvm::Type::getInt32Ty(*context_);
                return builder_->CreateLoad(intTy, operandVal, "derefval");
            }
        }
        case ast::UnaryExpr::OpKind::PreIncrement:
        case ast::UnaryExpr::OpKind::PreDecrement:
        case ast::UnaryExpr::OpKind::PostIncrement:
        case ast::UnaryExpr::OpKind::PostDecrement: {
            // Support only integer variables for now
            llvm::Value *addr = emitAddress(expr->operand.get());
            if(!addr) return nullptr;
            llvm::Type *valTy = llvm::Type::getInt32Ty(*context_);
            llvm::Value *oldVal = builder_->CreateLoad(valTy, addr, "oldinc");
            llvm::Value *one = llvm::ConstantInt::get(valTy, 1);
            bool isInc = (expr->op==ast::UnaryExpr::OpKind::PreIncrement || expr->op==ast::UnaryExpr::OpKind::PostIncrement);
            llvm::Value *newVal = isInc ? builder_->CreateAdd(oldVal, one, "inc") : builder_->CreateSub(oldVal, one, "dec");
            builder_->CreateStore(newVal, addr);
            // Pre returns new, post returns old
            bool isPost = (expr->op==ast::UnaryExpr::OpKind::PostIncrement || expr->op==ast::UnaryExpr::OpKind::PostDecrement);
            return isPost ? oldVal : newVal;
        }
        case ast::UnaryExpr::OpKind::Plus: {
            llvm::Value *v = visitExpr(expr->operand.get());
            return v;
        }
        case ast::UnaryExpr::OpKind::Minus: {
            llvm::Value *v = visitExpr(expr->operand.get());
            return builder_->CreateNeg(v, "negtmp");
        }
        case ast::UnaryExpr::OpKind::Not: {
            llvm::Value *v = visitExpr(expr->operand.get());
            return builder_->CreateNot(v, "nottmp");
        }
        case ast::UnaryExpr::OpKind::BitwiseNot: {
            llvm::Value *v = visitExpr(expr->operand.get());
            return builder_->CreateNot(v, "nottmp");
        }
        default:
            error("Unsupported unary operator");
            return nullptr;
    }
}

llvm::Value* IRGenerator::visitBinaryExpr(ast::BinaryExpr *expr) {
    if (expr->op == ast::BinaryExpr::OpKind::Assign) {
        llvm::Value *rhs = visitExpr(expr->right.get());
        llvm::Value *lhsAddr = emitAddress(expr->left.get());
        if(!lhsAddr) return nullptr;
        builder_->CreateStore(rhs, lhsAddr);
        return rhs;
    }
    // Compound assignments
    if (expr->op == ast::BinaryExpr::OpKind::AddAssign || expr->op == ast::BinaryExpr::OpKind::SubAssign ||
        expr->op == ast::BinaryExpr::OpKind::MulAssign || expr->op == ast::BinaryExpr::OpKind::DivAssign ||
        expr->op == ast::BinaryExpr::OpKind::ModAssign) {
        llvm::Value *lhsAddr = emitAddress(expr->left.get());
        if(!lhsAddr) return nullptr;
        llvm::Value *lhsVal = visitExpr(expr->left.get()); // loads
        llvm::Value *rhsVal = visitExpr(expr->right.get());
        llvm::Value *result=nullptr;
        switch(expr->op){
            case ast::BinaryExpr::OpKind::AddAssign: result=builder_->CreateAdd(lhsVal,rhsVal,"addeq"); break;
            case ast::BinaryExpr::OpKind::SubAssign: result=builder_->CreateSub(lhsVal,rhsVal,"subeq"); break;
            case ast::BinaryExpr::OpKind::MulAssign: result=builder_->CreateMul(lhsVal,rhsVal,"muleq"); break;
            case ast::BinaryExpr::OpKind::DivAssign: result=builder_->CreateSDiv(lhsVal,rhsVal,"diveq"); break;
            case ast::BinaryExpr::OpKind::ModAssign: result=builder_->CreateSRem(lhsVal,rhsVal,"modeq"); break;
            default: break;
        }
        builder_->CreateStore(result,lhsAddr);
        return result;
    }
    // Short-circuit logical AND / OR
    if (expr->op == ast::BinaryExpr::OpKind::LogicalAnd || expr->op == ast::BinaryExpr::OpKind::LogicalOr) {
        llvm::Function *function = builder_->GetInsertBlock()->getParent();
        llvm::BasicBlock *lhsBlock = builder_->GetInsertBlock();
        llvm::Value *lhsVal = visitExpr(expr->left.get());
        // Convert to i1 if needed
        if(lhsVal->getType()->isIntegerTy() && lhsVal->getType()->getIntegerBitWidth()!=1)
            lhsVal = builder_->CreateICmpNE(lhsVal, llvm::ConstantInt::get(lhsVal->getType(),0), "lhsbool");
        llvm::BasicBlock *rhsBlock = llvm::BasicBlock::Create(*context_, expr->op==ast::BinaryExpr::OpKind::LogicalAnd?"and.rhs":"or.rhs", function);
        llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context_, expr->op==ast::BinaryExpr::OpKind::LogicalAnd?"and.merge":"or.merge");
        if (expr->op == ast::BinaryExpr::OpKind::LogicalAnd) {
            builder_->CreateCondBr(lhsVal, rhsBlock, mergeBlock);
        } else { // LogicalOr
            builder_->CreateCondBr(lhsVal, mergeBlock, rhsBlock);
        }
        // RHS
        builder_->SetInsertPoint(rhsBlock);
        llvm::Value *rhsVal = visitExpr(expr->right.get());
        if(rhsVal->getType()->isIntegerTy() && rhsVal->getType()->getIntegerBitWidth()!=1)
            rhsVal = builder_->CreateICmpNE(rhsVal, llvm::ConstantInt::get(rhsVal->getType(),0), "rhsbool");
        builder_->CreateBr(mergeBlock);
        rhsBlock = builder_->GetInsertBlock();
        // Merge
        function->insert(function->end(), mergeBlock);
        builder_->SetInsertPoint(mergeBlock);
        llvm::PHINode *phi = builder_->CreatePHI(llvm::Type::getInt1Ty(*context_), 2, expr->op==ast::BinaryExpr::OpKind::LogicalAnd?"andphi":"orphi");
        if (expr->op == ast::BinaryExpr::OpKind::LogicalAnd) {
            phi->addIncoming(rhsVal, rhsBlock);
            phi->addIncoming(llvm::ConstantInt::getFalse(*context_), lhsBlock);
        } else {
            phi->addIncoming(llvm::ConstantInt::getTrue(*context_), lhsBlock);
            phi->addIncoming(rhsVal, rhsBlock);
        }
        // Extend to int32 like other comparisons
        return builder_->CreateZExt(phi, llvm::Type::getInt32Ty(*context_), "logicext");
    }
    llvm::Value *left = visitExpr(expr->left.get());
    llvm::Value *right = visitExpr(expr->right.get());
    if(!left || !right) return nullptr;
    switch (expr->op) {
        case ast::BinaryExpr::OpKind::Add: return builder_->CreateAdd(left,right,"addtmp");
        case ast::BinaryExpr::OpKind::Sub: return builder_->CreateSub(left,right,"subtmp");
        case ast::BinaryExpr::OpKind::Mul: return builder_->CreateMul(left,right,"multmp");
        case ast::BinaryExpr::OpKind::Div: return builder_->CreateSDiv(left,right,"divtmp");
        case ast::BinaryExpr::OpKind::Mod: return builder_->CreateSRem(left,right,"modtmp");
        case ast::BinaryExpr::OpKind::LT: { auto *cmp=builder_->CreateICmpSLT(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::GT: { auto *cmp=builder_->CreateICmpSGT(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::LE: { auto *cmp=builder_->CreateICmpSLE(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::GE: { auto *cmp=builder_->CreateICmpSGE(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::EQ: { auto *cmp=builder_->CreateICmpEQ(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::NE: { auto *cmp=builder_->CreateICmpNE(left,right,"cmptmp"); return builder_->CreateZExt(cmp, llvm::Type::getInt32Ty(*context_),"booltmp"); }
        case ast::BinaryExpr::OpKind::BitwiseAnd: return builder_->CreateAnd(left,right,"andtmp");
        case ast::BinaryExpr::OpKind::BitwiseOr: return builder_->CreateOr(left,right,"ortmp");
        case ast::BinaryExpr::OpKind::BitwiseXor: return builder_->CreateXor(left,right,"xortmp");
        case ast::BinaryExpr::OpKind::LeftShift: return builder_->CreateShl(left,right,"shltmp");
        case ast::BinaryExpr::OpKind::RightShift: return builder_->CreateAShr(left,right,"shrtmp");
        default: error("Unsupported binary operator"); return nullptr;
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

llvm::Value* IRGenerator::visitConditionalExpr(ast::ConditionalExpr *expr) {
    // Generate condition
    llvm::Value *condValue = visitExpr(expr->condition.get());
    if (!condValue) {
        error("Failed to generate condition for ternary operator");
        return nullptr;
    }
    
    // Convert condition to boolean
    condValue = builder_->CreateICmpNE(condValue, 
        llvm::ConstantInt::get(*context_, llvm::APInt(32, 0)), "condtmp");
    
    // Get current function
    llvm::Function *function = currentFunction_;
    if (!function) {
        error("Ternary operator outside function");
        return nullptr;
    }
    
    // Create basic blocks
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context_, "then", function);
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context_, "else");
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context_, "ifcont");
    
    // Branch based on condition
    builder_->CreateCondBr(condValue, thenBlock, elseBlock);
    
    // Generate true expression
    builder_->SetInsertPoint(thenBlock);
    llvm::Value *thenValue = visitExpr(expr->trueExpr.get());
    if (!thenValue) {
        error("Failed to generate true expression for ternary operator");
        return nullptr;
    }
    builder_->CreateBr(mergeBlock);
    thenBlock = builder_->GetInsertBlock(); // Update in case of nested expressions
    
    // Generate false expression
    function->insert(function->end(), elseBlock);
    builder_->SetInsertPoint(elseBlock);
    llvm::Value *elseValue = visitExpr(expr->falseExpr.get());
    if (!elseValue) {
        error("Failed to generate false expression for ternary operator");
        return nullptr;
    }
    builder_->CreateBr(mergeBlock);
    elseBlock = builder_->GetInsertBlock(); // Update in case of nested expressions
    
    // Merge block with PHI node
    function->insert(function->end(), mergeBlock);
    builder_->SetInsertPoint(mergeBlock);
    
    llvm::PHINode *phi = builder_->CreatePHI(thenValue->getType(), 2, "iftmp");
    phi->addIncoming(thenValue, thenBlock);
    phi->addIncoming(elseValue, elseBlock);
    
    return phi;
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
    } else if (cType == "_Bool") {
        return llvm::Type::getInt1Ty(*context_);  // C11 _Bool type
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
