# 📊 Dashboard IoT - ThingSpeak

Um projeto simples e elegante desenvolvido com **Python (Flask)**, **ThingSpeak**, e **Plotly**, que permite a visualização em tempo real de dados de **temperatura** e **umidade** coletados por sensores IoT conectados ao ThingSpeak.

---

## 🔧 Funcionalidades

✅ Leitura dos últimos dados de temperatura e umidade via API do ThingSpeak  
✅ Exibição dos últimos valores em destaque  
✅ Visualização gráfica interativa com Plotly  
✅ Interface responsiva e estilosa com Bootstrap 5  
✅ Background personalizado e visual futurista com Google Fonts

---

## 🌐 Pré-requisitos

Antes de rodar o projeto, certifique-se de ter o seguinte instalado:

- Python 3.8+
- pip

---

## 📁 Estrutura do Projeto

```

dashboard\_thingspeak/
│
├── app.py                  # Aplicação Flask principal
├── static/
│   └── background.jpg      # Imagem de fundo
├── templates/
│   └── index.html          # Template HTML com dashboard

````

---

## ⚙️ Instalação

1. Clone ou baixe este repositório.

2. Crie e ative um ambiente virtual (opcional, mas recomendado):

```bash
python -m venv venv
source venv/bin/activate  # Linux/Mac
venv\Scripts\activate     # Windows
````

3. Instale as dependências:

```bash
pip install flask requests pandas plotly
```

---

## ▶️ Como Executar

No terminal, dentro da pasta do projeto, execute:

```bash
python app.py
```

Acesse em seu navegador:
`http://127.0.0.1:5000/`

---

## 📡 Configuração do ThingSpeak

Este projeto está configurado para ler dados de um canal público ou privado do ThingSpeak.

* Edite o `app.py` para alterar seu `THINGSPEAK_CHANNEL_ID` e `THINGSPEAK_API_KEY`:

```python
THINGSPEAK_CHANNEL_ID = 'SEU_ID_DO_CANAL'
THINGSPEAK_API_KEY = 'SUA_CHAVE_API'  # Use '' se for canal público
```

---

## 🧠 Tecnologias Utilizadas

* Flask
* ThingSpeak API
* Plotly
* Bootstrap 5
* Google Fonts
* HTML/CSS

---

## ❤️ Agradecimentos

Projeto desenvolvido com carinho para fins de aprendizado e visualização de dados em projetos de IoT.
Feito com Flask, ThingSpeak e Plotly!

---

## 📃 Licença

Este projeto está sob a licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

