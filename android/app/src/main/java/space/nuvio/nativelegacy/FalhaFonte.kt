package space.nuvio.nativelegacy

import java.net.ConnectException
import java.net.URI

/** Classifica falhas que nao melhoram reabrindo a mesma URL final do TorBox. */
internal object FalhaFonte {
    fun conexaoCdnTorBox(url: String, erro: Throwable): Boolean {
        val host = try { URI(url).host?.lowercase() } catch (_: Exception) { null }
        if (host == null || !host.endsWith(".tb-cdn.st")) return false

        var causa: Throwable? = erro
        var nivel = 0
        while (causa != null && nivel++ < 16) {
            if (causa is ConnectException) return true
            causa = causa.cause
        }
        return false
    }
}
