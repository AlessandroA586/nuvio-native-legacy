// LAYOUT IMERSIVA (experimental): o estado da cena, sem gfx nem SDL.
//
// Uma fileira em cena (`fileira`), a de antes e a de depois so como
// fantasmas nas bordas. Trocar de fileira e UMA linha do tempo de relogio
// fixo (IMV_TROCA_MS, suave nas duas pontas, como o morph do menu): a que
// sai apaga e sobe, a arte troca, a que entra sobe em escada. Outra tecla no
// meio NAO enfileira: a cena e reapontada a partir de onde esta. OK abre o
// cartaz NO PROPRIO lugar (`abre` 0..1, reversivel); Voltar fecha.
//
// home_imersiva.inc desenha; tests/imersiva.c prova isto aqui.
#ifndef NV_IMERSIVA_H
#define NV_IMERSIVA_H

#define IMV_TROCA_MS   480.0f   // troca de fileira inteira (<= 600)
#define IMV_ABRE_MS    420.0f   // cartaz -> janela larga
#define IMV_ESCADA_MS   40.0f   // atraso por cartaz na subida em escada
#define IMV_ESCADA_N       5    // a partir do 5o cartaz, todos juntos

typedef struct {
  int   fileira;    // a fileira em cena (destino da troca)
  int   anterior;   // a que esta saindo; -1 = nenhuma
  int   dir;        // +1 desceu, -1 subiu
  float t;          // 0..1 da troca; 1 = assentada
  float abre;       // 0..1 do cartaz aberto no lugar
  int   aberto;     // alvo de `abre`
  int   coluna;     // o cartaz aberto
} Imersiva;

void  imv_iniciar(Imersiva *s, int fileira);
// Nova fileira em cena. Interrompe a troca em curso sem salto: a fileira mais
// visivel no momento vira a que sai, com a mesma opacidade.
void  imv_fileira(Imersiva *s, int nova);
void  imv_abrir(Imersiva *s, int coluna);
void  imv_fechar(Imersiva *s);
// Um quadro. `reduzido` (animacoes reduzidas) assenta na hora.
void  imv_passo(Imersiva *s, float dt, int reduzido);
float imv_suave(float t);
// Opacidade 0..1 da fileira em cena (`chega`) e da que sai.
float imv_chega(const Imersiva *s);
float imv_sai(const Imersiva *s);
// Subida em escada: progresso 0..1 do cartaz k (0 = o primeiro visivel).
float imv_escada(const Imersiva *s, int k);
// O fantasma de cima (lado -1) ou de baixo (+1) da fileira em cena; -1 = nao ha.
int   imv_fantasma(const Imersiva *s, int nFileiras, int lado);
int   imv_em_troca(const Imersiva *s);

#endif
