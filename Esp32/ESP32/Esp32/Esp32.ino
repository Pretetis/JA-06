#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Configurações Wi-Fi
const char* ssid = "SSID";
const char* password = "Senha";

// Configurações do servidor web para controle de LEDs
WebServer server(80);
const int ledPin1 = 27;
const int ledPin2 = 26;
const int ledPin3 = 25;

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
  }
}

// Funções do sensor de proximidade
void readProximity() {
  long duration, distance;
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distância: ");
  Serial.print(distance);
  Serial.println(" cm");

  display.clearDisplay();
  if (distance < DISTANCE_THRESHOLD && distance > 0) {
    drawHappyFace();
  } else {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Esperando movimento...");
    display.display();
  }
}

void drawHappyFace() {
  display.drawCircle(64, 32, 20, SSD1306_WHITE);
  display.fillCircle(54, 26, 3, SSD1306_WHITE);
  display.fillCircle(74, 26, 3, SSD1306_WHITE);
  drawSmile(64, 42, 20, 10);
  display.display();
}

void drawSmile(int x, int y, int w, int h) {
  for (int i = 0; i <= 180; i++) {
    int x0 = x + (w / 2) * cos(radians(i));
    int y0 = y + (h / 2) * sin(radians(i));
    display.drawPixel(x0, y0, SSD1306_WHITE);
  }
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
      client.println("Host: 192.168.100.21");
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
      client.println("Host: 192.168.100.21");
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
}

void loop() {
  server.handleClient();
  readDHT();
  readProximity();
  readSoilMoisture();
  readWaterLevel();
  delay(2000);
}
