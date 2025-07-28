#pragma once

#include <string>
#include <stdexcept>

namespace utils {

/**
 * Base class for compiler errors.
 */
class CompilerError : public std::runtime_error {
public:
    explicit CompilerError(const std::string &message) : std::runtime_error(message) {}
};

/**
 * Parse error.
 */
class ParseError : public CompilerError {
public:
    ParseError(const std::string &message, int line = 0, int column = 0)
        : CompilerError(formatMessage(message, line, column)), line_(line), column_(column) {}
    
    int getLine() const { return line_; }
    int getColumn() const { return column_; }
    
private:
    int line_;
    int column_;
    
    static std::string formatMessage(const std::string &message, int line, int column) {
        if (line > 0) {
            return "Parse error at line " + std::to_string(line) + 
                   (column > 0 ? ", column " + std::to_string(column) : "") + 
                   ": " + message;
        }
        return "Parse error: " + message;
    }
};

/**
 * Semantic error.
 */
class SemanticError : public CompilerError {
public:
    explicit SemanticError(const std::string &message) : CompilerError("Semantic error: " + message) {}
};

/**
 * Code generation error.
 */
class CodeGenError : public CompilerError {
public:
    explicit CodeGenError(const std::string &message) : CompilerError("Code generation error: " + message) {}
};

} // namespace utils
