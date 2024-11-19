#include <WiFi.h>
#include <ESP32Time.h>
#include <HTTPClient.h>
#include <esp_camera.h>  // Biblioteca para ESP32-CAM

// Configurações Wi-Fi
const char* ssid = "SSID";
const char* password = "Senha";
const char* serverUrl = "http://:5000/upload";   //ajustar IP

ESP32Time rtc;  // Biblioteca de RTC para verificar a hora
bool fotoEnviadaHoje = false;

void setup() {
  Serial.begin(115200);

  // Configuração da câmera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_reset = -1;
  config.pin_pwdn = -1;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  esp_camera_init(&config);

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");
  
  rtc.setTime(0, 0, 15, 1, 1, 2024);  // Defina a hora inicial para testes
}

void loop() {
  int currentHour = rtc.getHour(true);

  // Tira uma foto e envia ao servidor às 15 horas, se ainda não foi enviada no dia
  if (currentHour == 15 && !fotoEnviadaHoje) {
    enviarFoto();
    fotoEnviadaHoje = true;  // Marca como enviada
  }

  // Reset diário ao passar para uma nova hora
  if (currentHour != 15) {
    fotoEnviadaHoje = false;
  }

  delay(60000);  // Verifica a hora a cada minuto
}

// Função para capturar e enviar a foto
void enviarFoto() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  // Captura a imagem

  if (!fb) {
    Serial.println("Falha ao capturar a foto");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");

  int httpResponseCode = http.POST((uint8_t*)fb->buf, fb->len);  // Envia a imagem ao servidor

  if (httpResponseCode > 0) {
    Serial.printf("Imagem enviada. Código de resposta: %d\n", httpResponseCode);
  } else {
    Serial.printf("Erro ao enviar imagem: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
  esp_camera_fb_return(fb);  // Libera o buffer da câmera
}
