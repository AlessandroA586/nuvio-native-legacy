#!/usr/bin/env python3
"""Execute production crop request/acknowledgement with queued UI/pre-draw doubles."""
from pathlib import Path
import tempfile
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / 'android/app/src/main/java/space/nuvio/nativelegacy/NvPlayer.kt').read_text()
start = source.index('    private val janelaPedido =')
end = source.index('    @JvmStatic fun escolher(', start)
methods = source[start:end]
fixture = '''
import java.util.concurrent.atomic.AtomicInteger
object View { const val VISIBLE = 0 }
class Surface { var isValid = true }
class Holder { val surface = Surface() }
class Observer {
    interface OnPreDrawListener { fun onPreDraw(): Boolean }
    val listeners = mutableListOf<OnPreDrawListener>()
    fun addOnPreDrawListener(l: OnPreDrawListener) { listeners.add(l) }
    fun removeOnPreDrawListener(l: OnPreDrawListener) { listeners.remove(l) }
    fun draw() { listeners.toList().forEach { it.onPreDraw() } }
}
class SurfaceView {
    val holder = Holder(); val viewTreeObserver = Observer()
    var isLayoutRequested = false; var visibility = View.VISIBLE
    fun invalidate() {}
}
class Queue {
    val pending = mutableListOf<() -> Unit>()
    fun post(f: () -> Unit) { pending.add(f) }
    fun drain() { pending.toList().also { pending.clear() }.forEach { it() } }
}
object P {
    var superficie: SurfaceView? = SurfaceView()
    var player: Any? = Any()
    var recriando = false; var recriaAgendada = false
    val principal = Queue()
    fun definirJanela(x: Int, y: Int, w: Int, h: Int, encaixa: Boolean) {}
METHODS
}
fun main() {
    fun request(x: Int = 10) = P.janela(x, 20, 320, 180, 0)
    fun draw() { P.principal.drain(); P.superficie!!.viewTreeObserver.draw() }
    request(); check(!P.janelaPronta()); P.principal.drain()
    check(!P.janelaPronta()); draw(); check(P.janelaPronta())
    // Four 1.5-second retries: SDL can query before UI dispatch or pre-draw.
    repeat(4) {
        request()
        check(P.janelaPronta()) { "unchanged crop retry hid playing trailer before UI dispatch" }
        P.principal.drain()
        check(P.janelaPronta()) { "unchanged crop retry hid playing trailer before pre-draw" }
        draw(); check(P.janelaPronta())
    }
    request(11); check(!P.janelaPronta()); P.principal.drain()
    request(12); P.superficie!!.viewTreeObserver.draw()
    check(!P.janelaPronta()) { "stale pre-draw acknowledged changed crop" }
    draw(); check(P.janelaPronta())
    P.superficie = SurfaceView()
    check(!P.janelaPronta()) { "replacement surface inherited old acknowledgement" }
    request(12); check(!P.janelaPronta()); draw(); check(P.janelaPronta())
    println("android_window_ready: unchanged retries stay visible; changed crop/surface await pre-draw")
}
'''.replace('METHODS', methods).replace('android.view.ViewTreeObserver.OnPreDrawListener', 'Observer.OnPreDrawListener')
# Reuse the existing cached Kotlin compiler setup without running its fixture.
setup = (root / 'tests/android_window_layout.py').read_text()
setup = setup[setup.index('cache = Path.home()'):setup.index('with tempfile.TemporaryDirectory(')]
namespace = dict(Path=Path, os=__import__('os'))
exec(setup, namespace)
java, classpath, stdlib = (namespace[k] for k in ('java', 'classpath', 'stdlib'))
with tempfile.TemporaryDirectory(prefix='nuvio-window-ready-') as work:
    work = Path(work)
    file = work / 'Ready.kt'
    file.write_text(fixture)
    target = work / 'classes'
    subprocess.run([java, '-cp', classpath, 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
                    '-no-stdlib', '-no-reflect', '-nowarn', '-classpath', stdlib,
                    '-jvm-target', '17', '-d', str(target), str(file)], check=True)
    subprocess.run([java, '-cp', f'{target}:{stdlib}', 'ReadyKt'], check=True)
