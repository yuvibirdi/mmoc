#!/usr/bin/env python3
"""
MMOC Compiler Test Suite Runner
Automated testing following LLVM/Clang testing conventions
Relocated to tests/ directory.
"""
import sys
import subprocess
import argparse
import time
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict

@dataclass
class TestResult:
    name: str
    status: str  # PASS, FAIL, SKIP, XFAIL
    duration: float
    output: str = ""
    error: str = ""

class MMOCTestSuite:
    def __init__(self, compiler_path: str, test_dir: str):
        self.compiler_path = Path(compiler_path)
        self.test_dir = Path(test_dir)
        self.results: List[TestResult] = []

    def discover_tests(self) -> List[Path]:
        tests: List[Path] = []
        for pattern in ("**/*.c", "**/*.test"):
            tests.extend(self.test_dir.glob(pattern))
        return sorted(tests)

    def parse_directives(self, path: Path) -> Dict:
        directives: Dict = {'run': [], 'xfail': [], 'unsupported': []}
        for line in path.read_text().splitlines():
            stripped = line.strip()
            if stripped.startswith('// RUN:'):
                directives['run'].append(stripped[7:].strip())
            elif stripped.startswith('// XFAIL:'):
                directives['xfail'] = stripped[9:].strip().split(',')
            elif stripped.startswith('// UNSUPPORTED:'):
                directives['unsupported'] = stripped[15:].strip().split(',')
        return directives

    def subst(self, cmd: str, path: Path) -> str:
        subs = {
            '%s': str(path),
            '%t': str(path.with_suffix('.tmp')),
            '%T': str(path.parent / 'Output'),
            '%ccomp': str(self.compiler_path.absolute()),
        }
        for key, value in subs.items():
            cmd = cmd.replace(key, value)
        return cmd

    def run_test(self, path: Path) -> TestResult:
        start = time.time()
        name = str(path.relative_to(self.test_dir))
        try:
            directives = self.parse_directives(path)
            if directives['unsupported']:
                return TestResult(name, 'SKIP', time.time() - start, 'Test unsupported')
            runs = directives['run'] or ["%ccomp %s -o %t && %t"]
            out_lines: List[str] = []
            err_lines: List[str] = []
            for r in runs:
                cmd = self.subst(r, path)
                res = subprocess.run(
                    cmd,
                    shell=True,
                    text=True,
                    capture_output=True,
                    cwd=str(Path.cwd())
                )
                out_lines.append(f"$ {cmd}")
                if res.stdout:
                    out_lines.append(res.stdout)
                if res.stderr and res.returncode == 0:
                    out_lines.append(f"Warnings: {res.stderr}")
                if res.returncode != 0:
                    if any(k in res.stderr.lower() for k in ['error:', 'fatal error:', 'cannot open']):
                        err_lines.append(res.stderr)
                        return TestResult(
                            name,
                            'FAIL',
                            time.time() - start,
                            '\n'.join(out_lines),
                            '\n'.join(err_lines)
                        )
            tmp = path.with_suffix('.tmp')
            if tmp.exists():
                tmp.unlink()
            status = 'XFAIL' if directives['xfail'] else 'PASS'
            return TestResult(name, status, time.time() - start, '\n'.join(out_lines))
        except Exception as e:
            return TestResult(name, 'FAIL', time.time() - start, '', str(e))

    def run(self, filt: str = None) -> None:
        tests = self.discover_tests()
        if filt:
            tests = [t for t in tests if filt in str(t)]
        print(f"Running {len(tests)} tests...")
        print('=' * 60)
        for tf in tests:
            result = self.run_test(tf)
            self.results.append(result)
            color_map = {
                'PASS': '\033[92m',
                'FAIL': '\033[91m',
                'SKIP': '\033[93m',
                'XFAIL': '\033[93m'
            }
            color = color_map.get(result.status, '')
            reset = '\033[0m'
            print(f"{color}{result.status:<6}{reset} {result.name} ({result.duration:.3f}s)")
            if result.status == 'FAIL' and result.error:
                print('  Error:', result.error)
        self.summary()

    def summary(self) -> None:
        print('\n' + '=' * 60)
        total = len(self.results)
        passed = sum(r.status == 'PASS' for r in self.results)
        failed = sum(r.status == 'FAIL' for r in self.results)
        skipped = sum(r.status == 'SKIP' for r in self.results)
        xfail = sum(r.status == 'XFAIL' for r in self.results)
        print('Test Results Summary:')
        print(f'  Total:    {total}')
        print(f'  Passed:   {passed}')
        print(f'  Failed:   {failed}')
        print(f'  Skipped:  {skipped}')
        print(f'  Expected: {xfail}')
        rate = (passed / total * 100) if total else 0
        print(f'\nSuccess rate: {rate:.1f}%')
        sys.exit(0 if failed == 0 else 1)

def main():
    parser = argparse.ArgumentParser(description='MMOC Test Suite')
    parser.add_argument('-c', '--compiler', default='./build/ccomp')
    parser.add_argument('-t', '--test-dir', default='./tests')
    parser.add_argument('-f', '--filter')
    args = parser.parse_args()
    if not Path(args.compiler).exists():
        print(f'Error: compiler not found at {args.compiler}')
        sys.exit(1)
    if not Path(args.test_dir).exists():
        print(f'Error: test dir not found at {args.test_dir}')
        sys.exit(1)
    MMOCTestSuite(args.compiler, args.test_dir).run(args.filter)

if __name__ == '__main__':
    main()
