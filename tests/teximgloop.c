// Visible artwork must survive a budget smaller than the visible set.
#include "../src/tex_cache.c"
#include <assert.h>
#include <stdio.h>

static void decoded(int i) {
  snprintf(itens[i].caminho, sizeof itens[i].caminho, "https://test/tile%d.jpg", i);
  itens[i].hash = hashCaminho(itens[i].caminho);
  itens[i].estado = DECODIFICADO;
  itens[i].limite = itens[i].tetoUsado = 128;
  itens[i].ultimoQuadro = quadroAtual;
  itens[i].sup = SDL_CreateRGBSurfaceWithFormat(0, 128, 64, 32, SDL_PIXELFORMAT_ABGR8888);
  assert(itens[i].sup);
  SDL_FillRect(itens[i].sup, NULL, 0xff667788);
}

int main(void) {
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *window = SDL_CreateWindow("teximgloop", 0, 0, 64, 64,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window);
  SDL_GLContext gl = SDL_GL_CreateContext(window);
  assert(gl);
  nMax = 12;
  mtx = SDL_CreateMutex(); assert(mtx);
  orcamento = 1024 * 1024;
  for (int i = 0; i < 3; i++) {
    decoded(i);
    assert(tex_bombear(1) == 1);
  }
  GLuint visible[3] = {itens[0].tex, itens[1].tex, itens[2].tex};
  long resident = bytesUsados;
  orcamento = resident / 2;
  decoded(3);
  SDL_Surface *waiting = itens[3].sup;
  for (int frame = 0; frame < 60; frame++) {
    // Same order as main: pump, advance frame, draw/request.
    assert(tex_bombear(1) == 0);
    tex_novo_quadro();
    podar();
    for (int i = 0; i < 3; i++)
      assert(tex_obter_limite(itens[i].caminho, 128, 0, 0) == visible[i]);
    assert(tex_obter_limite(itens[3].caminho, 128, 0, 0) == 0);
    assert(itens[3].estado == DECODIFICADO && itens[3].sup == waiting);
    assert(bytesUsados == resident && !tex_despejos_quentes);
    assert(filaIni == filaFim && decIni == decFim);
  }
  // Once artwork leaves the screen, cold eviction lets the waiting upload run.
  tex_novo_quadro(); tex_novo_quadro();
  assert(tex_bombear(1) == 1);
  assert(itens[3].estado == PRONTO && bytesUsados <= orcamento);
  assert(!tex_despejos_quentes);
  for (int i = 0; i < nMax; i++)
    if (itens[i].tex) glDeleteTextures(1, &itens[i].tex);
  SDL_DestroyMutex(mtx);
  SDL_GL_DeleteContext(gl); SDL_DestroyWindow(window); SDL_Quit();
  puts("teximgloop: visible textures stable; uploads wait without re-requests");
  return 0;
}
