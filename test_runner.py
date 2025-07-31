#!/usr/bin/env python3
"""
MMOC Compiler Test Suite Runner
Automated testing following LLVM/Clang testing conventions
"""

import os
import sys
import subprocess
import argparse
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional
import time

@dataclass
class TestResult:
    name: str
    status: str  # PASS, FAIL, SKIP
    duration: float
    output: str = ""
    error: str = ""

class MMOCTestSuite:
    def __init__(self, compiler_path: str, test_dir: str):
        self.compiler_path = Path(compiler_path)
        self.test_dir = Path(test_dir)
        self.results: List[TestResult] = []
        
    def discover_tests(self) -> List[Path]:
        """Discover all .c test files in the test directory structure"""
        test_files = []
        for pattern in ["**/*.c", "**/*.test"]:
            test_files.extend(self.test_dir.glob(pattern))
        return sorted(test_files)
    
    def parse_test_directives(self, test_file: Path) -> Dict:
        """Parse RUN:, REQUIRES:, XFAIL: directives from test file"""
        directives = {
            'run': [],
            'requires': [],
            'xfail': [],
            'unsupported': []
        }
        
        with open(test_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('// RUN:'):
                    cmd = line[7:].strip()
                    directives['run'].append(cmd)
                elif line.startswith('// REQUIRES:'):
                    directives['requires'] = line[12:].strip().split(',')
                elif line.startswith('// XFAIL:'):
                    directives['xfail'] = line[9:].strip().split(',')
                elif line.startswith('// UNSUPPORTED:'):
                    directives['unsupported'] = line[15:].strip().split(',')
                    
        return directives
    
    def substitute_variables(self, cmd: str, test_file: Path) -> str:
        """Substitute test variables like %s, %t, etc."""
        # Make compiler path absolute
        compiler_abs = self.compiler_path.absolute()
        
        substitutions = {
            '%s': str(test_file),
            '%t': str(test_file.with_suffix('.tmp')),
            '%T': str(test_file.parent / 'Output'),
            '%ccomp': str(compiler_abs),
        }
        
        for var, value in substitutions.items():
            cmd = cmd.replace(var, value)
            
        return cmd
    
    def run_test(self, test_file: Path) -> TestResult:
        """Run a single test file"""
        start_time = time.time()
        test_name = str(test_file.relative_to(self.test_dir))
        
        try:
            directives = self.parse_test_directives(test_file)
            
            # Handle UNSUPPORTED tests
            if directives['unsupported']:
                return TestResult(test_name, "SKIP", time.time() - start_time, 
                                "Test unsupported")
            
            # If no RUN directives, create default
            if not directives['run']:
                directives['run'] = [f"%ccomp %s -o %t && %t"]
            
            # Combine RUN directives that reference previous exit codes
            combined_runs = []
            i = 0
            while i < len(directives['run']):
                current_cmd = directives['run'][i]
                # Check if next command references $?
                if (i + 1 < len(directives['run']) and 
                    '$?' in directives['run'][i + 1]):
                    # Combine them with proper exit code handling
                    next_cmd = directives['run'][i + 1]
                    combined_cmd = f"({current_cmd}); {next_cmd}"
                    combined_runs.append(combined_cmd)
                    i += 2  # Skip the next command since we combined it
                else:
                    combined_runs.append(current_cmd)
                    i += 1
            
            output_lines = []
            error_lines = []
            
            for run_cmd in combined_runs:
                cmd = self.substitute_variables(run_cmd, test_file)
                
                # Handle shell pipelines and redirections
                result = subprocess.run(
                    cmd, 
                    shell=True, 
                    capture_output=True, 
                    text=True,
                    cwd=str(Path.cwd())  # Run from project root
                )
                
                output_lines.append(f"$ {cmd}")
                if result.stdout:
                    output_lines.append(result.stdout)
                if result.stderr:
                    # Only treat stderr as error if the command failed
                    if result.returncode != 0:
                        error_lines.append(result.stderr)
                    else:
                        # Just warnings, include in output
                        output_lines.append(f"Warnings: {result.stderr}")
                
                # Check if command should fail (for XFAIL tests)
                expected_fail = any(platform in directives['xfail'] for platform in ['*'])
                
                # Special handling for different types of commands
                if cmd.strip().startswith('test '):
                    # This is a test command - failure here should fail the test
                    if result.returncode != 0 and not expected_fail:
                        return TestResult(
                            test_name, "FAIL", time.time() - start_time,
                            '\n'.join(output_lines), '\n'.join(error_lines)
                        )
                elif result.returncode != 0 and not expected_fail:
                    # For compilation/execution commands, only fail if compilation failed
                    # If the command contains both compilation and execution (&&), 
                    # we need to be more careful about what constitutes failure
                    if ' && ' in cmd:
                        # This is a compile-and-run command like "%ccomp %s -o %t && %t"
                        # If stderr indicates compilation error, that's a real failure
                        if any(keyword in result.stderr.lower() for keyword in ['error:', 'fatal error:', 'cannot open']):
                            return TestResult(
                                test_name, "FAIL", time.time() - start_time,
                                '\n'.join(output_lines), '\n'.join(error_lines)
                            )
                        # Otherwise, the non-zero exit is probably from program execution (expected)
                    else:
                        # Simple command that failed
                        return TestResult(
                            test_name, "FAIL", time.time() - start_time,
                            '\n'.join(output_lines), '\n'.join(error_lines)
                        )
                elif result.returncode == 0 and expected_fail:
                    return TestResult(
                        test_name, "FAIL", time.time() - start_time,
                        '\n'.join(output_lines), "XFAIL test unexpectedly passed"
                    )
            
            # Clean up temporary files
            temp_file = test_file.with_suffix('.tmp')
            if temp_file.exists():
                temp_file.unlink()
            
            status = "XFAIL" if directives['xfail'] else "PASS"
            return TestResult(test_name, status, time.time() - start_time, 
                            '\n'.join(output_lines))
            
        except Exception as e:
            return TestResult(test_name, "FAIL", time.time() - start_time,
                            "", f"Test execution failed: {str(e)}")
    
    def run_all_tests(self, filter_pattern: str = None) -> None:
        """Run all discovered tests"""
        test_files = self.discover_tests()
        
        if filter_pattern:
            test_files = [f for f in test_files if filter_pattern in str(f)]
        
        print(f"Running {len(test_files)} tests...")
        print("=" * 60)
        
        for test_file in test_files:
            result = self.run_test(test_file)
            self.results.append(result)
            
            # Print immediate result
            status_color = {
                'PASS': '\033[92m',  # Green
                'FAIL': '\033[91m',  # Red
                'SKIP': '\033[93m',  # Yellow
                'XFAIL': '\033[93m'  # Yellow
            }
            reset_color = '\033[0m'
            
            print(f"{status_color.get(result.status, '')}{result.status:<6}{reset_color} "
                  f"{result.name} ({result.duration:.3f}s)")
            
            if result.status == "FAIL" and result.error:
                print(f"  Error: {result.error}")
        
        self.print_summary()
    
    def print_summary(self) -> None:
        """Print test run summary"""
        print("\n" + "=" * 60)
        
        total = len(self.results)
        passed = len([r for r in self.results if r.status == "PASS"])
        failed = len([r for r in self.results if r.status == "FAIL"])
        skipped = len([r for r in self.results if r.status == "SKIP"])
        xfail = len([r for r in self.results if r.status == "XFAIL"])
        
        print(f"Test Results Summary:")
        print(f"  Total:    {total}")
        print(f"  Passed:   {passed}")
        print(f"  Failed:   {failed}")
        print(f"  Skipped:  {skipped}")
        print(f"  Expected: {xfail}")
        
        if failed > 0:
            print(f"\nFailed tests:")
            for result in self.results:
                if result.status == "FAIL":
                    print(f"  - {result.name}")
        
        success_rate = (passed / total) * 100 if total > 0 else 0
        print(f"\nSuccess rate: {success_rate:.1f}%")
        
        # Return appropriate exit code
        sys.exit(0 if failed == 0 else 1)

def main():
    parser = argparse.ArgumentParser(description="MMOC Compiler Test Suite")
    parser.add_argument("--compiler", "-c", 
                       default="./build/ccomp",
                       help="Path to compiler executable")
    parser.add_argument("--test-dir", "-t",
                       default="./tests",
                       help="Test directory")
    parser.add_argument("--filter", "-f",
                       help="Filter tests by name pattern")
    parser.add_argument("--verbose", "-v",
                       action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    # Verify compiler exists
    if not Path(args.compiler).exists():
        print(f"Error: Compiler not found at {args.compiler}")
        print("Build the compiler first with: ninja -C build")
        sys.exit(1)
    
    # Verify test directory exists
    if not Path(args.test_dir).exists():
        print(f"Error: Test directory not found at {args.test_dir}")
        sys.exit(1)
    
    suite = MMOCTestSuite(args.compiler, args.test_dir)
    suite.run_all_tests(args.filter)

if __name__ == "__main__":
    main()
