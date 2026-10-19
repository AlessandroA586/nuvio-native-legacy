// Production settings and save path; only remote writes are intercepted.
#define desc_repetir bibfonte_repeat
#include "../src/ajustes.c"
#undef desc_repetir
void bibfonte_repeat(void) {}
#include "../src/contalib.h"
#include "../src/agenda.h"
int test_simkl_write(const char *, const char *, int);
#define trakt_watchlist_tipo test_trakt_write
#define simkl_lista_tipo test_simkl_write
#include "../src/ilhasalvar.c"
#undef trakt_watchlist_tipo
#undef simkl_lista_tipo
#include "nvtmp.h"
#include <assert.h>
#include <unistd.h>

static int traktWrites, simklWrites, failures;
int test_trakt_write(const char *id, const char *type, int add) {
  assert(!strcmp(id, "tt1234567") && !strcmp(type, "series"));
  traktWrites += add ? 1 : -1; return 1;
}
int test_simkl_write(const char *id, const char *type, int add) {
  assert(!strcmp(id, "tt1234567") && !strcmp(type, "series"));
  simklWrites += add ? 1 : -1; return 1;
}
int bibfonte_library_count(void);
int bibfonte_discovery_watchlist(void);
const char *bibfonte_menu_label(int index);
#define CHECK(name, condition) do { int ok = !!(condition); \
  printf("%s: %s\n", ok ? "PASS" : "FAIL", name); failures += !ok; } while (0)

void bibfonte_library_shot(void);
#include "bibfonte_shots.inc"
int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IOLBF, 0);
  char dir[1024], path[1100];
  CatItem ci = {0};
  snprintf(dir, sizeof dir, "%s/nuvio-bibfonte-XXXXXX", nvt_dir());
  assert(mkdtemp(dir));
  assert(!setenv("NUVIO_DADOS", dir, 1));
  assert(!SDL_Init(SDL_INIT_TIMER));
  dados_iniciar(dir); assert(!strcmp(dados_dir(), dir));
  snprintf(dirAjustes, sizeof dirAjustes, "%s", dir);
  snprintf(path, sizeof path, "%s/simkl.txt", dir);
  FILE *f = fopen(path, "w"); assert(f); fputs("test-token\n", f); fclose(f);
  simklauth_carregar(); assert(simkl_ativo());
  strcpy(ci.imdb, "tt1234567"); strcpy(ci.tipo, "series"); strcpy(ci.titulo, "Saved series");
  cat_definir_tudo(&ci, 1, NULL, 0); assert(cat_n() == 1);
  salvos_iniciar();
  if (argc == 2) { screenshots(argv[1]); return 0; }
  // Factory Trakt choice with only Simkl connected must save into its feed.
  valor[AJ_SALVOS_DEST] = AJ_SALVOS_TRAKT;
  CHECK("detail/context save getters follow fallback", ajustes_salvos_no_simkl() && !ajustes_salvos_no_trakt());
  ilhasalvar_executar(&ci, 1);
  CHECK("Simkl-only + reaches Simkl", simklWrites == 1 && traktWrites == 0);
  CHECK("toast names effective destination", !strcmp(ilhasalvar_destino(), i18n("Plan to Watch do Simkl")));
  CHECK("+ appears in Library", bibfonte_library_count() == 1);
  // Discovery republishes a remote snapshot, losing transient catalog marks.
  CatItem remote = ci; remote.naLista = simklWrites == 1;
#ifndef BIBFONTE_BASELINE
  remote.listaFontes = remote.naLista ? 1u << AJ_SALVOS_SIMKL : 0;
#endif
  cat_definir_tudo(&remote, 1, NULL, 0);
  CHECK("Simkl save survives discovery rebuild", bibfonte_library_count() == 1);
  CHECK("menu offers remove on active source", !strcmp(bibfonte_menu_label(0), "Remover dos Salvos"));
  ilhasalvar_executar(&ci, 0);
  CHECK("Simkl-only remove reaches Simkl", simklWrites == 0);
  trakt_definir("test-token", "test-client");
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  trakt_esquecer();
  ilhasalvar_executar(&ci, 1);
  CHECK("disconnected Trakt + falls back to Simkl", simklWrites == 1);
  ilhasalvar_executar(&ci, 0);
  // A legacy salvosDestino was a write preference, not an account-library opt-out.
  snprintf(path, sizeof path, "%s/ajustes.txt", dir);
  f = fopen(path, "w"); assert(f); fputs("salvosDestino 1\n", f); fclose(f);
  ajustes_dir(dir);
  trakt_definir("test-token", "test-client");
  assert(contalib_ler_biblioteca("[{\"content_id\":\"tt7654321\",\"content_type\":\"series\",\"name\":\"Account series\"}]") == 1);
  CHECK("legacy account keeps Library source", !ajustes_salvos_no_trakt() && !ajustes_salvos_no_simkl());
  CHECK("account save getters agree with Library", !ajustes_salvos_no_trakt() && !ajustes_salvos_no_simkl());
  CHECK("Settings displays effective source", uxValorAtual(AJ_SALVOS_DEST) == AJ_SALVOS_LOCAL);
  CHECK("legacy account reaches Library", contalib_aplicar_catalogo() == 1 && bibfonte_library_count() == 1);
  assert(definirValorDireto(AJ_SALVOS_DEST, AJ_SALVOS_TRAKT));
  CHECK("explicit Trakt choice wins over account", ajustes_salvos_no_trakt());
  ajustes_dir(dir);
  CHECK("explicit choice survives reload", ajustes_salvos_no_trakt());
  ajustes_definir_salvos_destino(AJ_SALVOS_LOCAL);
  CHECK("Trakt watchlist still fetched for Agenda", bibfonte_discovery_watchlist() == 1);
  CHECK("Trakt-only title stays outside Nuvio Library", bibfonte_library_count() == 1);
  int followed = agenda_montar(), found = 0;
  for (int i = 0; i < followed; i++) if (!strcmp(agenda_lista(i)->imdb, "tt9999999")) found = 1;
  CHECK("Trakt series remains in Agenda", found);
#ifndef BIBFONTE_BASELINE
  // Merge must preserve membership even when the shared mark was already set.
  CatItem shared = *cat_item(1); shared.listaFontes = 1u << AJ_SALVOS_TRAKT;
  cat_mesclar_listas(&shared, 1);
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  CHECK("merge retains overlapping sources", bibfonte_library_count() == 2);
  cat_definir_na_lista(1, 0);
  CHECK("remove preserves other source and Agenda mark", cat_item(1)->naLista && cat_item(1)->listaFontes == 1u);
  ajustes_definir_salvos_destino(AJ_SALVOS_LOCAL);
  CHECK("account item remains after remote removal", bibfonte_library_count() == 1);
#endif
  // Both services connected; union membership must not decide the action.
  remote.naLista = 1;
#ifndef BIBFONTE_BASELINE
  remote.listaFontes = 1u << AJ_SALVOS_TRAKT;
#endif
  cat_definir_tudo(&remote, 1, NULL, 0);
  ajustes_definir_salvos_destino(AJ_SALVOS_SIMKL);
  CHECK("Trakt-only title excluded under Simkl", bibfonte_library_count() == 0);
  CHECK("Trakt-only title offers save under Simkl", !strcmp(bibfonte_menu_label(0), "Salvar"));
  int entrar = !biblioteca_na_lista(0);
  CHECK("Detail chooses add under Simkl", entrar);
  ilhasalvar_executar(cat_item(0), entrar);
  CHECK("Detail add reaches Simkl", simklWrites == 1);
  CHECK("menu offers remove after Simkl add", !strcmp(bibfonte_menu_label(0), "Remover dos Salvos"));
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  CHECK("Trakt membership survives Simkl add", biblioteca_na_lista(0));
  ajustes_definir_salvos_destino(AJ_SALVOS_SIMKL);
  ilhasalvar_executar(cat_item(0), !biblioteca_na_lista(0));
  CHECK("Detail remove reaches Simkl", simklWrites == 0);
  CHECK("Simkl remove preserves Trakt membership", cat_item(0)->naLista && !biblioteca_na_lista(0));
  CHECK("menu offers save after Simkl remove", !strcmp(bibfonte_menu_label(0), "Salvar"));
  // Saved sidebar must offer only titles removable from its selected source.
  SalvosEntrada sidebar[8];
  cat_definir_tudo(&ci, 1, NULL, 0);
  ajustes_definir_salvos_destino(AJ_SALVOS_LOCAL);
  salvos_definir(&ci, 1);
  CHECK("sidebar includes local save under Nuvio", salvos_uniao(sidebar, 8) == 1);
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  CHECK("sidebar excludes local-only save under Trakt", salvos_uniao(sidebar, 8) == 0);
  ajustes_definir_salvos_destino(AJ_SALVOS_SIMKL);
  CHECK("sidebar excludes local-only save under Simkl", salvos_uniao(sidebar, 8) == 0);
  cat_definir_tudo(&remote, 1, NULL, 0);
  CHECK("sidebar excludes Trakt-only title under Simkl", salvos_uniao(sidebar, 8) == 0);
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  CHECK("sidebar includes selected remote title", salvos_uniao(sidebar, 8) == 1 && sidebar[0].local == -1);
  CHECK("sidebar remote title offers remove", !strcmp(bibfonte_menu_label(0), "Remover dos Salvos"));
  ajustes_definir_salvos_destino(AJ_SALVOS_LOCAL);
  CHECK("sidebar preserves local save after source switches", salvos_uniao(sidebar, 8) == 1 && salvos_tem(ci.imdb));
  salvos_definir(&ci, 0);
  cat_definir_tudo(&remote, 1, NULL, 0);
#ifndef BIBFONTE_BASELINE
  // Upgraded profiles share a legacy write preference, not an explicit source.
  dados_gravar("ajustes.txt", "salvosDestino 1\n");
  dados_gravar("ajustes-p1.txt", "salvosDestino 1\n");
  dados_gravar("ajustes-p2.txt", "salvosDestino 1\n");
  ajustes_dir(dir);
  for (int profile = 1; profile <= 2; profile++) {
    ajustes_perfil_padrao();
    assert(ajustes_perfil_restaurar(profile));
    CHECK("upgraded profile initially uses Nuvio", ajustes_fonte_biblioteca() == AJ_SALVOS_LOCAL);
  }
  assert(ajustes_perfil_restaurar(1));
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  ajustes_perfil_guardar(1);
  ajustes_perfil_padrao();
  CHECK("profile reset clears explicit source", ajustes_fonte_biblioteca() == AJ_SALVOS_LOCAL);
  assert(ajustes_perfil_restaurar(2));
  CHECK("legacy B does not inherit A's explicit Trakt choice", ajustes_fonte_biblioteca() == AJ_SALVOS_LOCAL);
  CHECK("legacy B Settings displays Nuvio", uxValorAtual(AJ_SALVOS_DEST) == AJ_SALVOS_LOCAL);
  CHECK("legacy B excludes Trakt-only Library item", bibfonte_library_count() == 0);
  int beforeTrakt = traktWrites, beforeSimkl = simklWrites;
  ilhasalvar_executar(&ci, 1);
  CHECK("legacy B + stays in Nuvio", traktWrites == beforeTrakt && simklWrites == beforeSimkl && bibfonte_library_count() == 1);
  ilhasalvar_executar(&ci, 0);
  ajustes_perfil_guardar(2);
  // Reload defaults before restoring A: only its profile file can recover intent.
  dados_gravar("ajustes.txt", "salvosDestino 1\n");
  ajustes_dir(dir);
  ajustes_perfil_padrao();
  assert(ajustes_perfil_restaurar(1));
  CHECK("A restores explicit Trakt after reload", ajustes_fonte_biblioteca() == AJ_SALVOS_TRAKT);
  ajustes_definir_salvos_destino(AJ_SALVOS_SIMKL);
  ajustes_perfil_guardar(1);
  ajustes_perfil_padrao();
  assert(ajustes_perfil_restaurar(1));
  CHECK("A restores explicit Simkl with source value", ajustes_fonte_biblioteca() == AJ_SALVOS_SIMKL);
  // Direct restores also clear intent when a legacy/new profile has no marker.
  assert(ajustes_perfil_restaurar(2));
  CHECK("saved untouched B retains Nuvio precedence", ajustes_fonte_biblioteca() == AJ_SALVOS_LOCAL);
  ajustes_definir_salvos_destino(AJ_SALVOS_TRAKT);
  assert(!ajustes_perfil_restaurar(3));
  CHECK("missing profile does not inherit explicit source", ajustes_fonte_biblioteca() == AJ_SALVOS_LOCAL);
#endif
  SDL_Quit();
  printf("bibfonte: %d failure(s)\n", failures);
  return failures ? 1 : 0;
}
