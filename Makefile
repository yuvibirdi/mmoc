# Makefile for MMOC compiler
# This is a convenience wrapper around CMake

.PHONY: all build clean test setup install debug release format lint

# Default target
all: build

# Setup dependencies
setup:
	@echo "Setting up dependencies..."
	./setup.sh

# Configure and build (release by default)
build:
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build build --target all -j

# Debug build
debug:
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --target all -j

# Release build
release:
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build build --target all -j

# Generate ANTLR parser files manually
grammar:
	mkdir -p generated
	antlr4 -Dlanguage=Cpp -visitor -o generated grammar/C.g4

# Run tests
test: build
	ctest --test-dir build --output-on-failure

# Integration test
test-integration: build
	@echo "Running integration tests..."
	./build/ccomp test/inputs/hello.c -o test_hello && ./test_hello
	./build/ccomp test/inputs/arithmetic.c -o test_arithmetic && ./test_arithmetic
	@echo "Integration tests passed!"

# Clean build artifacts
clean:
	rm -rf build/
	rm -f a.out test_hello test_arithmetic *.ll *.o

# Clean everything including generated files
distclean: clean
	rm -rf generated/

# Format code
format:
	find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run static analysis
lint:
	cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem src/

# Install to system (requires admin privileges)
install: build
	sudo cmake --install build

# Show help
help:
	@echo "MMOC Compiler Build System"
	@echo "=========================="
	@echo ""
	@echo "Targets:"
	@echo "  setup          Install dependencies"
	@echo "  build          Build the compiler (default)"
	@echo "  debug          Build with debug info"
	@echo "  release        Build optimized release"
	@echo "  grammar        Generate ANTLR parser"
	@echo "  test           Run unit tests"
	@echo "  test-integration Run integration tests"
	@echo "  clean          Clean build artifacts"
	@echo "  distclean      Clean everything"
	@echo "  format         Format source code"
	@echo "  lint           Run static analysis"
	@echo "  install        Install to system"
	@echo "  help           Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make setup     # Install dependencies"
	@echo "  make           # Build the compiler"
	@echo "  make test      # Run tests"
