# MMOC Status Report  (V0.1)

## Fully Implemented
- Core program structure, variables, functions, control flow (if/while/for, break/continue)
- Arithmetic + comparison + bitwise ops
- Logical operators with short-circuit (&&, ||)
- Ternary operator ?: with PHI-based IR
- Pointers (multi-level), arrays (basic init), address-of/deref
- Preprocessor (#include, #define via clang -E)
- _Bool type and boolean normalization in branches
- Basic literals: int, char, string (narrow)
- Return handling with default insertion
- Compound assignment operators (+=, -=, *=, /=, %=)
- Prefix & postfix ++ / -- (basic int vars) with correct value semantics

## Partially Implemented 
- Type system: currently treats most arithmetic as int (no implicit promotion rules yet)
- Pointer arithmetic scaling not implemented
- Array indexing lowered manually (no GEP scaling yet)
- No struct/union/enum support yet
- No typedef resolution
- No floating point operations despite float/double literal placeholder


## TODO 
1. sizeof (expressions + primitive types) minimal constants
2. Pointer arithmetic scaling (Add/Sub when one side is pointer)
3. Comma operator sequencing (left eval then right result)
4. do-while loop
5. switch/case/default lowering (chain first)
6. Struct/union parsing path in AST + member access (no padding accuracy at first)
7. Enums (sequential value assignment)
8. Typedef table + resolution in parser/AST builder
9. Integer type spec combinations & promotion rules in TypeChecker
10. float/double arithmetic & constants
11. sizeof for aggregates + array size inference
12. Designated & nested initializers
13. Pointer arithmetic refinement (struct member size, array decay)
14. Function pointers & complex declarators
15. Variadics (prototype + builtin va_arg path) minimal
16. Storage duration & linkage semantics (static, extern)
17. Qualifiers propagation (const, volatile, restrict)
18. Alignment (_Alignas/_Alignof)
19. _Static_assert check
20. _Generic dispatch (basic selection) 
21. Atomics (_Atomic qualifier treat as plain for now)
22. _Noreturn annotation (suppress fallthrough return insertion)
23. Thread-local storage (_Thread_local)
24. Enhanced diagnostics & semantic checks

## Testing Status:  
- 41/41 tests passing.
- New tests upcoming: sizeof/basic_types.c, sizeof/expressions.c, pointer_arith/basic.c