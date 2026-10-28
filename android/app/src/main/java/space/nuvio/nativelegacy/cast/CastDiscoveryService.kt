package space.nuvio.nativelegacy.cast

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Build
import android.util.Log

class CastDiscoveryService(private val context: Context, private val port: Int) {
    companion object {
        private const val TAG = "NuvioCastDiscovery"
        private const val SERVICE_TYPE = "_nuviocast._tcp."
    }

    private var nsdManager: NsdManager? = null
    private var registrationListener: NsdManager.RegistrationListener? = null
    private var isRegistered = false

    fun start() {
        if (isRegistered) return
        try {
            nsdManager = context.getSystemService(Context.NSD_SERVICE) as? NsdManager
            val serviceInfo = NsdServiceInfo().apply {
                serviceName = "Nuvio TV (${Build.MODEL})"
                serviceType = SERVICE_TYPE
                setPort(port)
            }

            registrationListener = object : NsdManager.RegistrationListener {
                override fun onServiceRegistered(service: NsdServiceInfo) {
                    isRegistered = true
                    Log.i(TAG, "mDNS Service registered: ${service.serviceName} on port $port")
                }

                override fun onRegistrationFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
                    Log.e(TAG, "mDNS Registration failed: code $errorCode")
                    isRegistered = false
                }

                override fun onServiceUnregistered(arg0: NsdServiceInfo) {
                    isRegistered = false
                    Log.i(TAG, "mDNS Service unregistered")
                }

                override fun onUnregistrationFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
                    Log.e(TAG, "mDNS Unregistration failed: code $errorCode")
                }
            }

            nsdManager?.registerService(serviceInfo, NsdManager.PROTOCOL_DNS_SD, registrationListener)
        } catch (e: Exception) {
            Log.w(TAG, "Could not start mDNS service: ${e.message}")
        }
    }

    fun stop() {
        if (!isRegistered) return
        try {
            registrationListener?.let { nsdManager?.unregisterService(it) }
            isRegistered = false
        } catch (e: Exception) {
            Log.w(TAG, "Error unregistering mDNS: ${e.message}")
        }
    }
}
