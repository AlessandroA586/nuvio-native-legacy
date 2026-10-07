# P2P: janela de streaming (plano, #334)

## O que existe hoje (2.0.2)

- O nuvio-engine 0.1.3 (libtorrent 2.0.12) grava o arquivo escolhido INTEIRO em
  `<dados>/p2p/dados/<torrent>` (`posix_disk_io_constructor`, arquivo esparso).
  Nada do que ja tocou e apagado enquanto o player esta aberto.
- `disk_cache_capacity_bytes` do motor (nosso "mole") so despeja torrents
  INATIVOS inteiros (`DiskCacheManager::enforce`, LRU por pasta). O torrent
  tocando e protegido: a pasta cresce sem limite do lado do motor.
- O limite real e a nossa vigia (`p2pmotor.c`): pasta > teto duro = para.
  Teto duro: Automatico = min(1,5 GB, metade do livre); fixo (Ajustes, #334) =
  ate 2/4/8/16 GB deixando 512 MB livres.
- Pasta apagada ao parar o motor, ao subir de novo e (#334) no inicio do app.

Log do #334 (UE55AU7175, Tizen 6, 2.0.2): livre ao subir 1369-1379 MB, teto
duro 684-691 MB, a pasta cresce ~100-200 MB/min e o motor para em ~690 MB, cinco
vezes seguidas. Nenhuma sobra entre sessoes (livre igual a cada subida). Com
1,37 GB livres nenhum limite faz um 4K de varios GB caber: so a janela resolve.

## Janela: o que fazer no motor (C++, tools/p2p-motor)

Precisa de patch no nuvio-engine (fork no commit 02938d7); a API C nao expoe
pecas.

1. **Disk I/O proprio.** Trocar `posix_disk_io_constructor` por um
   `disk_interface` que embrulha o posix e conhece a janela. Ele:
   - escreve normalmente;
   - guarda quais pecas estao "despejadas" e, ao despejar, abre buraco no
     arquivo (`fallocate(FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE)`; ext4 da
     Tizen/webOS/Android aceita; sem suporte = nao despeja e cai no teto);
   - em `async_read` de peca despejada devolve erro de leitura, nunca zeros
     (zeros iriam para o player e para os pares).
2. **Politica (no stream bridge, que ja sabe a posicao pedida pelo HTTP Range):**
   - janela = N MB atras do ultimo byte servido + o read-ahead que o
     `set_piece_deadline` ja pede; N = teto duro - cabeca - cauda;
   - protegidas para sempre: primeiros e ultimos ~8 MB do arquivo (cabecalho
     MKV/MP4, Cues/moov) e as pecas de borda com outros arquivos;
   - despeja a peca mais antiga ATRAS da posicao quando a pasta passa de 90%
     do teto; nunca a frente.
3. **Voltar para tras (seek para trecho despejado).** libtorrent 2.0 nao tem
   "desmarcar peca". Caminho: `save_resume_data`, `remove_torrent` (sem apagar
   arquivos), re-adicionar com `have_pieces` sem as despejadas. Custa alguns
   segundos de reconexao; o player ve espera, nao erro.
4. **Upload.** Pares pedindo peca despejada: o erro de leitura do item 1 faz o
   libtorrent recusar o pedido. Medir se isso nao derruba a sessao
   (`handle_disk_error` pausa o torrent em alguns erros: usar um codigo que ele
   trate como "nao tenho", ou tirar a peca do bitfield anunciado com
   `share_mode`/superseed off).
5. **API C nova (API_VERSION 4):** `config.streaming_window_bytes` (0 =
   comportamento de hoje). `p2pmotor_motor.c` passa o teto duro menos a
   reserva; a vigia continua como rede de seguranca.

## Quando nem a janela cabe

Janela minima util ~ 2x o read-ahead (+ cabeca/cauda), ~300 MB para 4K. Teto
duro abaixo disso: recusar antes de tocar, com o numero ("esta TV tem X livres,
o P2P precisa de 300 MB"), como ja faz o "Falta espaço livre" de 256 MB.

## Prova antes de publicar

- teste do motor (mac, tests/p2pmotor_real.sh) com teto de 200 MB num torrent
  de 2 GB tocando do inicio ao fim, e um seek para tras de 50%;
- TV Samsung 6+ com pouco livre (a do #334 e o caso).
