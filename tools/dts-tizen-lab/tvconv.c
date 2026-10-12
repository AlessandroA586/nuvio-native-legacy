/* tvconv: host prototype of the proposed Tizen converter core.
 * Copies the video packets untouched, converts ONE DTS / TrueHD / MLP track to
 * AC3 (5.1 @ 640 kbps, or 2.0 @ 192 kbps), muxes to matroska / mpegts / mp4.
 * Audio is generated on a fixed global AC3 frame grid (n * 1536 samples from
 * t=0) with a pre-roll, so independently generated segments line up.
 *
 *   tvconv IN OUT [--fmt matroska|mpegts|mp4] [--audio N] [--start S] [--end E]
 *          [--no-downmix] [--index] [--preroll FRAMES] [--stats]
 * OUT may be "-" (stdout). --index prints the container keyframe index and exits.
 */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAME 1536
static double now(clockid_t c) { struct timespec t; clock_gettime(c, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static void die(const char *m, int r) { char b[128] = ""; if (r < 0) av_strerror(r, b, sizeof b); fprintf(stderr, "tvconv: %s %s\n", m, b); exit(1); }

/* Custom AVIO like the real engine (no FFmpeg protocols are built). */
static int rd(void *o, uint8_t *b, int n) { size_t k = fread(b, 1, n, (FILE *)o); return k ? (int)k : AVERROR_EOF; }
static int64_t sk(void *o, int64_t off, int wh) {
  FILE *f = o;
  if (wh == AVSEEK_SIZE) { long c = ftell(f); fseek(f, 0, SEEK_END); long e = ftell(f); fseek(f, c, SEEK_SET); return e; }
  return fseeko(f, off, wh & ~AVSEEK_FORCE) ? -1 : ftello(f);
}
static int wr(void *o, const uint8_t *b, int n) { return fwrite(b, 1, n, (FILE *)o) == (size_t)n ? n : AVERROR(EIO); }
/* DTS for MKV sources (MKV stores PTS only and no software video decoder is
 * built, so has_b_frames is unknown). dts[n] = min(pts[n..n+D]) - D*dur with
 * D >= the stream reorder depth: deterministic from any keyframe, monotonic,
 * dts <= pts, and identical whether a range is produced alone or in a run. */
#define LOOK 8
static AVPacket *vq[64]; static int vqn, vqout[64];
static int64_t vdur;
static void vq_emit(AVFormatContext *oc, AVStream *V, AVStream *ov, int force) {
  while (vqn > LOOK || (force && vqn > 0)) {
    AVPacket *p = vq[0]; int out = vqout[0];
    int64_t mn = p->pts; for (int i = 1; i <= LOOK && i < vqn; i++) if (vq[i]->pts < mn) mn = vq[i]->pts;
    static int64_t last = INT64_MIN;
    p->dts = mn - LOOK * vdur; if (last != INT64_MIN && p->dts <= last) p->dts = last + 1; last = p->dts;
    memmove(vq, vq + 1, (vqn - 1) * sizeof *vq); memmove(vqout, vqout + 1, (vqn - 1) * sizeof *vqout); vqn--;
    if (out) {
      av_packet_rescale_ts(p, V->time_base, ov->time_base); p->stream_index = 0;
      int r = av_interleaved_write_frame(oc, p); if (r < 0) die("write video", r);
    }
    av_packet_free(&p);
  }
}
int main(int argc, char **argv) {
  const char *in = NULL, *out = NULL, *fmt = "matroska";
  int dtsfull = 0, audio = -1, downmix = 1, index_only = 0, preroll = 2, stats = 0;
  double start = 0, end = INFINITY, tsoff = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--fmt")) fmt = argv[++i];
    else if (!strcmp(argv[i], "--audio")) audio = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--start")) start = atof(argv[++i]);
    else if (!strcmp(argv[i], "--end")) end = atof(argv[++i]);
    else if (!strcmp(argv[i], "--no-downmix")) downmix = 0;
    else if (!strcmp(argv[i], "--dts-full")) dtsfull = 1;
    else if (!strcmp(argv[i], "--index")) index_only = 1;
    else if (!strcmp(argv[i], "--preroll")) preroll = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--stats")) stats = 1;
    else if (!strcmp(argv[i], "--offset")) tsoff = atof(argv[++i]);
    else if (!in) in = argv[i]; else out = argv[i];
  }
  if (!in || (!out && !index_only)) { fprintf(stderr, "usage: tvconv IN OUT [opts]\n"); return 2; }
  av_log_set_level(AV_LOG_ERROR);
  AVFormatContext *ic = NULL; int r;
  FILE *inf = fopen(in, "rb"); if (!inf) die("cannot open input", 0);
  ic = avformat_alloc_context();
  ic->pb = avio_alloc_context(av_malloc(65536), 65536, 0, inf, rd, NULL, sk);
  if ((r = avformat_open_input(&ic, NULL, NULL, NULL)) < 0) die("open", r);
  if ((r = avformat_find_stream_info(ic, NULL)) < 0) die("info", r);
  int vs = -1;
  for (unsigned i = 0; i < ic->nb_streams; i++) {
    AVCodecParameters *p = ic->streams[i]->codecpar;
    if (p->codec_type == AVMEDIA_TYPE_VIDEO && vs < 0) vs = i;
    if (audio < 0 && (p->codec_id == AV_CODEC_ID_DTS || p->codec_id == AV_CODEC_ID_TRUEHD || p->codec_id == AV_CODEC_ID_MLP)) audio = i;
  }
  if (vs < 0 || audio < 0) die("need a video stream and a DTS/TrueHD/MLP stream", 0);
  AVStream *V = ic->streams[vs], *A = ic->streams[audio];
  if (index_only) {
    /* matroska defers Cues parsing until the first seek: force it */
    avformat_seek_file(ic, -1, INT64_MIN, 0, 0, AVSEEK_FLAG_BACKWARD);
    int n = avformat_index_get_entries_count(V);
    printf("index entries=%d duration=%.3f\n", n, ic->duration / (double)AV_TIME_BASE);
    for (int i = 0; i < n; i++) {
      const AVIndexEntry *e = avformat_index_get_entry(V, i);
      if (e->flags & AVINDEX_KEYFRAME) printf("kf %.6f pos=%lld\n", e->timestamp * av_q2d(V->time_base), (long long)e->pos);
    }
    return 0;
  }
  /* Decoder: request the encoded 5.1 presentation when the stream is wider. */
  const AVCodec *dc = avcodec_find_decoder(A->codecpar->codec_id);
  if (!dc) die("no decoder in this build", 0);
  AVCodecContext *dec = avcodec_alloc_context3(dc);
  avcodec_parameters_to_context(dec, A->codecpar);
  dec->pkt_timebase = A->time_base;
  int in_ch = A->codecpar->ch_layout.nb_channels;
  int out_ch = (in_ch == 0 || in_ch > 2) ? 6 : 2;
  AVChannelLayout target; av_channel_layout_default(&target, out_ch); /* 5.1 = 5.1(side) */
  /* DTS: the core is the backward-compatible 5.1 (or 2.0) mix, and AC3 640k
   * cannot carry more than it: decode the core only (cheapest, no XLL).
   * TrueHD/MLP: ask for the encoded 5.1 presentation (substream) when wider. */
  const char *mode = "full decode";
  if (A->codecpar->codec_id == AV_CODEC_ID_DTS && !dtsfull) { av_opt_set_int(dec->priv_data, "core_only", 1, 0); mode = "DTS core only"; }
  else if (downmix && out_ch == 6 && in_ch > 6) { av_opt_set_chlayout(dec->priv_data, "downmix", &target, 0); mode = "decoder 5.1 presentation"; }
  if (stats) fprintf(stderr, "source %s %d ch -> AC3 %d ch, %s\n", avcodec_get_name(A->codecpar->codec_id), in_ch, out_ch, mode);
  if ((r = avcodec_open2(dec, dc, NULL)) < 0) die("decoder open", r);
  const AVCodec *ec = avcodec_find_encoder(AV_CODEC_ID_AC3);
  if (!ec) die("no ac3 encoder", 0);
  AVCodecContext *enc = avcodec_alloc_context3(ec);
  av_channel_layout_copy(&enc->ch_layout, &target);
  enc->sample_rate = 48000; enc->time_base = (AVRational){1, 48000};
  enc->sample_fmt = AV_SAMPLE_FMT_FLTP; enc->bit_rate = out_ch > 2 ? 640000 : 192000;
  if ((r = avcodec_open2(enc, ec, NULL)) < 0) die("encoder open", r);
  /* Output */
  AVFormatContext *oc = NULL;
  avformat_alloc_output_context2(&oc, NULL, fmt, NULL);
  if (!oc) die("muxer", 0);
  AVStream *ov = avformat_new_stream(oc, NULL), *oa = avformat_new_stream(oc, NULL);
  avcodec_parameters_copy(ov->codecpar, V->codecpar); ov->codecpar->codec_tag = 0;
  ov->time_base = V->time_base;
  for (int i = 0; i < V->codecpar->nb_coded_side_data; i++) { /* mastering display / CLL / DOVI conf */
    const AVPacketSideData *sd = &V->codecpar->coded_side_data[i];
    av_packet_side_data_add(&ov->codecpar->coded_side_data, &ov->codecpar->nb_coded_side_data, sd->type, av_memdup(sd->data, sd->size), sd->size, 0);
  }
  avcodec_parameters_from_context(oa->codecpar, enc); oa->time_base = enc->time_base;
  av_dict_copy(&oa->metadata, A->metadata, 0);
  AVDictionary *mo = NULL;
  int seekable_out = strcmp(out, "-") != 0;   /* a file: Cues/duration/moov can be written at the end */
  if (!strcmp(fmt, "mp4") && !seekable_out) av_dict_set(&mo, "movflags", "frag_keyframe+empty_moov+default_base_moof+delay_moov", 0);
  if (!strcmp(fmt, "matroska")) av_dict_set(&mo, "live", "0", 0);
  FILE *outf = strcmp(out, "-") ? fopen(out, "wb") : stdout; if (!outf) die("cannot open output", 0);
  oc->pb = avio_alloc_context(av_malloc(65536), 65536, 1, outf, NULL, wr, seekable_out ? sk : NULL);
  /* One fixed timeline for every run: never let the muxer shift a run on its own. */
  oc->avoid_negative_ts = AVFMT_AVOID_NEG_TS_DISABLED;
  oc->output_ts_offset = (int64_t)(tsoff * AV_TIME_BASE);
  if ((r = avformat_write_header(oc, &mo)) < 0) die("write header", r);
  /* Seek: keyframe <= start-preroll for audio warm-up; video output starts at
   * the keyframe <= start (K). */
  int64_t K = AV_NOPTS_VALUE; /* video time_base */
  double warm = preroll * FRAME / 48000.0 + 0.25;
  AVPacket *pk = av_packet_alloc(), *op = av_packet_alloc();
  int64_t Kfix = AV_NOPTS_VALUE;
  if (start > 0) {
    /* 1) find K = video keyframe <= start; 2) re-seek earlier for audio warm-up */
    int64_t ts = (int64_t)(start * AV_TIME_BASE);
    if ((r = avformat_seek_file(ic, -1, INT64_MIN, ts, ts, AVSEEK_FLAG_BACKWARD)) < 0) die("seek", r);
    while (av_read_frame(ic, pk) >= 0) {
      int key = pk->stream_index == vs && (pk->flags & AV_PKT_FLAG_KEY);
      if (key) Kfix = pk->pts;
      av_packet_unref(pk); if (key) break;
    }
    int64_t ts2 = (int64_t)((Kfix * av_q2d(V->time_base) - warm) * AV_TIME_BASE);
    if (ts2 < 0) ts2 = 0;
    if ((r = avformat_seek_file(ic, -1, INT64_MIN, ts2, ts2, AVSEEK_FLAG_BACKWARD)) < 0) die("seek2", r);
  }
  AVFrame *fr = av_frame_alloc(), *ef = av_frame_alloc();
  SwrContext *swr = NULL; AVAudioFifo *fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, out_ch, 4096);
  int64_t grid = INT64_MIN;          /* sample position (48 kHz) of the next fifo sample */
  int64_t outStart48 = INT64_MIN;    /* first audio sample pts allowed out (= K) */
  int64_t end48 = isinf(end) ? INT64_MAX : (int64_t)llround(end * 48000);
  int64_t vEnd = isinf(end) ? INT64_MAX : (int64_t)llround(end / av_q2d(V->time_base));
  double tA = 0, c0 = now(CLOCK_PROCESS_CPUTIME_ID), w0 = now(CLOCK_MONOTONIC);
  long vOut = 0, aOut = 0, decodedSamples = 0; int vDone = 0, aDone = 0, eof = 0;
  int64_t firstV = AV_NOPTS_VALUE, lastV = 0, firstA = AV_NOPTS_VALUE, lastA = 0;
  /* Audio-before-K is decoded for warm-up only. We learn K from the first
   * video keyframe at or after the seek point. Audio packets that arrive
   * before K is known are decoded and kept in a small pending fifo. */
  if (Kfix != AV_NOPTS_VALUE) {
    K = Kfix; outStart48 = (int64_t)llround(K * av_q2d(V->time_base) * 48000);
    int64_t g = (outStart48 / FRAME) * FRAME - (int64_t)preroll * FRAME; grid = g < 0 ? 0 : g;
  }
  while (!(vDone && aDone)) {
    if (!eof) {
      r = av_read_frame(ic, pk);
      if (r == AVERROR_EOF) { eof = 1; avcodec_send_packet(dec, NULL); vDone = 1; }
      else if (r < 0) die("read", r);
    }
    if (!eof && pk->stream_index == vs) {
      static int vStarted;
      if (!vStarted) {
        int key = pk->flags & AV_PKT_FLAG_KEY;
        if (!key || (Kfix != AV_NOPTS_VALUE && pk->pts != Kfix)) { av_packet_unref(pk); continue; }
        vStarted = 1;
      }
      if (K == AV_NOPTS_VALUE) {
        K = pk->pts;
        double Ks = K * av_q2d(V->time_base);
        outStart48 = (int64_t)llround(Ks * 48000);
        /* start of the fifo: grid frame containing K minus preroll frames */
        int64_t g = (outStart48 / FRAME) * FRAME - (int64_t)preroll * FRAME;
        grid = g < 0 ? 0 : g;
        (void)warm;
      }
      static int lookLeft = -1;
      if (lookLeft < 0 && (pk->flags & AV_PKT_FLAG_KEY) && pk->pts >= vEnd) lookLeft = LOOK;
      if (lookLeft >= 0) {  /* past the boundary: lookahead only, never output */
        if (lookLeft-- == 0) { vDone = 1; av_packet_unref(pk); continue; }
        vq[vqn] = av_packet_clone(pk); vqout[vqn++] = 0; av_packet_unref(pk); vq_emit(oc, V, ov, 0); continue;
      }
      if (pk->duration <= 0 && V->avg_frame_rate.num) pk->duration = av_rescale_q(1, av_inv_q(V->avg_frame_rate), V->time_base);
      if (vDone) { av_packet_unref(pk); continue; }
      if (firstV == AV_NOPTS_VALUE) firstV = pk->pts;
      lastV = pk->pts > lastV ? pk->pts : lastV;
      if (!vdur) vdur = pk->duration > 0 ? pk->duration : 1;
      vq[vqn] = av_packet_clone(pk); vqout[vqn++] = 1; av_packet_unref(pk); vq_emit(oc, V, ov, 0);
      vOut++;
      continue;
    }
    if (!eof && pk->stream_index != audio) { av_packet_unref(pk); continue; }
    if (!eof) {
      double t1 = now(CLOCK_PROCESS_CPUTIME_ID);
      if ((r = avcodec_send_packet(dec, pk)) < 0 && r != AVERROR_INVALIDDATA) die("decode send", r);
      av_packet_unref(pk);
      tA += now(CLOCK_PROCESS_CPUTIME_ID) - t1;
    }
    double t1 = now(CLOCK_PROCESS_CPUTIME_ID);
    for (;;) {
      r = avcodec_receive_frame(dec, fr);
      if (r == AVERROR(EAGAIN)) break;
      if (r == AVERROR_EOF) { /* flush fifo + encoder */
        int left = av_audio_fifo_size(fifo);
        if (left > 0) {
          ef->format = AV_SAMPLE_FMT_FLTP; ef->sample_rate = 48000; av_channel_layout_copy(&ef->ch_layout, &target);
          ef->nb_samples = FRAME; av_frame_get_buffer(ef, 0);
          av_samples_set_silence(ef->extended_data, 0, FRAME, out_ch, AV_SAMPLE_FMT_FLTP);
          av_audio_fifo_read(fifo, (void **)ef->extended_data, left);
          ef->pts = grid; grid += FRAME; avcodec_send_frame(enc, ef); av_frame_unref(ef);
        }
        avcodec_send_frame(enc, NULL);
        break;
      }
      if (r < 0) { av_frame_unref(fr); continue; } /* corrupt unit while syncing */
      decodedSamples += fr->nb_samples;
      if (grid == INT64_MIN || fr->best_effort_timestamp == AV_NOPTS_VALUE) { av_frame_unref(fr); continue; } /* K not known yet */
      /* The first frames after open/seek can carry the timestamp of a packet the
       * decoder skipped while hunting for a sync (TrueHD: 13 ms early). They are
       * warm-up anyway: never anchor on them. */
      { static int warmFrames; if (warmFrames++ < 2) { av_frame_unref(fr); continue; } }
      if (fr->sample_rate != 48000) die("non-48k source not handled by prototype", 0);
      if (!swr) {
        swr_alloc_set_opts2(&swr, &target, AV_SAMPLE_FMT_FLTP, 48000, &fr->ch_layout, fr->format, fr->sample_rate, 0, NULL);
        if ((r = swr_init(swr)) < 0) die("swr", r);
        if (stats) { char d[64]; av_channel_layout_describe(&fr->ch_layout, d, sizeof d); fprintf(stderr, "decoder delivers %s (%d ch)\n", d, fr->ch_layout.nb_channels); }
      }
      { static int warned; static AVChannelLayout first; static int ff=-1;
        if (ff < 0) { av_channel_layout_copy(&first, &fr->ch_layout); ff = fr->format; }
        else if ((av_channel_layout_compare(&first, &fr->ch_layout) || ff != fr->format) && !warned++) {
          char a[64], b[64]; av_channel_layout_describe(&first, a, 64); av_channel_layout_describe(&fr->ch_layout, b, 64);
          fprintf(stderr, "LAYOUT/FORMAT CHANGED midstream: %s/%s -> %s/%s\n", a, av_get_sample_fmt_name(ff), b, av_get_sample_fmt_name(fr->format)); } }
      /* Sample-exact anchor. Container ticks are coarse (MKV: 1 ms = 48
       * samples) and the first frames after a seek can be stale, so each frame
       * only CONSTRAINS the offset: true_start - samples_counted in
       * [pts - tick/2, pts + tick/2]. The intersection over the warm-up frames
       * is snapped to the codec frame grid, then locked: from there on the
       * position is offset + count, and a timestamp only matters if it jumps. */
      static int64_t lo = INT64_MIN, hi = INT64_MAX, cum = 0, off = INT64_MIN;
      int64_t half = av_rescale_q(1, A->time_base, (AVRational){1, 48000}) / 2 + 1;
      int64_t praw = av_rescale_q(fr->best_effort_timestamp, A->time_base, (AVRational){1, 48000});
      if (off == INT64_MIN) {
        int64_t l = praw - half - cum, h = praw + half - cum;
        if (l > hi || h < lo) { lo = l; hi = h; }            /* inconsistent: restart window */
        else { if (l > lo) lo = l; if (h < hi) hi = h; }
      } else if (llabs(praw - (off + cum)) > 4800) {
        fprintf(stderr, "audio timestamp jump %+lld samples: resync\n", (long long)(praw - (off + cum)));
        off = INT64_MIN; lo = praw - half; hi = praw + half; cum = 0;
      }
      int64_t p;
      if (off == INT64_MIN && praw + fr->nb_samples < grid + av_audio_fifo_size(fifo)) {
        p = praw;                                             /* still before the window: no lock yet */
      } else {
        if (off == INT64_MIN) {
          int64_t fs = fr->nb_samples, mid = lo + (hi - lo) / 2, snap = (mid + fs / 2) / fs * fs;
          off = (snap >= lo && snap <= hi) ? snap : mid;
          if (stats) fprintf(stderr, "anchor locked: window [%lld,%lld] (%lld samples) -> %lld%s\n", (long long)lo, (long long)hi,
                             (long long)(hi - lo), (long long)off, off == snap ? " (on frame grid)" : "");
        }
        p = off + cum;
      }
      cum += fr->nb_samples;

      uint8_t **buf = NULL; av_samples_alloc_array_and_samples(&buf, NULL, out_ch, fr->nb_samples + 64, AV_SAMPLE_FMT_FLTP, 0);
      int n = swr_convert(swr, buf, fr->nb_samples + 64, (const uint8_t **)fr->extended_data, fr->nb_samples);
      int64_t fifoEnd = grid + av_audio_fifo_size(fifo);
      int skip = 0;
      if (p + n <= fifoEnd) skip = n;                 /* entirely before the window */
      else if (p < fifoEnd) skip = (int)(fifoEnd - p); /* trim the head */
      else if (p > fifoEnd) {                          /* gap: pad silence */
        int gap = (int)(p - fifoEnd);
        if (gap > 48000) die("audio gap > 1 s", 0);
        uint8_t **z = NULL; av_samples_alloc_array_and_samples(&z, NULL, out_ch, gap, AV_SAMPLE_FMT_FLTP, 0);
        av_samples_set_silence(z, 0, gap, out_ch, AV_SAMPLE_FMT_FLTP);
        av_audio_fifo_write(fifo, (void **)z, gap); av_freep(&z[0]); av_freep(&z);
      }
      if (n > skip) {
        uint8_t *planes[8]; for (int c = 0; c < out_ch; c++) planes[c] = buf[c] + skip * sizeof(float);
        av_audio_fifo_write(fifo, (void **)planes, n - skip);
      }
      av_freep(&buf[0]); av_freep(&buf); av_frame_unref(fr);
      while (av_audio_fifo_size(fifo) >= FRAME) {
        ef->format = AV_SAMPLE_FMT_FLTP; ef->sample_rate = 48000; av_channel_layout_copy(&ef->ch_layout, &target);
        ef->nb_samples = FRAME; av_frame_get_buffer(ef, 0);
        av_audio_fifo_read(fifo, (void **)ef->extended_data, FRAME);
        ef->pts = grid; grid += FRAME;
        if ((r = avcodec_send_frame(enc, ef)) < 0) die("encode send", r);
        av_frame_unref(ef);
      }
    }
    for (;;) {
      r = avcodec_receive_packet(enc, op);
      if (r == AVERROR(EAGAIN)) break;
      if (r == AVERROR_EOF) { aDone = 1; break; }
      if (r < 0) die("encode", r);
      if (op->pts < outStart48) { av_packet_unref(op); continue; }          /* pre-roll */
      if (op->pts >= end48) { aDone = 1; av_packet_unref(op); continue; }    /* next segment */
      if (firstA == AV_NOPTS_VALUE) firstA = op->pts;
      lastA = op->pts;
      av_packet_rescale_ts(op, enc->time_base, oa->time_base); op->stream_index = 1;
      if ((r = av_interleaved_write_frame(oc, op)) < 0) die("write audio", r);
      aOut++;
    }
    tA += now(CLOCK_PROCESS_CPUTIME_ID) - t1;
    if (aDone && !vDone && eof) vDone = 1;
  }
  vq_emit(oc, V, ov, 1);
  av_write_trailer(oc);
  avio_flush(oc->pb); fflush(outf);
  if (stats) {
    double cpu = now(CLOCK_PROCESS_CPUTIME_ID) - c0, wall = now(CLOCK_MONOTONIC) - w0;
    double media = (lastV - firstV) * av_q2d(V->time_base) + 1.0 / 24;
    fprintf(stderr, "video pkts=%ld first=%.3f last=%.3f | audio pkts=%ld first=%.4f last=%.4f | "
            "media=%.2fs wall=%.2fs cpu=%.2fs audio-cpu=%.2fs (%.1f%% of realtime, one core) decoded=%ld\n",
            vOut, firstV * av_q2d(V->time_base), lastV * av_q2d(V->time_base), aOut, firstA / 48000.0, lastA / 48000.0,
            media, wall, cpu, tA, 100.0 * tA / media, decodedSamples);
  }
  return 0;
}
