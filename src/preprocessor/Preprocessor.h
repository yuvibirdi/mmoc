#pragma once

#include <string>
#include <vector>

namespace preprocessor {

/**
 * Simple preprocessor that handles basic C preprocessing.
 * Uses clang -E for macro expansion and include resolution.
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
    std::vector<std::string> includeDirs_;
    std::vector<std::string> macroDefinitions_;
    bool verbose_ = false;
    
    /**
     * Build clang -E command line.
     */
    std::string buildClangCommand(const std::string &inputFile, const std::string &outputFile);
    
    /**
     * Execute command and return output.
     */
    std::string executeCommand(const std::string &command);
    
    /**
     * Log a message if verbose mode is enabled.
     */
    void log(const std::string &message);
};

} // namespace preprocessor
