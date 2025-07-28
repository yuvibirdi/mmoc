#include <gtest/gtest.h>
#include "ast/Expr.h"
#include "ast/Stmt.h"

// Test AST node creation and basic functionality
class ASTTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ASTTest, IntegerLiteralCreation) {
    auto lit = std::make_unique<ast::IntegerLiteral>(42);
    EXPECT_EQ(lit->value, 42);
    EXPECT_EQ(lit->toString(), "42");
}

TEST_F(ASTTest, BinaryExpressionCreation) {
    auto left = std::make_unique<ast::IntegerLiteral>(10);
    auto right = std::make_unique<ast::IntegerLiteral>(20);
    auto expr = std::make_unique<ast::BinaryExpr>(
        std::move(left), 
        std::move(right), 
        ast::BinaryExpr::OpKind::Add
    );
    
    EXPECT_EQ(expr->toString(), "(10 + 20)");
}

TEST_F(ASTTest, FunctionDeclCreation) {
    std::vector<std::pair<std::string, std::string>> params = {{"int", "x"}, {"int", "y"}};
    auto body = std::make_unique<ast::CompoundStmt>(std::vector<std::unique_ptr<ast::Stmt>>());
    
    auto func = std::make_unique<ast::FunctionDecl>("add", "int", params, std::move(body));
    
    EXPECT_EQ(func->name, "add");
    EXPECT_EQ(func->returnType, "int");
    EXPECT_EQ(func->parameters.size(), 2);
    EXPECT_TRUE(func->isDefinition());
}

TEST_F(ASTTest, ReturnStmtCreation) {
    auto expr = std::make_unique<ast::IntegerLiteral>(42);
    auto ret = std::make_unique<ast::ReturnStmt>(std::move(expr));
    
    EXPECT_EQ(ret->toString(), "return 42;");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
