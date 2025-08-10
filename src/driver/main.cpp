#include "driver/Driver.h"
#include <iostream>
#include <string>
#include <vector>

void printUsage(const std::string &programName) {
    std::cout << "Usage: " << programName << " [options] <input.c>\n"
              << "Options:\n"
              << "  -o <file>      Specify output file (default: a.out)\n"
              << "  -v             Verbose output\n"
              << "  -d             Debug mode (emit LLVM IR)\n"
              << "  -E             Preprocess only\n"
              << "  -I <dir>       Add include directory\n"
              << "  -D <macro>     Define macro\n"
              << "  -h, --help     Show this help message\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile;
    std::string outputFile = "a.out";
    bool verbose = false;
    bool debug = false;
    bool preprocessOnly = false;
    
    driver::Driver driver;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "-d") {
            debug = true;
        } else if (arg == "-E") {
            preprocessOnly = true;
        } else if (arg == "-I") {
            if (i + 1 < argc) {
                driver.addIncludeDirectory(argv[++i]);
            } else {
                std::cerr << "Error: -I requires an argument\n";
                return 1;
            }
        } else if (arg == "-D") {
            if (i + 1 < argc) {
                driver.addMacroDefinition(argv[++i]);
            } else {
                std::cerr << "Error: -D requires an argument\n";
                return 1;
            }
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return 1;
            }
        } else if (arg.front() != '-') {
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files not supported\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option " << arg << "\n";
            return 1;
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Configure driver
    driver.setVerbose(verbose);
    driver.setDebug(debug);
    driver.setPreprocessOnly(preprocessOnly);
    
    return driver.compile(inputFile, outputFile);
}
