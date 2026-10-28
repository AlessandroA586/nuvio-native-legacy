package space.nuvio.nativelegacy.cast

data class CastPlayRequest(
    val title: String = "",
    val streamUrl: String = "",
    val videoId: String = "",
    val positionMs: Long = 0L,
    val seasonNumber: Int? = null,
    val episodeNumber: Int? = null,
    val headers: Map<String, String> = emptyMap(),
)

data class CastControlRequest(
    val command: String = "",
    val positionMs: Long = 0L,
)

data class CastDeviceStatus(
    val status: String = "ok",
    val deviceName: String,
    val deviceId: String,
    val state: String,
    val positionMs: Long,
    val durationMs: Long,
    val title: String,
    val videoId: String,
)
