// #382: exercita os pedidos reais sem rede, com curl e relogio falsos.
#include <assert.h>
#include <stdarg.h>
#include <time.h>
static unsigned relogio = 1000;
static int clockFalso(clockid_t id, struct timespec *t) {
  (void)id; t->tv_sec = relogio / 1000; t->tv_nsec = (relogio % 1000) * 1000000;
  return 0;
}
#define clock_gettime clockFalso
#include "../src/rede.c"
#include "../src/redesaude.c"

typedef struct {
  size_t (*write)(void *, size_t, size_t, void *);
  void *data;
} Fake;
static int chamadas, notas, erro, http, recuperar;
static void *init(void) { return calloc(1, sizeof(Fake)); }
static void cleanup(void *c) { free(c); }
static int option(void *c, int o, ...) {
  va_list ap;
  va_start(ap, o);
  if (o == OPT_WRITEFUNCTION) ((Fake *)c)->write = va_arg(ap, size_t (*)(void *, size_t, size_t, void *));
  if (o == OPT_WRITEDATA) ((Fake *)c)->data = va_arg(ap, void *);
  va_end(ap); return 0;
}
static int perform(void *c) {
  Fake *f = c;
  chamadas++;
  if (recuperar && chamadas == 2) {
    // Duas falhas finais anteriores, mas este pedido ainda pode recuperar.
    assert(!rede_saude_offline());
    http = 200;
    f->write("{}", 1, 2, f->data);
    return 0;
  }
  relogio += 2000;
  return erro;
}
static int info(void *c, int o, ...) {
  va_list ap;
  (void)c;
  va_start(ap, o);
  if (o == INFO_RESPONSE_CODE) *va_arg(ap, long *) = http;
  if (o == INFO_NUM_CONNECTS) *va_arg(ap, long *) = 0;
  va_end(ap); return 0;
}
static void nota(int codigo, const char *url) {
  notas++;
  rede_saude_nota(codigo, url);
}
static void reset(void) {
  memset(&global, 0, sizeof global);
  chamadas = notas = http = recuperar = 0;
  relogio = 1000;
}
static void falhasAnteriores(void) {
  rede_saude_nota(6, "https://a.example/meta");
  relogio += 2000;
  rede_saude_nota(6, "https://b.example/meta");
}
int main(int argc, char **argv) {
  char *r;
  int st;
  pronto = 1; curl_init = init; curl_cleanup = cleanup;
  curl_setopt_f = option; curl_perform = perform; curl_getinfo = info;
  rede_avisar_saude(nota);
  reset(); falhasAnteriores(); erro = 28;
  if (argc > 1 && !strcmp(argv[1], "http")) {
    // HTTP recebido + corpo lento continua falha do pedido, mas ha internet.
    http = 503;
    r = rede_postar_st("https://c.example/meta", 10, NULL, "{}", &st);
    assert(!r && chamadas == 1 && notas == 1);
    assert(!rede_saude_offline());
    assert(global.falhas == 0);
    puts("PASS: resposta HTTP com curl28 nao declara offline.");
    return 0;
  }
  recuperar = 1;
  r = rede_baixar_st("https://c.example/meta", 10, NULL, &st);
  assert(r && st == 200 && chamadas == 2 && notas == 1);
  free(r);
  assert(!rede_saude_offline() && rede_saude_seq() == 0);

  // Sem resposta de varios hosts: resultados finais ainda declaram offline.
  reset(); falhasAnteriores(); erro = 28;
  r = rede_baixar_st("https://c.example/meta", 10, NULL, &st);
  assert(!r && st == 0 && chamadas == 2 && notas == 1);
  assert(rede_saude_offline() && rede_saude_seq() == 1);
  // HTTP4xx/5xx basta para reconhecer a volta, mesmo sem corpo completo.
  erro = 56; http = 503;
  r = rede_postar_st("https://c.example/meta", 10, NULL, "{}", &st);
  assert(!r);
  assert(!rede_saude_offline() && rede_saude_seq() == 2);
  puts("PASS: retry recuperado, falhas finais distintas e volta por HTTP.");
  return 0;
}
