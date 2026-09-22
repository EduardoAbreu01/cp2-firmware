#include <WiFi.h>

const int PINO_R = 14;
const int PINO_G = 27;
const int PINO_B = 26;

int leituras[5];
int leiturasOrdenadas[5];

unsigned long inicioSessao;
unsigned long ultimaLeitura;

int indiceLeitura = 0;
int soma = 0;

bool estadoAlerta = false;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

void ledAzul() {
  analogWrite(PINO_R, 255);
  analogWrite(PINO_G, 255);
  analogWrite(PINO_B, 0);
}

void ledVerde() {
  analogWrite(PINO_R, 255);
  analogWrite(PINO_G, 0);
  analogWrite(PINO_B, 255);
}

void ledVermelho() {
  analogWrite(PINO_R, 0);
  analogWrite(PINO_G, 255);
  analogWrite(PINO_B, 255);
}

void ordenarLeituras() {
  for (int i = 0; i < 5; i++) {
    leiturasOrdenadas[i] = leituras[i];
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4 - i; j++) {
      if (leiturasOrdenadas[j] > leiturasOrdenadas[j + 1]) {
        int temporario = leiturasOrdenadas[j];
        leiturasOrdenadas[j] = leiturasOrdenadas[j + 1];
        leiturasOrdenadas[j + 1] = temporario;
      }
    }
  }
}

void exibirLeiturasOriginais() {
  Serial.print("Valores originais: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(leituras[i]);
    if (i < 4) {
      Serial.print(" ");
    }
  }
  Serial.println();
}

void exibirLeiturasOrdenadas() {
  Serial.print("Valores ordenados: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(leiturasOrdenadas[i]);
    if (i < 4) {
      Serial.print(" ");
    }
  }
  Serial.println();
}

int calcularMediana() {
  return leiturasOrdenadas[2];
}

void atualizarEstado(int mediana) {
  if (mediana >= 16) {
    estadoAlerta = true;
  } else if (mediana <= 14) {
    estadoAlerta = false;
  }
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

    ordenarLeituras();
    exibirLeiturasOriginais();
    exibirLeiturasOrdenadas();

    int mediana = calcularMediana();

    Serial.print("Mediana da sessao: ");
    Serial.print(mediana);
    Serial.println(" cm");

    atualizarEstado(mediana);

    if (estadoAlerta == true) {
      ledVermelho();
      Serial.println("Estado: ALERTA");
      Serial.println("LED: VERMELHO");
    } else {
      ledVerde();
      Serial.println("Estado: NORMAL");
      Serial.println("LED: VERDE");
    }

    Serial.println();
    Serial.println("Proxima sessao em 48 segundos.");
    Serial.println();
  }
}

void iniciarSessao() {
  inicioSessao = millis();
  ultimaLeitura = inicioSessao;
  indiceLeitura = 0;
  soma = 0;

  Serial.println("====================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 2.0");
  Serial.println("====================================");
  Serial.println("Nova sessao iniciada.");
  Serial.println();

  realizarLeitura();
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_R, OUTPUT);
  pinMode(PINO_G, OUTPUT);
  pinMode(PINO_B, OUTPUT);

  estadoAlerta = false;
  ledVerde();

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
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 2.0");
  Serial.println("====================================");
  Serial.println("Firmware 2.0 em execução.");
  Serial.println("Estado inicial: NORMAL");
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