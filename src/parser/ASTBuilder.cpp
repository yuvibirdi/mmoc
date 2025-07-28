#include "parser/ASTBuilder.h"
#include "CParser.h"
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <iostream>

namespace parser {

antlrcpp::Any ASTBuilder::visitTranslationUnit(CParser::TranslationUnitContext *ctx) {
    std::vector<std::unique_ptr<ast::Node>> declarations;
    
    for (auto *extDecl : ctx->externalDeclaration()) {
        auto result = visit(extDecl);
        try {
            auto *node_ptr = std::any_cast<ast::Node*>(result);
            if (node_ptr) {
                declarations.push_back(std::unique_ptr<ast::Node>(node_ptr));
            }
        } catch (const std::bad_any_cast&) {
            // Skip invalid nodes
        }
    }
    
    // Return raw pointer that will be wrapped by caller
    return static_cast<ast::TranslationUnit*>(new ast::TranslationUnit(std::move(declarations)));
}

antlrcpp::Any ASTBuilder::visitExternalDeclaration(CParser::ExternalDeclarationContext *ctx) {
    if (ctx->functionDefinition()) {
        return visit(ctx->functionDefinition());
    } else if (ctx->declaration()) {
        return visit(ctx->declaration());
    }
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitFunctionDefinition(CParser::FunctionDefinitionContext *ctx) {
    std::string returnType = extractTypeFromSpecifiers(ctx->declarationSpecifiers());
    std::string name = extractIdentifierName(ctx->declarator()->directDeclarator());
    
    std::vector<std::pair<std::string, std::string>> parameters;
    if (auto *directDecl = ctx->declarator()->directDeclarator()) {
        for (auto *child : directDecl->children) {
            if (auto *paramList = dynamic_cast<CParser::ParameterTypeListContext*>(child)) {
                parameters = extractParameters(paramList);
                break;
            }
        }
    }
    
    auto result = visit(ctx->compoundStatement());
    auto *body_ptr = std::any_cast<ast::CompoundStmt*>(result);
    auto body = std::unique_ptr<ast::CompoundStmt>(body_ptr);
    
    // Return raw pointer
    return static_cast<ast::Node*>(new ast::FunctionDecl(name, returnType, std::move(parameters), std::move(body)));
}

antlrcpp::Any ASTBuilder::visitDeclaration(CParser::DeclarationContext *ctx) {
    std::string type = extractTypeFromSpecifiers(ctx->declarationSpecifiers());
    
    if (ctx->initDeclaratorList()) {
        // For now, handle single variable declarations
        auto *initDecl = ctx->initDeclaratorList()->initDeclarator(0);
        std::string name = extractIdentifierName(initDecl->declarator()->directDeclarator());
        
        std::unique_ptr<ast::Expr> initializer = nullptr;
        if (initDecl->initializer()) {
            auto result = visit(initDecl->initializer()->assignmentExpression());
            auto *expr_ptr = std::any_cast<ast::Expr*>(result);
            initializer = std::unique_ptr<ast::Expr>(expr_ptr);
        }
        
        return static_cast<ast::Node*>(new ast::VarDecl(name, type, std::move(initializer)));
    }
    
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitCompoundStatement(CParser::CompoundStatementContext *ctx) {
    std::vector<std::unique_ptr<ast::Stmt>> statements;
    
    if (ctx->blockItemList()) {
        for (auto *item : ctx->blockItemList()->blockItem()) {
            if (item->statement()) {
                auto result = visit(item->statement());
                auto *stmt_ptr = std::any_cast<ast::Stmt*>(result);
                if (stmt_ptr) {
                    statements.push_back(std::unique_ptr<ast::Stmt>(stmt_ptr));
                }
            } else if (item->declaration()) {
                auto result = visit(item->declaration());
                auto *decl_ptr = std::any_cast<ast::Node*>(result);
                if (auto *varDecl = dynamic_cast<ast::VarDecl*>(decl_ptr)) {
                    statements.push_back(std::unique_ptr<ast::VarDecl>(varDecl));
                }
            }
        }
    }
    
    return static_cast<ast::CompoundStmt*>(new ast::CompoundStmt(std::move(statements)));
}

antlrcpp::Any ASTBuilder::visitExpressionStatement(CParser::ExpressionStatementContext *ctx) {
    if (auto *exprCtx = ctx->expression()) {
        auto expr = extractExpr(visit(exprCtx));
        return std::any(static_cast<ast::Stmt*>(new ast::ExprStmt(std::move(expr))));
    }
    return std::any(static_cast<ast::Stmt*>(new ast::ExprStmt(nullptr)));
}

std::any ASTBuilder::visitJumpStatement(CParser::JumpStatementContext *ctx) {
    if (ctx->Return()) {
        auto expr = ctx->expression() ? extractExpr(visit(ctx->expression())) : nullptr;
        return std::any(static_cast<ast::Stmt*>(new ast::ReturnStmt(std::move(expr))));
    } else if (ctx->Break()) {
        return std::any(static_cast<ast::Stmt*>(new ast::BreakStmt()));
    } else if (ctx->Continue()) {
        return std::any(static_cast<ast::Stmt*>(new ast::ContinueStmt()));
    }
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitExpression(CParser::ExpressionContext *ctx) {
    // For comma operator, just return the last expression for simplicity
    auto exprs = ctx->assignmentExpression();
    return visit(exprs.back());
}

antlrcpp::Any ASTBuilder::visitAssignmentExpression(CParser::AssignmentExpressionContext *ctx) {
    if (ctx->assignmentOperator()) {
        auto leftResult = visit(ctx->unaryExpression());
        auto left = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(leftResult));
        auto rightResult = visit(ctx->assignmentExpression());
        auto right = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(rightResult));
        
        // For now, only handle simple assignment
        return std::any(static_cast<ast::Expr*>(new ast::BinaryExpr(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::Assign
        )));
    }
    return visit(ctx->conditionalExpression());
}

antlrcpp::Any ASTBuilder::visitConditionalExpression(CParser::ConditionalExpressionContext *ctx) {
    // TODO: Implement ternary operator when needed
    return visit(ctx->logicalOrExpression());
}

antlrcpp::Any ASTBuilder::visitLogicalOrExpression(CParser::LogicalOrExpressionContext *ctx) {
    auto leftResult = visit(ctx->logicalAndExpression(0));
    auto left = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(leftResult));
    
    for (size_t i = 1; i < ctx->logicalAndExpression().size(); ++i) {
        auto rightResult = visit(ctx->logicalAndExpression(i));
        auto right = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(rightResult));
        left = std::make_unique<ast::BinaryExpr>(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::LogicalOr
        );
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitLogicalAndExpression(CParser::LogicalAndExpressionContext *ctx) {
    auto leftResult = visit(ctx->inclusiveOrExpression(0));
    auto left = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(leftResult));
    
    for (size_t i = 1; i < ctx->inclusiveOrExpression().size(); ++i) {
        auto rightResult = visit(ctx->inclusiveOrExpression(i));
        auto right = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(rightResult));
        left = std::make_unique<ast::BinaryExpr>(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::LogicalAnd
        );
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitInclusiveOrExpression(CParser::InclusiveOrExpressionContext *ctx) {
    auto leftResult = visit(ctx->exclusiveOrExpression(0));
    auto left = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(leftResult));
    
    for (size_t i = 1; i < ctx->exclusiveOrExpression().size(); ++i) {
        auto rightResult = visit(ctx->exclusiveOrExpression(i));
        auto right = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(rightResult));
        left = std::make_unique<ast::BinaryExpr>(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::BitwiseOr
        );
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitExclusiveOrExpression(CParser::ExclusiveOrExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->andExpression(0)));
    
    for (size_t i = 1; i < ctx->andExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->andExpression(i)));
        left = std::make_unique<ast::BinaryExpr>(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::BitwiseXor
        );
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitAndExpression(CParser::AndExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->equalityExpression(0)));
    
    for (size_t i = 1; i < ctx->equalityExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->equalityExpression(i)));
        left = std::make_unique<ast::BinaryExpr>(
            std::move(left), std::move(right), ast::BinaryExpr::OpKind::BitwiseAnd
        );
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitEqualityExpression(CParser::EqualityExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->relationalExpression(0)));
    
    for (size_t i = 1; i < ctx->relationalExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->relationalExpression(i)));
        
        ast::BinaryExpr::OpKind op;
        if (i - 1 < ctx->children.size()) {
            auto *token = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i - 1]);
            if (token && token->getSymbol()->getType() == CParser::Equal) {
                op = ast::BinaryExpr::OpKind::EQ;
            } else {
                op = ast::BinaryExpr::OpKind::NE;
            }
        } else {
            op = ast::BinaryExpr::OpKind::EQ;
        }
        
        left = std::make_unique<ast::BinaryExpr>(std::move(left), std::move(right), op);
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitRelationalExpression(CParser::RelationalExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->shiftExpression(0)));
    
    for (size_t i = 1; i < ctx->shiftExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->shiftExpression(i)));
        
        // Simplified - assume < for now
        ast::BinaryExpr::OpKind op = ast::BinaryExpr::OpKind::LT;
        
        left = std::make_unique<ast::BinaryExpr>(std::move(left), std::move(right), op);
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitShiftExpression(CParser::ShiftExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->additiveExpression(0)));
    
    for (size_t i = 1; i < ctx->additiveExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->additiveExpression(i)));
        
        // Simplified - assume << for now
        ast::BinaryExpr::OpKind op = ast::BinaryExpr::OpKind::LeftShift;
        
        left = std::make_unique<ast::BinaryExpr>(std::move(left), std::move(right), op);
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitAdditiveExpression(CParser::AdditiveExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->multiplicativeExpression(0)));
    
    for (size_t i = 1; i < ctx->multiplicativeExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->multiplicativeExpression(i)));
        
        ast::BinaryExpr::OpKind op;
        if (i - 1 < ctx->children.size()) {
            auto *token = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i - 1]);
            if (token && token->getSymbol()->getType() == CParser::Plus) {
                op = ast::BinaryExpr::OpKind::Add;
            } else {
                op = ast::BinaryExpr::OpKind::Sub;
            }
        } else {
            op = ast::BinaryExpr::OpKind::Add;
        }
        
        left = std::make_unique<ast::BinaryExpr>(std::move(left), std::move(right), op);
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitMultiplicativeExpression(CParser::MultiplicativeExpressionContext *ctx) {
    auto left = extractExpr(visit(ctx->castExpression(0)));
    
    for (size_t i = 1; i < ctx->castExpression().size(); ++i) {
        auto right = extractExpr(visit(ctx->castExpression(i)));
        
        ast::BinaryExpr::OpKind op;
        if (i - 1 < ctx->children.size()) {
            auto *token = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i - 1]);
            if (token) {
                switch (token->getSymbol()->getType()) {
                    case CParser::Star: op = ast::BinaryExpr::OpKind::Mul; break;
                    case CParser::Div: op = ast::BinaryExpr::OpKind::Div; break;
                    case CParser::Mod: op = ast::BinaryExpr::OpKind::Mod; break;
                    default: op = ast::BinaryExpr::OpKind::Mul; break;
                }
            } else {
                op = ast::BinaryExpr::OpKind::Mul;
            }
        } else {
            op = ast::BinaryExpr::OpKind::Mul;
        }
        
        left = std::make_unique<ast::BinaryExpr>(std::move(left), std::move(right), op);
    }
    
    return std::any(left.release());
}

antlrcpp::Any ASTBuilder::visitCastExpression(CParser::CastExpressionContext *ctx) {
    // For now, skip cast expressions and just visit unary
    return visit(ctx->unaryExpression());
}

antlrcpp::Any ASTBuilder::visitUnaryExpression(CParser::UnaryExpressionContext *ctx) {
    if (ctx->postfixExpression()) {
        return visit(ctx->postfixExpression());
    } else if (ctx->unaryOperator()) {
        auto operand = extractExpr(visit(ctx->castExpression()));
        
        ast::UnaryExpr::OpKind op;
        auto *opCtx = ctx->unaryOperator();
        std::string opText = opCtx->getText();
        
        if (opText == "&") op = ast::UnaryExpr::OpKind::AddressOf;
        else if (opText == "*") op = ast::UnaryExpr::OpKind::Dereference;
        else if (opText == "+") op = ast::UnaryExpr::OpKind::Plus;
        else if (opText == "-") op = ast::UnaryExpr::OpKind::Minus;
        else if (opText == "~") op = ast::UnaryExpr::OpKind::BitwiseNot;
        else if (opText == "!") op = ast::UnaryExpr::OpKind::Not;
        else op = ast::UnaryExpr::OpKind::Plus;
        
        return std::any(static_cast<ast::Expr*>(new ast::UnaryExpr(std::move(operand), op, true)));
    }
    
    return visit(ctx->postfixExpression());
}

antlrcpp::Any ASTBuilder::visitPostfixExpression(CParser::PostfixExpressionContext *ctx) {
    if (ctx->primaryExpression()) {
        return visit(ctx->primaryExpression());
    }
    
    // Handle function calls, array subscripts, etc.
    // For now, just return primary expression
    return visit(ctx->primaryExpression());
}

antlrcpp::Any ASTBuilder::visitPrimaryExpression(CParser::PrimaryExpressionContext *ctx) {
    if (ctx->Identifier()) {
        return std::any(static_cast<ast::Expr*>(new ast::Identifier(ctx->Identifier()->getText())));
    } else if (ctx->Constant()) {
        std::string text = ctx->Constant()->getText();
        
        // Determine constant type
        if (text.find('.') != std::string::npos || text.find('e') != std::string::npos || 
            text.find('E') != std::string::npos) {
            return std::any(static_cast<ast::Expr*>(new ast::FloatingLiteral(parseFloatingConstant(text))));
        } else if (text.front() == '\'' && text.back() == '\'') {
            return std::any(static_cast<ast::Expr*>(new ast::CharacterLiteral(parseCharacterConstant(text))));
        } else {
            return std::any(static_cast<ast::Expr*>(new ast::IntegerLiteral(parseIntegerConstant(text))));
        }
    } else if (!ctx->StringLiteral().empty()) {
        return std::any(static_cast<ast::Expr*>(new ast::StringLiteral(parseStringLiteral(ctx->StringLiteral(0)->getText()))));
    } else if (ctx->expression()) {
        return visit(ctx->expression());
    }
    
    return nullptr;
}

// Helper methods implementation

std::string ASTBuilder::extractTypeFromSpecifiers(CParser::DeclarationSpecifiersContext *ctx) {
    std::string type;
    for (auto *child : ctx->children) {
        if (auto *typeSpec = dynamic_cast<CParser::TypeSpecifierContext*>(child)) {
            // Use text-based parsing since the grammar uses literals
            std::string text = typeSpec->getText();
            if (text == "int") type += "int";
            else if (text == "char") type += "char";
            else if (text == "float") type += "float";
            else if (text == "double") type += "double";
            else if (text == "void") type += "void";
            else if (text == "long") type += "long";
            else if (text == "short") type += "short";
            else if (text == "signed") type += "signed";
            else if (text == "unsigned") type += "unsigned";
            else type += text; // Default fallback
        }
    }
    return type.empty() ? "int" : type; // Default to int
}

std::string ASTBuilder::extractIdentifierName(CParser::DirectDeclaratorContext *ctx) {
    if (ctx->Identifier()) {
        return ctx->Identifier()->getText();
    }
    
    // Handle recursive cases like "directDeclarator '(' parameterTypeList ')'"
    // The identifier should be in the child directDeclarator
    if (ctx->directDeclarator()) {
        return extractIdentifierName(ctx->directDeclarator());
    }
    
    return "";
}

std::vector<std::pair<std::string, std::string>> ASTBuilder::extractParameters(CParser::ParameterTypeListContext *ctx) {
    std::vector<std::pair<std::string, std::string>> params;
    
    if (ctx->parameterList()) {
        for (auto *paramDecl : ctx->parameterList()->parameterDeclaration()) {
            std::string type = extractTypeFromSpecifiers(paramDecl->declarationSpecifiers());
            std::string name;
            if (paramDecl->declarator()) {
                name = extractIdentifierName(paramDecl->declarator()->directDeclarator());
            }
            params.emplace_back(type, name);
        }
    }
    
    return params;
}

long long ASTBuilder::parseIntegerConstant(const std::string &text) {
    try {
        if (text.size() > 2 && text.substr(0, 2) == "0x") {
            return std::stoll(text, nullptr, 16);
        } else if (text.size() > 1 && text[0] == '0') {
            return std::stoll(text, nullptr, 8);
        } else {
            return std::stoll(text, nullptr, 10);
        }
    } catch (const std::exception&) {
        return 0;
    }
}

double ASTBuilder::parseFloatingConstant(const std::string &text) {
    try {
        return std::stod(text);
    } catch (const std::exception&) {
        return 0.0;
    }
}

char ASTBuilder::parseCharacterConstant(const std::string &text) {
    if (text.size() >= 3) {
        return text[1]; // Simple case: 'a' -> a
    }
    return '\0';
}

std::string ASTBuilder::parseStringLiteral(const std::string &text) {
    if (text.size() >= 2) {
        return text.substr(1, text.size() - 2); // Remove quotes
    }
    return "";
}

std::unique_ptr<ast::Expr> ASTBuilder::extractExpr(const antlrcpp::Any &result) {
    try {
        auto *ptr = std::any_cast<ast::Expr*>(result);
        return std::unique_ptr<ast::Expr>(ptr);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

std::unique_ptr<ast::Stmt> ASTBuilder::extractStmt(const antlrcpp::Any &result) {
    try {
        auto *ptr = std::any_cast<ast::Stmt*>(result);
        return std::unique_ptr<ast::Stmt>(ptr);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

// Missing method implementations (stubs for now)

antlrcpp::Any ASTBuilder::visitStatement(CParser::StatementContext *ctx) {
    if (ctx->compoundStatement()) {
        return visit(ctx->compoundStatement());
    } else if (ctx->expressionStatement()) {
        return visit(ctx->expressionStatement());
    } else if (ctx->selectionStatement()) {
        return visit(ctx->selectionStatement());
    } else if (ctx->iterationStatement()) {
        return visit(ctx->iterationStatement());
    } else if (ctx->jumpStatement()) {
        return visit(ctx->jumpStatement());
    }
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitDeclarator(CParser::DeclaratorContext *ctx) {
    // Simplified implementation
    return visit(ctx->directDeclarator());
}

antlrcpp::Any ASTBuilder::visitDirectDeclarator(CParser::DirectDeclaratorContext *ctx) {
    // Simplified implementation - just return identifier name
    if (ctx->Identifier()) {
        return std::any(ctx->Identifier()->getText());
    }
    return std::any(std::string(""));
}

antlrcpp::Any ASTBuilder::visitIterationStatement(CParser::IterationStatementContext *ctx) {
    // Stub implementation
    (void)ctx; // Suppress unused parameter warning
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitSelectionStatement(CParser::SelectionStatementContext *ctx) {
    // Stub implementation  
    (void)ctx; // Suppress unused parameter warning
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitParameterDeclaration(CParser::ParameterDeclarationContext *ctx) {
    // Stub implementation
    (void)ctx; // Suppress unused parameter warning
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitDeclarationSpecifiers(CParser::DeclarationSpecifiersContext *ctx) {
    // Stub implementation
    (void)ctx; // Suppress unused parameter warning
    return nullptr;
}

} // namespace parser
