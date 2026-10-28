package space.nuvio.nativelegacy.cast

import android.content.Context
import android.util.Log

object CastReceiverManager {
    private const val TAG = "NuvioCastManager"
    private const val PORT = 8765

    private var server: CastHttpServer? = null
    private var discovery: CastDiscoveryService? = null
    private var isStarted = false

    @Synchronized
    fun start(context: Context) {
        if (isStarted) return
        isStarted = true
        Log.i(TAG, "Starting NuvioCast Receiver on port $PORT...")
        try {
            val appCtx = context.applicationContext
            server = CastHttpServer(appCtx, PORT).apply { start() }
            discovery = CastDiscoveryService(appCtx, PORT).apply { start() }
            Log.i(TAG, "NuvioCast Receiver ready")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize CastReceiver: ${e.message}", e)
        }
    }

    @Synchronized
    fun stop() {
        if (!isStarted) return
        isStarted = false
        Log.i(TAG, "Stopping NuvioCast Receiver...")
        try {
            discovery?.stop()
            server?.stop()
        } catch (e: Exception) {
            Log.w(TAG, "Error stopping CastReceiver: ${e.message}")
        } finally {
            discovery = null
            server = null
        }
    }
}
