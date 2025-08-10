#pragma once

#include "ast/Node.h"
#include <memory>

namespace ast {

/**
 * Base class for all expressions.
 */
struct Expr : public Node {
    virtual ~Expr() = default;
};

/**
 * Integer literal expression.
 */
struct IntegerLiteral : public Expr {
    long long value;
    
    explicit IntegerLiteral(long long val) : value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

/**
 * Floating point literal expression.
 */
struct FloatingLiteral : public Expr {
    double value;
    
    explicit FloatingLiteral(double val) : value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

/**
 * Character literal expression.
 */
struct CharacterLiteral : public Expr {
    char value;
    
    explicit CharacterLiteral(char val) : value(val) {}
    
    std::string toString() const override {
        return "'" + std::string(1, value) + "'";
    }
};

/**
 * String literal expression.
 */
struct StringLiteral : public Expr {
    std::string value;
    
    explicit StringLiteral(std::string val) : value(std::move(val)) {}
    
    std::string toString() const override {
        return "\"" + value + "\"";
    }
};

/**
 * Identifier expression.
 */
struct Identifier : public Expr {
    std::string name;
    
    explicit Identifier(std::string n) : name(std::move(n)) {}
    
    std::string toString() const override {
        return name;
    }
};

/**
 * Binary expression (e.g., a + b, a * b).
 */
struct BinaryExpr : public Expr {
    enum class OpKind {
        Add, Sub, Mul, Div, Mod,
        LT, GT, LE, GE, EQ, NE,
        LogicalAnd, LogicalOr,
        BitwiseAnd, BitwiseOr, BitwiseXor,
        LeftShift, RightShift,
        Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign
    };
    
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    OpKind op;
    
    BinaryExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r, OpKind operation)
        : left(std::move(l)), right(std::move(r)), op(operation) {}
    
    std::string toString() const override;
    
private:
    std::string opToString() const;
};

/**
 * Unary expression (e.g., -a, !a, ++a).
 */
struct UnaryExpr : public Expr {
    enum class OpKind {
        Plus, Minus, Not, BitwiseNot,
        PreIncrement, PostIncrement,
        PreDecrement, PostDecrement,
        AddressOf, Dereference
    };
    
    std::unique_ptr<Expr> operand;
    OpKind op;
    bool isPrefix;
    
    UnaryExpr(std::unique_ptr<Expr> operand, OpKind operation, bool prefix = true)
        : operand(std::move(operand)), op(operation), isPrefix(prefix) {}
    
    std::string toString() const override;
    
private:
    std::string opToString() const;
};

/**
 * Function call expression.
 */
struct CallExpr : public Expr {
    std::unique_ptr<Expr> function;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(std::unique_ptr<Expr> func, std::vector<std::unique_ptr<Expr>> args)
        : function(std::move(func)), arguments(std::move(args)) {}
    
    std::string toString() const override;
};

/**
 * Array subscript expression (e.g., arr[index]).
 */
struct ArraySubscriptExpr : public Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    
    ArraySubscriptExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx)
        : array(std::move(arr)), index(std::move(idx)) {}
    
    std::string toString() const override {
        return array->toString() + "[" + index->toString() + "]";
    }
};

/**
 * Member access expression (e.g., obj.member).
 */
struct MemberExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::string member;
    bool isArrow; // true for ->, false for .
    
    MemberExpr(std::unique_ptr<Expr> obj, std::string mem, bool arrow = false)
        : object(std::move(obj)), member(std::move(mem)), isArrow(arrow) {}
    
    std::string toString() const override {
        return object->toString() + (isArrow ? "->" : ".") + member;
    }
};

/**
 * Ternary conditional expression (e.g., condition ? true_expr : false_expr).
 */
struct ConditionalExpr : public Expr {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueExpr;
    std::unique_ptr<Expr> falseExpr;
    
    ConditionalExpr(std::unique_ptr<Expr> cond, std::unique_ptr<Expr> trueE, std::unique_ptr<Expr> falseE)
        : condition(std::move(cond)), trueExpr(std::move(trueE)), falseExpr(std::move(falseE)) {}
    
    std::string toString() const override {
        return condition->toString() + " ? " + trueExpr->toString() + " : " + falseExpr->toString();
    }
};

} // namespace ast
