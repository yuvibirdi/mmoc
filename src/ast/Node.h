#pragma once

#include <memory>
#include <vector>
#include <string>

namespace ast {

/**
 * Base class for all AST nodes.
 * Uses virtual destructor to enable RTTI via dynamic_cast.
 */
struct Node {
    virtual ~Node() = default;
    
    // Source location information
    int line = 0;
    int column = 0;
    
    virtual std::string toString() const = 0;
};

} // namespace ast
