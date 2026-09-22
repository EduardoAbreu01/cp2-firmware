#include <WiFi.h>

const int PINO_R = 14;
const int PINO_G = 27;
const int PINO_B = 26;

int leituras[5];

unsigned long inicioSessao;
unsigned long ultimaLeitura;

int indiceLeitura = 0;
int soma = 0;

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

    Serial.print("Media da sessao: ");
    Serial.print(media, 1);
    Serial.println(" cm");

    Serial.println("Proxima sessao em aproximadamente 38 segundos.");
    Serial.println();
  }
}

void iniciarSessao() {
  inicioSessao = millis();
  ultimaLeitura = inicioSessao;

  indiceLeitura = 0;
  soma = 0;


  realizarLeitura();
}

const char* ssid = "Wokwi-GUEST";
const char* password = "";

void setup() {
  Serial.begin(115200);
  pinMode(PINO_R, OUTPUT);
  pinMode(PINO_G, OUTPUT);
  pinMode(PINO_B, OUTPUT);

  analogWrite(PINO_R, 255);
  analogWrite(PINO_G, 255);
  analogWrite(PINO_B, 0);

  randomSeed(analogRead(0));

  Serial.print("A ligar à rede Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Ligação Wi-Fi estabelecida com sucesso!");
  Serial.print("Endereço IP atribuído: ");
  Serial.println(WiFi.localIP());

  Serial.println("====================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 1.0");
  Serial.println("====================================");
}

void loop() {
  unsigned long agora = millis();

  if (indiceLeitura < 5) {
    if (agora - ultimaLeitura >= 2000) {
      realizarLeitura();
    }
  }

  if (agora - inicioSessao >= 48000) {
    iniciarSessao();
  }
}