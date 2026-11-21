file://<WORKSPACE>/src/miniscala/CPSToCEmitter.scala
### java.lang.IllegalArgumentException: Comparison method violates its general contract!

occurred in the presentation compiler.

presentation compiler configuration:


action parameters:
offset: 7890
uri: file://<WORKSPACE>/src/miniscala/CPSToCEmitter.scala
text:
```scala
package miniscala

import java.io.{ FileWriter, BufferedWriter }
import RegisterCPSTreeModule._

/**
 * CPS IR → C source emitter.
 * Produces a single .c file that #includes l3runtime.h and is
 * ready to be compiled with emcc (or any C99 compiler).
 *
 * Conventions
 * ───────────
 *  • Every L3 value (integer or block pointer) is typed L3Val.
 *  • L₃ functions become C functions: L3Val f(L3Val I0, ..., L3Val In).
 *  • Continuations become goto-labels inside their enclosing C function.
 *  • The return continuation (register I3 in the ASM backend) is
 *    recognised by checking whether the retC Name is the I3 register;
 *    AppF to it emits `return` instead of a call + goto.
 *  • LetF is two-pass: forward declarations first, then definitions,
 *    so mutually recursive functions compile without ordering issues.
 */
object CPSToCEmitter extends (String => (Tree => Unit)) {

  // The return-continuation register is I3 (index 3 of the In bank)
  private val ReturnReg = ASMRegisterFile.in(3)

  def apply(fileName: String): (Tree => Unit) = { tree =>
    val out = new BufferedWriter(new FileWriter(fileName))

    def emit(s: String): Unit = out.write(s)
    def emitLn(s: String = ""): Unit = { out.write(s); out.write("\n") }

    // ── Name helpers ─────────────────────────────────────────────────

    // Produce a legal C identifier from a Symbol name.
    // Scala symbols can contain '$' which is not valid mid-identifier in C.
    def symName(s: Symbol): String =
      "l3_" + s.name.replace('$', '_').replace('-', '_')

    // Name for a register variable inside a function body.
    def regName(n: Name): String = n match {
      case Reg(ASMRegister(base, idx)) =>
        ASMRegisterBank.prefix(base.bank) + idx
      case Label(sym) => symName(sym)
    }

    // ── Register collection ───────────────────────────────────────────────────
    // Walk a tree and collect every register name that appears as a
    // binding site (LetL or LetP result). These need to be pre-declared
    // at the top of the enclosing C function to avoid redeclaration errors.

    // def collectRegs(tree: Tree): Set[String] = tree match {
    //   case LetL(r, _, body) =>
    //     Set(regName(r)) ++ collectRegs(body)
    //   case LetP(r, _, _, body) =>
    //     Set(regName(r)) ++ collectRegs(body)
    //   case LetC(cnts, body) =>
    //     collectRegs(body) ++ cnts.flatMap(c => collectRegs(c.body))
    //   case LetF(_, body) =>
    //     collectRegs(body)   // fun bodies are collected separately per-function
    //   case _ =>
    //     Set.empty
    // }

    def collectRegs(tree: Tree): Set[String] = tree match {
      case LetL(r, _, body) =>
        Set(regName(r)) ++ collectRegs(body)

      case LetP(r, _, _, body) =>
        Set(regName(r)) ++ collectRegs(body)

      case LetC(cnts, body) =>
        collectRegs(body) ++
        cnts.flatMap { c =>
          c.args.map(regName).toSet ++ collectRegs(c.body)
        }

      case LetF(_, body) =>
        collectRegs(body)

      case _ =>
        Set.empty
    }

    // Name for a continuation label (goto target).
    def cntLabel(sym: Symbol): String = "cnt_" + symName(sym)

    // ── Type / primitive helpers ──────────────────────────────────────

    // Emit the C expression for a primitive applied to its arguments.
    def emitPrim(p: CPSValuePrimitive, args: Seq[Name]): String = {
      val a = args.map(regName)
      p match {
        case CPSAdd          => s"${a(0)} + ${a(1)}"
        case CPSSub          => s"${a(0)} - ${a(1)}"
        case CPSMul          => s"${a(0)} * ${a(1)}"
        case CPSDiv          => s"${a(0)} / ${a(1)}"
        case CPSMod          => s"${a(0)} % ${a(1)}"
        case CPSArithShiftL  => s"${a(0)} << ${a(1)}"
        case CPSArithShiftR  => s"${a(0)} >> ${a(1)}"
        case CPSAnd          => s"${a(0)} & ${a(1)}"
        case CPSOr           => s"${a(0)} | ${a(1)}"
        case CPSXOr          => s"${a(0)} ^ ${a(1)}"
        case CPSId           => a(0)
        case CPSByteRead     => "l3_brea()"
        case CPSByteWrite    => s"(l3_bwri(${a(0)}), 0)"
        case CPSBlockAlloc(tag) => s"gc_alloc($tag, ${a(0)})"
        case CPSBlockTag     => s"l3_tag(${a(0)})"
        case CPSBlockLength  => s"l3_size(${a(0)})"
        case CPSBlockGet     => s"l3_get(${a(0)}, ${a(1)})"
        case CPSBlockSet     => s"(l3_set(${a(0)}, ${a(1)}, ${a(2)}), 0)"
      }
    }

    // C comparison operator for a CPS test primitive.
    def testOp(p: CPSTestPrimitive): String = p match {
      case CPSLt => "<"
      case CPSLe => "<="
      case CPSEq => "=="
      case CPSNe => "!="
      case CPSGe => ">="
      case CPSGt => ">"
    }

    // ── Function signature helpers ────────────────────────────────────

    // Build the C parameter list from the In-register arguments
    // of a CPS function definition.
    def paramList(args: Seq[Name]): String =
      args.map(n => s"L3Val ${regName(n)}").mkString(", ")

    // ── First pass: collect all LetF definitions for hoisting ─────────

    // We need forward declarations before we emit main(), so mutually
    // recursive functions don't need to be in source order.
    def collectFuns(tree: Tree): Seq[FunDef] = tree match {
      case LetL(_, _, body)       => collectFuns(body)
      case LetP(_, _, _, body)    => collectFuns(body)
      case LetC(cnts, body)       =>
        collectFuns(body) ++ cnts.flatMap(c => collectFuns(c.body))
      case LetF(funs, body)       =>
        funs ++ funs.flatMap(f => collectFuns(f.body)) ++ collectFuns(body)
      case _                      => Seq()
    }

    def collectConts(tree: Tree): Map[Symbol, Seq[Name]] = tree match {
      case LetL(_, _, body) =>
        collectConts(body)

      case LetP(_, _, _, body) =>
        collectConts(body)

      case LetC(cnts, body) =>
        cnts.map {
          case CntDef(Label(name), args, _) => name -> args
        }.toMap ++
        collectConts(body) ++
        cnts.flatMap(c => collectConts(c.body))

      case LetF(_, body) =>
        collectConts(body)

      case _ =>
        Map.empty
    }

    // ── Body emitter ──────────────────────────────────────────────────

    // Emit the body of a single CPS function or the top-level program.
    // `retContName` is the Name that acts as the return continuation
    // for the current function (so AppC on it becomes `return`).
    def emitBody(tree: Tree, indent: String, contEnv: Map[Symbol, Seq[Name]]): Unit = tree match {

      // ── Literal binding ──────────────────────────────────────────
      case LetL(r, v, body) =>
        // emitLn(s"${indent}L3Val ${regName(r)} = $v;")
        emitLn(s"${indent}${regName(r)} = $v;")
        emitBody(body, indent, contEnv)

      // ── Primitive op ─────────────────────────────────────────────
      case LetP(r, prim, args, body) =>
        // emitLn(s"${indent}L3Val ${regName(r)} = ${emitPrim(prim, args)};")
        emitLn(s"${indent}${regName(r)} = ${emitPrim(prim, args)};")
        emitBody(body, indent, contEnv)

      // ── Continuation bindings ─────────────────────────────────────
      // Emit the *body* first (continuations are only jumped to, never
      // fallen through to from outside), then emit each continuation
      // as a labeled block. This mirrors what CPSToASMTranslator does
      // with its `conts` mutable map.
      case LetC(cnts, body) =>
        emitBody(body, indent, contEnv)
        for (CntDef(Label(name), args, cntBody) <- cnts) {
          emitLn(s"${indent}${cntLabel(name)}:;")
          // Declare the continuation's parameter variables.
          // In the register CPS these are already allocated registers,
          // but we need them visible at the label site.
          for (arg <- args)
            emitLn(s"${indent}  L3Val ${regName(arg)} = ${regName(arg)};")
          emitBody(cntBody, indent + "  ", contE@@)
        }

      // ── Function definitions ──────────────────────────────────────
      // Functions are already hoisted; skip here (body still emitted).
      case LetF(_, body) =>
        emitBody(body, indent)

      // ── Continuation application ──────────────────────────────────
      // If the target label is our return continuation → return.
      // Otherwise assign args and goto the label.
      case AppC(Label(cnt), args) =>
        // Emit nothing for the arg assignment if cnt is a local goto;
        // the register names are already the right variables.
        // (Register CPS guarantees args are already in the right regs.)
        emitLn(s"${indent}goto ${cntLabel(cnt)};")

      // AppC on the return register (I3) — this is a return.
      case AppC(Reg(ReturnReg), args) =>
        val retVal = if (args.nonEmpty) regName(args.head) else "0"
        emitLn(s"${indent}return $retVal;")

      // ── Function application ──────────────────────────────────────
      // Tail call: retC is the return register I3 → emit `return`.
      case AppF(f, Reg(ReturnReg), args) =>
        val argList = args.map(regName).mkString(", ")
        emitLn(s"${indent}return ((L3FnPtr)${regName(f)})($argList);")

      // Non-tail call: retC is a local continuation label.
      case AppF(f, Label(retC), args) =>
        val argList = args.map(regName).mkString(", ")
        // The return value lands in the first argument register of retC.
        // We use a temporary then goto the continuation.
        emitLn(s"${indent}L3Val _ret = ((L3FnPtr)${regName(f)})($argList);")
        emitLn(s"${indent}goto ${cntLabel(retC)};  /* _ret holds result */")

      // ── Conditional branch ────────────────────────────────────────
      case If(p, Seq(a, b), Label(thenC), Label(elseC)) =>
        emitLn(s"${indent}if (${regName(a)} ${testOp(p)} ${regName(b)})")
        emitLn(s"${indent}  goto ${cntLabel(thenC)};")
        emitLn(s"${indent}else")
        emitLn(s"${indent}  goto ${cntLabel(elseC)};")

      // ── Program termination ───────────────────────────────────────
      case Halt(r) =>
        emitLn(s"${indent}exit((int)${regName(r)});")

      case other =>
        sys.error(s"CPSToCEmitter: unhandled node: $other")
    }

    // ── Top-level emit ────────────────────────────────────────────────

    emitLn("/* Auto-generated by CPSToCEmitter — do not edit */")
    emitLn("#include \"l3runtime.h\"")
    emitLn()

    // Function pointer typedef — every L3 function has the same shape.
    // We cast to it at every call site, letting the actual arity vary.
    emitLn("typedef L3Val (*L3FnPtr)();")
    emitLn()

    val allFuns = collectFuns(tree)

    // Forward declarations (so mutually recursive functions compile).
    for (FunDef(Label(name), retC, args, body) <- allFuns) {
      emitLn(s"static L3Val ${symName(name)}(${paramList(args)});")
    }
    if (allFuns.nonEmpty) emitLn()

    // Full function definitions.
    for (FunDef(Label(name), retC, args, body) <- allFuns) {
      emitLn(s"static L3Val ${symName(name)}(${paramList(args)}) {")
      // Pre-declare all locals used in this function body
      val regs = collectRegs(body) -- args.map(regName).toSet
      for (r <- regs.toSeq.sorted)
        emitLn(s"  L3Val $r = 0;")
      emitBody(body, "  ")
      emitLn("}")
      emitLn()
    }

    // Entry point — the top-level CPS tree is the program body.
    emitLn("int main(void) {")
    val mainRegs = collectRegs(tree)
    for (r <- mainRegs.toSeq.sorted)
      emitLn(s"  L3Val $r = 0;")
    emitBody(tree, "  ")
    emitLn("  return 0;")
    emitLn("}")

    out.close()
  }
}

```



#### Error stacktrace:

```
java.base/java.util.TimSort.mergeLo(TimSort.java:781)
	java.base/java.util.TimSort.mergeAt(TimSort.java:518)
	java.base/java.util.TimSort.mergeForceCollapse(TimSort.java:461)
	java.base/java.util.TimSort.sort(TimSort.java:254)
	java.base/java.util.Arrays.sort(Arrays.java:1233)
	scala.collection.SeqOps.sorted(Seq.scala:728)
	scala.collection.SeqOps.sorted$(Seq.scala:719)
	scala.collection.immutable.List.scala$collection$immutable$StrictOptimizedSeqOps$$super$sorted(List.scala:79)
	scala.collection.immutable.StrictOptimizedSeqOps.sorted(StrictOptimizedSeqOps.scala:82)
	scala.collection.immutable.StrictOptimizedSeqOps.sorted$(StrictOptimizedSeqOps.scala:82)
	scala.collection.immutable.List.sorted(List.scala:79)
	dotty.tools.pc.completions.Completions.completions(Completions.scala:145)
	dotty.tools.pc.completions.CompletionProvider.completions(CompletionProvider.scala:139)
	dotty.tools.pc.ScalaPresentationCompiler.complete$$anonfun$1(ScalaPresentationCompiler.scala:150)
```
#### Short summary: 

java.lang.IllegalArgumentException: Comparison method violates its general contract!