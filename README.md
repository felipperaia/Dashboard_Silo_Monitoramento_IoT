# README.md

# Dashboard IoT — Monitoramento de Silos (ThingSpeak + Flask + Plotly)

> **Dashboard funcional para monitoramento de silos de soja.**
> Visual limpo, indicadores operacionais, notificações mock (Telegram / SMS / Pop-up), e back-end seguro via variáveis de ambiente.
> Produzido para observabilidade prática e apresentação profissional.

---

![status-badge](https://img.shields.io/badge/status-ready-brightgreen) ![python](https://img.shields.io/badge/python-3.11%2B-blue) ![flask](https://img.shields.io/badge/flask-2.x-lightgrey) ![license](https://img.shields.io/badge/license-MIT-green)

---

## Sumário (rápido)

1. 🚀 Sobre
2. 🧭 Recursos principais
3. 🛠️ Instalação & Execução local (Windows / Linux / macOS)
4. 🔐 Configuração via variáveis de ambiente (`.env`)
5. ☁️ Deploy (Render — recomendado)
6. 🧩 Arquitetura & caminhos dos arquivos
7. 🧰 Boas práticas & notas operacionais
8. 📜 Contribuição, licensa e créditos

---

# 1 — Sobre

Este repositório contém um **dashboard web** construído com **Flask** que consome dados do ThingSpeak e apresenta:

* gráficos interativos (Plotly) de temperatura e umidade;
* cards de métricas (último valor, média, mínimos/máximos);
* painel de usuário estático (“Administrador”) com avatar e opções;
* modal de gerenciamento de notificações (Telegram, SMS, Pop-up) — UI pronta para integração;
* modal de exemplo de alerta (texto operacional para silos de soja);
* backend refatorado para ler credenciais/segredos via variáveis de ambiente (suporta `.env` por `python-dotenv`).

O design foca em legibilidade sobre uma imagem de fundo (silo), com paleta suave e ícones sóbrios — ideal para apresentações executivas ou operação agrícola.

---

# 2 — Recursos principais

* Interface responsiva com Bootstrap 5
* Gráficos Plotly exportados como HTML embutido (CDN)
* Segurança: credenciais via ENV (`THINGSPEAK_CHANNEL_ID`, `THINGSPEAK_API_KEY`, etc.)
* Preparado para produção com Gunicorn
* Arquivo `.env.example` incluído
* README detalhado para deploy em Render (deploy único recomendado)
* Documentação de troubleshooting (NumPy/venv no Windows)

---

# 3 — Instalação & Execução (Local)

> Recomendado: Python **3.11** (Windows: instale e marque *Add to PATH*).

### Clone

```bash
git clone https://github.com/seuusuario/seu-repo.git
cd seu-repo
```

### Crie e ative virtualenv

**Linux / macOS**

```bash
python3.11 -m venv .venv
source .venv/bin/activate
```

**Windows (PowerShell)**

```powershell
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
```

### Instale dependências

```bash
pip install --upgrade pip setuptools wheel
pip install -r requirements.txt
```

### Configurar variáveis de ambiente (ex.: copiar `.env.example`)

Crie um arquivo `.env` na raiz (veja seção 4).

### Rodar localmente

```bash
python app.py
```

Abra `http://127.0.0.1:5000/`.

---

# 4 — Variáveis de Ambiente (ENV)

Não deixe segredos no repositório. Use `.env` localmente e configure variáveis no painel do provedor em produção.

**Arquivo `.env.example`**

```env
THINGSPEAK_CHANNEL_ID=sua_id_aqui
THINGSPEAK_API_KEY=sua_api_key_aqui
NUM_RESULTS=numero_de_resultados
FLASK_DEBUG=true/false
```

* `THINGSPEAK_CHANNEL_ID` — ID do canal ThingSpeak
* `THINGSPEAK_API_KEY` — chave (vazia se público)
* `NUM_RESULTS` — número de registros a buscar (padrão 100)
* `FLASK_DEBUG` — `True`/`False`

> **Atenção**: Não comite o `.env` real. Use `.gitignore` (já incluído) para proteger o arquivo.

---

# 5 — Deploy (Produção)

### Opção recomendada: **Render** (deploy único — backend + frontend juntos)

1. Faça push do repositório para o GitHub.
2. No painel do Render: **New → Web Service** → conecte ao repo.
3. Build Command:

   ```bash
   pip install -r requirements.txt
   ```
4. Start Command:

   ```bash
   gunicorn "app:app" --bind 0.0.0.0:$PORT --workers 3 --threads 2 --timeout 120
   ```
5. Defina variáveis de ambiente no painel (THINGSPEAK_*, NUM_RESULTS, etc).
6. Deploy e acesse a URL pública gerada.

7. Acesso a nosso exemplo: https://dashboard-silo-monitoramento-iot.onrender.com/

---

# 6 — Estrutura do projeto (principais arquivos)

```
├─ .env.example
├─ app.py                 # aplicação Flask (ponto WSGI: app)
├─ requirements.txt
├─ Procfile               # opcional (compatibilidade Heroku)
├─ templates/
│  └─ index.html
├─ static/
│  └─ background.jpg
└─ README.md
```

---

# 7 — Boas práticas & notas operacionais

* NÃO comite `.env` ou chaves privadas.
* Para enviar notificações reais (Telegram / Twilio SMS) siga passos:

  * Criar bot Telegram e colocar `TELEGRAM_BOT_TOKEN` e `TELEGRAM_CHAT_ID` nas ENV.
  * Integrar com Twilio (para SMS) com `TWILIO_SID`, `TWILIO_AUTH_TOKEN`, `TWILIO_FROM`.
* Considere armazenar thresholds de alerta em DB (ou arquivo YAML) para deixar os alertas automáticos configuráveis.
* Para múltiplos usuários, adicione autenticação (Flask-Login / OAuth).

---

# 8 — Contribuição & estilo

Se quiser colaborar:

* Faça **fork** → branch com feature → PR com descrição do que mudou.
* Testes e documentação são bem-vindos.
* Mantenha o padrão de formatação (PEP8 para Python, HTML limpo para templates).

---

# Licença

MIT License — copie o arquivo `LICENSE` incluso neste repositório.

---

# Updates Futuros

Feito com atenção ao monitoramento agrícola e usabilidade.
Caso queira atualizar também.

* implemente envio real por Telegram ou SMS;
* adicione thresholds automáticos que disparem a campainha;
* gere arquivos Docker / GitHub Actions para CI/CD;