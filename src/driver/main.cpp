#include "driver/Driver.h"
#include <iostream>
#include <string>
#include <vector>

void printUsage(const std::string &programName) {
    std::cout << "Usage: " << programName << " [options] <input.c>\n"
              << "Options:\n"
              << "  -o <file>   Specify output file (default: a.out)\n"
              << "  -v          Verbose output\n"
              << "  -d          Debug mode (emit LLVM IR)\n"
              << "  -h, --help  Show this help message\n";
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
    
    // Create driver and compile
    driver::Driver driver;
    driver.setVerbose(verbose);
    driver.setDebug(debug);
    
    return driver.compile(inputFile, outputFile);
}
