#include "../src/streams.c"
#include <assert.h>
#include <sys/mman.h>

// #205: quick filters (MP4 only, Cached, Dubbed) must say how many sources they hide.
static void fonte(Stream *s, const char *prov, int mp4) {
  memset(s, 0, sizeof *s);
  snprintf(s->provedor, sizeof s->provedor, "%s", prov);
  s->mp4 = mp4; s->altura = 1080;
}

int main(void) {
  Stream v[4];
  char nomes[128];
  fonte(&v[0], "PlavioHD", 1); fonte(&v[1], "Torrentio", 0);
  fonte(&v[2], "Comet", 0);    fonte(&v[3], "Comet", 0);
  lista = v; n = 4; canalFolha = 1;
  atualizarProvedores();
  filtro = 0;
  assert(!chipsOcultos(NULL, 0) && !fraseDicaChips() && "no chip on: nothing to warn");
  soMp4 = 1;
  assert(chipsOcultos(nomes, sizeof nomes) == 3 && "MP4 only hides 3 of 4");
  assert(strstr(nomes, "MP4") && strstr(fraseDicaChips(), ": 3."));
  assert(!fraseVazioChips() && "one source still passes");
  filtro = 2; // Torrentio tab: every source of it is hidden
  assert(chipsOcultos(NULL, 0) == 1 && nFiltrados() == 0);
  assert(fraseVazioChips() && strstr(fraseVazioChips(), "MP4") && "empty state names the filters");
  soMp4 = 0;
  assert(!chipsOcultos(NULL, 0) && !fraseVazioChips());
  // A closed sheet must not inspect retained sources, with any quick filters enabled.
  void *guard = mmap(NULL, sizeof v, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  assert(guard != MAP_FAILED);
  lista = guard; filtro = 0; aberta = 0; anim = 0;
  for (int chips = 1; chips < 8; chips++) {
    soMp4 = chips & 1; soCache = chips & 2; soDub = chips & 4;
    stream_folha_atualizar(1.0f / 60, 0);
    assert(!aberta && anim == 0);
  }
  lista = v;
  assert(munmap(guard, sizeof v) == 0);
  puts("fontes chips ocultos: ok");
  return 0;
}
