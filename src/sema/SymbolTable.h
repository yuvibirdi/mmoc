#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace sema {

/**
 * Symbol information.
 */
struct Symbol {
    std::string name;
    std::string type;
    bool isFunction;
    
    Symbol(const std::string &n, const std::string &t, bool func = false)
        : name(n), type(t), isFunction(func) {}
};

/**
 * Symbol table with scope management.
 */
class SymbolTable {
public:
    SymbolTable() = default;
    
    /**
     * Enter a new scope.
     */
    void enterScope();
    
    /**
     * Exit current scope.
     */
    void exitScope();
    
    /**
     * Add a symbol to the current scope.
     */
    bool addSymbol(const std::string &name, const std::string &type, bool isFunction = false);
    
    /**
     * Look up a symbol in all scopes.
     */
    Symbol* lookupSymbol(const std::string &name);
    
    /**
     * Check if symbol exists in current scope only.
     */
    bool existsInCurrentScope(const std::string &name) const;
    
private:
    std::vector<std::unordered_map<std::string, std::unique_ptr<Symbol>>> scopes_;
    size_t currentScope_ = 0;
};

} // namespace sema
