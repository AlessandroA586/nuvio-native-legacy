// Cast OK routing across detail layouts, media kinds, and info-tab states.
#include <assert.h>
#define pessoa_pedir capture_person
#define ajustes_home_layout fixture_layout
#define extras_n_colecao fixture_collection_count
#include "../src/detail.c"
#undef pessoa_pedir
#undef ajustes_home_layout
#undef extras_n_colecao

static int layout, collection_count, requests;
static long requested_id;
static char requested_name[64], requested_photo[512];
int fixture_layout(void) { return layout; }
int fixture_collection_count(void) { return collection_count; }
void capture_person(long id, const char *name, const char *photo) {
  requests++;
  requested_id = id;
  snprintf(requested_name, sizeof requested_name, "%s", name);
  snprintf(requested_photo, sizeof requested_photo, "%s", photo);
}

int main(void) {
  CatItem title = {0};
  int cases = 0;
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  SDL_Delay(2); // Zero ticks is the handler's sentinel for no OK press.
  title.nElenco = 3;
  for (int actor = 0; actor < title.nElenco; actor++) {
    title.elenco[actor].tmdb = 5150 + actor;
    snprintf(title.elenco[actor].nome, sizeof title.elenco[actor].nome, "Actor %d", actor);
    snprintf(title.elenco[actor].foto, sizeof title.elenco[actor].foto, "photo-%d.jpg", actor);
  }
  for (layout = 0; layout < HOME_LAYOUT_N; layout++) {
    for (int series = 0; series < 2; series++) {
      snprintf(title.tipo, sizeof title.tipo, "%s", series ? "series" : "movie");
      cat_definir_tudo(&title, 1, NULL, 0);
      idx = 0;
      for (int tabs = 0; tabs < 2; tabs++) {
        collection_count = tabs ? 2 : 0;
        abaInfo = 0;
        assert((secaoN(SEC_ABAS_INFO) > 0) == (series && tabs));
        assert(secaoN(SEC_ELENCO) == title.nElenco);
        for (int carousel = 0; carousel <= (layout == HOME_LAYOUT_DINAMICA); carousel++) {
          carro = carousel;
          carCheia = 1;
          carPos = carAplicado = 0;
          for (int keypad = 0; keypad < 2; keypad++) {
            for (int actor = 0; actor < title.nElenco; actor++) {
              SDL_Event event = {0};
              aberto = 1; saindo = 0; nivel = 1;
              pessoaAberta = colListaAberta = 0;
              foco.fileira = SEC_ELENCO; foco.coluna = actor;
              okDesceEm = 0;
              requests = 0; requested_id = 0;
              event.key.keysym.sym = keypad ? SDLK_KP_ENTER : SDLK_RETURN;
              event.type = SDL_KEYDOWN;
              detail_evento(&event);
              event.type = SDL_KEYUP;
              detail_evento(&event);
              if (requests != 1 || requested_id != title.elenco[actor].tmdb || !pessoaAberta) {
                fprintf(stderr, "FAIL: layout=%d series=%d tabs=%d carousel=%d keypad=%d actor=%d requests=%d id=%ld open=%d\n",
                        layout, series, tabs, carousel, keypad, actor, requests, requested_id, pessoaAberta);
                return 1;
              }
              assert(!strcmp(requested_name, title.elenco[actor].nome));
              assert(!strcmp(requested_photo, title.elenco[actor].foto));
              cases++;
            }
          }
        }
      }
    }
  }
  // A collection in the shared slot must not request the actor at column zero.
  snprintf(title.tipo, sizeof title.tipo, "series");
  cat_definir_tudo(&title, 1, NULL, 0);
  idx = 0; collection_count = 2; abaInfo = 1;
  assert(abaIdDe(abaInfo) == ABA_COLECAO);
  for (layout = 0; layout < HOME_LAYOUT_N; layout++) {
    SDL_Event event = {0};
    carro = 0; aberto = 1; saindo = 0; nivel = 1;
    pessoaAberta = colListaAberta = 0;
    foco.fileira = SEC_ELENCO; foco.coluna = 0;
    okDesceEm = 0; requests = 0;
    event.key.keysym.sym = SDLK_RETURN;
    event.type = SDL_KEYDOWN; detail_evento(&event);
    event.type = SDL_KEYUP; detail_evento(&event);
    assert(!requests && !pessoaAberta && colListaAberta);
  }
  printf("PASS: %d cast OK cases across Modern, Standard, and Apple TV detail layouts; collection routing preserved\n", cases);
  SDL_Quit();
  return 0;
}
