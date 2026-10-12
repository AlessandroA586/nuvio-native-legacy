// Token recusado nao volta ao Supabase: node servidor/recomendacoes/teste-sessao.mjs
const src = new URL("./src/index.js", import.meta.url).href;
const m = await import(src);
const w = m.default;
const db = new Map(); let chamadas = 0;
const stmt = (sql) => ({ bind: (...a) => ({
  first: async () => { if (sql.startsWith("SELECT id, nome FROM sessao")) { const r = db.get(a[0]); return r && r.expira > a[1] ? r : null; } return null; },
  run: async () => { if (sql.startsWith("INSERT INTO sessao")) { if (a.length === 2) db.set(a[0], { id: "", nome: "", expira: a[1] }); else db.set(a[0], { id: a[1], nome: a[2], expira: a[3] }); } return {}; },
  all: async () => ({ results: [] }) }) });
const env = { DB: { prepare: stmt, batch: async () => [] }, SUPABASE_URL: "https://sb", SUPABASE_ANON_KEY: "k" };
globalThis.fetch = async (u) => { chamadas++; return new Response("{}", { status: 401 }); };
const jwt = (exp) => "h." + Buffer.from(JSON.stringify({ exp })).toString("base64url") + ".s";
const pede = (tok) => w.fetch(new Request("https://x/v1/amigos/atividade", { headers: { authorization: "Bearer " + tok, "x-nuvio-auth": "nuvio" } }), env, { waitUntil() {} });
const now = Math.floor(Date.now() / 1000);
for (let i = 0; i < 5; i++) await pede(jwt(now - 60));
console.log("vencido x5 -> fetch", chamadas);
for (let i = 0; i < 5; i++) await pede("lixo");
console.log("ilegivel x5 -> fetch", chamadas);
for (let i = 0; i < 5; i++) { const r = await pede(jwt(now + 3600)); if (i == 0) console.log("status", r.status); }
if (chamadas !== 1) { console.log("FALHOU: esperava 1 chamada, houve", chamadas); process.exit(1); }
console.log("ok");
