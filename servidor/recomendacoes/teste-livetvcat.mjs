// Workers AI and KV doubles only; no network or credentials.
import assert from "node:assert/strict";
import { rotaLiveTvCategorias, nomeBase, NOMES_MAX, CONFIANCA_MIN, CACHE_SEGUNDOS, SECOES } from "./src/livetvcat.js";

const ok = (name, cond) => { assert.ok(cond, name); console.log("ok", name); };
const request = (nomes) => new Request("https://x/livetv/categorias", {
  method: "POST", headers: { "cf-connecting-ip": "192.0.2.1" }, body: JSON.stringify({ nomes }),
});
const calls = [], writes = [];
const values = new Map();
const tab = { "canal foo": ["Esportes", 0.95], "canal bar": ["Filmes", 0.4], "canal baz": ["Outros", 1],
  "boundary": ["Notícias", CONFIANCA_MIN], "invalid": ["constructor", 1], "missing": ["Filmes", undefined],
  "too high": ["Filmes", 2] };
const env = {
  CATS: { get: async (k) => values.get(k) ?? null,
    put: async (k, v, opts) => { values.set(k, v); writes.push(opts); } },
  AI: { run: async (model, input) => {
    calls.push(input);
    assert.equal(model, "typesafe/jev");
    assert.ok(Object.keys(input.questions).length <= 64);
    const names = JSON.parse(input.state);
    const answers = {};
    for (const [key, question] of Object.entries(input.questions)) {
      assert.equal(question.type, "choice");
      assert.deepEqual(question.criteria, SECOES);
      assert.ok(question.instructions.includes(key));
      const [choice, confidence] = tab[names[key]] ?? ["Outros", 1];
      answers[key] = { choice, confidence, probabilities: {} };
    }
    return { answers };
  } },
};
// Any accidental external fetch must fail the test.
globalThis.fetch = () => { throw new Error("external fetch forbidden"); };
let limitArgs;
const limit = async (...args) => { limitArgs = args; return true; };
const route = (names, e = env, l = limit) => rotaLiveTvCategorias(request(names), e, l, 1234);

ok("normalization matches quality and word boundaries", nomeBase("[FHD] Canal-Foo 4K") === "canal foo" && nomeBase("HDMI Canal") === "hdmi canal");
let r = await route(["Canal Foo HD", "Canal Bar", "Canal Baz", "canal foo", "boundary", "invalid", "missing", "too high"]);
let cats = (await r.json()).cats;
ok("cache misses use Workers AI, one question per base name", r.status === 200 && calls.length === 1 && Object.keys(calls[0].questions).length === 7);
ok("confident choice creates section", cats["canal foo"] === "Esportes");
ok("threshold, Others and invalid answers", cats["canal bar"] === null && cats["canal baz"] === null && cats.boundary === "Notícias" && cats.invalid === null && !("missing" in cats) && !("too high" in cats));
ok("KV TTL is 30 days", writes.length === 5 && writes.every((o) => o.expirationTtl === CACHE_SEGUNDOS && CACHE_SEGUNDOS === 2592000));
ok("existing IP limiter receives route prefix and limits", limitArgs[1] === "livetvcat:192.0.2.1" && limitArgs[2] === 12 && limitArgs[3] === 600000 && limitArgs[4] === 1234);

calls.length = 0;
r = await route(["Canal Foo FHD", "Canal Bar", "Canal Baz"]);
cats = (await r.json()).cats;
ok("cache hits, including null, skip AI", calls.length === 0 && cats["canal foo"] === "Esportes" && cats["canal bar"] === null);
r = await route(["Canal Foo", "new channel"]);
ok("mixed request only asks missing name", calls.length === 1 && Object.values(JSON.parse(calls[0].state)).join() === "new channel");

calls.length = 0;
r = await route(Array.from({ length: 200 }, (_, i) => "Channel " + i));
ok("200 names processed in batches of at most 64", Object.keys((await r.json()).cats).length === 200 && calls.length === 4);
const broken = { ...env, AI: { run: async () => { throw new Error("offline"); } } };
r = await route(["offline channel"], broken);
ok("AI failure stays absent and uncached", r.status === 200 && !("offline channel" in (await r.json()).cats) && !values.has("c:offline channel"));
r = await route(["no answer"], { ...env, AI: { run: async () => ({ answers: {} }) } });
ok("missing answer stays uncached", !("no answer" in (await r.json()).cats) && !values.has("c:no answer"));
for (const bad of [Array(NOMES_MAX + 1).fill("a"), [1], ["x".repeat(201)], ["https://host/secret"], ["//host/path"], ["www.host.tv/path"], ["bad\nname"]])
  ok("invalid names rejected: " + JSON.stringify(bad).slice(0, 50), (await route(bad)).status === 400);
ok("malformed body rejected", (await rotaLiveTvCategorias(new Request("https://x", { method: "POST", body: "{" }), env)).status === 400);
ok("missing AI binding returns 503", (await route(["a"], { CATS: env.CATS })).status === 503);
ok("missing cache binding returns 503", (await route(["a"], { AI: env.AI })).status === 503);
ok("rate limit returns 429", (await route(["a"], env, async () => false)).status === 429);
console.log("livetvcat: all passed");
