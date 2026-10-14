// Host walk: real Settings drawing, no account, network or device writes.
static void perf_frame(unsigned frame, FILE *csv, int check) {
  int r0 = txt_rasterizadas;
  Uint64 t0 = SDL_GetPerformanceCounter();
  gfx_novo_quadro(); txt_novo_quadro(); tex_novo_quadro();
  ajustes_atualizar(1.0f / 60.0f, frame * 16);
  glClear(GL_COLOR_BUFFER_BIT);
  ajustes_desenhar(frame * 16);
  double ms = (SDL_GetPerformanceCounter() - t0) * 1000.0 / SDL_GetPerformanceFrequency();
  if (csv) fprintf(csv, "%u,%.6f,%d,%d,%d,%d\n", frame, ms, gfx_n_rect, txt_rasterizadas - r0, perf_uploads, gfx_n_outros);
  if (check && txt_rasterizadas != r0) {
    fprintf(stderr, "FAIL unchanged scroll frame %u recreated %d text textures\n", frame, txt_rasterizadas - r0);
    exit(1);
  }
  perf_uploads = 0;
  glFinish(); // drain outside CPU timing; no vsync/queue carryover
}
static void perf_walk(SDL_Window *win) {
  const char *dir = getenv("NUVIO_AJUSTES_PERF");
  char path[700], id[40];
  unsigned frame = 0;
  int op, sec, maxsec = 0, n;
  const char *key, *label;
  for (n = 0; ajustes_teste_cena_item(n, &op, &sec, &key, &label); n++) if (sec > maxsec) maxsec = sec;
  snprintf(path, sizeof path, "%s/frames.csv", dir);
  FILE *csv = fopen(path, "w"); assert(csv);
  fprintf(csv, "frame,cpu_ms,draw_calls,text_renders,texture_uploads,other_gl\n");
  for (sec = 0; sec <= maxsec; sec++) {
    snprintf(id, sizeof id, "v2-sec-%d", sec);
    assert(ajustes_teste_quadro(id));
    // Warm every Down position in this section before checking reuse.
    for (int pass = 0; pass < 2; pass++) {
      assert(ajustes_teste_quadro(id));
      for (int step = 0; step < 48; step++) {
        tecla(SDLK_DOWN);
        for (int q = 0; q < 8; q++) perf_frame(frame++, pass ? csv : NULL, pass && getenv("NUVIO_PERF_ASSERT"));
      }
    }
  }
  fclose(csv);
  assert(ajustes_teste_quadro("v2-menu"));
  // Existing capture helper gives a settled PNG at the same deterministic clock.
  snprintf(path, sizeof path, "%s/index.png", dir); captura(path, win);
  assert(ajustes_teste_quadro("v2-sec-0"));
  snprintf(path, sizeof path, "%s/section.png", dir); captura(path, win);
  printf("Settings walk: %d sections, %u frames; %s\n", maxsec + 1, frame / 2, dir);
}
