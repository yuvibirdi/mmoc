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
    std::string baseType = extractTypeFromSpecifiers(ctx->declarationSpecifiers());
    
    if (ctx->initDeclaratorList()) {
        // For now, handle single variable declarations
        auto *initDecl = ctx->initDeclaratorList()->initDeclarator(0);
        auto *declarator = initDecl->declarator();
        
        // Build full type including pointers
        std::string fullType = baseType;
        if (declarator->pointer()) {
            // Count the number of '*' characters for pointer depth
            std::string pointerText = declarator->pointer()->getText();
            for (char c : pointerText) {
                if (c == '*') {
                    fullType += "*";
                }
            }
        }
        
        std::string name = extractIdentifierName(declarator->directDeclarator());
        
        std::unique_ptr<ast::Expr> initializer = nullptr;
        if (initDecl->initializer()) {
            auto result = visit(initDecl->initializer()->assignmentExpression());
            auto *expr_ptr = std::any_cast<ast::Expr*>(result);
            initializer = std::unique_ptr<ast::Expr>(expr_ptr);
        }
        
        return static_cast<ast::Node*>(new ast::VarDecl(name, fullType, std::move(initializer)));
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
        
        // Parse the assignment operator type
        std::string opText = ctx->assignmentOperator()->getText();
        ast::BinaryExpr::OpKind op = ast::BinaryExpr::OpKind::Assign; // Default
        
        if (opText == "=") op = ast::BinaryExpr::OpKind::Assign;
        else if (opText == "+=") op = ast::BinaryExpr::OpKind::AddAssign;
        else if (opText == "-=") op = ast::BinaryExpr::OpKind::SubAssign;
        else if (opText == "*=") op = ast::BinaryExpr::OpKind::MulAssign;
        else if (opText == "/=") op = ast::BinaryExpr::OpKind::DivAssign;
        else if (opText == "%=") op = ast::BinaryExpr::OpKind::ModAssign;
        
        return std::any(static_cast<ast::Expr*>(new ast::BinaryExpr(
            std::move(left), std::move(right), op
        )));
    }
    return visit(ctx->conditionalExpression());
}

antlrcpp::Any ASTBuilder::visitConditionalExpression(CParser::ConditionalExpressionContext *ctx) {
    // Handle ternary operator: condition ? true_expr : false_expr
    auto conditionResult = visit(ctx->logicalOrExpression());
    auto condition = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(conditionResult));
    
    // Check if this is actually a ternary expression
    if (ctx->expression() && ctx->conditionalExpression()) {
        // This is a ternary expression
        auto trueResult = visit(ctx->expression());
        auto trueExpr = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(trueResult));
        
        auto falseResult = visit(ctx->conditionalExpression());
        auto falseExpr = std::unique_ptr<ast::Expr>(std::any_cast<ast::Expr*>(falseResult));
        
        return std::any(static_cast<ast::Expr*>(new ast::ConditionalExpr(
            std::move(condition), std::move(trueExpr), std::move(falseExpr))));
    } else {
        // Just a regular logical OR expression
        return std::any(condition.release());
    }
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
        
        ast::BinaryExpr::OpKind op = ast::BinaryExpr::OpKind::LT; // Default
        
        // Find the operator between expressions
        if (2*i - 1 < ctx->children.size()) {
            auto *token = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[2*i - 1]);
            if (token) {
                std::string opText = token->getText();
                if (opText == "<") op = ast::BinaryExpr::OpKind::LT;
                else if (opText == ">") op = ast::BinaryExpr::OpKind::GT;
                else if (opText == "<=") op = ast::BinaryExpr::OpKind::LE;
                else if (opText == ">=") op = ast::BinaryExpr::OpKind::GE;
            }
        }
        
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
    size_t idx = 0; int prefixInc=0, prefixDec=0; bool sawSizeof=false; std::string sizeofType;
    while (idx < ctx->children.size()) {
        if (auto *term = dynamic_cast<antlr4::tree::TerminalNode*>(ctx->children[idx])) {
            int t = term->getSymbol()->getType();
            if (t == CParser::PlusPlus) { ++prefixInc; ++idx; continue; }
            if (t == CParser::MinusMinus) { ++prefixDec; ++idx; continue; }
            if (t == CParser::Sizeof) { sawSizeof=true; ++idx; continue; }
        }
        break;
    }
    if (sawSizeof) {
        // Very basic: sizeof <postfixExpression or unaryOperator castExpression>
        // If pattern is: sizeof ( type ) we cannot easily distinguish; fallback constant 4
        return std::any(static_cast<ast::Expr*>(new ast::IntegerLiteral(4)));
    }
    std::unique_ptr<ast::Expr> base;
    if (auto *post = ctx->postfixExpression()) {
        base = extractExpr(visit(post));
    } else if (ctx->unaryOperator()) {
        auto operand = extractExpr(visit(ctx->castExpression()));
        std::string opText = ctx->unaryOperator()->getText();
        ast::UnaryExpr::OpKind op;
        if (opText=="&") op=ast::UnaryExpr::OpKind::AddressOf; else if (opText=="*") op=ast::UnaryExpr::OpKind::Dereference; else if (opText=="+") op=ast::UnaryExpr::OpKind::Plus; else if (opText=="-") op=ast::UnaryExpr::OpKind::Minus; else if (opText=="~") op=ast::UnaryExpr::OpKind::BitwiseNot; else if (opText=="!") op=ast::UnaryExpr::OpKind::Not; else op=ast::UnaryExpr::OpKind::Plus;
        base = std::make_unique<ast::UnaryExpr>(std::move(operand), op, true);
    } else {
        return visitChildren(ctx);
    }
    int net = prefixInc - prefixDec;
    while (net != 0) {
        ast::UnaryExpr::OpKind op = net>0 ? ast::UnaryExpr::OpKind::PreIncrement : ast::UnaryExpr::OpKind::PreDecrement;
        base = std::make_unique<ast::UnaryExpr>(std::move(base), op, true);
        net += (net>0 ? -1 : 1);
    }
    return std::any(base.release());
}

antlrcpp::Any ASTBuilder::visitPostfixExpression(CParser::PostfixExpressionContext *ctx) {
    std::unique_ptr<ast::Expr> expr;
    if (ctx->primaryExpression()) {
        expr = extractExpr(visit(ctx->primaryExpression()));
    } else {
        return nullptr;
    }
    size_t i = 1; // child index after primaryExpression
    while (i < ctx->children.size()) {
        auto *child = ctx->children[i];
        if (auto *term = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            int tt = term->getSymbol()->getType();
            if (tt == CParser::PlusPlus) {
                expr = std::make_unique<ast::UnaryExpr>(std::move(expr), ast::UnaryExpr::OpKind::PostIncrement, false);
                ++i; continue;
            } else if (tt == CParser::MinusMinus) {
                expr = std::make_unique<ast::UnaryExpr>(std::move(expr), ast::UnaryExpr::OpKind::PostDecrement, false);
                ++i; continue;
            } else if (tt == CParser::LeftParen) {
                // Function call: '(' argumentExpressionList? ')'
                std::vector<std::unique_ptr<ast::Expr>> args;
                // Look ahead to see if we have an argumentExpressionList
                if (i + 1 < ctx->children.size()) {
                    if (auto *argList = dynamic_cast<CParser::ArgumentExpressionListContext*>(ctx->children[i+1])) {
                        // Collect each assignmentExpression
                        for (auto *ae : argList->assignmentExpression()) {
                            auto anyArg = visit(ae);
                            auto *raw = std::any_cast<ast::Expr*>(anyArg);
                            args.push_back(std::unique_ptr<ast::Expr>(raw));
                        }
                        // Expect following ')'
                        i += 3; // '(', argList, ')'
                    } else {
                        // No arguments -> expect immediate ')'
                        i += 2; // '(', ')'
                    }
                } else {
                    // Malformed, break
                    ++i;
                }
                expr = std::make_unique<ast::CallExpr>(std::move(expr), std::move(args));
                continue;
            } else if (tt == CParser::LeftBracket) {
                // Array subscript: expr '[' expression ']'
                if (i + 2 < ctx->children.size()) {
                    auto *indexChild = ctx->children[i+1];
                    auto indexAny = visit(indexChild);
                    auto indexExpr = extractExpr(indexAny);
                    expr = std::make_unique<ast::ArraySubscriptExpr>(std::move(expr), std::move(indexExpr));
                    i += 3; continue; // '[', expr, ']'
                }
            }
        }
        ++i; // Fallback advance
    }
    return std::any(expr.release());
}

antlrcpp::Any ASTBuilder::visitPrimaryExpression(CParser::PrimaryExpressionContext *ctx) {
    if (ctx->Identifier()) {
        return std::any(static_cast<ast::Expr*>(new ast::Identifier(ctx->Identifier()->getText())));
    } else if (ctx->Constant()) {
        std::string text = ctx->Constant()->getText();
        if (text.find('.') != std::string::npos || text.find('e') != std::string::npos || text.find('E') != std::string::npos) {
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
        if (ptr) return std::unique_ptr<ast::Stmt>(ptr);
    } catch (const std::bad_any_cast&) {
        // fallthrough to try derived types
    }
    try {
        auto *ptr = std::any_cast<ast::CompoundStmt*>(result);
        if (ptr) return std::unique_ptr<ast::Stmt>(ptr);
    } catch (const std::bad_any_cast&) {
        // ignore
    }
    try {
        auto *ptr = std::any_cast<ast::VarDecl*>(result);
        if (ptr) return std::unique_ptr<ast::Stmt>(ptr);
    } catch (const std::bad_any_cast&) {
        // ignore
    }
    return nullptr;
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
    // Check if this is a while statement by looking for 'while' token
    bool isWhileStatement = false;
    bool isForStatement = false;
    
    for (auto child : ctx->children) {
        if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            if (terminal->getText() == "while") {
                isWhileStatement = true;
                break;
            } else if (terminal->getText() == "for") {
                isForStatement = true;
                break;
            }
        }
    }
    
    if (isWhileStatement) {
        // Parse while statement: while '(' expression ')' statement
        if (ctx->children.size() >= 5) {
            // Get condition expression (index 2, between parentheses)
            auto conditionCtx = ctx->children[2];
            auto conditionResult = visit(conditionCtx);
            auto condition = std::any_cast<ast::Expr*>(conditionResult);
            
            // Get body statement (index 4)
            auto bodyCtx = ctx->children[4];
            auto bodyResult = visit(bodyCtx);
            
            // Handle both CompoundStmt and regular Stmt
            ast::Stmt* body = nullptr;
            try {
                body = std::any_cast<ast::Stmt*>(bodyResult);
            } catch (const std::bad_any_cast&) {
                // Try CompoundStmt
                auto compoundBody = std::any_cast<ast::CompoundStmt*>(bodyResult);
                body = static_cast<ast::Stmt*>(compoundBody);
            }
            
            return std::any(static_cast<ast::Stmt*>(new ast::WhileStmt(
                std::unique_ptr<ast::Expr>(condition),
                std::unique_ptr<ast::Stmt>(body)
            )));
        }
    } else if (isForStatement) {
        // Parse for statement: for '(' forCondition ')' statement
        if (ctx->children.size() >= 5) {
            // Get the forCondition context (index 2)
            auto forConditionCtx = dynamic_cast<CParser::ForConditionContext*>(ctx->children[2]);
            if (forConditionCtx) {
                // Parse the three parts of the for condition
                ast::Stmt* init = nullptr;
                ast::Expr* condition = nullptr;
                ast::Expr* increment = nullptr;
                
                // Get init (forDeclaration or expression)
                if (forConditionCtx->forDeclaration()) {
                    auto initResult = visit(forConditionCtx->forDeclaration());
                    init = std::any_cast<ast::Stmt*>(initResult);
                } else if (forConditionCtx->expression()) {
                    auto initResult = visit(forConditionCtx->expression());
                    auto initExpr = std::any_cast<ast::Expr*>(initResult);
                    // Wrap expression in an expression statement
                    init = new ast::ExprStmt(std::unique_ptr<ast::Expr>(initExpr));
                }
                
                // Get condition (first forExpression)
                if (forConditionCtx->forExpression().size() > 0) {
                    auto condResult = visit(forConditionCtx->forExpression(0));
                    condition = std::any_cast<ast::Expr*>(condResult);
                }
                
                // Get increment (second forExpression)
                if (forConditionCtx->forExpression().size() > 1) {
                    auto incResult = visit(forConditionCtx->forExpression(1));
                    increment = std::any_cast<ast::Expr*>(incResult);
                }
                
                // Get body statement (index 4)
                auto bodyCtx = ctx->children[4];
                auto bodyResult = visit(bodyCtx);
                
                // Handle both CompoundStmt and regular Stmt
                ast::Stmt* body = nullptr;
                try {
                    body = std::any_cast<ast::Stmt*>(bodyResult);
                } catch (const std::bad_any_cast&) {
                    // Try CompoundStmt
                    auto compoundBody = std::any_cast<ast::CompoundStmt*>(bodyResult);
                    body = static_cast<ast::Stmt*>(compoundBody);
                }
                
                return std::any(static_cast<ast::Stmt*>(new ast::ForStmt(
                    std::unique_ptr<ast::Stmt>(init),
                    std::unique_ptr<ast::Expr>(condition),
                    std::unique_ptr<ast::Expr>(increment),
                    std::unique_ptr<ast::Stmt>(body)
                )));
            }
        }
    }
    
    // For other iteration statements (do-while), return stub for now
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitSelectionStatement(CParser::SelectionStatementContext *ctx) {
    // Check if this is an if statement by looking for 'if' token
    bool isIfStatement = false;
    for (auto child : ctx->children) {
        if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            if (terminal->getText() == "if") {
                isIfStatement = true;
                break;
            }
        }
    }
    
    if (isIfStatement) {
        // Get the condition expression
        auto conditionResult = visit(ctx->expression());
        auto condition = extractExpr(conditionResult);
        
        // Get the then statement  
        auto thenResult = visit(ctx->statement(0));
        auto thenStmt = extractStmt(thenResult);
        
        // Get the else statement if present
        std::unique_ptr<ast::Stmt> elseStmt = nullptr;
        if (ctx->statement().size() > 1) {
            auto elseResult = visit(ctx->statement(1));
            elseStmt = extractStmt(elseResult);
        }
        
        return std::any(static_cast<ast::Stmt*>(new ast::IfStmt(
            std::move(condition), std::move(thenStmt), std::move(elseStmt)
        )));
    }
    
    // TODO: Handle switch statement
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

antlrcpp::Any ASTBuilder::visitForDeclaration(CParser::ForDeclarationContext *ctx) {
    // For declarations are typically simple variable declarations like "int i = 0"
    // We'll reuse the declaration visitor logic
    if (ctx->declarationSpecifiers() && ctx->initDeclaratorList()) {
        // Get type
        std::string type = extractTypeFromSpecifiers(ctx->declarationSpecifiers());
        
        // Get the first initDeclarator from the list (for now, only handle single declarations)
        auto *initDeclList = ctx->initDeclaratorList();
        if (initDeclList && !initDeclList->initDeclarator().empty()) {
            auto *initDecl = initDeclList->initDeclarator(0);
            if (initDecl && initDecl->declarator() && initDecl->declarator()->directDeclarator()) {
                std::string name = extractIdentifierName(initDecl->declarator()->directDeclarator());
                
                // Handle initializer if present
                ast::Expr* init = nullptr;
                if (initDecl->initializer() && initDecl->initializer()->assignmentExpression()) {
                    auto initResult = visit(initDecl->initializer()->assignmentExpression());
                    init = std::any_cast<ast::Expr*>(initResult);
                }
                
                // Create a variable declaration statement
                auto varDecl = new ast::VarDecl(name, type, std::unique_ptr<ast::Expr>(init));
                return std::any(static_cast<ast::Stmt*>(varDecl));
            }
        }
    }
    
    return nullptr;
}

antlrcpp::Any ASTBuilder::visitForExpression(CParser::ForExpressionContext *ctx) {
    // ForExpression can have multiple assignmentExpressions separated by commas
    // For now, we'll just return the first one (or last one for comma operator semantics)
    if (!ctx->assignmentExpression().empty()) {
        // Return the last expression (comma operator semantics)
        auto lastExpr = ctx->assignmentExpression().back();
        return visit(lastExpr);
    }
    
    return nullptr;
}

} // namespace parser
