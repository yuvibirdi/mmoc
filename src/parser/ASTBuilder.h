#pragma once

#include "CBaseVisitor.h"
#include "ast/Node.h"
#include "ast/Expr.h"
#include "ast/Stmt.h"
#include <memory>
#include <string>

namespace parser {

/**
 * ANTLR visitor that builds AST nodes from the parse tree.
 */
class ASTBuilder : public CBaseVisitor {
public:
    ASTBuilder() = default;
    
    // Top-level
    antlrcpp::Any visitTranslationUnit(CParser::TranslationUnitContext *ctx) override;
    antlrcpp::Any visitExternalDeclaration(CParser::ExternalDeclarationContext *ctx) override;
    
    // Function definitions
    antlrcpp::Any visitFunctionDefinition(CParser::FunctionDefinitionContext *ctx) override;
    
    // Declarations
    antlrcpp::Any visitDeclaration(CParser::DeclarationContext *ctx) override;
    antlrcpp::Any visitDeclarationSpecifiers(CParser::DeclarationSpecifiersContext *ctx) override;
    antlrcpp::Any visitDeclarator(CParser::DeclaratorContext *ctx) override;
    antlrcpp::Any visitDirectDeclarator(CParser::DirectDeclaratorContext *ctx) override;
    antlrcpp::Any visitParameterDeclaration(CParser::ParameterDeclarationContext *ctx) override;
    
    // Statements
    antlrcpp::Any visitStatement(CParser::StatementContext *ctx) override;
    antlrcpp::Any visitCompoundStatement(CParser::CompoundStatementContext *ctx) override;
    antlrcpp::Any visitExpressionStatement(CParser::ExpressionStatementContext *ctx) override;
    antlrcpp::Any visitSelectionStatement(CParser::SelectionStatementContext *ctx) override;
    antlrcpp::Any visitIterationStatement(CParser::IterationStatementContext *ctx) override;
    antlrcpp::Any visitJumpStatement(CParser::JumpStatementContext *ctx) override;
    
    // Expressions
    antlrcpp::Any visitExpression(CParser::ExpressionContext *ctx) override;
    antlrcpp::Any visitAssignmentExpression(CParser::AssignmentExpressionContext *ctx) override;
    antlrcpp::Any visitConditionalExpression(CParser::ConditionalExpressionContext *ctx) override;
    antlrcpp::Any visitLogicalOrExpression(CParser::LogicalOrExpressionContext *ctx) override;
    antlrcpp::Any visitLogicalAndExpression(CParser::LogicalAndExpressionContext *ctx) override;
    antlrcpp::Any visitInclusiveOrExpression(CParser::InclusiveOrExpressionContext *ctx) override;
    antlrcpp::Any visitExclusiveOrExpression(CParser::ExclusiveOrExpressionContext *ctx) override;
    antlrcpp::Any visitAndExpression(CParser::AndExpressionContext *ctx) override;
    antlrcpp::Any visitEqualityExpression(CParser::EqualityExpressionContext *ctx) override;
    antlrcpp::Any visitRelationalExpression(CParser::RelationalExpressionContext *ctx) override;
    antlrcpp::Any visitShiftExpression(CParser::ShiftExpressionContext *ctx) override;
    antlrcpp::Any visitAdditiveExpression(CParser::AdditiveExpressionContext *ctx) override;
    antlrcpp::Any visitMultiplicativeExpression(CParser::MultiplicativeExpressionContext *ctx) override;
    antlrcpp::Any visitCastExpression(CParser::CastExpressionContext *ctx) override;
    antlrcpp::Any visitUnaryExpression(CParser::UnaryExpressionContext *ctx) override;
    antlrcpp::Any visitPostfixExpression(CParser::PostfixExpressionContext *ctx) override;
    antlrcpp::Any visitPrimaryExpression(CParser::PrimaryExpressionContext *ctx) override;
    
private:
    // Helper methods
    std::string extractTypeFromSpecifiers(CParser::DeclarationSpecifiersContext *ctx);
    std::string extractIdentifierName(CParser::DirectDeclaratorContext *ctx);
    std::vector<std::pair<std::string, std::string>> extractParameters(CParser::ParameterTypeListContext *ctx);
    
    // Extract nodes from ANTLR Any results
    std::unique_ptr<ast::Expr> extractExpr(const antlrcpp::Any &result);
    std::unique_ptr<ast::Stmt> extractStmt(const antlrcpp::Any &result);
    
    // Convert ANTLR tokens to binary operators
    ast::BinaryExpr::OpKind tokenToBinaryOp(antlr4::Token *token);
    ast::UnaryExpr::OpKind tokenToUnaryOp(antlr4::Token *token);
    
    // Parse numeric constants
    long long parseIntegerConstant(const std::string &text);
    double parseFloatingConstant(const std::string &text);
    char parseCharacterConstant(const std::string &text);
    std::string parseStringLiteral(const std::string &text);
};

} // namespace parser
