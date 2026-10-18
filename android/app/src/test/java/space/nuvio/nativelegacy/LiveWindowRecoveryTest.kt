package space.nuvio.nativelegacy

import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import org.junit.Assert.*
import org.junit.Test
import java.lang.reflect.Proxy

class LiveWindowRecoveryTest {
    private class Fixture {
        val calls = ArrayList<String>()
        val status = ArrayList<Boolean>()
        val scheduled = LinkedHashMap<Runnable, Long>()
        var active = true
        var failed = 0
        var throwOnPrepare = false
        var afterPrepare: (() -> Unit)? = null
        // A strict real Player interface boundary: any URL replacement, release,
        // play/pause, volume or track-selection change fails the test immediately.
        val player = Proxy.newProxyInstance(Player::class.java.classLoader,
            arrayOf(Player::class.java)) { _, method, _ ->
            when (method.name) {
                "seekToDefaultPosition" -> calls.add("live position")
                "prepare" -> {
                    calls.add("prepare")
                    if (throwOnPrepare) throw IllegalStateException("stopped player")
                    afterPrepare?.invoke()
                }
                else -> error("Unexpected Player operation: ${method.name}")
            }
            null
        } as Player
        val recovery = LiveWindowRecovery(
            schedule = { task, delay -> scheduled[task] = delay },
            unschedule = { task -> scheduled.remove(task) },
            active = { active },
            status = { status.add(it) },
        )
        fun recover(now: Long = 0, code: Int = PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW) =
            recovery.recover(player, code, now) { failed++ }
        fun timeout(): Runnable = scheduled.keys.single()
    }

    @Test fun expiredLiveWindowSeeksAndPreparesSamePlayerWithoutReplacingSource() {
        val f = Fixture()
        assertTrue(f.recover())
        assertEquals(listOf("live position", "prepare"), f.calls)
        assertEquals(0, f.failed)
        assertTrue(f.recovery.ready())
        assertTrue(f.scheduled.isEmpty())
        assertFalse(f.recovery.ready())
        assertEquals(listOf(true, false), f.status)
    }

    @Test fun ordinaryNetworkAuthenticationAndDecoderErrorsKeepNormalFallback() {
        val f = Fixture()
        for (code in listOf(2001, 2002, 2004, 4001, 4003)) assertFalse(f.recover(code = code))
        assertTrue(f.calls.isEmpty())
        assertTrue(f.status.isEmpty())
        assertTrue(f.scheduled.isEmpty())
    }

    @Test fun stalledRecoveryFallsBackOnceWithinNativeWatchdogDeadline() {
        val f = Fixture()
        assertTrue(f.recover())
        assertEquals(8_000L, f.scheduled.values.single())
        val task = f.timeout()
        task.run()
        task.run()
        assertEquals(1, f.failed)
        assertEquals(listOf(true, false), f.status)
    }

    @Test fun readyInvalidatesAlreadyQueuedTimeout() {
        val f = Fixture()
        assertTrue(f.recover())
        val old = f.timeout()
        assertTrue(f.recovery.ready())
        old.run()
        assertEquals(0, f.failed)
    }

    @Test fun stoppedOrReplacedSessionCannotRecoverOrReportFailure() {
        val f = Fixture()
        assertTrue(f.recover())
        val task = f.timeout()
        f.active = false
        f.recovery.cancel()
        task.run()
        assertFalse(f.recover())
        assertEquals(0, f.failed)
        assertTrue(f.scheduled.isEmpty())
        assertEquals(listOf("live position", "prepare"), f.calls)
    }

    @Test fun sessionInvalidationAloneAlsoGuardsLateTimeout() {
        val f = Fixture()
        assertTrue(f.recover())
        f.active = false
        f.timeout().run()
        assertEquals(0, f.failed)
    }

    @Test fun repeatedErrorsBeforeReadyUseNormalFallbackInsteadOfLooping() {
        val f = Fixture()
        assertTrue(f.recover())
        assertFalse(f.recover(1))
        assertEquals(listOf("live position", "prepare"), f.calls)
    }

    @Test fun rapidSuccessAndFailureCyclesAreBoundedButLaterOutageCanRecover() {
        val f = Fixture()
        assertTrue(f.recover(0)); f.recovery.ready()
        assertTrue(f.recover(10_000)); f.recovery.ready()
        assertFalse(f.recover(59_999))
        assertEquals(4, f.calls.size)
        assertTrue(f.recover(60_000))
        assertEquals(6, f.calls.size)
    }

    @Test fun oldTimeoutCannotAbortNewRecovery() {
        val f = Fixture()
        assertTrue(f.recover())
        val old = f.timeout()
        f.recovery.ready()
        assertTrue(f.recover(1))
        old.run()
        assertEquals(0, f.failed)
        assertTrue(f.recovery.ready())
    }

    @Test fun synchronousPlayerFailureLeavesNoHiddenRetryPending() {
        val f = Fixture()
        f.throwOnPrepare = true
        assertFalse(f.recover())
        assertTrue(f.scheduled.isEmpty())
        assertEquals(0, f.failed)
        assertFalse(f.recovery.ready())
    }

    @Test fun synchronousReadyDoesNotLeaveWatchdogArmed() {
        val f = Fixture()
        f.afterPrepare = { f.recovery.ready() }
        assertTrue(f.recover())
        assertTrue(f.scheduled.isEmpty())
        assertEquals(0, f.failed)
    }
}
