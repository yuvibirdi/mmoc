#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

namespace preprocessor {

/**
 * Simple, in-process C preprocessor.
 * Supports:
 *  - #include "..." and <...> using provided include dirs and current dir
 *  - #define/#undef for object-like and simple function-like macros
 *  - #ifdef/#ifndef/#else/#elif/#endif with basic defined() expressions
 *  - Macro expansion on non-directive lines
 *
 * This is a pragmatic subset sufficient for our compiler tests; not a complete
 * C preprocessor. It intentionally ignores pragmas and many exotic features.
 */
class Preprocessor {
public:
    Preprocessor() = default;
    
    /**
     * Preprocess a source file.
     * @param inputFile Path to the source file
     * @param outputFile Path to write preprocessed output (optional)
     * @return Preprocessed source code as string
     */
    std::string preprocess(const std::string &inputFile, const std::string &outputFile = "");
    
    /**
     * Add an include directory to the search path.
     */
    void addIncludeDirectory(const std::string &dir);
    
    /**
     * Add a macro definition (e.g., "DEBUG=1").
     */
    void addMacroDefinition(const std::string &macro);
    
    /**
     * Set verbose output.
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
private:
    struct Macro {
        bool functionLike = false;
        std::vector<std::string> params; // for function-like
        std::string body;                // replacement list
    };

    std::vector<std::string> includeDirs_;
    std::vector<std::string> macroDefinitions_; // raw specs passed in via CLI/APIs
    std::unordered_map<std::string, Macro> macros_; // parsed and active
    bool verbose_ = false;

    /**
     * Core preprocessors
     */
    std::string preprocessFileInternal(const std::string &filePath);
    std::string preprocessStringInternal(const std::string &source, const std::string &currentFileDir);

    /** Resolve an include target to a file path (empty if not found). */
    std::string resolveInclude(const std::string &target, bool isSystem, const std::string &currentFileDir);

    /** Read a file into a string, throws on failure. */
    static std::string readFileToString(const std::string &path);

    /** Directive handling */
    bool handleDirective(const std::string &line, const std::string &currentFileDir, std::ostringstream &out, bool isActive);
    void handleDefine(const std::string &rest);
    void handleUndef(const std::string &rest);
    bool handleInclude(const std::string &rest, const std::string &currentFileDir, std::ostringstream &out, bool isActive);

    /** Conditional compilation state */
    struct IfFrame {
        bool parentActive;  // whether all outer conditions are active
        bool thisActive;    // whether current branch is active
        bool anyTrue;       // whether any branch has been taken in this group
    };
    std::vector<IfFrame> ifStack_;
    bool isCurrentlyActive() const;
    void pushIf(bool cond);
    void handleElse();
    void handleElif(const std::string &expr);
    void popIf();

    /** Expression evaluation for #if and #elif (minimal: defined(X), !, numbers) */
    bool evalExpr(const std::string &expr);

    /** Macros */
    void defineMacroFromSpec(const std::string &spec);
    static bool isIdentStart(char c);
    static bool isIdentChar(char c);

    /** Expand macros within a single logical line. */
    std::string expandMacros(const std::string &line);

    /** Utility */
    static std::string trim(const std::string &s);
    static void splitCommaArgs(const std::string &s, std::vector<std::string> &out);

    /** Log a message if verbose mode is enabled. */
    void log(const std::string &message);
};

} // namespace preprocessor
