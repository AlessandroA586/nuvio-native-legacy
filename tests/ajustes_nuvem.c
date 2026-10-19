// Included by ajustes_sync.c: real settings, session, sync, disk; only HTTP replaced.
// Losing any device scalar/store, leaking it to another TV, or dropping unknown
// fields breaks this test. Each reinstall starts a fresh process and directory.
#include <dirent.h>
static void fileWrite(const char *path, const void *buf, size_t n) {
  FILE *f = fopen(path, "wb"); assert(f);
  assert(fwrite(buf, 1, n, f) == n); assert(!fclose(f));
}
static char *fileRead(const char *path) {
  FILE *f = fopen(path, "rb"); assert(f);
  assert(!fseek(f, 0, SEEK_END)); long n = ftell(f); assert(n >= 0);
  rewind(f); char *s = malloc((size_t)n + 1); assert(s);
  assert(fread(s, 1, (size_t)n, f) == (size_t)n); fclose(f); s[n] = 0; return s;
}
static void checkFile(const char *name, const char *expected) {
  char *s = dados_ler(name); check(s && !strcmp(s, expected), name); free(s);
}
static int recoveryStage(const char *dir, const char *stage) {
  char path[1024]; int expected[AJ_N];
  setenv("NUVIO_DEVICE_ID", !strcmp(stage, "device-other") ? "tv-B" : "tv-A", 1);
  setenv("NUVIO_DADOS", dir, 1); dados_iniciar(NULL); player_dir(dir);
  ajustes_dir(dir); fil_definir_perfil(1);
  if (!strcmp(stage, "device-reopen")) {
    snprintf(path, sizeof path, "%s/../expected.bin", dir); char *raw = fileRead(path);
    memcpy(expected, raw, sizeof expected); free(raw);
    for (int i = 0; i < AJ_N; i++) {
      if (!CHAVE[i] || CHAVE[i][0] == '-' ||
          (OPCOES[i].tipo != OP_ESCOLHA && OPCOES[i].tipo != OP_NUMERO)) continue;
      check(valor[i] == expected[i], CHAVE[i]);
    }
    puts("PASS: device-reopen"); return 0;
  }
  login();
  if (!strcmp(stage, "device-save")) {
    cloud = strdup("{\"future_web\":{\"keep\":42},\"c_settings_v1\":{\"future_ext\":42,\"values\":{\"c_future\":42}}}");
    cycle();
    for (int i = 0; i < AJ_N; i++) {
      if (!CHAVE[i] || CHAVE[i][0] == '-' ||
          (OPCOES[i].tipo != OP_ESCOLHA && OPCOES[i].tipo != OP_NUMERO)) continue;
      int v = OPCOES[i].tipo == OP_ESCOLHA ? (valor[i] ? 0 : 1) :
              (valor[i] == OPCOES[i].min ? OPCOES[i].max : OPCOES[i].min);
      valor[i] = limita(i, v);
    }
    valor[AJ_RESOLUCAO] = RES_4K; valor[AJ_VIDRO] = 0;
    valor[AJ_GPU_EFEITOS] = 1; valor[AJ_TEMA] = AJ_TEMA_DINAMICA;
    valor[AJ_IDIOMA] = 4; valor[AJ_PROVEDORES_ESCOPO] = 1;
    valor[AJ_DET_TRAILER_SOM] = 1; // Retired setting; only unified trailer sound is configurable.
    valor[AJ_PLAYER_EXTERNO] = 2; valor[AJ_PLAYER_ANIME] = 3; // Uninstalled package asks on reopening.
    // Save every scalar plus the extra records written by gravar().
    for (int i = 0; i < EX_NFONTES; i++) notaFontes[i] = EX_NFONTES - 1 - i;
    bibliotecaFonteEscolhida = memoriaEscolhida = 1;
    gravar(); memcpy(expected, valor, sizeof expected);
    snprintf(path, sizeof path, "%s/../expected.bin", dir); fileWrite(path, expected, sizeof expected);
    dados_gravar("fanart.txt", "0123456789abcdef");
    dados_gravar("seekr.txt", "own-seekr-key");
    dados_gravar("curadoria.txt", "https://curated.invalid/feed");
    dados_gravar("player-externo.txt", "test.player\ntest.anime\n");
    dados_gravar("ltv-urls-p1.txt", "configured-streams\n");
    dados_gravar("xtream-p1.txt", "portal=test.invalid\nusuario=alice\nsenha=secret\n");
    dados_gravar("sessao-private.txt", "DO-NOT-UPLOAD");
    char nome[128]; ajustes_provedores_arquivo(nome, sizeof nome, "fontefalha");
    dados_gravar(nome, "provedores 1\n");
    fil_definir_limite(3); fil_gravar_registro();
    player_leg_estilo_carregar();
    VideoLegendaEstilo style = *player_leg_estilo(); style.tamanho = 160;
    assert(player_leg_estilo_definir(&style, PLR_LEG_FONTE));
    player_aspecto_definir(PLR_ASP_ESTICAR);
    // Deliberately no sync_proteger: direct file editors must still upload.
    cycle();
    check(pushes > 0, "external/device edits uploaded without dirty notification");
    check(strstr(cloud, "future_web") && strstr(cloud, "future_ext") && strstr(cloud, "c_future"), "unknown web and C fields preserved");
    check(!strstr(cloud, "DO-NOT-UPLOAD") && !strstr(cloud, "444f2d4e4f542d55504c4f4144"), "session files excluded");
    snprintf(path, sizeof path, "%s/../cloud.json", dir); fileWrite(path, cloud, strlen(cloud));
  } else {
    snprintf(path, sizeof path, "%s/../cloud.json", dir); cloud = fileRead(path);
    snprintf(path, sizeof path, "%s/../expected.bin", dir); char *raw = fileRead(path);
    memcpy(expected, raw, sizeof expected); free(raw);
    cycle();
    int other = !strcmp(stage, "device-other");
    for (int i = 0; i < AJ_N; i++) {
      if (!CHAVE[i] || CHAVE[i][0] == '-' ||
          (OPCOES[i].tipo != OP_ESCOLHA && OPCOES[i].tipo != OP_NUMERO)) continue;
      if (other && ajusteAparelho(i)) continue;
      check(valor[i] == expected[i], CHAVE[i]);
    }
    if (other) {
      check(valor[AJ_RESOLUCAO] == valorPadrao[AJ_RESOLUCAO], "second TV keeps own resolution");
      check(valor[AJ_GPU_EFEITOS] == valorPadrao[AJ_GPU_EFEITOS], "second TV keeps own GPU setting");
      check(valor[AJ_VIDRO] == valorPadrao[AJ_VIDRO], "second TV keeps own glass setting");
      ajustes_aplicar_blob("{\"c_settings_v1\":{\"values\":{\"c_vidroLocal\":1,\"c_modernSidebarBlur\":1}}}");
      check(valor[AJ_VIDRO] == valorPadrao[AJ_VIDRO] && valor[AJ_RAIL_BLUR] == valorPadrao[AJ_RAIL_BLUR], "legacy shared blob cannot override device GPU choices");
      check(!dados_ler("fanart.txt"), "second TV receives no device credentials");
      // A second TV writes its section without losing TV A's snapshot.
      valor[AJ_RESOLUCAO] = 0; sync_proteger_ajustes_locais(); cycle();
      snprintf(path, sizeof path, "%s/../cloud.json", dir); fileWrite(path, cloud, strlen(cloud));
    } else {
      checkFile("fanart.txt", "0123456789abcdef"); checkFile("seekr.txt", "own-seekr-key");
      checkFile("ltv-urls-p1.txt", "configured-streams\n");
      checkFile("xtream-p1.txt", "portal=test.invalid\nusuario=alice\nsenha=secret\n");
      char protectedPath[1024]; struct stat mode;
      snprintf(protectedPath, sizeof protectedPath, "%s/xtream-p1.txt", dir);
      check(!stat(protectedPath, &mode) && !(mode.st_mode & 077), "restored credentials retain owner-only permissions");
      check(!strcmp(curadoria_url(), "https://curated.invalid/feed"), "curation applied in memory");
      check(fil_limite_gravado() == 3, "same TV row quota restored");
      check(player_aspecto() == PLR_ASP_ESTICAR && player_leg_estilo()->tamanho == 160, "player and subtitle prefs restored");
      check(ajustes_nota_fonte(0) == EX_NFONTES - 1, "rating order restored");
      check(bibliotecaFonteEscolhida && memoriaEscolhida, "explicit choice markers restored");
      // Partial blobs never reset missing shared/device keys or stores.
      ajustes_aplicar_blob("{\"features\":{\"hero_section_enabled\":true}}");
      check(valor[AJ_RESOLUCAO] == RES_4K && valor[AJ_IDIOMA] == 4, "partial blob preserves absent keys");
      sessao_sair(); sync_esquecer_usuario();
      memcpy(valor, valorPadrao, sizeof valor); login(); cycle();
      check(valor[AJ_RESOLUCAO] == RES_4K && valor[AJ_IDIOMA] == 4, "logout/login restores same TV");
    }
  }
  printf("PASS: %s\n", stage); free(cloud); return 0;
}
static int recovery(const char *exe, const char *root) {
  const char *stages[] = {"device-save", "device-other", "device-restore", "device-reinstall", "device-reopen"};
  for (int i = 0; i < 5; i++) {
    char dir[1024]; snprintf(dir, sizeof dir, "%s/%s", root, i == 4 ? "device-reinstall" : stages[i]);
    if (i != 4) assert(!mkdir(dir, 0700));
    pid_t pid = fork(); assert(pid >= 0);
    if (!pid) { execl(exe, exe, dir, stages[i], (char *)NULL); _exit(127); }
    int status; assert(waitpid(pid, &status, 0) == pid);
    if (!WIFEXITED(status) || WEXITSTATUS(status)) return 1;
  }
  return 0;
}
