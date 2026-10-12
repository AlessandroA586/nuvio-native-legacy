// #390: o "Modelo de URL dos posteres" colado do celular (ou digitado) com
// mais de 299 caracteres, chave com maiusculas e {imdb} no fim nao era salvo:
// a modal cortava em 299 (PP_MODELO_MAX - 1), o alfabeto do campo baixava a
// caixa da chave, e o validador recusava o que sobrava sem gravar nada.
// Caminho de verdade: modal do campo -> texto do celular -> pstDefinir ->
// posteres.txt -> leitura de volta -> URL montada. A chave e de mentira.
#include "dados.h"
#include "posterprov.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ajustes_teste_poster_modelo(const char *colado, char *lido, size_t n);
void ajustes_teste_poster_recarregar(char *lido, size_t n);

static int falhas;
#define OK(c, m) do { if (c) printf("ok   %s\n", m); else { printf("FALHA %s\n", m); falhas++; } } while (0)

int main(void) {
  char modelo[600], lido[1024], u[PP_URL_MAX];
  int aviso, k;
  dados_iniciar(NULL);   // NUVIO_DADOS: diretorio temporario do .sh
  // ~380 caracteres: chave de mentira com maiusculas, config longa, {imdb} no fim.
  k = snprintf(modelo, sizeof modelo,
               "https://posters.exemplo.invalid/FAKEKEY_ABCdef0123456789XYZ/config=");
  while (k < 360) k += snprintf(modelo + k, sizeof modelo - (size_t)k, "Ab1-Cd2_Ef3~");
  k += snprintf(modelo + k, sizeof modelo - (size_t)k, "/poster/{type}/{imdb}.jpg");
  printf("modelo de %d caracteres\n", k);
  assert(k > 300 && k < 400);

  aviso = ajustes_teste_poster_modelo(modelo, lido, sizeof lido);
  printf("aviso longo=%d\n", aviso);
  OK(aviso == 0, "o modelo longo e aceito (sem aviso de invalido)");
  OK(!strcmp(lido, modelo), "posteres.txt devolve o modelo inteiro, na mesma caixa");
  OK(posterprov_cfg()->prov == PP_MODELO && !strcmp(posterprov_cfg()->modelo, modelo),
     "o provedor ativo recebe o modelo inteiro");
  OK(posterprov_montar_url(posterprov_cfg(), "tt0111161", 0, "movie", u, sizeof u) &&
     strstr(u, "FAKEKEY_ABCdef") && strstr(u, "/poster/movie/tt0111161.jpg"),
     "a URL do cartaz sai com a chave intacta e o {imdb} trocado");

  // O curto de sempre continua igual.
  aviso = ajustes_teste_poster_modelo("https://meu.servidor.invalid/{type}/{imdb}.jpg", lido, sizeof lido);
  printf("aviso curto=%d lido=%s\n", aviso, lido);
  OK(aviso == 0 && !strcmp(lido, "https://meu.servidor.invalid/{type}/{imdb}.jpg"), "modelo curto inalterado");
  // #390: aliases passam pelo mesmo caminho de gravacao do modelo proprio.
  { const char *aliases[] = { "{imdb_id}", "{tmdb_id}", "{id}" };
    const char *ids[] = { "tt0111161", "278", "tt0111161" };
    int a;
    for (a = 0; a < 3; a++) {
      char esperado[PP_URL_MAX];
      snprintf(modelo, sizeof modelo, "https://posters.exemplo.invalid/{type}/%s.jpg", aliases[a]);
      aviso = ajustes_teste_poster_modelo(modelo, lido, sizeof lido);
      snprintf(esperado, sizeof esperado, "https://posters.exemplo.invalid/movie/%s.jpg", ids[a]);
      OK(aviso == 0 && !strcmp(lido, modelo) &&
         posterprov_montar_url(posterprov_cfg(), "tt0111161", 278, "movie", u, sizeof u) &&
         !strcmp(u, esperado), aliases[a]);
      if (a == 2) {
        OK(posterprov_montar_url(posterprov_cfg(), "tmdb:t1399", 0, "series", u, sizeof u) &&
           !strcmp(u, "https://posters.exemplo.invalid/series/1399.jpg"),
           "{id} usa TMDB quando nao ha IMDb");
        OK(posterprov_montar_url(posterprov_cfg(), "tt0111161", 0, "movie", u, sizeof u) &&
           !strcmp(u, esperado), "{id} aceita somente IMDb");
        OK(posterprov_montar_url(posterprov_cfg(), "", 278, "movie", u, sizeof u) &&
           !strcmp(u, "https://posters.exemplo.invalid/movie/278.jpg"), "{id} aceita somente TMDB");
        OK(!posterprov_montar_url(posterprov_cfg(), "", 0, "movie", u, sizeof u),
           "{id} sem identificador continua recusado");
      } else {
        OK(!posterprov_montar_url(posterprov_cfg(), a == 0 ? "tmdb:278" : "tt0111161",
                                 0, "movie", u, sizeof u), "alias nao inventa identificador ausente");
      }
      // 12 marcadores: a sintaxe cabe em 400, mas o montado toca o limite.
      { int max = a == 1 ? 19 : 23, i;
        k = snprintf(modelo, sizeof modelo, "https://posters.exemplo.invalid/");
        for (i = 0; i < 12; i++) k += snprintf(modelo + k, sizeof modelo - (size_t)k, "%s", aliases[a]);
        while (k + 12 * (max - (int)strlen(aliases[a])) < PP_URL_MAX - 1) modelo[k++] = 'a';
        modelo[k] = 0;
        OK(posterprov_modelo_cabe(modelo), "alias com pior caso de 511 bytes cabe");
        modelo[k++] = 'a'; modelo[k] = 0;
        OK(posterprov_modelo_valido(modelo) && !posterprov_modelo_cabe(modelo),
           "alias com pior caso de 512 bytes e recusado por tamanho");
      }
    }
  }
  // Sem marcador continua recusado.
  aviso = ajustes_teste_poster_modelo("https://meu.servidor.invalid/x.jpg", lido, sizeof lido);
  OK(aviso != 0, "modelo sem marcador continua recusado");

  // Codex P2: o modelo cabe em 400, mas o MONTADO tem de caber em PP_URL_MAX.
  // 40 x {imdb} viram 40 x tt0111161 = 520 caracteres: salvo, nunca montaria.
  { char m[PP_MODELO_MAX], u2[PP_URL_MAX];
    int j = snprintf(m, sizeof m, "https://posters.exemplo.invalid/");
    int i;
    for (i = 0; i < 40; i++) j += snprintf(m + j, sizeof m - (size_t)j, "{imdb}");
    while (j < 400) m[j++] = 'a';
    m[j] = 0;
    assert(strlen(m) == 400);
    aviso = ajustes_teste_poster_modelo(m, lido, sizeof lido);
    printf("aviso 40x{imdb}=%d\n", aviso);
    OK(aviso != 0, "400 caracteres com 40 x {imdb} (montado > 512) e recusado ao salvar");
    // Copilot no #408: recusado por TAMANHO, nao pela sintaxe. O aviso de
    // sintaxe ("use http(s):// e {imdb}...") mandaria corrigir o que esta certo.
    { int sintaxe = ajustes_teste_poster_modelo("https://meu.servidor.invalid/x.jpg", lido, sizeof lido);
      printf("aviso sintaxe=%d tamanho=%d\n", sintaxe, aviso);
      OK(sintaxe != 0 && aviso != sintaxe, "longo demais tem aviso proprio, diferente do de sintaxe"); }
    // 400 com um {imdb} so continua valendo e monta.
    j = snprintf(m, sizeof m, "https://posters.exemplo.invalid/FAKEKEY/{imdb}/");
    while (j < 400) m[j++] = 'b';
    m[j] = 0;
    aviso = ajustes_teste_poster_modelo(m, lido, sizeof lido);
    OK(aviso == 0 && !strcmp(lido, m) &&
       posterprov_montar_url(posterprov_cfg(), "tt0111161", 0, "movie", u2, sizeof u2),
       "400 caracteres com um {imdb} e aceito e monta"); }

  // Codex P2 (rodada 2): o pior caso vale so para o que se DIGITA. O modelo que
  // uma versao anterior gravou (< 300, marcadores conhecidos) e lido de volta
  // mesmo que o pior caso passe de 512: 299 caracteres com 13 x {imdb} = 520 no
  // pior caso, 338 com tt0111161 — monta.
  { char m[PP_MODELO_MAX], arq[PP_MODELO_MAX + 16], u3[PP_URL_MAX];
    int j = snprintf(m, sizeof m, "https://posters.exemplo.invalid/");
    int i;
    for (i = 0; i < 13; i++) j += snprintf(m + j, sizeof m - (size_t)j, "{imdb}");
    while (j < 299) m[j++] = 'c';
    m[j] = 0;
    snprintf(arq, sizeof arq, "modelo=%s\n", m);
    assert(dados_gravar("posteres.txt", arq));
    ajustes_teste_poster_recarregar(lido, sizeof lido);
    OK(!strcmp(lido, m), "modelo gravado por versao anterior (pior caso > 512) e mantido ao abrir");
    { PosterProvCfg c;
      memset(&c, 0, sizeof c);
      c.prov = PP_MODELO;
      snprintf(c.modelo, sizeof c.modelo, "%s", lido);
      OK(posterprov_montar_url(&c, "tt0111161", 0, "movie", u3, sizeof u3) && strlen(u3) == 338,
         "e ele monta (338 caracteres com tt0111161)"); } }

  if (falhas) { printf("%d falha(s)\n", falhas); return 1; }
  printf("tudo ok\n");
  return 0;
}
