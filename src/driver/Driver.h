#pragma once

#include <string>
#include <memory>
#include <vector>

namespace ast {
    struct TranslationUnit;
}

namespace driver {

/**
 * Main compiler driver.
 */
class Driver {
public:
    Driver() = default;
    
    /**
     * Compile a source file.
     * @param inputFile Path to the source file
     * @param outputFile Path to the output file (default: a.out)
     * @return 0 on success, non-zero on error
     */
    int compile(const std::string &inputFile, const std::string &outputFile = "a.out");
    
    /**
     * Set verbose output.
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    /**
     * Set debug mode (emit LLVM IR instead of object code).
     */
    void setDebug(bool debug) { debug_ = debug; }
    
    /**
     * Set preprocessing-only mode (run preprocessor and exit).
     */
    void setPreprocessOnly(bool preprocessOnly) { preprocessOnly_ = preprocessOnly; }
    
    /**
     * Add an include directory to the preprocessor search path.
     */
    void addIncludeDirectory(const std::string &dir);
    
    /**
     * Add a macro definition to the preprocessor.
     */
    void addMacroDefinition(const std::string &macro);
    
private:
    bool verbose_ = false;
    bool debug_ = false;
    bool preprocessOnly_ = false;
    std::vector<std::string> includeDirs_;
    std::vector<std::string> macroDefinitions_;
    
    /**
     * Preprocess the input file.
     */
    std::string preprocessFile(const std::string &filename);
    
    /**
     * Parse source code from string and build AST.
     */
    std::unique_ptr<ast::TranslationUnit> parseString(const std::string &source);
    
    /**
     * Parse the input file and build AST.
     */
    std::unique_ptr<ast::TranslationUnit> parseFile(const std::string &filename);
    
    /**
     * Generate LLVM IR from AST.
     */
    bool generateIR(ast::TranslationUnit *ast, const std::string &outputFile);
    
    /**
     * Compile LLVM IR to object file.
     */
    bool compileToObject(const std::string &irFile, const std::string &objectFile);
    
    /**
     * Link object file to executable.
     */
    bool linkExecutable(const std::string &objectFile, const std::string &executableFile);
    
    void log(const std::string &message);
};

} // namespace driver
