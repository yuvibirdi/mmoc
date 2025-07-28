#include "ast/Stmt.h"
#include <sstream>

namespace ast {

std::string IfStmt::toString() const {
    std::string result = "if (" + condition->toString() + ") " + thenStmt->toString();
    if (elseStmt) {
        result += " else " + elseStmt->toString();
    }
    return result;
}

std::string ForStmt::toString() const {
    std::ostringstream oss;
    oss << "for (";
    if (init) oss << init->toString();
    oss << " ";
    if (condition) oss << condition->toString();
    oss << "; ";
    if (increment) oss << increment->toString();
    oss << ") " << body->toString();
    return oss.str();
}

std::string CompoundStmt::toString() const {
    std::ostringstream oss;
    oss << "{\n";
    for (const auto& stmt : statements) {
        oss << "  " << stmt->toString() << "\n";
    }
    oss << "}";
    return oss.str();
}

std::string VarDecl::toString() const {
    std::string result = type + " " + name;
    if (initializer) {
        result += " = " + initializer->toString();
    }
    result += ";";
    return result;
}

std::string FunctionDecl::toString() const {
    std::ostringstream oss;
    oss << returnType << " " << name << "(";
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i].first << " " << parameters[i].second;
    }
    oss << ")";
    
    if (body) {
        oss << " " << body->toString();
    } else {
        oss << ";";
    }
    
    return oss.str();
}

std::string TranslationUnit::toString() const {
    std::ostringstream oss;
    for (const auto& decl : declarations) {
        oss << decl->toString() << "\n";
    }
    return oss.str();
}

} // namespace ast
