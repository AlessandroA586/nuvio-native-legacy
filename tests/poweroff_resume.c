// Exercise the real player checkpoint and disk writer; never close the player.
#define main retained_fixture_main
#include "player_retido.c"
#undef main
#include "dados.h"
#include "progresso.h"
#include "traktscrobble.h"
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int nv_test_continuarLocal(CatItem *, int);
void nv_test_episodes(int index) { (void)index; }
int nv_test_enrich(CatItem *items, int count) { (void)items; return count; }
int nv_test_enrich_session(CatItem *items, int count, uint64_t session) {
  (void)session; return nv_test_enrich(items, count);
}
static int syncRequests, pointRequests;
void nv_test_sync(void) { syncRequests++; }
int nv_test_tracker(int event, const char *id, double p, double d, double a, int watched) {
  (void)id; (void)p; (void)d; (void)a; (void)watched;
  if (event == SCR_EV_PONTO) pointRequests++;
  return 0;
}
static int fileSynced, renamed, directorySynced;
int nv_test_fsync(int fd) {
  int rc = fsync(fd);
  if (!rc) { if (renamed) directorySynced++; else fileSynced++; }
  return rc;
}
int nv_test_rename(const char *from, const char *to) {
  int rc = rename(from, to);
  if (!rc && strstr(to, "progresso.txt")) { assert(fileSynced); renamed = 1; }
  return rc;
}
static double seconds = 100;
int nv_test_clock(clockid_t clock, struct timespec *ts) {
  (void)clock; ts->tv_sec = (time_t)seconds; ts->tv_nsec = 0; return 0;
}
int main(int argc, char **argv) {
  const char *mode = argc > 1 ? argv[1] : "periodic";
  int cadence = !strcmp(mode, "cadence"), background = !strcmp(mode, "background");
  if (argc > 2) {
    assert(SDL_Init(SDL_INIT_TIMER) == 0);
    dados_iniciar(""); perfis_definir_ativo(1);
    CatItem c = {0};
    strcpy(c.imdb, "ttPoweroff"); strcpy(c.tipo, "movie"); strcpy(c.meta, "120 min");
    cat_definir(&c, 1); posicao = 0; duracao = 7200;
    player_abrir(0, "https://fixture.invalid/movie.mp4");
    player_atualizar(.01f, SDL_GetTicks());
    for (int i = 1; i <= (cadence ? 340 : 40); i++) {
      seconds++; posicao = i;
      player_atualizar(1.0f, SDL_GetTicks());
    }
    assert(syncRequests == (cadence ? 1 : 0));
    assert(pointRequests == (cadence ? 2 : 0));
    if (background) {
      player_ciclo_evento(SDL_APP_WILLENTERBACKGROUND);
      assert(syncRequests == 1 && pointRequests == 2);
    }
    assert(fileSynced && renamed && directorySynced);
    kill(getpid(), SIGKILL);
    _exit(99);
  }
  pid_t child = fork(); assert(child >= 0);
  if (!child) { execl(argv[0], argv[0], mode, "play", (char *)NULL); _exit(98); }
  int status; assert(waitpid(child, &status, 0) == child);
  assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
  dados_iniciar(""); perfis_definir_ativo(1);
  CatItem cached = {0};
  strcpy(cached.imdb, "ttPoweroff"); strcpy(cached.tipo, "movie");
  strcpy(cached.titulo, "Offline movie");
  cat_definir(&cached, 1);
  prog_invalidar();
  ProgRegistro r;
  assert(prog_por_chave("ttPoweroff", &r));
  assert(r.posSeg == (cadence ? 330 : background ? 40 : 30) && r.pendente);
  assert(prog_continuar_de_perfil_min(1, 1, 90, &r));
  assert(!strcmp(r.contentId, "ttPoweroff") && r.posSeg >= 30);
  CatItem row[2];
  assert(nv_test_continuarLocal(row, 2) == 1);
  assert(!strcmp(row[0].imdb, "ttPoweroff") && row[0].progresso >= 1);
  printf("poweroff_resume: %s: SIGKILL, durable checkpoint %.0f s, Continue Watching restored\n", mode, r.posSeg);
  return 0;
}
