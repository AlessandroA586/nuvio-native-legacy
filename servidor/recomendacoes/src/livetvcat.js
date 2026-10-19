// POST /livetv/categorias — sugestao de categoria para canais de TV ao vivo que
// a heuristica local do app (guia.c, categoriaPorNome) nao classificou.
//
// O corpo e {nomes:[...]}: SO nomes de canal, ate 200, nunca URL. Cada nome
// vira o nome-base (mesma regra de nv_nome_base em src/livetv_regras.h: sem
// marcas de qualidade, minusculo ASCII) e o cache em KV e por nome-base, 30
// dias. So o que nao esta no cache vai ao Jev (typesafe.ai, Choice + Score).
// A chave TYPESAFE_KEY e segredo deste worker e NUNCA vai ao app.
//
// Resposta: {cats:{<nome-base>: "<secao>" | null}}. null = o Jev respondeu mas
// sem confianca (ou "Outros"): o app deixa o canal sem categoria. Nome ausente
// = nao processado agora (teto de consultas por pedido); nao entra no cache.

export const NOMES_MAX = 200;
export const JEV_POR_PEDIDO = 40;
export const CONFIANCA_MIN = 1.2;          // Score vai de 0 a 2
export const CACHE_SEGUNDOS = 30 * 24 * 3600;
const LIMITE_PEDIDOS = 12, JANELA_MS = 10 * 60 * 1000;
const JEV_URL = "https://api.typesafe.ai/v1/systemone";

// As chaves de secao que categoriaPorNome (guia.c) usa, com a ideia de cada uma.
export const SECOES = {
  "Esportes": "Sports channel: football, motorsport, fighting, tennis, sports news and betting",
  "Notícias": "News channel or weather",
  "Infantil": "Children's channel or cartoons",
  "Documentários": "Documentaries: nature, history, science, crime and investigation",
  "Filmes": "Movie channel",
  "Séries": "TV series and general scripted entertainment channel",
  "Música": "Music, music videos or radio",
  "Variedades": "Lifestyle, reality, cooking, travel, shopping and general entertainment",
  "Abertos": "Free-to-air national or regional broadcaster, religious or educational public TV",
  "Outros": "None of the above, or the name says nothing about the content",
};

const MARCAS = ["4k", "uhd", "2160p", "2160", "fhd", "fullhd", "1080p", "1080i", "1080",
  "hd+", "hd", "720p", "720", "sd", "576p", "576", "480p", "480",
  "hevc", "h265", "h.265", "h264", "h.264", "50fps", "60fps"];
const alnum = (c) => c !== undefined && /[0-9A-Za-z]/.test(c);

// Porta de nv_nome_base (livetv_regras.h); mudou la, muda aqui.
export function nomeBase(nome) {
  const s = String(nome ?? "");
  let o = "", espaco = false, i = 0;
  while (i < s.length) {
    if (i === 0 || !alnum(s[i - 1])) {
      const m = MARCAS.find((k) => s.slice(i, i + k.length).toLowerCase() === k && !alnum(s[i + k.length]));
      if (m) { i += m.length; espaco = true; continue; }
    }
    const c = s[i];
    if (" \t-_()[]".includes(c)) { espaco = true; i++; continue; }
    if (espaco && o) o += " ";
    espaco = false;
    o += /[A-Z]/.test(c) ? c.toLowerCase() : c;
    i++;
  }
  return o;
}

const resp = (dados, status = 200) => new Response(JSON.stringify(dados), {
  status,
  headers: { "content-type": "application/json; charset=utf-8", "access-control-allow-origin": "*" },
});

async function perguntarJev(env, base, buscar) {
  const r = await buscar(JEV_URL, {
    method: "POST",
    headers: { authorization: "Bearer " + env.TYPESAFE_KEY, "content-type": "application/json" },
    body: JSON.stringify({
      state: "TV channel name: " + base, model: "jev-latest",
      questions: {
        categoria: { type: "choice", instructions: "Which section of a TV guide holds this channel", criteria: SECOES },
        confianca: { type: "score", instructions: "How sure is the section, judging by the name alone",
          criteria: ["A guess", "Probable", "Clear, a well-known channel"] },
      },
    }),
  });
  if (!r.ok) throw new Error("jev " + r.status);
  const a = (await r.json()).answers;
  const cat = a?.categoria?.choice, conf = a?.confianca?.score;
  return cat in SECOES && cat !== "Outros" && conf >= CONFIANCA_MIN ? cat : null;
}

export async function rotaLiveTvCategorias(req, env, buscar = fetch, limite, agora = Date.now()) {
  if (!env.TYPESAFE_KEY) return resp({ erro: "sem chave" }, 503);
  const ip = req.headers.get("cf-connecting-ip") || "0.0.0.0";
  if (limite && !(await limite(env, "livetvcat:" + ip, LIMITE_PEDIDOS, JANELA_MS, agora)))
    return resp({ erro: "muitos pedidos" }, 429);
  let nomes;
  try { nomes = (await req.json()).nomes; } catch { return resp({ erro: "json invalido" }, 400); }
  if (!Array.isArray(nomes) || nomes.length > NOMES_MAX || nomes.some((n) => typeof n !== "string" || n.length > 200))
    return resp({ erro: "nomes invalidos" }, 400);

  const bases = [...new Set(nomes.map(nomeBase).filter(Boolean))];
  const cats = {};
  const faltam = [];
  for (const b of bases) {
    const c = await env.CATS?.get("c:" + b);
    if (c === null || c === undefined) faltam.push(b);
    else cats[b] = c === "" ? null : c;
  }
  await Promise.all(faltam.slice(0, JEV_POR_PEDIDO).map(async (b) => {
    try {
      cats[b] = await perguntarJev(env, b, buscar);
      await env.CATS?.put("c:" + b, cats[b] ?? "", { expirationTtl: CACHE_SEGUNDOS });
    } catch { /* Jev fora: o nome fica ausente e nao entra no cache */ }
  }));
  return resp({ cats });
}
