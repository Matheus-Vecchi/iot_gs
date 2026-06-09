#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"

#define DHTPIN 15
#define DHTTYPE DHT22
#define PINO_SOLO 34      
#define LED_ALERTA 2      
#define LED_IRRIGACAO 4   

#define LARGURA_OLED 128
#define ALTURA_OLED 64

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, -1);
WebServer server(80);


const char* ssid = "Wokwi-GUEST";
const char* senha = "";

float temperatura = 0;
float umidadeAr = 0;
int umidadeSolo = 0;
bool irrigacaoLigada = false;
bool alertaSolo = false;

const int LIMIAR_SOLO = 40;
const float LIMIAR_TEMP = 35.0;

String paginaHTML() {
  String html = "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'>";
  html += "<title>Eclipse Protocol</title></head><body>";
  html += "<h2>Eclipse Protocol - Monitoramento</h2>";
  html += "<p>Temperatura: <span id='temp'>--</span></p>";
  html += "<p>Umidade do ar: <span id='umiAr'>--</span></p>";
  html += "<p>Umidade do solo: <span id='umiSolo'>--</span></p>";
  html += "<p>Status: <span id='status'>--</span></p>";
  html += "<script>";
  html += "async function atualizar(){";
  html += "const r=await fetch('/api/sensors');const d=await r.json();";
  html += "document.getElementById('temp').innerText=d.temperatura+' C';";
  html += "document.getElementById('umiAr').innerText=d.umidade_ar+' %';";
  html += "document.getElementById('umiSolo').innerText=d.umidade_solo+' %';";
  html += "document.getElementById('status').innerText=d.alerta_solo?'SOLO SECO - IRRIGANDO':'SOLO OK';";
  html += "}";
  html += "setInterval(atualizar,2000);atualizar();";
  html += "</script></body></html>";
  return html;
}

void rotaRaiz() {
  server.send(200, "text/html", paginaHTML());
}

void rotaSensors() {
  String json = "{";
  json += "\"temperatura\":" + String(temperatura, 1) + ",";
  json += "\"umidade_ar\":" + String(umidadeAr, 1) + ",";
  json += "\"umidade_solo\":" + String(umidadeSolo) + ",";
  json += "\"alerta_solo\":" + String(alertaSolo ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void rotaStatus() {
  String json = "{";
  json += "\"irrigacao_ligada\":" + String(irrigacaoLigada ? "true" : "false") + ",";
  json += "\"alerta_solo\":" + String(alertaSolo ? "true" : "false") + ",";
  json += "\"limiar_solo\":" + String(LIMIAR_SOLO);
  json += "}";
  server.send(200, "application/json", json);
}

void rotaIrrigacao() {
  irrigacaoLigada = !irrigacaoLigada;
  digitalWrite(LED_IRRIGACAO, irrigacaoLigada ? HIGH : LOW);
  String json = "{\"irrigacao_ligada\":" + String(irrigacaoLigada ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_ALERTA, OUTPUT);
  pinMode(LED_IRRIGACAO, OUTPUT);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha no OLED");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Eclipse Protocol");
  display.println("Conectando WiFi...");
  display.display();

  WiFi.begin(ssid, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", rotaRaiz);
  server.on("/api/sensors", rotaSensors);
  server.on("/api/status", rotaStatus);
  server.on("/api/irrigation", HTTP_POST, rotaIrrigacao);
  server.begin();
}

void loop() {
  server.handleClient();

  float t = dht.readTemperature();
  float u = dht.readHumidity();
  if (!isnan(t)) temperatura = t;
  if (!isnan(u)) umidadeAr = u;

  int leituraSolo = analogRead(PINO_SOLO);          // 0 a 4095
  umidadeSolo = map(leituraSolo, 0, 4095, 0, 100);  // converte para %

  bool soloSeco = umidadeSolo < LIMIAR_SOLO;
  bool calorExcessivo = temperatura > LIMIAR_TEMP;

  alertaSolo = soloSeco;
  digitalWrite(LED_ALERTA, soloSeco ? HIGH : LOW);

  if (soloSeco || calorExcessivo) {
    irrigacaoLigada = true;
    digitalWrite(LED_IRRIGACAO, HIGH);
  } else {
    irrigacaoLigada = false;
    digitalWrite(LED_IRRIGACAO, LOW);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Eclipse Protocol");
  display.println("----------------");
  display.print("Temp: "); display.print(temperatura, 1); display.println(" C");
  display.print("Umi Ar: "); display.print(umidadeAr, 1); display.println(" %");
  display.print("Solo: "); display.print(umidadeSolo); display.println(" %");
  display.println("----------------");
  display.println(alertaSolo ? "ALERTA: SOLO SECO" : "Solo OK");
  display.display();

  delay(1000);
}