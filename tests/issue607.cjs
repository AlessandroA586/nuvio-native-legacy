'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const cp = require('node:child_process');
const vm = require('node:vm');

// Exercise the C stringification used by EM_JS, including compiler escapes.
function bridge(file, name) {
  const source = fs.readFileSync(file, 'utf8');
  const match = source.match(new RegExp('EM_JS\\([^,]+, ' + name + ',[\\s\\S]*?\\n\\}\\);'));
  assert(match, name);
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'issue607-'));
  try {
    fs.writeFileSync(path.join(dir, 'bridge.c'), '#include <stdio.h>\n' +
      '#define EM_JS(ret,name,args,...) const char *body = #__VA_ARGS__;\n' +
      match[0] + '\nint main(void) { puts(body); }\n');
    cp.execFileSync('cc', [path.join(dir, 'bridge.c'), '-o', path.join(dir, 'bridge')], {stdio:'pipe'});
    return cp.execFileSync(path.join(dir, 'bridge'), {encoding:'utf8'}).trim().slice(1, -1);
  } finally { fs.rmSync(dir, {recursive:true, force:true}); }
}

function probe() {
  const heap = new Int32Array(32);
  let length = '9';
  class XHR {
    open() {} setRequestHeader() {}
    send() { this.status = 200; this.responseURL = 'https://fixture.invalid/final'; }
    getResponseHeader(key) { return key === 'Content-Length' ? length : null; }
  }
  const context = {XMLHttpRequest:XHR, HEAP32:heap, UTF8ToString:x=>x, stringToUTF8() {}};
  const run = vm.runInNewContext('(function(url,cabs,dst,tam,status,mime,mimeTam,corpo,prazo) {' +
    bridge('src/rede.c', 'nv_url_sonda') + '})', context);
  for (const [raw, expected] of [['9',9], [' 64 ',64], ['65',-1], ['ddd',-1], ['9x',-1], [null,-1]]) {
    length = raw;
    assert.equal(run('https://fixture.invalid',0,8,256,4,0,0,12,1000), 1);
    assert.equal(heap[3], expected, 'Content-Length ' + raw);
  }
  console.log('PASS: generated probe validates numeric Content-Length');
}

const cases = {probe};
(async () => {
  const selected = process.argv.slice(2);
  for (const name of selected.length ? selected : Object.keys(cases)) await cases[name]();
})().catch(error => { console.error(error); process.exitCode = 1; });
