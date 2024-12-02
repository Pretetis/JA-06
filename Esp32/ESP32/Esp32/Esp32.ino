#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Configurações Wi-Fi
const char* ssid = "SSID";
const char* password = "SENHA";

// Configurações do servidor web para controle de LEDs
WebServer server(80);
const int ledPin1 = 27;
const int ledPin2 = 26;
const int ledPin3 = 25;

// Configuração do cliente NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800); // Fuso horário de Brasília (UTC-3)

// Variáveis para data/hora
String formattedDate;
String formattedTime;

// Configurações do sensor DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const char* serverName = "http://:5000/endpoint"; //ajustar IP

// Configurações do sensor de proximidade e tela OLED
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define DISTANCE_THRESHOLD 20

// Configurações do sensor de umidade do solo
const int soilMoisturePin = 34;
const char* soilMoistureUrl = "http://:5000/data"; //ajustar IP

// Configurações do sensor de nível de água
const int waterLevelPin = 32;
const char* waterLevelUrl = "http://:5000/update"; //ajustar IP

// Funções para alternar LEDs
void handleLed1() {
  digitalWrite(ledPin1, !digitalRead(ledPin1));
  server.send(200, "text/plain", "LED 1 foi alternado!");
}

void handleLed2() {
  digitalWrite(ledPin2, !digitalRead(ledPin2));
  server.send(200, "text/plain", "LED 2 foi alternado!");
}

void handleLed3() {
  digitalWrite(ledPin3, !digitalRead(ledPin3));
  server.send(200, "text/plain", "LED 3 foi alternado!");
}

void handleRoot() {
  String html = "<html><head><title>Controle de LEDs</title></head><body>";
  html += "<h1>Controle de LEDs</h1>";
  html += "<button onclick=\"fetch('/led1')\">Alternar LED 1</button>";
  html += "<button onclick=\"fetch('/led2')\">Alternar LED 2</button>";
  html += "<button onclick=\"fetch('/led3')\">Alternar LED 3</button>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Função de leitura do sensor DHT11
void readDHT() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha ao ler o sensor DHT!");
  } else {
    Serial.print("Temperatura: ");
    Serial.println(temperatura);
    Serial.print("Umidade: ");
    Serial.println(umidade);

    sendDHTData(temperatura, umidade);  // Envia os dados ao Flask
  }
}

// Função de leitura do sensor DHT11 e envio para o servidor Flask
void sendDHTData(float temperatura, float umidade) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "temperature=" + String(temperatura) + "&humidity=" + String(umidade);
    
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      Serial.print("Resposta do servidor: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro na requisição HTTP: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // Fecha a conexão
  } else {
    Serial.println("Falha na conexão Wi-Fi.");
  }
}

// Funções do sensor de proximidade
unsigned long lastFaceDrawTime = 0;
const unsigned long faceDisplayDuration = 5000; // 5 segundos

void readProximity() {
    long duration, distance;

    // Mede a distância
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;

    // Verifica a distância
    if (distance < DISTANCE_THRESHOLD && distance > 0) {
        // Loop para manter o rosto feliz enquanto a distância for menor que o limite
        unsigned long startTime = millis();
        while (true) {
            // Recalcula a distância
            digitalWrite(TRIGGER_PIN, LOW);
            delayMicroseconds(2);
            digitalWrite(TRIGGER_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(TRIGGER_PIN, LOW);
            duration = pulseIn(ECHO_PIN, HIGH);
            distance = duration * 0.034 / 2;

            // Se a distância ultrapassar o limite ou o tempo exceder o limite, sai do loop
            if (distance >= DISTANCE_THRESHOLD || (millis() - startTime > faceDisplayDuration)) {
                break;
            }

            // Exibe o rosto feliz
            display.clearDisplay();
            drawHappyFace();
            display.display();

            // Pequeno atraso para evitar sobrecarga
            delay(100);
        }
    } else {
        // Exibe os dados apenas se a distância estiver acima do limite
        float temperatura = dht.readTemperature();
        float umidade = dht.readHumidity();
        int soilMoistureValue = analogRead(soilMoisturePin);
        int waterLevelValue = analogRead(waterLevelPin);

        displayData(temperatura, umidade, soilMoistureValue, waterLevelValue, distance);
    }
}

void drawHappyFace() {
  int centerX = 64; // Centro do rosto na horizontal
  int centerY = 40; // Centro do rosto ajustado para a parte azul
  int radius = 20;  // Raio do círculo

  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);   // Cabeça
  display.fillCircle(centerX - 10, centerY - 6, 3, SSD1306_WHITE); // Olho esquerdo
  display.fillCircle(centerX + 10, centerY - 6, 3, SSD1306_WHITE); // Olho direito
  drawSmile(centerX, centerY + 6, radius - 10, 5);               // Sorriso

  display.display();
}

void drawSmile(int x, int y, int w, int h) {
  for (int i = 0; i <= 180; i++) {
    int x0 = x + (w / 2) * cos(radians(i));
    int y0 = y + (h / 2) * sin(radians(i));
    display.drawPixel(x0, y0, SSD1306_WHITE);
  }
}

void displayData(float temperatura, float umidade, int soilMoisture, int waterLevel, long distance) {
  display.clearDisplay();

    // Atualiza o horário via NTP
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  // Exibe o horário no topo do display (amarelo)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  // Posição inicial no canto superior esquerdo
  display.print("Hora: ");
  display.print(currentTime);
  
  // Exibe as informações na tela OLED
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.print("Temp: ");
  display.print(temperatura);
  display.print("C");

  display.setCursor(0, 26);
  display.print("Umid: ");
  display.print(umidade);
  display.print("%");

  display.setCursor(0, 36);
  display.print("Umid Solo: ");
  display.print(soilMoisture);

  display.setCursor(0, 46);
  display.print("Nivel Agua: ");
  display.print(String(waterLevel));


  display.display();
}

// Função de leitura do sensor de umidade do solo
void readSoilMoisture() {
  int sensorValueAnalog = analogRead(soilMoisturePin);
  Serial.print("Umidade do Solo: ");
  Serial.println(sensorValueAnalog);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(soilMoistureUrl, 80)) {
      String postData = "message=" + String(sensorValueAnalog);
      client.println("POST /data HTTP/1.1");
      client.println("Host: seuIP");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.println(postData);
    }
    client.stop();
  }
}

// Função de leitura do sensor de nível de água
void readWaterLevel() {
  int waterLevel = analogRead(waterLevelPin);
  Serial.print("Nível de água: ");
  Serial.println(waterLevel);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(waterLevelUrl, 80)) {
      String postData = "waterLevel=" + String(waterLevel);
      client.println("POST /update HTTP/1.1");
      client.println("Host: seuIP");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.println(postData);
    }
    client.stop();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos de LEDs
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);

  // Configuração do sensor de proximidade e OLED
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao inicializar a tela OLED"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // Inicialização do sensor DHT
  dht.begin();

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Inicialização do servidor
  server.on("/", handleRoot);
  server.on("/led1", handleLed1);
  server.on("/led2", handleLed2);
  server.on("/led3", handleLed3);
  server.begin();

  // Inicializa o cliente NTP
  timeClient.begin();
  timeClient.update();
}

void loop() {
  server.handleClient();

  // Lê os sensores
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int soilMoistureValue = analogRead(soilMoisturePin);
  int waterLevelValue = analogRead(waterLevelPin);

  // Atualiza os dados e exibe no display
  readProximity();
  displayData(temperatura, umidade, soilMoistureValue, waterLevelValue, DISTANCE_THRESHOLD);

  // Atualiza os dados do servidor
  readDHT();
  readSoilMoisture();
  readWaterLevel();

  delay(1000); // Tempo entre atualizações
}
