// Teste de POST /livetv/categorias (src/livetvcat.js) em Node puro: o Jev e o
// KV sao dubles, nada sai para a rede.
//
//   node servidor/recomendacoes/teste-livetvcat.mjs
import assert from "node:assert/strict";
import { rotaLiveTvCategorias, nomeBase, NOMES_MAX, JEV_POR_PEDIDO } from "./src/livetvcat.js";

const ok = (nome, cond) => { assert.ok(cond, nome); console.log("ok", nome); };

ok("nomeBase: tira marca de qualidade e caixa", nomeBase("Canal Foo HD") === "canal foo" && nomeBase("[FHD] Canal-Foo 4K") === "canal foo");
ok("nomeBase: HDMI nao perde o HD do meio da palavra", nomeBase("HDMI Canal") === "hdmi canal");

// Jev duble: resposta por nome-base (cat, conf); registra o que recebeu.
let chamadas;
const jev = (tab) => async (url, opts) => {
  const corpo = JSON.parse(opts.body);
  chamadas.push({ url, auth: opts.headers.authorization, corpo });
  const base = corpo.state.replace("TV channel name: ", "");
  const [cat, conf] = tab[base] || ["Outros", 2];
  return new Response(JSON.stringify({ answers: { categoria: { choice: cat }, confianca: { score: conf } } }));
};
const kv = () => { const m = new Map(); return { m, get: async (k) => m.get(k) ?? null, put: async (k, v) => { m.set(k, v); } }; };
const pedido = (nomes) => new Request("https://x/livetv/categorias", { method: "POST", body: JSON.stringify({ nomes }) });
const TAB = { "canal foo": ["Esportes", 2], "canal bar": ["Filmes", 0.4], "canal baz": ["Outros", 2] };

chamadas = [];
const env = { TYPESAFE_KEY: "k", CATS: kv() };
let r = await rotaLiveTvCategorias(pedido(["Canal Foo HD", "Canal Bar", "Canal Baz", "canal foo"]), env, jev(TAB));
let j = await r.json();
ok("200 com secao acima do limiar", r.status === 200 && j.cats["canal foo"] === "Esportes");
ok("abaixo do limiar vira null", j.cats["canal bar"] === null);
ok("'Outros' vira null", j.cats["canal baz"] === null);
ok("uma consulta por nome-base (duplicata some)", chamadas.length === 3);
ok("a chave vai no Bearer e a URL e a do Jev", chamadas[0].auth === "Bearer k" && chamadas[0].url === "https://api.typesafe.ai/v1/systemone");

chamadas = [];
r = await rotaLiveTvCategorias(pedido(["Canal Foo", "Canal Bar", "Canal Baz"]), env, jev(TAB));
j = await r.json();
ok("cache: segundo pedido nao chama o Jev", chamadas.length === 0 && j.cats["canal foo"] === "Esportes" && j.cats["canal bar"] === null);

chamadas = [];
r = await rotaLiveTvCategorias(pedido(["Canal Novo"]), env, jev({ "canal novo": ["Notícias", 1.2] }));
ok("limiar: 1.2 passa", (await r.json()).cats["canal novo"] === "Notícias" && chamadas.length === 1);

// Jev fora: ausente da resposta e nada cacheado
const env2 = { TYPESAFE_KEY: "k", CATS: kv() };
r = await rotaLiveTvCategorias(pedido(["Canal Foo"]), env2, async () => new Response("x", { status: 500 }));
ok("Jev 500: nome ausente e sem cache", r.status === 200 && !("canal foo" in (await r.json()).cats) && env2.CATS.m.size === 0);

// teto de consultas por pedido
chamadas = [];
const muitos = Array.from({ length: 100 }, (_, i) => "Canal " + i);
r = await rotaLiveTvCategorias(pedido(muitos), { TYPESAFE_KEY: "k", CATS: kv() }, jev({}));
ok("teto de consultas ao Jev por pedido", chamadas.length === JEV_POR_PEDIDO);

// validacao, chave, limite
ok("mais de 200 nomes: 400", (await rotaLiveTvCategorias(pedido(Array(NOMES_MAX + 1).fill("a")), env, jev(TAB))).status === 400);
ok("nome nao-string: 400", (await rotaLiveTvCategorias(pedido([1]), env, jev(TAB))).status === 400);
ok("sem TYPESAFE_KEY: 503", (await rotaLiveTvCategorias(pedido(["a"]), { CATS: kv() }, jev(TAB))).status === 503);
ok("limite por IP estourado: 429", (await rotaLiveTvCategorias(pedido(["a"]), env, jev(TAB), async () => false)).status === 429);
console.log("tudo ok");
