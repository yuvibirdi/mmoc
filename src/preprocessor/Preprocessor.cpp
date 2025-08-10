#include "preprocessor/Preprocessor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

namespace preprocessor {

std::string Preprocessor::preprocess(const std::string &inputFile, const std::string &outputFile) {
    log("Preprocessing " + inputFile);
    
    // Build clang -E command
    std::string command = buildClangCommand(inputFile, outputFile);
    
    // Execute command
    std::string result = executeCommand(command);
    
    // If output file specified, write to file
    if (!outputFile.empty()) {
        std::ofstream file(outputFile);
        if (!file) {
            throw std::runtime_error("Cannot write to output file: " + outputFile);
        }
        file << result;
        file.close();
        log("Preprocessed output written to " + outputFile);
    }
    
    return result;
}

void Preprocessor::addIncludeDirectory(const std::string &dir) {
    includeDirs_.push_back(dir);
    log("Added include directory: " + dir);
}

void Preprocessor::addMacroDefinition(const std::string &macro) {
    macroDefinitions_.push_back(macro);
    log("Added macro definition: " + macro);
}

std::string Preprocessor::buildClangCommand(const std::string &inputFile, const std::string &outputFile) {
    std::ostringstream oss;
    
    // Use clang -E for preprocessing
    oss << "clang -E";
    
    // Add include directories
    for (const auto &dir : includeDirs_) {
        oss << " -I" << dir;
    }
    
    // Add macro definitions
    for (const auto &macro : macroDefinitions_) {
        oss << " -D" << macro;
    }
    
    // Add input file
    oss << " " << inputFile;
    
    // Add output file if specified
    if (!outputFile.empty()) {
        oss << " -o " << outputFile;
    }
    
    // Suppress some clang warnings for cleaner output
    oss << " -Wno-everything";
    
    return oss.str();
}

std::string Preprocessor::executeCommand(const std::string &command) {
    log("Executing: " + command);
    
    // Open pipe to command
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }
    
    // Read output
    std::ostringstream result;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    
    // Close pipe and check return code
    int exitCode = pclose(pipe);
    if (exitCode != 0) {
        throw std::runtime_error("Command failed with exit code " + std::to_string(exitCode) + ": " + command);
    }
    
    return result.str();
}

void Preprocessor::log(const std::string &message) {
    if (verbose_) {
        std::cerr << "[Preprocessor] " << message << std::endl;
    }
}

} // namespace preprocessor
