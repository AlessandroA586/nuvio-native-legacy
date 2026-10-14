// Local 1080p Home fixture, both synopsis sizes in pt/de/ru; no network.
#define main unused_herologo_main
#include "herologo_shot.c"
#undef main
#include "idiomacod.h"

int main(int argc, char **argv) {
  assert(argc == 3);
  const char *dir = argv[1], *out = argv[2];
  const int idiomas[] = {IDIOMA_PT, IDIOMA_DE, IDIOMA_RU};
  const char *cod[] = {"pt", "de", "ru"};
  const char *syn[] = {
    "Durante uma viagem por uma cidade desconhecida, uma jovem descobre cartas que revelam a história de sua família. Com a ajuda de um velho amigo, ela precisa reconstruir o passado e decidir em quem confiar antes que os segredos mudem para sempre o seu futuro.",
    "Während einer Reise durch eine unbekannte Stadt entdeckt eine junge Frau Briefe, die die Geschichte ihrer Familie enthüllen. Gemeinsam mit einem alten Freund muss sie die Vergangenheit rekonstruieren und entscheiden, wem sie vertrauen kann, bevor die Geheimnisse ihre Zukunft für immer verändern.",
    "Во время путешествия по незнакомому городу молодая женщина находит письма, раскрывающие историю её семьи. Вместе со старым другом она должна восстановить прошлое и решить, кому можно доверять, прежде чем тайны навсегда изменят её будущее."
  };
  dados_iniciar("deploy/app/art");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("Synopsis", 0, 0, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl);
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar()); assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192); artehero_definir_falhou(tex_falhou);
  gfx_icones_dir("deploy/app/art"); ajustes_dir(dir);
  assert(home_iniciar("deploy/app/art"));
  static CatItem c; CatFileira fil = {0};
  snprintf(c.imdb, sizeof c.imdb, "fixture-synopsis");
  snprintf(c.tipo, sizeof c.tipo, "movie");
  snprintf(c.titulo, sizeof c.titulo, "Uma viagem inesperada");
  snprintf(c.genero, sizeof c.genero, "Drama");
  snprintf(c.meta, sizeof c.meta, "2024");
  char art[700]; assert(realpath("deploy/app/art", art));
  snprintf(c.backdrop, sizeof c.backdrop, "%s/bg/01.jpg", art);
  snprintf(c.backdropCatalogo, sizeof c.backdropCatalogo, "%s", c.backdrop);
  snprintf(c.poster, sizeof c.poster, "%s/poster/01.jpg", art);
  snprintf(fil.chave, sizeof fil.chave, "fixture");
  snprintf(fil.titulo, sizeof fil.titulo, "Cinema"); fil.n = 1;
  for (int lang = 0; lang < 3; lang++) {
    assert(ajustes_shot_valor("idioma", idiomas[lang] + 1));
    snprintf(c.sinopse, sizeof c.sinopse, "%s", syn[lang]);
    cat_definir_tudo(&c, 1, &fil, 1);
    for (int large = 0; large < 2; large++) {
      assert(ajustes_shot_valor("heroSynopsisSizeLocal", large));
      char path[700]; snprintf(path, sizeof path, "%s/%s-%s.bmp", out, cod[lang], large ? "grande" : "normal");
      quadros(w, 90, path);
    }
  }
  tex_encerrar(); txt_encerrar(); SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(w); SDL_Quit(); return 0;
}
