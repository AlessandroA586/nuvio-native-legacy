// Settings "Tamanho do buffer" -> MiB for the next open, with the free-RAM clamp.
#include "../src/cacheboost.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  // Auto stays Auto (backend default), whatever the memory.
  assert(cacheboost_buffer_mb(0, 4096) == 0);
  assert(cacheboost_buffer_mb(0, -1) == 0);
  // Plenty of RAM: the option as asked.
  assert(cacheboost_buffer_mb(1, 4096) == 64);
  assert(cacheboost_buffer_mb(2, 4096) == 128);
  assert(cacheboost_buffer_mb(3, 4096) == 256);
  // LG C5 report: ~500 MiB free before the buffer -> 256 fits above the floor.
  assert(cacheboost_buffer_mb(3, 500) == 256);
  // Tight RAM steps down the ladder, never below the floor.
  assert(cacheboost_buffer_mb(3, 447) == 128);
  assert(cacheboost_buffer_mb(3, 320) == 128);
  assert(cacheboost_buffer_mb(3, 300) == 64);
  assert(cacheboost_buffer_mb(2, 255) == 0);
  assert(cacheboost_buffer_mb(1, 256) == 64);
  // Unknown memory or out-of-range option -> Auto.
  assert(cacheboost_buffer_mb(3, -1) == 0);
  assert(cacheboost_buffer_mb(4, 4096) == 0 && cacheboost_buffer_mb(-1, 4096) == 0);
  puts("buffer_tamanho: ok");
  return 0;
}
