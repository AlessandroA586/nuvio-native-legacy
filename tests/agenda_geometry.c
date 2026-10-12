// Android TV must keep the calendar and episode panel inside 1920x1080.
#include "../src/agendaui.c"
#include <assert.h>

float gfx_escala_ui(void) { return 1.0f; }
float ajustes_rail_largura_fixa(void) { return 144.0f; }
int agenda_n(void) { return 0; }
const AgItem *agenda_lista(int i) { (void)i; return NULL; }
int agenda_dias(const char *data) { (void)data; return -1; }

int main(void) {
  layout_tela_definir(1920, 1080);
  layout_modo_definir(0);
  AgMes m = agMesMedir();
  float calendarBottom = (m.gradeY + 6 * m.celH + 5 * m.espacY) * agEscala();
  printf("TV: calendar bottom=%.0f, episode panel top=%.0f, bottom=%.0f\n",
         calendarBottom, m.painel.y * agEscala(),
         (m.painel.y + m.painel.h) * agEscala());
  fflush(stdout);
  assert(calendarBottom <= 1080.0f);
  assert(m.painel.y >= 0 && m.painel.h > 0);
  assert((m.painel.y + m.painel.h) * agEscala() <= 1080.0f);
  assert(m.painel.x >= m.x + m.gradeW);
  AgC1 l = c1();
  assert(l.artW > 0 && l.artH > 0 && l.pnX >= l.x0 + l.artW);

  // Same aspect ratio in Mobile retains the stacked, scrollable calendar.
  layout_modo_definir(1);
  m = agMesMedir(); l = c1();
  assert(m.painel.y > m.gradeY + 6 * m.celH + 5 * m.espacY);
  assert(m.painel.x == m.x && l.artW == 0 && l.artH == 0);
  layout_tela_definir(1080, 2400);
  m = agMesMedir();
  assert(m.painel.y > m.gradeY + 6 * m.celH + 5 * m.espacY);
  escForcada = 1.0f;
  assert(!agColunaUnica()); // Social panel retains its own geometry.
  puts("PASS: Agenda TV fits 1080; Mobile calendar remains stacked");
  return 0;
}
