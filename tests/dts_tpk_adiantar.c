// Fixture helper for tests/dts_tpk_servidor.sh, built against the SYSTEM FFmpeg
// (the one behind the ffmpeg CLI, via pkg-config): the .tpk FFmpeg has no
// Matroska muxer and no file protocol.
//
//   dts_tpk_adiantar <in.mkv> <out.mkv> <lead seconds>
//
// Rewrites `in` so each audio packet sits in the file next to the video of
// `lead` seconds EARLIER: the audio runs that far ahead of its video (a badly
// interleaved file). Timestamps are untouched; only the order changes.
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { AVPacket *p; double k; } Ord;
static int cmpOrd(const void *a, const void *b) {
  double x = ((const Ord *)a)->k, y = ((const Ord *)b)->k;
  return x < y ? -1 : x > y;
}

int main(int argc, char **argv) {
  AVFormatContext *ic = NULL, *oc = NULL;
  Ord *v = NULL;
  int n = 0, cap = 0, rr = 1;
  double lead;
  if (argc != 4) { fprintf(stderr, "usage: %s <in> <out> <lead s>\n", argv[0]); return 2; }
  lead = atof(argv[3]);
  av_log_set_level(AV_LOG_ERROR);
  if (avformat_open_input(&ic, argv[1], NULL, NULL) < 0 || avformat_find_stream_info(ic, NULL) < 0) return 1;
  if (avformat_alloc_output_context2(&oc, NULL, "matroska", argv[2]) < 0 || !oc) return 1;
  for (unsigned i = 0; i < ic->nb_streams; i++) {
    AVStream *o = avformat_new_stream(oc, NULL);
    if (!o) return 1;
    avcodec_parameters_copy(o->codecpar, ic->streams[i]->codecpar);
    o->codecpar->codec_tag = 0;
    o->time_base = ic->streams[i]->time_base;
  }
  for (;;) {
    AVPacket *p = av_packet_alloc();
    if (!p || av_read_frame(ic, p) < 0) { av_packet_free(&p); break; }
    if (n == cap) { cap = cap ? cap * 2 : 4096; v = realloc(v, (size_t)cap * sizeof *v); if (!v) return 1; }
    int64_t ts = p->dts != AV_NOPTS_VALUE ? p->dts : p->pts;
    double t = ts != AV_NOPTS_VALUE ? ts * av_q2d(ic->streams[p->stream_index]->time_base) : 0;
    if (ic->streams[p->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) t -= lead;
    v[n].p = p; v[n].k = t; n++;
  }
  qsort(v, (size_t)n, sizeof *v, cmpOrd);
  if (avio_open(&oc->pb, argv[2], AVIO_FLAG_WRITE) >= 0 && avformat_write_header(oc, NULL) >= 0) {
    rr = 0;
    for (int i = 0; i < n; i++) {
      av_packet_rescale_ts(v[i].p, ic->streams[v[i].p->stream_index]->time_base, oc->streams[v[i].p->stream_index]->time_base);
      if (av_write_frame(oc, v[i].p) < 0) rr = 1;
    }
    if (av_write_trailer(oc) < 0) rr = 1;
    avio_closep(&oc->pb);
  }
  for (int i = 0; i < n; i++) av_packet_free(&v[i].p);
  free(v);
  avformat_free_context(oc);
  avformat_close_input(&ic);
  return rr;
}
