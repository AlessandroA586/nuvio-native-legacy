package space.nuvio.nativelegacy

import androidx.media3.common.C
import androidx.media3.exoplayer.upstream.DefaultLoadErrorHandlingPolicy
import androidx.media3.exoplayer.upstream.LoadErrorHandlingPolicy
import java.io.IOException
import java.net.ConnectException
import java.net.URI

/** Marca uma falha de conexao depois das tentativas do downloader paralelo. */
internal class FalhaConexaoParalela(causa: IOException) : IOException(causa)

/** Classifica falhas que nao melhoram reabrindo a mesma URL final do TorBox. */
internal object FalhaFonte {
    fun conexaoCdnTorBox(url: String, erro: Throwable): Boolean {
        val host = try { URI(url).host?.lowercase() } catch (_: Exception) { null }
        if (host == null || !host.endsWith(".tb-cdn.st")) return false

        var causa: Throwable? = erro
        var nivel = 0
        var paralela = false
        var conexao = false
        while (causa != null && nivel++ < 16) {
            if (causa is FalhaConexaoParalela) paralela = true
            if (causa is ConnectException) conexao = true
            causa = causa.cause
        }
        return paralela && conexao
    }
}

/** Mantem a politica Media3 para HLS/DASH e corta so o retry ja esgotado acima. */
internal class PoliticaErroFonte : DefaultLoadErrorHandlingPolicy() {
    override fun getRetryDelayMsFor(loadErrorInfo: LoadErrorHandlingPolicy.LoadErrorInfo): Long =
        if (temFalhaParalela(loadErrorInfo.exception)) C.TIME_UNSET
        else super.getRetryDelayMsFor(loadErrorInfo)

    private fun temFalhaParalela(erro: Throwable): Boolean {
        var causa: Throwable? = erro
        repeat(16) {
            if (causa is FalhaConexaoParalela) return true
            causa = causa?.cause ?: return false
        }
        return false
    }
}
