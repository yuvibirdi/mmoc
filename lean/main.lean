/- Lean 4 Skeleton for a Mini CompCert-like C Compiler -/

/-! # Mini C Subset Syntax -/

inductive Expr
| const : Int → Expr
| var   : String → Expr
| add   : Expr → Expr → Expr
| sub   : Expr → Expr → Expr
| mul   : Expr → Expr → Expr
| div   : Expr → Expr → Expr
  deriving Repr

inductive Stmt
| skip    : Stmt
| assign  : String → Expr → Stmt
| seq     : Stmt → Stmt → Stmt
| ifthen  : Expr → Stmt → Stmt
| while   : Expr → Stmt → Stmt
| return  : Expr → Stmt
  deriving Repr

abbrev Env := List (String × Int)

/-! # Big-step Evaluation Semantics -/

inductive EvalExpr : Expr → Env → Int → Prop
| const_eval : ∀ (n : Int) (env : Env), EvalExpr (Expr.const n) env n
| var_eval : ∀ (x : String) (v : Int) (env : Env),
    (x, v) ∈ env → EvalExpr (Expr.var x) env v
| add_eval : ∀ (e1 e2 : Expr) (v1 v2 : Int) (env : Env),
    EvalExpr e1 env v1 → EvalExpr e2 env v2 →
    EvalExpr (Expr.add e1 e2) env (v1 + v2)
| sub_eval : ∀ (e1 e2 : Expr) (v1 v2 : Int) (env : Env),
    EvalExpr e1 env v1 → EvalExpr e2 env v2 →
    EvalExpr (Expr.sub e1 e2) env (v1 - v2)
| mul_eval : ∀ (e1 e2 : Expr) (v1 v2 : Int) (env : Env),
    EvalExpr e1 env v1 → EvalExpr e2 env v2 →
    EvalExpr (Expr.mul e1 e2) env (v1 * v2)
| div_eval : ∀ (e1 e2 : Expr) (v1 v2 : Int) (env : Env),
    EvalExpr e1 env v1 → EvalExpr e2 env v2 → v2 ≠ 0 →
    EvalExpr (Expr.div e1 e2) env (v1 / v2)

/-! # Interpreter for Semantics Testing -/

def lookup (env : Env) (x : String) : Option Int :=
  env.find? (·.1 = x) |>.map (·.2)

def update (env : Env) (x : String) (v : Int) : Env :=
  let env' := env.filter (λ (y, _) => y ≠ x)
  (x, v) :: env'

/-! # Pretty Printer (IR or Target Output Stub) -/

def Expr.toString : Expr → String
| .const n => toString n
| .var x   => x
| .add e1 e2 => s!"({e1.toString} + {e2.toString})"
| .sub e1 e2 => s!"({e1.toString} - {e2.toString})"
| .mul e1 e2 => s!"({e1.toString} * {e2.toString})"
| .div e1 e2 => s!"({e1.toString} / {e2.toString})"

/-! # Next Steps
- Define EvalStmt
- Write compile passes (AST → IR)
- Prove transformations preserve semantics
- Output LLVM IR as string
-/
