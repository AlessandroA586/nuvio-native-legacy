package space.nuvio.nativelegacy

import java.net.ConnectException
import java.net.SocketTimeoutException
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class FalhaFonteTest {
    @Test
    fun `conexao recusada em no TorBox pede outra fonte`() {
        val erro = RuntimeException("source", FalhaConexaoParalela(ConnectException("refused")))
        assertTrue(FalhaFonte.conexaoCdnTorBox("https://nexus-128.neur.tb-cdn.st/video", erro))
    }

    @Test
    fun `nao confunde outro host ou timeout com no morto`() {
        assertFalse(FalhaFonte.conexaoCdnTorBox("https://comet.feels.legal/video", ConnectException()))
        assertFalse(FalhaFonte.conexaoCdnTorBox("https://tb-cdn.st.example/video", ConnectException()))
        assertFalse(FalhaFonte.conexaoCdnTorBox("https://nexus-128.neur.tb-cdn.st/video", SocketTimeoutException()))
        // HLS/DASH e o caminho HTTP de uma conexao nao carregam a marca do
        // downloader paralelo, portanto conservam a politica normal do Media3.
        assertFalse(FalhaFonte.conexaoCdnTorBox("https://nexus-128.neur.tb-cdn.st/live.m3u8", ConnectException()))
    }
}
