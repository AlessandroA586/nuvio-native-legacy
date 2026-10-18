package space.nuvio.nativelegacy

import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.common.util.UnstableApi

// Recover the existing live player before asking native code to replace its URL.
// The source's default live position respects its advertised live window/holdback.
@androidx.annotation.OptIn(UnstableApi::class)
internal class LiveWindowRecovery(
    private val schedule: (Runnable, Long) -> Unit,
    private val unschedule: (Runnable) -> Unit,
    private val active: () -> Boolean,
    private val status: (Boolean) -> Unit = {},
) {
    private var pending: Runnable? = null
    private var windowStart = -1L
    private var attempts = 0

    fun recover(player: Player, errorCode: Int, nowMs: Long, failed: () -> Unit): Boolean {
        if (errorCode != PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW || !active()) return false
        // A second error before READY is a failed recovery, not another hidden retry.
        if (pending != null) return false
        if (windowStart < 0 || nowMs - windowStart >= 60_000L) {
            windowStart = nowMs
            attempts = 0
        }
        if (attempts >= 2) return false
        attempts++
        val deadline = object : Runnable {
            override fun run() {
                if (pending !== this) return
                pending = null
                if (active()) { status(false); failed() }
            }
        }
        pending = deadline
        try {
            status(true)
            // No setMediaItem, stop, release, play or URL lookup: retain the
            // session, surface, track choices, volume and requested pause state.
            player.seekToDefaultPosition()
            player.prepare()
            if (pending === deadline) schedule(deadline, 8_000L)
        } catch (_: Exception) {
            cancel()
            return false
        }
        return true
    }

    fun ready(): Boolean {
        val recovering = pending != null
        cancel()
        return recovering
    }

    fun cancel() {
        val task = pending ?: return
        pending = null
        unschedule(task)
        status(false)
    }
}
