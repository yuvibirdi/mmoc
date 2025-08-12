#include "driver/Driver.h"
#include "parser/ASTBuilder.h"
#include "codegen/IRGenerator.h"
#include "preprocessor/Preprocessor.h"
#include "utils/Error.h"
#include "ast/Stmt.h"

#include "antlr4-runtime.h"
#include "CLexer.h"
#include "CParser.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace driver {

int Driver::compile(const std::string &inputFile, const std::string &outputFile) {
    try {
        log("Compiling " + inputFile + " to " + outputFile);
        
        // Preprocess the input file
        std::string preprocessedSource = preprocessFile(inputFile);
        
        if (preprocessOnly_) {
            // Output preprocessed source and exit
            if (outputFile != "a.out") {
                std::ofstream file(outputFile);
                if (!file) {
                    std::cerr << "Error: Cannot write to output file: " << outputFile << std::endl;
                    return 1;
                }
                file << preprocessedSource;
            } else {
                std::cout << preprocessedSource;
            }
            return 0;
        }
        
        // Parse the preprocessed source
        auto ast = parseString(preprocessedSource);
        if (!ast) {
            std::cerr << "Error: Failed to parse " << inputFile << std::endl;
            return 1;
        }
        
        // Generate LLVM IR
        std::string irFile = outputFile + ".ll";
        if (!generateIR(ast.get(), irFile)) {
            std::cerr << "Error: Failed to generate LLVM IR" << std::endl;
            return 1;
        }
        
        if (debug_) {
            log("Generated LLVM IR: " + irFile);
            return 0;
        }
        
        // Compile to object file
        std::string objectFile = outputFile + ".o";
        if (!compileToObject(irFile, objectFile)) {
            std::cerr << "Error: Failed to compile to object file" << std::endl;
            return 1;
        }
        
        // Link to executable
        if (!linkExecutable(objectFile, outputFile)) {
            std::cerr << "Error: Failed to link executable" << std::endl;
            return 1;
        }
        
        // Clean up intermediate files
        if (!debug_) {
            std::remove(irFile.c_str());
            std::remove(objectFile.c_str());
        }
        
        log("Successfully compiled " + inputFile + " to " + outputFile);
        return 0;
        
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

void Driver::addIncludeDirectory(const std::string &dir) {
    includeDirs_.push_back(dir);
}

void Driver::addMacroDefinition(const std::string &macro) {
    macroDefinitions_.push_back(macro);
}

std::string Driver::preprocessFile(const std::string &filename) {
    log("Preprocessing " + filename);
    
    preprocessor::Preprocessor preprocessor;
    preprocessor.setVerbose(verbose_);
    
    // Add include directories
    for (const auto &dir : includeDirs_) {
        preprocessor.addIncludeDirectory(dir);
    }
    
    // Add macro definitions
    for (const auto &macro : macroDefinitions_) {
        preprocessor.addMacroDefinition(macro);
    }
    
    return preprocessor.preprocess(filename);
}

std::unique_ptr<ast::TranslationUnit> Driver::parseString(const std::string &source) {
    // Create ANTLR input stream
    antlr4::ANTLRInputStream input(source);
    
    // Create lexer
    CLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    
    // Create parser
    CParser parser(&tokens);
    
    // Parse the translation unit
    auto *tree = parser.translationUnit();
    
    // Build AST
    parser::ASTBuilder builder;
    auto result = builder.visit(tree);
    
    // Convert raw pointer back to unique_ptr
    auto *translation_unit_ptr = std::any_cast<ast::TranslationUnit*>(result);
    return std::unique_ptr<ast::TranslationUnit>(translation_unit_ptr);
}

std::unique_ptr<ast::TranslationUnit> Driver::parseFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Create ANTLR input stream
    antlr4::ANTLRInputStream input(content);
    
    // Create lexer
    CLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    
    // Create parser
    CParser parser(&tokens);
    
    // Parse the translation unit
    auto *tree = parser.translationUnit();
    
    // Build AST
    parser::ASTBuilder builder;
    auto result = builder.visit(tree);
    
    // Convert raw pointer back to unique_ptr
    auto *translation_unit_ptr = std::any_cast<ast::TranslationUnit*>(result);
    return std::unique_ptr<ast::TranslationUnit>(translation_unit_ptr);
}

bool Driver::generateIR(ast::TranslationUnit *ast, const std::string &outputFile) {
    try {
        codegen::IRGenerator generator;
        std::string ir = generator.generateIR(ast);
        
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            return false;
        }
        
        file << ir;
        return true;
        
    } catch (const std::exception &e) {
        std::cerr << "IR Generation error: " << e.what() << std::endl;
        return false;
    }
}

bool Driver::compileToObject(const std::string &irFile, const std::string &objectFile) {
    std::string command = "clang -c -Wno-override-module " + irFile + " -o " + objectFile;
    log("Executing: " + command);
    
    int result = std::system(command.c_str());
    return result == 0;
}

bool Driver::linkExecutable(const std::string &objectFile, const std::string &executableFile) {
    std::string command = "clang " + objectFile + " -o " + executableFile;
    log("Executing: " + command);
    
    int result = std::system(command.c_str());
    return result == 0;
}

void Driver::log(const std::string &message) {
    if (verbose_) {
        std::cout << "[mmoc] " << message << std::endl;
    }
}

} // namespace driver
