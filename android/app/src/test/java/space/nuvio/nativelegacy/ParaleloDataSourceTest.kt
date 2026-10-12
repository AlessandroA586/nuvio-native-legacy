package space.nuvio.nativelegacy

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.ByteArrayInputStream
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.SocketTimeoutException
import java.net.URL
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

class ParaleloDataSourceTest {
    private open class ConexaoFalsa(url: URL) : HttpURLConnection(url) {
        val desconectou = AtomicBoolean()
        override fun connect() = Unit
        override fun usingProxy(): Boolean = false
        override fun disconnect() { desconectou.set(true) }
        override fun getInputStream(): InputStream = ByteArrayInputStream(byteArrayOf(1))
        override fun getResponseCode(): Int = HTTP_OK
        override fun getResponseMessage(): String = "OK"
    }

    @Test fun aberturaInicialRepeteFalhaDeConexaoComBackoffLimitado() {
        val aberturas = AtomicInteger()
        val conexoes = ArrayList<ConexaoFalsa>()
        val esperas = ArrayList<Long>()
        val fonte = ParaleloDataSource(null, emptyMap(),
            novaConexao = { url ->
                val tentativa = aberturas.incrementAndGet()
                val c = if (tentativa < 3) object : ConexaoFalsa(url) {
                    override fun getResponseCode(): Int = throw IOException("sem rota")
                } else ConexaoFalsa(url)
                conexoes += c
                c
            },
            dormir = { esperas += it })

        fonte.conectarComRetry(URL("https://example.invalid/video.mkv"), 0, 1023)
        fonte.close()

        assertEquals(3, aberturas.get())
        assertEquals(listOf(250L, 750L), esperas)
        assertTrue(conexoes.take(2).all { it.desconectou.get() })
    }

    @Test fun timeoutDeSocketTambemRecebeTresTentativas() {
        val aberturas = AtomicInteger()
        val esperas = ArrayList<Long>()
        val fonte = ParaleloDataSource(null, emptyMap(),
            novaConexao = { url ->
                object : ConexaoFalsa(url) {
                    override fun getResponseCode(): Int {
                        aberturas.incrementAndGet()
                        throw SocketTimeoutException("timeout")
                    }
                }
            },
            dormir = { esperas += it })

        try {
            fonte.conectarComRetry(URL("https://example.invalid/video.mkv"), 0, 1023)
        } catch (e: FalhaDownloadParalelo) {
            assertTrue(e.cause is SocketTimeoutException)
        }
        finally { fonte.close() }

        assertEquals(3, aberturas.get())
        assertEquals(listOf(250L, 750L), esperas)
    }

    @Test fun closeDesconectaAberturaEmVoo() {
        val entrou = CountDownLatch(1)
        val saiu = CountDownLatch(1)
        val conexao = object : ConexaoFalsa(URL("https://example.invalid/video.mkv")) {
            override fun getResponseCode(): Int {
                entrou.countDown()
                while (!desconectou.get()) Thread.sleep(5)
                throw IOException("desconectada")
            }
        }
        val fonte = ParaleloDataSource(null, emptyMap(), novaConexao = { conexao }, dormir = {})
        val terminouComErro = AtomicBoolean()
        val t = Thread {
            try { fonte.conectarComRetry(conexao.url, 0, 1023) }
            catch (_: IOException) { terminouComErro.set(true) }
            finally { saiu.countDown() }
        }
        t.start()
        assertTrue(entrou.await(2, TimeUnit.SECONDS))

        fonte.close()

        assertTrue(saiu.await(2, TimeUnit.SECONDS))
        assertTrue(conexao.desconectou.get())
        assertTrue(terminouComErro.get())
        // O finally libera `saiu` imediatamente antes de Thread.run retornar.
        t.join(200)
        assertFalse(t.isAlive)
    }
}
