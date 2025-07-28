#include "sema/SymbolTable.h"

namespace sema {

void SymbolTable::enterScope() {
    scopes_.emplace_back();
    currentScope_ = scopes_.size() - 1;
}

void SymbolTable::exitScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
        currentScope_ = scopes_.empty() ? 0 : scopes_.size() - 1;
    }
}

bool SymbolTable::addSymbol(const std::string &name, const std::string &type, bool isFunction) {
    if (scopes_.empty()) {
        enterScope(); // Create global scope if needed
    }
    
    auto &currentScopeMap = scopes_[currentScope_];
    
    if (currentScopeMap.find(name) != currentScopeMap.end()) {
        return false; // Symbol already exists in current scope
    }
    
    currentScopeMap[name] = std::make_unique<Symbol>(name, type, isFunction);
    return true;
}

Symbol* SymbolTable::lookupSymbol(const std::string &name) {
    // Search from current scope to global scope
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second.get();
        }
    }
    return nullptr;
}

bool SymbolTable::existsInCurrentScope(const std::string &name) const {
    if (scopes_.empty()) {
        return false;
    }
    
    const auto &currentScopeMap = scopes_[currentScope_];
    return currentScopeMap.find(name) != currentScopeMap.end();
}

} // namespace sema
