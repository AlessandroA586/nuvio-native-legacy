// A live player's currentPosition is relative to a moving window, not monotonic.
#include <assert.h>
#include <stdio.h>
#include "../src/livestall.h"

static int rolling(unsigned window, int resumed) {
  NvLiveStall s; nv_ls_zerar(&s);
  if(resumed){s.tentativa=2;nv_ls_reaberto(&s,1000);}
  for(unsigned elapsed=0;elapsed<=120000;elapsed+=250) {
    double position=20.0+(elapsed%window)/1000.0;
    int result=nv_ls_passo(&s,1000+elapsed,position,1,0,0,0);
    if(result!=NV_LS_NADA) {
      fprintf(stderr,"FAIL: healthy rolling window=%u ms, elapsed=%u ms, position=%.2f triggered restart=%d\n",window,elapsed,position,result);
      return 0;
    }
  }
  assert(s.tentativa==0); // continuous playback also earns the ordinary retry reset
  return 1;
}
int main(void) {
  const unsigned windows[]={4000,6000,10000,15000,30000};
  for(unsigned i=0;i<sizeof windows/sizeof windows[0];i++) {
    if(!rolling(windows[i],0)||!rolling(windows[i],1))return 1;
  }
  NvLiveStall s;nv_ls_zerar(&s);
  assert(nv_ls_passo(&s,1000,20,1,0,0,0)==NV_LS_NADA);
  assert(nv_ls_passo(&s,2000,21,1,0,0,0)==NV_LS_NADA);
  // Rebase alone is NOT proof of playback. A truly frozen stream still times out
  // eight seconds from its last forward movement, even after the window moved.
  for(unsigned t=3000;t<10000;t+=250)assert(nv_ls_passo(&s,t,11,1,0,0,0)==NV_LS_NADA);
  assert(nv_ls_passo(&s,10000,11,1,0,0,0)==NV_LS_AGENDOU);
  nv_ls_zerar(&s);nv_ls_passo(&s,1000,100,1,0,0,0);
  for(unsigned t=2000;t<9000;t+=1000)assert(nv_ls_passo(&s,t,101-t/1000.0,1,0,0,0)==NV_LS_NADA);
  assert(nv_ls_passo(&s,9000,92,1,0,0,0)==NV_LS_AGENDOU);
  nv_ls_zerar(&s);nv_ls_passo(&s,1000,20,1,0,0,0);
  // Explicit buffering and EOF keep their existing meanings after a rebase.
  assert(nv_ls_passo(&s,2000,10,1,0,0,8000)==NV_LS_AGENDOU);
  nv_ls_zerar(&s);nv_ls_passo(&s,1000,20,1,0,0,0);
#ifdef NV_TPK
  assert(nv_ls_passo(&s,2000,10,1,0,1,0)==NV_LS_REABRIR);
#else
  assert(nv_ls_passo(&s,2000,10,1,0,1,0)==NV_LS_AGENDOU);
#endif
  nv_ls_zerar(&s);nv_ls_passo(&s,1000,20,1,0,0,0);
  for(unsigned t=2000;t<60000;t+=1000)assert(nv_ls_passo(&s,t,10,1,1,0,60000)==NV_LS_NADA);
  puts("PASS: sliding live windows keep playing; actual stalls, buffering, EOF, pause and retry limits remain protected");
}
