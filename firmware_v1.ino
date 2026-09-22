#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

const int PINO_R = 14;
const int PINO_G = 27;
const int PINO_B = 26;

const char* VERSAO_ATUAL = "1.0";

const char* URL_MANIFESTO = "https://raw.githubusercontent.com/EduardoAbreu01/cp2-firmware/main/version.json";

int leituras[5];

unsigned long inicioSessao;
unsigned long ultimaLeitura;

int indiceLeitura = 0;
int soma = 0;

int sessoesCompletas = 0;
bool otaVerificada = false;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

void ledAzul() {
  analogWrite(PINO_R, 255);
  analogWrite(PINO_G, 255);
  analogWrite(PINO_B, 0);
}

String extrairCampoJSON(String json, String campo) {
  String procurado = "\"" + campo + "\"";
  int inicio = json.indexOf(procurado);
  if (inicio == -1) {
    return "";
  }
  
  inicio = json.indexOf(":", inicio);
  if (inicio == -1) {
    return "";
  }
  
  inicio++;
  while (inicio < json.length() && (json[inicio] == ' ' || json[inicio] == '\t' || json[inicio] == '\n' || json[inicio] == '\r')) {
    inicio++;
  }
  
  if (json[inicio] != '"') {
    return "";
  }
  
  inicio++;
  int fim = json.indexOf('"', inicio);
  if (fim == -1) {
    return "";
  }
  
  return json.substring(inicio, fim);
}

bool versaoMaisNova(String versaoDisponivel) {
  float atual = String(VERSAO_ATUAL).toFloat();
  float disponivel = versaoDisponivel.toFloat();
  return disponivel > atual;
}

bool realizarOTA(String urlFirmware) {
  Serial.println();
  Serial.println("====================================");
  Serial.println("INICIANDO ATUALIZACAO OTA");
  Serial.println("====================================");
  Serial.print("URL do firmware: ");
  Serial.println(urlFirmware);

  WiFiClientSecure cliente;
  cliente.setInsecure();
  HTTPClient http;

  Serial.println("[OTA] Conectando ao servidor...");
  if (!http.begin(cliente, urlFirmware)) {
    Serial.println("[OTA] Erro ao iniciar conexao HTTP.");
    return false;
  }

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int codigoHTTP = http.GET();

  Serial.print("[OTA] Codigo HTTP: ");
  Serial.println(codigoHTTP);

  if (codigoHTTP != HTTP_CODE_OK) {
    Serial.print("[OTA] Erro ao baixar firmware. Codigo: ");
    Serial.println(codigoHTTP);
    http.end();
    return false;
  }

  int tamanhoArquivo = http.getSize();
  Serial.print("[OTA] Tamanho do firmware: ");
  Serial.print(tamanhoArquivo);
  Serial.println(" bytes");

  if (tamanhoArquivo <= 0) {
    Serial.println("[OTA] Tamanho do firmware invalido.");
    http.end();
    return false;
  }

  if (!Update.begin(tamanhoArquivo)) {
    Serial.println("[OTA] Nao foi possivel iniciar gravacao.");
    Update.printError(Serial);
    http.end();
    return false;
  }

  Serial.println("[OTA] Gravacao iniciada.");
  WiFiClient* stream = http.getStreamPtr();
  size_t totalGravado = 0;

  while (http.connected() && totalGravado < (size_t)tamanhoArquivo) {
    size_t disponivel = stream->available();
    if (disponivel > 0) {
      uint8_t buffer[1024];
      size_t quantidade = stream->readBytes(buffer, min(disponivel, sizeof(buffer)));
      size_t gravado = Update.write(buffer, quantidade);
      totalGravado += gravado;

      Serial.print("\r[OTA] Progresso: ");
      float progresso = (totalGravado * 100.0) / tamanhoArquivo;
      Serial.print(progresso, 1);
      Serial.print("%");
    }
    delay(1);
  }
  Serial.println();

  if (totalGravado != (size_t)tamanhoArquivo) {
    Serial.println("[OTA] Download incompleto.");
    Update.abort();
    http.end();
    return false;
  }

  if (!Update.end()) {
    Serial.println("[OTA] Erro ao finalizar atualizacao.");
    Update.printError(Serial);
    http.end();
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("[OTA] Atualizacao nao foi finalizada.");
    http.end();
    return false;
  }

  http.end();
  Serial.println("[OTA] Firmware gravado com sucesso!");
  Serial.println("[OTA] Reiniciando ESP32...");
  delay(2000);
  ESP.restart();
  return true;
}

void verificarAtualizacao() {
  Serial.println();
  Serial.println("====================================");
  Serial.println("VERIFICANDO ATUALIZACAO");
  Serial.println("====================================");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] Wi-Fi desconectado.");
    Serial.println("[OTA] Atualizacao nao realizada.");
    return;
  }

  Serial.println("[OTA] Wi-Fi conectado.");
  WiFiClientSecure cliente;
  cliente.setInsecure();
  HTTPClient http;

  Serial.println("[OTA] Consultando version.json...");

  if (!http.begin(cliente, URL_MANIFESTO)) {
    Serial.println("[OTA] Nao foi possivel acessar o manifesto.");
    return;
  }

  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int codigoHTTP = http.GET();

  if (codigoHTTP != HTTP_CODE_OK) {
    Serial.print("[OTA] Erro ao acessar manifesto. Codigo HTTP: ");
    Serial.println(codigoHTTP);
    http.end();
    return;
  }

  String manifesto = http.getString();
  http.end();

  String versaoDisponivel = extrairCampoJSON(manifesto, "version");
  String urlFirmware = extrairCampoJSON(manifesto, "url");

  if (versaoDisponivel == "" || urlFirmware == "") {
    Serial.println("[OTA] Manifesto invalido.");
    return;
  }

  Serial.print("[OTA] Versao instalada: ");
  Serial.println(VERSAO_ATUAL);
  Serial.print("[OTA] Versao disponivel: ");
  Serial.println(versaoDisponivel);
  Serial.print("[OTA] URL do firmware: ");
  Serial.println(urlFirmware);

  if (!versaoMaisNova(versaoDisponivel)) {
    Serial.println("[OTA] A versao instalada ja e a mais recente.");
    return;
  }

  Serial.println("[OTA] Nova versao encontrada!");
  Serial.print("[OTA] Atualizando de ");
  Serial.print(VERSAO_ATUAL);
  Serial.print(" para ");
  Serial.println(versaoDisponivel);

  realizarOTA(urlFirmware);
}

void realizarLeitura() {
  leituras[indiceLeitura] = random(10, 21);
  soma += leituras[indiceLeitura];

  Serial.print("Leitura ");
  Serial.print(indiceLeitura + 1);
  Serial.print(": ");
  Serial.print(leituras[indiceLeitura]);
  Serial.println(" cm");

  indiceLeitura++;
  ultimaLeitura = millis();

  if (indiceLeitura == 5) {
    float media = soma / 5.0;

    Serial.println();
    Serial.print("Media da sessao: ");
    Serial.print(media, 1);
    Serial.println(" cm");

    sessoesCompletas++;
    Serial.print("Sessoes completas: ");
    Serial.println(sessoesCompletas);
    Serial.println();

    if (sessoesCompletas >= 3 && !otaVerificada) {
      otaVerificada = true;
      Serial.println("3 ciclos completos. Iniciando verificacao OTA...");
      verificarAtualizacao();
    }
    
    Serial.println("Proxima sessao em aproximadamente 40 segundos.");
    Serial.println();
  }
}

void iniciarSessao() {
  inicioSessao = millis();
  ultimaLeitura = inicioSessao;
  indiceLeitura = 0;
  soma = 0;

  Serial.println("====================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 1.0");
  Serial.println("====================================");
  Serial.println();

  realizarLeitura();
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_R, OUTPUT);
  pinMode(PINO_G, OUTPUT);
  pinMode(PINO_B, OUTPUT);

  ledAzul();
  randomSeed(analogRead(0));

  Serial.print("A ligar à rede Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long inicioWiFi = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - inicioWiFi < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Ligação Wi-Fi estabelecida com sucesso!");
    Serial.print("Endereço IP atribuído: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Nao foi possivel conectar ao Wi-Fi.");
    Serial.println("[WIFI] O firmware continuara executando.");
  }

  Serial.println("====================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 1.0");
  Serial.println("====================================");
  Serial.println("Firmware 1.0 em execução.");
  Serial.println();

  iniciarSessao();
}

void loop() {
  unsigned long agora = millis();

  if (agora - inicioSessao >= 48000) {
    iniciarSessao();
    return;
  }

  if (indiceLeitura < 5) {
    if (agora - ultimaLeitura >= 2000) {
      realizarLeitura();
    }
  }
}
