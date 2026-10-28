package space.nuvio.nativelegacy.cast

import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.util.Log
import org.json.JSONObject
import androidx.media3.exoplayer.ExoPlayer
import space.nuvio.nativelegacy.NuvioActivity
import space.nuvio.nativelegacy.NvPlayer
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.Executors

class CastHttpServer(
    private val context: Context,
    private val port: Int = 8765
) {
    companion object {
        private const val TAG = "NuvioCastHttpServer"
    }

    private var serverSocket: ServerSocket? = null
    private val threadPool = Executors.newCachedThreadPool()
    private val mainHandler = Handler(Looper.getMainLooper())
    @Volatile private var isRunning = false

    @Volatile var currentTitle: String = ""
    @Volatile var currentVideoId: String = ""

    fun start() {
        if (isRunning) return
        isRunning = true
        threadPool.execute {
            try {
                serverSocket = ServerSocket(port)
                Log.i(TAG, "Cast HTTP Server listening on port $port")
                while (isRunning) {
                    val socket = serverSocket?.accept() ?: break
                    threadPool.execute { handleClient(socket) }
                }
            } catch (e: Exception) {
                if (isRunning) {
                    Log.e(TAG, "Server error: ${e.message}", e)
                }
            }
        }
    }

    fun stop() {
        isRunning = false
        try {
            serverSocket?.close()
        } catch (_: Exception) {}
        serverSocket = null
    }

    private fun handleClient(socket: Socket) {
        try {
            socket.soTimeout = 8000
            val reader = BufferedReader(InputStreamReader(socket.getInputStream(), Charsets.UTF_8))
            val output = socket.getOutputStream()

            val requestLine = reader.readLine() ?: return socket.close()
            val parts = requestLine.split(" ")
            if (parts.size < 2) return socket.close()

            val method = parts[0].uppercase()
            val path = parts[1]

            val headers = mutableMapOf<String, String>()
            var contentLength = 0
            while (true) {
                val line = reader.readLine() ?: break
                if (line.isEmpty()) break
                val colon = line.indexOf(':')
                if (colon > 0) {
                    val key = line.substring(0, colon).trim().lowercase()
                    val value = line.substring(colon + 1).trim()
                    headers[key] = value
                    if (key == "content-length") {
                        contentLength = value.toIntOrNull() ?: 0
                    }
                }
            }

            if (method == "OPTIONS") {
                sendResponse(output, 204, "No Content", "text/plain", "")
                return socket.close()
            }

            var body = ""
            if (contentLength > 0) {
                val charBuffer = CharArray(contentLength)
                var readTotal = 0
                while (readTotal < contentLength) {
                    val read = reader.read(charBuffer, readTotal, contentLength - readTotal)
                    if (read < 0) break
                    readTotal += read
                }
                body = String(charBuffer, 0, readTotal)
            }

            when {
                path == "/status" || path == "/info" || path == "/" -> {
                    handleStatus(output)
                }
                path == "/play" && method == "POST" -> {
                    handlePlay(body, output)
                }
                path == "/control" && method == "POST" -> {
                    handleControl(body, output)
                }
                else -> {
                    sendResponse(output, 404, "Not Found", "application/json", "{\"error\":\"Not Found\"}")
                }
            }
        } catch (e: Exception) {
            Log.w(TAG, "Request handling failed: ${e.message}")
        } finally {
            try { socket.close() } catch (_: Exception) {}
        }
    }

    private fun getExoPlayer(): ExoPlayer? {
        return try {
            val field = NvPlayer::class.java.getDeclaredField("player")
            field.isAccessible = true
            field.get(null) as? ExoPlayer
        } catch (_: Exception) {
            null
        }
    }

    private fun handleStatus(output: OutputStream) {
        val deviceId = try {
            Settings.Secure.getString(context.contentResolver, Settings.Secure.ANDROID_ID) ?: Build.SERIAL
        } catch (_: Exception) {
            "unknown-tv"
        }
        val deviceName = "Nuvio TV (${Build.MODEL})"

        val p = getExoPlayer()
        val isPlaying = p?.isPlaying ?: false
        val playbackState = p?.playbackState ?: 1 // 1: IDLE, 2: BUFFERING, 3: READY, 4: ENDED
        val state = when {
            playbackState == 2 -> "BUFFERING"
            isPlaying -> "PLAYING"
            playbackState == 3 -> "PAUSED"
            playbackState == 4 -> "ENDED"
            else -> "IDLE"
        }
        val posMs = p?.currentPosition ?: 0L
        val durMs = p?.duration?.takeIf { it > 0 } ?: 0L

        val json = JSONObject().apply {
            put("status", "ok")
            put("device_name", deviceName)
            put("device_id", deviceId)
            put("state", state)
            put("position_ms", posMs)
            put("duration_ms", durMs)
            put("title", currentTitle)
            put("video_id", currentVideoId)
        }.toString()

        sendResponse(output, 200, "OK", "application/json", json)
    }

    private fun handlePlay(body: String, output: OutputStream) {
        try {
            val json = JSONObject(body)
            val streamUrl = json.optString("stream_url", "")
            val title = json.optString("title", "")
            val videoId = json.optString("video_id", "")
            val positionMs = json.optLong("position_ms", 0L)
            val headersObj = json.optJSONObject("headers")

            if (streamUrl.isBlank()) {
                sendResponse(output, 400, "Bad Request", "application/json", "{\"error\":\"Missing stream_url\"}")
                return
            }

            currentTitle = title
            currentVideoId = videoId

            val headerBuilder = StringBuilder()
            if (headersObj != null) {
                val keys = headersObj.keys()
                while (keys.hasNext()) {
                    val k = keys.next()
                    val v = headersObj.optString(k, "")
                    if (k.isNotBlank() && v.isNotBlank()) {
                        headerBuilder.append("$k: $v\n")
                    }
                }
            }
            val formattedHeaders = headerBuilder.toString()

            mainHandler.post {
                bringActivityToFront()
                NvPlayer.abrirPosicao(streamUrl, formattedHeaders, positionMs.toInt(), 0)
            }

            sendResponse(output, 200, "OK", "application/json", "{\"status\":\"ok\"}")
        } catch (e: Exception) {
            Log.e(TAG, "Error in /play: ${e.message}", e)
            sendResponse(output, 500, "Internal Server Error", "application/json", "{\"error\":\"${e.message}\"}")
        }
    }

    private fun handleControl(body: String, output: OutputStream) {
        try {
            val json = JSONObject(body)
            val command = json.optString("command", "").uppercase()
            val positionMs = json.optLong("position_ms", 0L)

            mainHandler.post {
                when (command) {
                    "PAUSE" -> NvPlayer.pausar(1)
                    "RESUME" -> NvPlayer.pausar(0)
                    "SEEK" -> NvPlayer.buscar(positionMs.toInt())
                    "STOP" -> NvPlayer.parar()
                }
            }

            sendResponse(output, 200, "OK", "application/json", "{\"status\":\"ok\"}")
        } catch (e: Exception) {
            Log.e(TAG, "Error in /control: ${e.message}", e)
            sendResponse(output, 500, "Internal Server Error", "application/json", "{\"error\":\"${e.message}\"}")
        }
    }

    private fun bringActivityToFront() {
        try {
            val intent = Intent(context, NuvioActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_SINGLE_TOP)
            }
            context.startActivity(intent)
        } catch (e: Exception) {
            Log.w(TAG, "Could not bring activity to front: ${e.message}")
        }
    }

    private fun sendResponse(
        output: OutputStream,
        statusCode: Int,
        statusText: String,
        contentType: String,
        body: String
    ) {
        val bytes = body.toByteArray(Charsets.UTF_8)
        val headers = "HTTP/1.1 $statusCode $statusText\r\n" +
                "Content-Type: $contentType; charset=utf-8\r\n" +
                "Content-Length: ${bytes.size}\r\n" +
                "Connection: close\r\n" +
                "Access-Control-Allow-Origin: *\r\n" +
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n" +
                "Access-Control-Allow-Headers: Content-Type, Authorization\r\n\r\n"
        output.write(headers.toByteArray(Charsets.US_ASCII))
        if (bytes.isNotEmpty()) {
            output.write(bytes)
        }
        output.flush()
    }
}
