import os
from flask import Flask, render_template
import requests
import pandas as pd
import plotly.graph_objs as go
from datetime import datetime

# opcional: carrega .env se existir
# Isso permite que as variáveis de ambiente sejam lidas de um arquivo .env
# (bom para separar configurações sensíveis como chaves de API e não deixar no código)
try:
    from dotenv import load_dotenv
    load_dotenv()
except Exception:
    pass

# Inicialização da aplicação Flask
app = Flask(__name__)

# Carregar configurações via variáveis de ambiente
# Essas variáveis devem ser definidas em produção (ex: no Render, Railway ou Heroku)
THINGSPEAK_CHANNEL_ID = os.environ.get('THINGSPEAK_CHANNEL_ID')
THINGSPEAK_API_KEY = os.environ.get('THINGSPEAK_API_KEY')  # pode ser vazio se o canal for público
NUM_RESULTS = int(os.environ.get('NUM_RESULTS', 100))  # quantidade de registros buscados (padrão = 100)

# URL base da API do ThingSpeak para coletar dados do canal especificado
THINGSPEAK_URL = f"https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json"

# Função que busca dados do ThingSpeak e retorna um DataFrame
def get_thingspeak_data():
    # Parâmetros da requisição (quantos resultados queremos)
    params = {'results': NUM_RESULTS}
    if THINGSPEAK_API_KEY:  # se tiver API KEY, inclui na requisição
        params['api_key'] = THINGSPEAK_API_KEY

    try:
        # Faz a requisição HTTP para a API do ThingSpeak
        response = requests.get(THINGSPEAK_URL, params=params, timeout=10)
    except Exception as e:
        # Caso de erro de conexão ou timeout
        return None, f"Erro ao acessar a API do ThingSpeak: {e}"

    # Caso o status da resposta não seja 200 (OK)
    if response.status_code != 200:
        return None, f"Erro ao acessar a API do ThingSpeak: status {response.status_code}"

    # Converte a resposta JSON em dicionário Python
    payload = response.json()
    data = payload.get('feeds', [])  # pega a lista de registros dentro de "feeds"

    if not data:
        # Se não houver dados, retorna DataFrame vazio com colunas esperadas
        df_empty = pd.DataFrame(columns=['created_at', 'field1', 'field2'])
        return df_empty, None

    # Cria DataFrame a partir dos dados
    df = pd.DataFrame(data)

    # Converte colunas para os tipos corretos
    df['created_at'] = pd.to_datetime(df['created_at'], errors='coerce')  # datas
    df['umidade'] = pd.to_numeric(df.get('field1', pd.Series()), errors='coerce')  # field1 = umidade
    df['temperatura'] = pd.to_numeric(df.get('field2', pd.Series()), errors='coerce')  # field2 = temperatura

    # Remove registros que não tenham data (created_at inválido)
    df.dropna(subset=['created_at'], inplace=True)

    return df, None

# Rota principal do site (quando acessamos "/")
@app.route('/')
def index():
    # Busca dados da API
    df, error = get_thingspeak_data()

    if error:
        # Caso tenha ocorrido algum erro, mostra a mensagem no navegador
        return f"<h1>{error}</h1>"

    # Caso não tenha dados disponíveis ou estejam vazios
    if df.empty or df[['umidade','temperatura']].dropna().empty:
        last_temp = "N/A"
        last_umidade = "N/A"
        avg_temp = "N/A"
        avg_umid = "N/A"
        min_temp = "N/A"
        max_temp = "N/A"
        min_umid = "N/A"
        max_umid = "N/A"
        last_update = "Nunca"

        # Cria placeholders para os gráficos
        now = datetime.now()
        timestamps = [now]
        temps = [None]
        umids = [None]
    else:
        # Ordena o DataFrame por data
        df.sort_values('created_at', inplace=True)

        # Pega os últimos valores de temperatura e umidade
        last_temp = round(df['temperatura'].dropna().iloc[-1], 2) if not df['temperatura'].dropna().empty else "N/A"
        last_umidade = round(df['umidade'].dropna().iloc[-1], 2) if not df['umidade'].dropna().empty else "N/A"

        # Calcula estatísticas básicas
        avg_temp = round(df['temperatura'].mean(), 2)
        avg_umid = round(df['umidade'].mean(), 2)
        min_temp = round(df['temperatura'].min(), 2)
        max_temp = round(df['temperatura'].max(), 2)
        min_umid = round(df['umidade'].min(), 2)
        max_umid = round(df['umidade'].max(), 2)

        # Última atualização
        last_update = df['created_at'].iloc[-1].strftime('%Y-%m-%d %H:%M:%S')

        # Dados para os gráficos
        timestamps = df['created_at']
        temps = df['temperatura']
        umids = df['umidade']

    # --- Criação dos gráficos Plotly ---

    # Gráfico da Temperatura
    temp_trace = go.Scatter(x=timestamps, y=temps, mode='lines+markers', name='Temperatura (°C)')
    temp_graph = go.Figure(data=[temp_trace])
    temp_graph.update_layout(margin=dict(l=20, r=20, t=30, b=20),
                             title='Temperatura',
                             xaxis_title='Data',
                             yaxis_title='°C',
                             template='plotly_white',
                             height=360)

    # Gráfico da Umidade
    umidade_trace = go.Scatter(x=timestamps, y=umids, mode='lines+markers', name='Umidade (%)')
    umidade_graph = go.Figure(data=[umidade_trace])
    umidade_graph.update_layout(margin=dict(l=20, r=20, t=30, b=20),
                                title='Umidade',
                                xaxis_title='Data',
                                yaxis_title='%',
                                template='plotly_white',
                                height=360)

    # Exporta gráficos para HTML e insere dentro do template
    temp_div = temp_graph.to_html(full_html=False, include_plotlyjs='cdn')
    umidade_div = umidade_graph.to_html(full_html=False, include_plotlyjs=False)

    # Variáveis que serão passadas ao template HTML (index.html)
    context = {
        'last_temp': last_temp,       # último valor de temperatura
        'last_umidade': last_umidade, # último valor de umidade
        'avg_temp': avg_temp,         # média de temperatura
        'avg_umid': avg_umid,         # média de umidade
        'min_temp': min_temp,         # mínima temperatura
        'max_temp': max_temp,         # máxima temperatura
        'min_umid': min_umid,         # mínima umidade
        'max_umid': max_umid,         # máxima umidade
        'last_update': last_update,   # última atualização (timestamp)
        'temp_plot': temp_div,        # gráfico de temperatura
        'umidade_plot': umidade_div,  # gráfico de umidade
        'user_name': 'Administrador'  # nome do usuário exibido no dashboard
    }

    # Renderiza o template index.html passando as variáveis do contexto
    return render_template('index.html', **context)

# Ponto de entrada do programa
if __name__ == '__main__':
    # Define se o Flask roda em modo debug (útil em desenvolvimento)
    debug_flag = os.environ.get('FLASK_DEBUG', 'True').lower() in ['1','true','yes']
    # Roda a aplicação no host 0.0.0.0 (acessível em rede) e porta 5000
    app.run(host='0.0.0.0', port=5000, debug=debug_flag)
