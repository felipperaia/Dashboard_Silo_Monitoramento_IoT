import os
from flask import Flask, render_template
import requests
import pandas as pd
import plotly.graph_objs as go
from datetime import datetime

# opcional: carrega .env se existir
try:
    from dotenv import load_dotenv
    load_dotenv()
except Exception:
    pass

app = Flask(__name__)

# Carregar configurações via ENV (mantenha-as em .env em produção)
THINGSPEAK_CHANNEL_ID = os.environ.get('THINGSPEAK_CHANNEL_ID')
THINGSPEAK_API_KEY = os.environ.get('THINGSPEAK_API_KEY')  # leave empty if public
NUM_RESULTS = int(os.environ.get('NUM_RESULTS', 100))
THINGSPEAK_URL = f"https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json"

def get_thingspeak_data():
    params = {'results': NUM_RESULTS}
    if THINGSPEAK_API_KEY:
        params['api_key'] = THINGSPEAK_API_KEY

    try:
        response = requests.get(THINGSPEAK_URL, params=params, timeout=10)
    except Exception as e:
        return None, f"Erro ao acessar a API do ThingSpeak: {e}"

    if response.status_code != 200:
        return None, f"Erro ao acessar a API do ThingSpeak: status {response.status_code}"

    payload = response.json()
    data = payload.get('feeds', [])
    if not data:
        # retornar dataframe vazio com colunas esperadas
        df_empty = pd.DataFrame(columns=['created_at', 'field1', 'field2'])
        return df_empty, None

    df = pd.DataFrame(data)
    # tratamento e conversão
    df['created_at'] = pd.to_datetime(df['created_at'], errors='coerce')
    df['umidade'] = pd.to_numeric(df.get('field1', pd.Series()), errors='coerce')
    df['temperatura'] = pd.to_numeric(df.get('field2', pd.Series()), errors='coerce')
    # remove registros sem dados essenciais
    df.dropna(subset=['created_at'], inplace=True)
    return df, None

@app.route('/')
def index():
    df, error = get_thingspeak_data()
    if error:
        # mensagem simples — você pode criar uma página de erro separada se preferir
        return f"<h1>{error}</h1>"

    # se dataframe vazio, crie placeholders
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
        # gráficos vazios com datas atuais como placeholders
        now = datetime.now()
        timestamps = [now]
        temps = [None]
        umids = [None]
    else:
        # ordenar por tempo
        df.sort_values('created_at', inplace=True)
        last_temp = round(df['temperatura'].dropna().iloc[-1], 2) if not df['temperatura'].dropna().empty else "N/A"
        last_umidade = round(df['umidade'].dropna().iloc[-1], 2) if not df['umidade'].dropna().empty else "N/A"

        avg_temp = round(df['temperatura'].mean(), 2)
        avg_umid = round(df['umidade'].mean(), 2)
        min_temp = round(df['temperatura'].min(), 2)
        max_temp = round(df['temperatura'].max(), 2)
        min_umid = round(df['umidade'].min(), 2)
        max_umid = round(df['umidade'].max(), 2)
        last_update = df['created_at'].iloc[-1].strftime('%Y-%m-%d %H:%M:%S')

        timestamps = df['created_at']
        temps = df['temperatura']
        umids = df['umidade']

    # Gráficos Plotly
    temp_trace = go.Scatter(x=timestamps, y=temps, mode='lines+markers', name='Temperatura (°C)')
    umidade_trace = go.Scatter(x=timestamps, y=umids, mode='lines+markers', name='Umidade (%)')

    temp_graph = go.Figure(data=[temp_trace])
    temp_graph.update_layout(margin=dict(l=20, r=20, t=30, b=20),
                             title='Temperatura',
                             xaxis_title='Data',
                             yaxis_title='°C',
                             template='plotly_white',
                             height=360)

    umidade_graph = go.Figure(data=[umidade_trace])
    umidade_graph.update_layout(margin=dict(l=20, r=20, t=30, b=20),
                                title='Umidade',
                                xaxis_title='Data',
                                yaxis_title='%',
                                template='plotly_white',
                                height=360)

    temp_div = temp_graph.to_html(full_html=False, include_plotlyjs='cdn')
    umidade_div = umidade_graph.to_html(full_html=False, include_plotlyjs=False)

    # Variáveis para template
    context = {
        'last_temp': last_temp,
        'last_umidade': last_umidade,
        'avg_temp': avg_temp,
        'avg_umid': avg_umid,
        'min_temp': min_temp,
        'max_temp': max_temp,
        'min_umid': min_umid,
        'max_umid': max_umid,
        'last_update': last_update,
        'temp_plot': temp_div,
        'umidade_plot': umidade_div,
        'user_name': 'Administrador'
    }

    return render_template('index.html', **context)

if __name__ == '__main__':
    debug_flag = os.environ.get('FLASK_DEBUG', 'True').lower() in ['1','true','yes']
    app.run(host='0.0.0.0', port=5000, debug=debug_flag)
