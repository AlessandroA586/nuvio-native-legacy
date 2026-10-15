const fs = require('node:fs');
const vm = require('node:vm');
const cp = require('node:child_process');
const assert = require('node:assert/strict');
const source = fs.readFileSync('src/rede.c', 'utf8');
const probe = source.match(/int rede_url_final_vidaa\([^]*?\n\}/)[0];
fs.mkdirSync('build/tests-vidaa-review', {recursive:true});
fs.writeFileSync('build/tests-vidaa-review/probe.c', `
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static int response;
static char *nv_http(const char *m,const char *u,const char *c,void *b,int *n,int *s,char *d,int t,void *e,int et,void *r) {
assert(!strcmp(m,"HEAD")); assert(!c);
*s=response; if (!response) return NULL; strcpy(d,"https://media/final"); return malloc(1);
}
${probe}
int main(void) { char dst[128];
int statuses[]={200,206,403,404,405,500,501,0};
for (int i=0;i<8;i++) { response=statuses[i]; assert(rede_url_final_vidaa("https://media",1,dst,sizeof dst)==(response==0||response==405||response==501?-1:response<300?1:0)); }
}
`);
let failures = 0;
try {
  cp.execFileSync('cc', ['build/tests-vidaa-review/probe.c','-o','build/tests-vidaa-review/probe']);
  cp.execFileSync('build/tests-vidaa-review/probe', {stdio:'pipe'});
  console.log('PASS: HTTP probe rejects completed error responses');
} catch(e) { failures++; console.error('FAIL: HTTP probe rejects completed error responses'); }
try {
  const shell = fs.readFileSync('tools/vidaa-shell.html','utf8');
  const fallback = shell.slice(shell.indexOf("  if (NV_MODO === 'mt'"), shell.indexOf('  // DIAGNOSTICO'));
  let redirected;
  vm.runInNewContext(fallback, {NV_MODO:'mt',SharedArrayBuffer:undefined,self:{},localStorage:{setItem(){throw Error('blocked');}},location:{pathname:'/app/mt/index.html',search:'?x=1',replace(v){redirected=v;}},console:{log(){}}});
  assert.equal(redirected, '/app/st/index.html?x=1');
  console.log('PASS: MT fallback survives blocked storage');
} catch(e) { failures++; console.error('FAIL: MT fallback survives blocked storage'); }
process.exitCode = failures ? 1 : 0;
