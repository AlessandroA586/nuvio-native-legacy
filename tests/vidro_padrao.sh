#!/bin/bash
# Glass standard (docs/issues/vidro-padrao.md): every panel/popup/dialog/island
# paints its body through the shared gfx_vidro family (one backdrop per frame),
# never with its own dark translucent gfx_cor rect. Grep-based; no link.
#
#   bash tests/vidro_padrao.sh
set -u
cd "$(dirname "$0")/.."
fail=0

# 1) No file may draw the glass-body colour (.055/.059/.071 family) with gfx_cor,
#    except the listed knobs/previews (file:count).
# episodios/posplay: 34px/32px chips over a still, one per row - a glass pass each would blow the budget.
allow='src/episodios.c:1 src/posplay.c:1 src/ajustes_ux_previa.inc:3 src/ajustes_ux_desenho.inc:3 src/faixas.c:1 src/legendasui.c:1 src/registro_ilha.inc:1 src/ajustes_ux_secoes.inc:3'
pat='gfx_cor\([^;]*\b0?\.0(54|55)f, ?0?\.0(58|59)f, ?0?\.0(68|71)f'
for f in src/*.c src/*.h src/*.inc; do
  n=$(grep -cE "$pat" "$f" || true)
  [ "$n" -eq 0 ] && continue
  max=0
  for e in $allow; do [ "${e%%:*}" = "$f" ] && max=${e##*:}; done
  if [ "$n" -gt "$max" ]; then
    echo "FAIL $f: $n own translucent glass rect(s), allowed $max"; grep -nE "$pat" "$f" | cut -c1-140; fail=1
  fi
done

# 2) Known surfaces must call the shared helper (file|regex).
need='
src/plrui.c|gfx_vidro_material
src/telemetria.c|gfx_vidro_material
src/atualizacao.c|gfx_vidro_material
src/teclado.c|gfx_vidro_material
src/celbotao.c|gfx_vidro_material
src/novidades170.c|gfx_vidro_material
src/ajustes_ux_desenho.inc|gfx_vidro_material
src/ilha.c|gfx_vidro_painel
src/menu.c|gfx_vidro\(
src/guia.c|gfx_vidro_folha
src/streams.c|gfx_vidro_folha
src/amigosfil.c|gfx_vidro_folha
'
while IFS='|' read -r f re; do
  [ -z "$f" ] && continue
  grep -qE "$re" "$f" || { echo "FAIL $f: missing $re"; fail=1; }
done <<< "$need"

[ "$fail" -eq 0 ] && echo "vidro padrao: PASS"
exit $fail
