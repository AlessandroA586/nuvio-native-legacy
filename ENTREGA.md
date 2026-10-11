# Integração das decisões — 2.0.5

Worktree `agente/205-decisoes`, início `6a68b96e`; sem push. Um merge por branch, na ordem aprovada. Mensagens em inglês com coautoria solicitada.

- **#326 — agente/203-326:** piso do Continuar 1–20%, padrão 1%. Pré-carga já coberta por `fontecache_precarregar_proximo`: preservada a integração, sem duplicar fios. Ajustes append-only e traduções existentes preservadas; só chaves novas adicionadas. 33 testes de Continuar/Home/progresso/perfil/addons/pós-player/Ajustes: **rc 0** (`326-results.txt` + `326-final-results.txt`, repetição corrigida). `idiomas.py` e `varredura-i18n.py`: **rc 0**. Corrigidos mocks antigos em `cwdesmarcado` (base rc 0) e `episodiosdup`; contrato append-only atualizado.

- **#338 — agente/203-338:** estatísticas ao vivo pela Central, áudio/legenda em uso e buffer Android; mantido o diagnóstico mais recente da integração. Alvo do buffer acompanha o preset. 21 verificações (Central/Ajustes/estatísticas/cache/JNI/TPK/diagnóstico e Kotlin Android): **rc 0**. Corrigido link dos testes TPK que passavam na base. `idiomas.py` e `varredura-i18n.py`: **rc 0**. `player.sh` **rc 134** (linha 161) e `video_acb_lg.sh` **rc 1** (símbolos webOS): mesmos resultados na base `6a68b96e`, sem regressão.

Logs locais: `/tmp/nv-205-decisoes-logs/`. Suíte geral: 578 scripts, vários compilam o app inteiro (~45–60 s cada); serão usados os testes das áreas tocadas. `bw` sem autenticação; credenciais não consultadas.
