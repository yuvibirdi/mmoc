#include "ast/Expr.h"
#include <sstream>

namespace ast {

std::string BinaryExpr::toString() const {
    return "(" + left->toString() + " " + opToString() + " " + right->toString() + ")";
}

std::string BinaryExpr::opToString() const {
    switch (op) {
        case OpKind::Add: return "+";
        case OpKind::Sub: return "-";
        case OpKind::Mul: return "*";
        case OpKind::Div: return "/";
        case OpKind::Mod: return "%";
        case OpKind::LT: return "<";
        case OpKind::GT: return ">";
        case OpKind::LE: return "<=";
        case OpKind::GE: return ">=";
        case OpKind::EQ: return "==";
        case OpKind::NE: return "!=";
        case OpKind::LogicalAnd: return "&&";
        case OpKind::LogicalOr: return "||";
        case OpKind::BitwiseAnd: return "&";
        case OpKind::BitwiseOr: return "|";
        case OpKind::BitwiseXor: return "^";
        case OpKind::LeftShift: return "<<";
        case OpKind::RightShift: return ">>";
        case OpKind::Assign: return "=";
        default: return "?";
    }
}

std::string UnaryExpr::toString() const {
    if (isPrefix) {
        return opToString() + operand->toString();
    } else {
        return operand->toString() + opToString();
    }
}

std::string UnaryExpr::opToString() const {
    switch (op) {
        case OpKind::Plus: return "+";
        case OpKind::Minus: return "-";
        case OpKind::Not: return "!";
        case OpKind::BitwiseNot: return "~";
        case OpKind::PreIncrement:
        case OpKind::PostIncrement: return "++";
        case OpKind::PreDecrement:
        case OpKind::PostDecrement: return "--";
        case OpKind::AddressOf: return "&";
        case OpKind::Dereference: return "*";
        default: return "?";
    }
}

std::string CallExpr::toString() const {
    std::ostringstream oss;
    oss << function->toString() << "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << arguments[i]->toString();
    }
    oss << ")";
    return oss.str();
}

} // namespace ast
