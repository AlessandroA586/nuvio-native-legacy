// O JS de verdade do window.nvTexto (tools/tizen-shell.html), num DOM fingido:
// deteccao do microfone, abrir/fechar e o que cada tecla do teclado da Samsung
// entrega ao C (fim 1 = entra no campo, 0 = cancelado).
//   node tests/wgt_voz_shell.js   (chamado por tests/wgt_voz.sh)
"use strict";
const fs = require("fs");
const path = require("path");
const assert = require("assert");
const html = fs.readFileSync(path.join(__dirname, "../tools/tizen-shell.html"), "utf8");
const ini = html.indexOf("  // TECLADO DA SAMSUNG PARA O BOTAO DE MICROFONE DAS BUSCAS");
assert(ini > 0, "bloco do nvTexto nao achado no shell");
const fimBloco = html.indexOf("\n  })();", ini);
const codigo = html.slice(ini, fimBloco + "\n  })();".length);

function montar(capacidade) {
  const ouv = {};
  const el = { value: "", focado: false, ouv: {},
    addEventListener(t, f) { (this.ouv[t] = this.ouv[t] || []).push(f); },
    focus() { this.focado = true; },
    blur() { if (!this.focado) return; this.focado = false; (this.ouv.blur || []).forEach(f => f({})); },
    setSelectionRange() {} };
  const canvas = { focus() {} };
  const window = {
    addEventListener(t, f, cap) { (ouv[t] = ouv[t] || []).push(f); },
    tizen: capacidade === "lanca" ? { systeminfo: { getCapability() { throw new Error("NotSupported"); } } }
         : capacidade === undefined ? undefined
         : { systeminfo: { getCapability(k) { assert.strictEqual(k, "http://tizen.org/feature/microphone"); return capacidade; } } },
  };
  const document = { getElementById: id => id === "nvTextoCampo" ? el : id === "canvas" ? canvas : null };
  const logs = [];
  const console = { log: s => logs.push(s), warn: s => logs.push(s) };
  new Function("window", "document", "console", "tizen", codigo)(window, document, console, window.tizen);
  (ouv.load || []).forEach(f => f({}));
  const tecla = (alvo, keyCode) => {
    const ev = { keyCode, preventDefault() {}, stopPropagation() {}, stopImmediatePropagation() {} };
    if (alvo === "janela") (ouv.keydown || []).forEach(f => f(ev));
    else (el.ouv.keydown || []).forEach(f => f(ev));
  };
  const digita = v => { el.value = v; (el.ouv.input || []).forEach(f => f({})); };
  return { t: window.nvTexto, el, tecla, digita, logs };
}

// Deteccao: getCapability true/false, sem resposta, sem tizen.
assert.strictEqual(montar(true).t.microfone, 1);
assert.strictEqual(montar(false).t.microfone, 0);
assert.strictEqual(montar("lanca").t.microfone, -1);
assert.strictEqual(montar(undefined).t.microfone, -1);

let m = montar(true);
// Abrir: foco no <input> (o teclado da Samsung sobe), campo vazio.
m.t.abrir("", 1);
assert(m.el.focado && m.t.aberto && m.el.value === "" && m.t.fim === -1);
const s0 = m.t.seq;
m.digita("o poderoso chefão");
assert(m.t.seq > s0 && m.t.valor === "o poderoso chefão");
// Concluido (65376): entrega.
m.tecla("campo", 65376);
assert(m.t.fim === 1 && !m.t.aberto && !m.el.focado);

// Voltar (10009, ouvinte de captura da janela): cancela mesmo com texto.
m.t.fim = -1; m.t.abrir("", 1); m.digita("matrix");
m.tecla("janela", 10009);
assert(m.t.fim === 0 && !m.t.aberto);
// Cancel do teclado (65385): cancela.
m.t.fim = -1; m.t.abrir("", 1); m.digita("matrix");
m.tecla("campo", 65385);
assert(m.t.fim === 0);
// O teclado fechou sozinho (blur) depois da fala: o que foi dito vale.
m.t.fim = -1; m.t.abrir("", 1); m.digita("duna");
m.el.blur();
assert(m.t.fim === 1 && m.t.valor === "duna");
// ...e sem nada dito: cancelado.
m.t.fim = -1; m.t.abrir("", 1);
m.el.blur();
assert(m.t.fim === 0);
// O app fechou (Voltar na tela): esconde sem avisar fim.
m.t.fim = -1; m.t.abrir("", 1); m.digita("x");
m.t.fechar();
assert(m.t.fim === -1 && !m.t.aberto && !m.el.focado);
// Fechado, teclas nao fazem nada.
m.tecla("janela", 10009);
assert(m.t.fim === -1);

console.log("PASS: wgt_voz_shell (deteccao do microfone; Concluido entrega, Voltar/Cancel cancelam, fechar sozinho entrega o que foi dito)");
