// ALL AI GENERATED
package miniscala

import java.io.IOException
import RegisterCPSTreeModule._

/**
 * C source file writer for the MiniScala → C → WASM backend.
 *
 * Mirrors the shape of ASMFileWriter exactly:
 *   (fileName: String) => (tree: Tree) => Unit
 *
 * Usage in the compiler driver:
 *   CPSCFileWriter("out/program.c")(cpsTree)
 *
 * The emitter (CPSToCEmitter) owns all content decisions — what goes
 * in the file, in what order, including the #include preamble.
 * This wrapper owns only the filename and error boundary.
 */
object CPSCFileWriter extends (String => (Tree => Unit)) {

  def apply(fileName: String): (Tree => Unit) = { tree =>
    try {
      CPSToCEmitter(fileName)(tree)
    } catch {
      case e: IOException =>
        sys.error(s"CPSCFileWriter: could not write '$fileName': ${e.getMessage}")
    }
  }
}
