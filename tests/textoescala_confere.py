#!/usr/bin/env python3
"""Compare two spy logs (tests/textoescala_espiao.c): argv = nome base alvo fator [paisagem].

`paisagem` exempts 16:9 art cards (episode/trailer tiles carrying text), which are
components, not posters."""
import sys, statistics

def ler(p):
    t, c, r = [], [], []
    for l in open(p):
        k, *v = l.split()
        v = [float(x) for x in v]
        (t if k == "T" else c if k == "C" else r).append(v)
    return t, c, r

nome, base, alvo, fator = sys.argv[1], sys.argv[2], sys.argv[3], float(sys.argv[4])
paisagem = len(sys.argv) > 5 and sys.argv[5] == "paisagem"
def cartaz(w, h): return not (paisagem and w / h > 1.5)
t0, c0, r0 = ler(base)
t1, c1, r1 = ler(alvo)
ok = True
def falha(msg):
    global ok
    ok = False
    print("  FAIL %s: %s" % (nome, msg))

if not t0 or not t1:
    falha("no text drawn (%d/%d)" % (len(t0), len(t1)))
else:
    m = statistics.median(h for (h,) in t1) / statistics.median(h for (h,) in t0)
    print("  %s text height x%.3f (want ~%.2f)" % (nome, m, fator))
    if abs(m - fator) > 0.08: falha("text did not scale")
if r0 and r1:
    m = statistics.median(h for _, h in r1) / statistics.median(h for _, h in r0)
    print("  %s button/pill height x%.3f" % (nome, m))
    if abs(m - fator) > 0.12: falha("buttons did not scale")
if c0:
    c0 = [(w, h) for w, h in c0 if cartaz(w, h)]
    c1 = [(w, h) for w, h in c1 if cartaz(w, h)]
    tam = {(round(w), round(h)) for w, h in c0}
    for w, h in {(round(w), round(h)) for w, h in c1}:
        if not any(abs(w - a) <= 2 and abs(h - b) <= 2 for a, b in tam):
            falha("poster rect %dx%d is new at the larger scale" % (w, h))
    print("  %s poster sizes: %d at base, %d at target, none resized" % (nome, len(tam), len({(round(w), round(h)) for w, h in c1})))
sys.exit(0 if ok else 1)
