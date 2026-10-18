// #205: the Tizen shell picks the wasm heap (256 -> 192 -> 160 MiB) and turns
// the pool's repeated memory errors into one action.
const fs = require('fs');
const assert = require('assert');
const html = fs.readFileSync(__dirname + '/../tools/tizen-shell.html', 'utf8');
const m = /\/\/ NV-MEM-INICIO\n([\s\S]*?)\/\/ NV-MEM-FIM/.exec(html);
assert(m, 'NV-MEM block missing from tizen-shell.html');
const { nvEscolherHeap, nvAcaoErroHeap, nvHeapAbaixo } = new Function(
  m[1] + '; return { nvEscolherHeap, nvAcaoErroHeap, nvHeapAbaixo };')();

// WebAssembly.Memory mock: throws above limitMiB, records every attempt.
function mockW(limitMiB, tries) {
  return { Memory: function (d) {
    tries.push(d.initial / 16);
    assert(d.shared === true && d.initial === d.maximum, 'shared, fixed-size memory');
    if (d.initial / 16 > limitMiB) throw new RangeError('WebAssembly.Memory(): could not allocate memory');
    this.pages = d.initial;
  } };
}
function pick(limit, dev, teto) { const t = []; const r = nvEscolherHeap(mockW(limit, t), dev, teto || 0); return { r, t }; }

let c = pick(1024, 2);                       // 1.5 GB+ TV: unchanged 256 MiB, pool 20
assert.deepStrictEqual([c.r.mib, c.r.pool, c.t], [256, 20, [256]]);
c = pick(200, 2);                            // 256 fails, 192 fits
assert.deepStrictEqual([c.r.mib, c.r.pool, c.t], [192, 12, [256, 192]]);
c = pick(170, 2);
assert.deepStrictEqual([c.r.mib, c.r.pool], [160, 8]);
c = pick(1024, 1);                           // 1 GB TV: never even tries 256
assert.deepStrictEqual([c.r.mib, c.r.pool, c.t], [192, 12, [192]]);
c = pick(1024, 1, 160);                      // stored ceiling after a worker failure
assert.deepStrictEqual([c.r.mib, c.r.pool, c.t], [160, 8, [160]]);
c = pick(100, 1);                            // nothing fits: no memory, error kept
assert(c.r.mib === 0 && !c.r.mem && /could not allocate/.test(c.r.erro.message));
assert.deepStrictEqual(c.t, [192, 160]);

// Worker storm: 17 identical errors -> exactly one action, rest ignored.
let st = { mib: 192, tratado: false };
const msg = 'Uncaught RangeError: WebAssembly.Memory(): could not allocate memory';
const acoes = [];
for (let i = 0; i < 17; i++) acoes.push(nvAcaoErroHeap(msg, st));
assert.strictEqual(acoes[0], 'baixar');
assert(acoes.slice(1).every((a) => a === 'dup'));
assert.strictEqual(nvHeapAbaixo(192), 160);
// Lowest step (or no heap at all): show the message once.
st = { mib: 160, tratado: false };
assert.deepStrictEqual([nvAcaoErroHeap(msg, st), nvAcaoErroHeap(msg, st)], ['mostrar', 'dup']);
st = { mib: 0, tratado: false };
assert.strictEqual(nvAcaoErroHeap('could not allocate memory', st), 'mostrar');
assert.strictEqual(nvAcaoErroHeap('Uncaught TypeError: x', { mib: 192, tratado: false }), null);
console.log('PASS tizen-heap-fallback');
