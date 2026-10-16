// Player setting + stream headers -> internal / external / ask.
#include "extplayer.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  int avisar;
  avisar = 9; assert(extplayer_decidir(0, "", &avisar) == EXTPLAYER_INTERNO && !avisar);
  avisar = 9; assert(extplayer_decidir(0, "Referer: x\r\n", &avisar) == EXTPLAYER_INTERNO && !avisar);
  avisar = 9; assert(extplayer_decidir(1, "", &avisar) == EXTPLAYER_EXTERNO && !avisar);
  avisar = 9; assert(extplayer_decidir(1, NULL, &avisar) == EXTPLAYER_EXTERNO && !avisar);
  avisar = 9; assert(extplayer_decidir(2, "", &avisar) == EXTPLAYER_PERGUNTAR && !avisar);
  // Headers cannot ride an Intent: fall back to internal and tell the user.
  avisar = 0; assert(extplayer_decidir(1, "User-Agent: x\r\n", &avisar) == EXTPLAYER_INTERNO && avisar);
  avisar = 0; assert(extplayer_decidir(2, "User-Agent: x\r\n", &avisar) == EXTPLAYER_INTERNO && avisar);
  // Out-of-range value acts as Internal.
  avisar = 9; assert(extplayer_decidir(7, "", &avisar) == EXTPLAYER_INTERNO && !avisar);
  assert(extplayer_decidir(1, "", NULL) == EXTPLAYER_EXTERNO);
  puts("extplayer_decisao: ok");
  return 0;
}
