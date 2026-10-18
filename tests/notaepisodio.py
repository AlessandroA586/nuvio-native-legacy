"""Execute the episode-card source guards from detail.c with independent toggles."""
import pathlib
import re
import subprocess
import tempfile

source = pathlib.Path("src/detail.c").read_text()
block = source[source.index("    int nota = 0;", source.index("static void desenhaEpisodio(")):]
guard = re.search(r"if \((nota > 0.*?)\)\s*\n\s*x \+= desenhaNotaEpisodio", block).group(1)
with tempfile.TemporaryDirectory() as directory:
    path = pathlib.Path(directory)
    (path / "guard.c").write_text("""
#include <assert.h>
enum { EX_TRAKT, EX_IMDB, EX_TMDB };
static int enabled[3];
static int ajustes_nota_titulo(int source) { return enabled[source]; }
int main(void) {
  int nota = 80;
  for (int imdb = 0; imdb < 2; imdb++)
    for (int trakt = 0; trakt < 2; trakt++) {
      enabled[EX_IMDB] = imdb; enabled[EX_TRAKT] = trakt;
      assert((GUARD) == trakt);
    }
  nota = 0; enabled[EX_TRAKT] = 1;
  assert(!(GUARD));
}
""".replace("GUARD", guard))
    subprocess.run(["cc", str(path / "guard.c"), "-o", str(path / "guard")], check=True)
    subprocess.run([str(path / "guard")], check=True)
print("episode source independence: PASS")
