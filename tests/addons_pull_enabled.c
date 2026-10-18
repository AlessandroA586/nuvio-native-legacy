// Exercise the production account parser, pull merge and applied cache.
// Existing sync fixture supplies disk and addon-list doubles; no network.
#define main syncaddons_fixture_main
#include "syncaddons.c"
#undef main
#include "../src/sync.c"
#include <assert.h>

static void pull(const char *json) {
  nAddonsRem = lerAddons(json);
  addonsAplicar();
}
static void expect(const char *label, const char *letters) {
  char actual[40];
  tvLetras(actual, sizeof actual);
  if (strcmp(actual, letters)) {
    fprintf(stderr, "FAIL %s: got %s, expected %s\n", label, actual, letters);
    exit(1);
  }
}
int main(void) {
  // Synthetic account-table rows, with only placeholder identities/endpoints.
  // enabled:false reproduces the suspected web-default failure; other shapes exercise
  // compatibility. This is not a captured response from the reporter's account.
  static const char web[] =
    "[{\"user_id\":\"fixture\",\"profile_id\":1,\"sort_order\":0,"
      "\"name\":\"B\",\"url\":\"https://b.EXEMPLO/manifest.json\",\"enabled\":false},"
     "{\"sort_order\":1,\"name\":\"A\",\"url\":\"https://a.exemplo/manifest.json\",\"enabled\":false},"
     "{\"sort_order\":2,\"name\":\"C\",\"url\":\"https://c.exemplo/manifest.json\"},"
     "{\"sort_order\":3,\"name\":\"D\",\"url\":\"https://d.exemplo/manifest.json\",\"enabled\":{\"stream\":false}},"
     "{\"sort_order\":4,\"name\":\"E\",\"url\":\"https://e.exemplo/manifest.json\",\"enabled\":false}]";
  snprintf(usuarioDoCiclo, sizeof usuarioDoCiclo, "conta-a");
  addonsPerfilCiclo = 1;
  copiaGeracaoDoCiclo = contacache_geracao();
  tvAcrescentar('A'); tvAcrescentar('B'); tvAcrescentar('C');
  tvAcrescentar('D'); tvAcrescentar('X');
  // Production export returns normalized bases, not manifest endpoints.
  for (int i = 0; i < nTv; i++) {
    char base[NV_ADDON_URL_MAX];
    nv_addon_base(tv[i].url, base, sizeof base);
    snprintf(tv[i].url, sizeof tv[i].url, "%s", base);
  }
  pull(web);
  expect("web false preserves local ON; remote add/remove/order", "BACDe");
  int applied = aplicacoes;
  pull(web);
  confere("identical merged pull does not reinstall", aplicacoes == applied);
  // Explicit local OFF survives the next sync, even if remote defaults ON.
  tv[1].ativo = 0;
  pull("[{\"name\":\"A\",\"url\":\"https://a.exemplo/manifest.json\",\"enabled\":true},"
        "{\"name\":\"B\",\"url\":\"https://b.exemplo/manifest.json\"}]");
  expect("local OFF and remote removal", "aB");
  // Missing, null, string, numeric and nested flags must not change local OFF.
  const char *flags[] = {"", ",\"enabled\":null", ",\"enabled\":\"false\"",
                         ",\"enabled\":0", ",\"enabled\":{\"stream\":true}"};
  for (unsigned i = 0; i < sizeof flags / sizeof flags[0]; i++) {
    char json[512];
    snprintf(json, sizeof json,
             "[{\"url\":\"https://a.exemplo/manifest.json\"%s},"
              "{\"url\":\"https://b.exemplo/manifest.json\",\"enabled\":false}]", flags[i]);
    pull(json);
    expect("unknown flags preserve local toggles", "aB");
  }
  char *saved = contacache_ler(CC_ADDONS_APLICADOS, 1, usuarioDoCiclo, NULL);
  assert(saved);
  nTv = 0;
  nAddonsRem = lerAddons(saved);
  addons_definir_lista(addonsRem, nAddonsRem);
  free(saved);
  expect("restart restores merged ON and explicit local OFF", "aB");
  // Removing an addon also removes its local preference; reinstall is remote.
  pull("[{\"url\":\"https://b.exemplo/manifest.json\",\"enabled\":true}]");
  pull("[{\"url\":\"https://a.exemplo/manifest.json\",\"enabled\":true},"
        "{\"url\":\"https://b.exemplo/manifest.json\",\"enabled\":false}]");
  expect("reinstall uses remote state; remaining ON stays ON", "AB");
  assert(!falhas);
  puts("PASS: production addon pull preserves local toggles and remote membership/order");
  return 0;
}
