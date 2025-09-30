#include <WiFi.h>
#include <PubSubClient.h>

// ===== CONFIGURAÇÕES WIFI =====
const char* SSID_WIFI     = "ssid_wifi_aqui";
const char* SENHA_WIFI    = "senha_wifi_aqui"; 

// ===== THINGSPEAK MQTT =====
// Pegue em Devices > MQTT > seu device
const char* ID_CLIENTE_MQTT = "idcliente_aqui";  // algo como "channels/3093339/publish/XYZ"
const char* USUARIO_MQTT    = "usuarioMQTT_aqui"; // algo como "XYZ"
const char* SENHA_MQTT      = "sua_senhaMQTT_aqui";   // algo como "ABCDEF1234567890"
const char* CORRETOR_MQTT   = "mqtt3.thingspeak.com";   // ou outro, se usar TLS
const uint16_t PORTA_MQTT   = 1883; // ou 8883 com TLS

String TOPICO_MQTT = String("channels/") + "3093339" + "/publish";

// ====== LIMITES / FILTRO ======
float TEMP_MAXIMA       = 30.0f;  // °C
float UMID_MINIMA       = 45.0f;  // %
bool  PUBLICAR_QUANDO_OK = true;  // true: publica só quando dentro dos limites; false: só quando fora

// ====== AMOSTRAGEM / ENVIO ======
const uint8_t  N_AMOSTRAS   = 5;        // média de 5 amostras
const unsigned long AMOSTRA_MS   = 2000;   // 1 leitura a cada 2s
const unsigned long PUBLICAR_MS  = 20000;  // ThingSpeak >=15s (usamos 20s)

// ====== FAIL-SAFE: BUFFER OFFLINE ======
struct PacoteMedia {
  float t, h;
  unsigned long ts; // millis() no momento do cálculo
};

const uint16_t CAPACIDADE_FILA_OFFLINE = 100;  // capacidade do buffer
PacoteMedia filaOffline[CAPACIDADE_FILA_OFFLINE];
uint16_t cabecaFila = 0, caudaFila = 0;        // circular

bool filaVazia() { return cabecaFila == caudaFila; }
bool filaCheia() { return (uint16_t)((cabecaFila + 1) % CAPACIDADE_FILA_OFFLINE) == caudaFila; }

bool enfileirarMedia(float t, float h) {
  if (filaCheia()) {
    // descarta o mais antigo para sempre caber o mais recente (fail-safe)
    caudaFila = (caudaFila + 1) % CAPACIDADE_FILA_OFFLINE;
  }
  filaOffline[cabecaFila] = { t, h, millis() };
  cabecaFila = (cabecaFila + 1) % CAPACIDADE_FILA_OFFLINE;
  return true;
}

bool desenfileirarMedia(PacoteMedia &pkt) {
  if (filaVazia()) return false;
  pkt = filaOffline[caudaFila];
  caudaFila = (caudaFila + 1) % CAPACIDADE_FILA_OFFLINE;
  return true;
}

// ====== WATCHDOG / DEADLINE ======
#if USE_WDT
  #include "esp_task_wdt.h"
  const int WDT_TIMEOUT_S = 10;  // 10s
#endif

const unsigned long PRAZO_PUBLICACAO_MS = 30UL * 60UL * 1000UL; // 30 min sem publicar => opcional restart
unsigned long ultimaPublicacaoComSucesso = 0;

// ====== ATUADOR ======
#define PINO_RELE 2
bool regraAtiva = false;
const float HISTERSE_TEMP = 0.5f;
const float HISTERSE_UMID = 2.0f;

// ====== ESTADO ======
WiFiClient clienteWifi;
PubSubClient clienteMqtt(clienteWifi);
unsigned long ultimaAmostra   = 0;
unsigned long ultimaPublicacao = 0;

// Backoff para reconexão
unsigned long proximaTentativaWifiMs = 0;
unsigned long proximaTentativaMqttMs = 0;
unsigned long backoffWifiMs = 1000;  // 1s → 2s → 4s → 8s ... máx 30s
unsigned long backoffMqttMs = 1000;

unsigned long limitarBackoff(unsigned long v) { return v > 30000 ? 30000 : v; }

// ====== SIMULAÇÃO (trocar quando tiver sensores) ======
float lerTemperaturaSim() { 
  return 20 + (esp_random() % 300) / 10.0;  // 20.0 a 50.0 °C
}

float lerUmidadeSim() { 
  return 30 + (esp_random() % 600) / 10.0;  // 30.0 a 90.0 %
}

// ====== SERIAL ======
void iniciarSerial() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0 < 1500)) { delay(10); }
  Serial.println("\n[BOOT] Serial iniciado.");
}

// ====== WIFI ======
void garantirWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long agora = millis();
  if (agora < proximaTentativaWifiMs) return;

  Serial.printf("[WiFi] Conectando a \"%s\"...\n", SSID_WIFI);
  WiFi.begin(SSID_WIFI, SENHA_WIFI);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(250);
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Conectado | IP: %s | RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    backoffWifiMs = 1000; // reseta backoff
  } else {
    backoffWifiMs = limitarBackoff(backoffWifiMs * 2);
    proximaTentativaWifiMs = agora + backoffWifiMs;
    Serial.printf("[WiFi] Falha. Próxima tentativa em %lums.\n", backoffWifiMs);
  }
}

// ====== MQTT ======
void garantirMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (clienteMqtt.connected()) return;

  unsigned long agora = millis();
  if (agora < proximaTentativaMqttMs) return;

  clienteMqtt.setServer(CORRETOR_MQTT, PORTA_MQTT);
  Serial.printf("[MQTT] Conectando em %s:%u ... ", CORRETOR_MQTT, PORTA_MQTT);
  if (clienteMqtt.connect(ID_CLIENTE_MQTT, USUARIO_MQTT, SENHA_MQTT)) {
    Serial.println("OK");
    backoffMqttMs = 1000; // reseta backoff
  } else {
    Serial.printf("FALHA (rc=%d)\n", clienteMqtt.state());
    backoffMqttMs = limitarBackoff(backoffMqttMs * 2);
    proximaTentativaMqttMs = agora + backoffMqttMs;
    Serial.printf("[MQTT] Próxima tentativa em %lums.\n", backoffMqttMs);
  }
}

// ====== REGRA LOCAL (com histerese) ======
void atualizarRegra(float t, float h) {
  bool deveAlarmar = (t > TEMP_MAXIMA) || (h < UMID_MINIMA);

  if (regraAtiva) {
    if (t <= (TEMP_MAXIMA - HISTERSE_TEMP) && h >= (UMID_MINIMA + HISTERSE_UMID)) {
      regraAtiva = false;
      Serial.println("[REGRA] Normalizou. Atuador DESLIGADO.");
    }
  } else {
    if (deveAlarmar) {
      regraAtiva = true;
      Serial.println("[REGRA] Limite violado. Atuador LIGADO.");
    }
  }
  digitalWrite(PINO_RELE, regraAtiva ? HIGH : LOW);
}

// ====== FILTRO ======
bool passaFiltro(float mediaT, float mediaH) {
  bool ok = (mediaT <= TEMP_MAXIMA) && (mediaH >= UMID_MINIMA);
  return PUBLICAR_QUANDO_OK ? ok : !ok;
}

// ====== PUBLICAÇÃO (um pacote) ======
bool publicarMedia(float t, float h) {
  if (WiFi.status() != WL_CONNECTED || !clienteMqtt.connected()) {
    Serial.println("[ENVIO] Sem Wi-Fi/MQTT. Não publicado.");
    return false;
  }
  if (millis() - ultimaPublicacao < PUBLICAR_MS) {
    Serial.println("[ENVIO] Janela mínima ThingSpeak ainda não atingida.");
    return false;
  }

  char payload[100];
  snprintf(payload, sizeof(payload), "field1=%.2f&field2=%.2f&status=MQTTPUBLISH", t, h);
  bool ok = clienteMqtt.publish(TOPICO_MQTT.c_str(), payload);
  if (ok) {
    ultimaPublicacao = millis();
    ultimaPublicacaoComSucesso = millis();
    Serial.printf("[ENVIO] OK -> T=%.2f | H=%.2f\n", t, h);
  } else {
    Serial.println("[ENVIO] FALHA no publish().");
  }
  return ok;
}

// ====== DRENAR FILA OFFLINE ======
void tentarDrenarFilaOffline() {
  if (WiFi.status() != WL_CONNECTED || !clienteMqtt.connected()) return;
  if (millis() - ultimaPublicacao < PUBLICAR_MS) return;

  if (!filaVazia()) {
    PacoteMedia pkt;
    if (desenfileirarMedia(pkt)) {
      Serial.printf("[QUEUE] Tentando publicar pacote offline (%lu ms atrás): T=%.2f H=%.2f\n",
                    (unsigned long)(millis() - pkt.ts), pkt.t, pkt.h);
      publicarMedia(pkt.t, pkt.h);
    }
  }
}

// ====== ESTATÍSTICA: média de 5 ======
float somaT = 0.0f, somaH = 0.0f;
uint8_t contadorAmostras = 0;

void coletarEAvaliarMedia() {
  if (millis() - ultimaAmostra < AMOSTRA_MS) return;
  ultimaAmostra = millis();

  float t = lerTemperaturaSim();
  float h = lerUmidadeSim();
  somaT += t; somaH += h; contadorAmostras++;

  Serial.printf("[AMOSTRA] #%u | T=%.2f °C | H=%.2f %%\n", contadorAmostras, t, h);
  atualizarRegra(t, h);

  if (contadorAmostras >= N_AMOSTRAS) {
    float mediaT = somaT / contadorAmostras;
    float mediaH = somaH / contadorAmostras;
    Serial.printf("[ESTAT] Média(%u) -> T=%.2f | H=%.2f\n", contadorAmostras, mediaT, mediaH);

    // zera acumulador para próxima janela
    somaT = somaH = 0.0f; 
    contadorAmostras = 0;

    // aplica filtro; se não passar, não envia e não guarda
    if (!passaFiltro(mediaT, mediaH)) {
      Serial.println("[FILTRO] Média reprovada. Não enviar / não enfileirar.");
      return;
    }

    // Tenta publicar; se falhar, guarda na fila offline
    if (!publicarMedia(mediaT, mediaH)) {
      enfileirarMedia(mediaT, mediaH);
      Serial.printf("[QUEUE] Pacote OFFLINE enfileirado. Tam=%u\n",
                    (uint16_t)((cabecaFila + CAPACIDADE_FILA_OFFLINE - caudaFila) % CAPACIDADE_FILA_OFFLINE));
    }
  }
}

// ====== SETUP / LOOP ======
void setup() {
  iniciarSerial();
  pinMode(PINO_RELE, OUTPUT);
  digitalWrite(PINO_RELE, LOW);

#if USE_WDT
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
  Serial.printf("[WDT] Habilitado (%ds).\n", WDT_TIMEOUT_S);
#endif

  randomSeed(esp_random());
  WiFi.mode(WIFI_STA);
  ultimaPublicacaoComSucesso = millis();
  garantirWiFi();
  garantirMQTT();

  Serial.println("[STATUS] Sistema pronto. Coletando amostras...");
}

void loop() {
#if USE_WDT
  esp_task_wdt_reset();
#endif

  garantirWiFi();
  garantirMQTT();
  clienteMqtt.loop();

  coletarEAvaliarMedia();
  tentarDrenarFilaOffline();

#if AUTO_RESTART
  // Reinício controlado se passar muito tempo sem publicar
  if (millis() - ultimaPublicacaoComSucesso > PRAZO_PUBLICACAO_MS) {
    Serial.println("[SAFETY] Sem publicações há muito tempo. Reiniciando firmware...");
    delay(500);
    ESP.restart();
  }
#endif

  delay(10);
}
