// Exercise the real Home renderer with local art and audit its draw calls.
#define main homelayouts_fixture_main
#include "homelayouts_shot.c"
#undef main
#include <math.h>

static int heroInfo, heroBackdrops, stageArtwork;
static GLuint artworkTexture;
static void auditRect(GfxRect r, GLuint tex, GfxModo modo, float foco,
                      float px, float py, float raio, float cr, float cg, float cb, float ca) {
  if (r.y * gfx_escala() < 480 && ca * gfx_opacidade_grupo > .004f) {
    if (modo == GFX_HERO || modo == GFX_HERO_CHEIO ||
        modo == GFX_HERO_CAM || modo == GFX_HERO_CHEIO_CAM) heroBackdrops++;
    if (modo == GFX_TEXTO || modo == GFX_MARCA) heroInfo++;
    if (modo == GFX_VITRINE && tex) { stageArtwork++; artworkTexture = tex; }
  }
  gfx_rect(r, tex, modo, foco, px, py, raio, cr, cg, cb, ca);
}
static void auditText(TxtLinha l, float x, float y, float a) {
  if (y * gfx_escala() < 480 && a * gfx_opacidade_grupo > .004f && l.tex) heroInfo++;
  txt_desenhar_alpha(l, x, y, a);
}
// Keep instrumentation in the test translation unit, with no production hooks.
#define gfx_rect auditRect
#define txt_desenhar_alpha auditText
#ifdef PALCO_BASELINE
#include PALCO_BASELINE
#else
#include "../src/home.c"
#endif
#undef gfx_rect
#undef txt_desenhar_alpha

static char shotPath[900];
static void capture(const char *dir, const char *phase, int mode, const char *state) {
  snprintf(shotPath, sizeof shotPath, "%s/%s-%d-g%d-m%d-%s.bmp", dir, phase,
           (int)roundf(gfx_escala_txt() * 100), ajustes_vidro(), mode, state);
  heroInfo = heroBackdrops = stageArtwork = 0; artworkTexture = 0;
  if (!strcmp(state, "stage")) holoA = 1.0f;
  quadros(1, shotPath);
}
static void blankHero(void) {
  SDL_Surface *s = SDL_LoadBMP(shotPath);
  assert(s);
  int changed = 0;
  for (int y = 110; y < 350; y++) for (int x = 100; x < 700; x++) {
    Uint32 pixel; Uint8 r, g, b;
    memcpy(&pixel, (char *)s->pixels + y * s->pitch + x * 4, 4);
    SDL_GetRGB(pixel, s->format, &r, &g, &b);
    if (fabsf(r - NV_COR_FUNDO_R * 255) > 2 ||
        fabsf(g - NV_COR_FUNDO_G * 255) > 2 ||
        fabsf(b - NV_COR_FUNDO_B * 255) > 2) changed++;
  }
  SDL_FreeSurface(s);
  assert(changed == 0);
}
static void setting(int layout, int glass, int mode) {
  char extra[120];
  snprintf(extra, sizeof extra, "socialLocal 1\npalcoFundoLocal %d", mode);
  setenv("NV_AJ", extra, 1);
  ajusta(layout, glass);
  assert(ajustes_palco_fundo() == mode);
}

int main(int argc, char **argv) {
  assert(argc == 5);
  const char *dir = argv[1], *phase = argv[2];
  int glass = atoi(argv[4]);
  setenv("NV_STAGE_SCALE", argv[3], 1);
  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  assert(dirDados && strstr(dirDados, "nuvio-homelayouts-shot"));
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Stage background regression", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  SDL_GLContext gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_borrao_iniciar(480, 270);
  artehero_definir_falhou(tex_falhou);
  gfx_icones_dir("deploy/app/art");
  ajustes_dir(dirDados);
  assert(ajustes_palco_fundo() == 0);
  assert(ajustes_escala_texto() == .85f);
  setting(0, glass, 0); // Run legacy appearance migrations once.
  setting(0, glass, 0); // Then use one stable theme for every mode.
  assert(home_iniciar("deploy/app/art"));
  AddonRemoto addon = { .nome = "Fixture", .url = "http://127.0.0.1:1/manifest.json", .ativo = 1 };
  assert(addons_definir_lista(&addon, 1));
  static CatItem items[12];
  static CatFileira rows[2];
  for (int r = 0; r < 2; r++) {
    snprintf(rows[r].chave, sizeof rows[r].chave, "%s", r ? "stage_movie" : "pop_movie");
    snprintf(rows[r].titulo, sizeof rows[r].titulo, "%s", r ? "Latest Horror - Movie" : "Popular - Movie");
    snprintf(rows[r].tipo, sizeof rows[r].tipo, "movie");
    rows[r].ini = r * 6; rows[r].n = 6;
    for (int c = 0; c < 6; c++) {
      int i = r * 6 + c;
      CatItem *it = &items[i];
      snprintf(it->imdb, sizeof it->imdb, "tt90%05d", i);
      snprintf(it->tipo, sizeof it->tipo, "movie");
      snprintf(it->titulo, sizeof it->titulo, "%s", NOMES[i]);
      snprintf(it->poster, sizeof it->poster, "deploy/app/art/poster/%02d.jpg", i);
      snprintf(it->backdrop, sizeof it->backdrop, "deploy/app/art/%02d.jpg", i);
      snprintf(it->logo, sizeof it->logo, "deploy/app/art/logo/%02d.png", i);
      snprintf(it->meta, sizeof it->meta, "2026 · 112 min");
      snprintf(it->genero, sizeof it->genero, "Movie · Comedy · Horror");
      snprintf(it->sinopse, sizeof it->sinopse, "A quiet town discovers a mystery. One night changes everything for the friends who return home.");
      it->nota = 76;
    }
  }
  cat_definir_tudo(items, 12, rows, 2);
  quadros(1, NULL);
  assert(fil_definir_tipo("stage_movie", FIL_TIPO_PALCO));
  quadros(90, NULL);
  tecla(SDLK_DOWN); quadros(90, NULL);
  capture(dir, phase, 0, "normal");
  assert(heroInfo > 0 && heroBackdrops > 0);
  tecla(SDLK_DOWN); quadros(1, NULL);
  capture(dir, phase, 0, "enter");
  assert(heroBackdrops > 0); // First focus frames retain the fading hero.
  quadros(90, NULL);
  capture(dir, phase, 0, "stage");
#ifdef PALCO_BASELINE
  // Untouched renderer: capture proof, then demonstrate the missing behavior.
  fflush(stdout);
  quadros(510, NULL); capture(dir, phase, 0, "stage"); // same settle time as Current
  assert(heroInfo == 0 && heroBackdrops == 0);
  return 0;
#else
  assert(heroInfo == 0 && heroBackdrops == 0 && stageArtwork == 0);
  blankHero();
  double blackFill = fillVisUlt;
  int blackRects = rectUlt;
  setting(0, glass, 1); quadros(600, NULL); // hero re-enters; settle timed fades
  capture(dir, phase, 1, "stage");
  assert(heroInfo > 0 && heroBackdrops > 0);
  double currentFill = fillVisUlt;
  int currentRects = rectUlt;
  setting(0, glass, 2); quadros(90, NULL);
  capture(dir, phase, 2, "stage");
  assert(heroInfo == 0 && heroBackdrops == 0 && stageArtwork == 1);
  const CatItem *focused = cat_item_exato(fileiraItemIndice(&fileiras[foco.fileira], foco.coluna));
  assert(focused && palcoArte(focused));
  assert(artworkTexture == tex_obter_hero(focused->backdrop));
  assert(artworkTexture != tex_obter(focused->poster));
  assert(blackFill < currentFill && fillVisUlt <= currentFill);
  assert(blackRects < currentRects && rectUlt <= currentRects);
  printf("scale=%s glass=%d fill black=%.3f current=%.3f artwork=%.3f; draw assertions passed\n",
         argv[3], glass, blackFill, currentFill, fillVisUlt);
  CatItem missing = *focused;
  missing.backdrop[0] = 0; assert(!palcoArte(&missing));
  snprintf(missing.backdrop, sizeof missing.backdrop, "%s", missing.poster);
  assert(!palcoArte(&missing));
  snprintf(missing.poster, sizeof missing.poster, "https://image.tmdb.org/t/p/w500/same.jpg");
  snprintf(missing.backdrop, sizeof missing.backdrop, "https://image.tmdb.org/t/p/original/same.jpg");
  assert(!palcoArte(&missing));
  int op = ajustes_rapido_op("palcoFundoLocal");
  assert(op >= 0);
  assert(ajustes_rapido_passo(op, 1)); // Persist Artwork -> Black.
  ajustes_dir(dirDados); assert(ajustes_palco_fundo() == 0);
  ajustes_aplicar_blob("{\"palcoFundoLocal\":2}");
  assert(ajustes_palco_fundo() == 0); // Account cannot override device choice.
  setting(0, glass, 2);
  // Missing backdrop must render exactly the same empty hero as Black.
  CatItem *live = (CatItem *)focused;
  char saved[sizeof live->backdrop]; memcpy(saved, live->backdrop, sizeof saved);
  snprintf(live->backdrop, sizeof live->backdrop, "%s", live->poster);
  capture(dir, phase, 2, "fallback");
  assert(heroInfo == 0 && heroBackdrops == 0 && stageArtwork == 0);
  blankHero(); memcpy(live->backdrop, saved, sizeof saved);
  tecla(SDLK_UP); quadros(1, NULL);
  capture(dir, phase, 2, "leave");
  quadros(90, NULL);
  capture(dir, phase, 2, "normal-return");
  assert(heroInfo > 0 && heroBackdrops > 0);
  int otherInfo[3] = {0}, otherBackdrops[3] = {0};
  for (int mode = 0; mode < 3; mode++) for (int layout = 1; layout <= 2; layout++) {
    setting(layout, glass, mode); quadros(90, NULL);
    tecla(SDLK_DOWN); quadros(90, NULL);
    capture(dir, phase, mode, layout == 1 ? "standard" : "dynamic");
    if (layout == 2) {
      assert(palcoFundoDestino() == mode);
      if (mode == 0) {
        assert(palcoFundoAlpha == 0 && heroInfo == 0 && heroBackdrops == 0 && stageArtwork == 0);
        blankHero();
      } else if (mode == 1) assert(heroInfo > 0 && heroBackdrops > 0);
      else assert(heroInfo == 0 && heroBackdrops == 0 && stageArtwork == 1);
    } else {
      assert(palcoFundoDestino() == 1 && palcoFundoAlpha == 1.0f);
      if (!mode) { otherInfo[layout] = heroInfo; otherBackdrops[layout] = stageArtwork; }
      else assert(heroInfo == otherInfo[layout] && stageArtwork == otherBackdrops[layout]);
    }
    tecla(SDLK_UP); quadros(90, NULL);
  }
  tex_encerrar(); txt_encerrar();
  SDL_GL_DeleteContext(gl); SDL_DestroyWindow(janela); SDL_Quit();
  return 0;
#endif
}
