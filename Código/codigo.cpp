#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const char* default_SSID = "Wokwi-GUEST";
const char* default_PASSWORD = "";
const char* default_BROKER_MQTT = "20.171.10.58";
const int default_BROKER_PORT = 1883;
const char* default_TOPICO_SUBSCRIBE = "/ESP/jogador001/cmd";
const char* default_TOPICO_PUBLISH_VELOCIDADE = "/ESP/jogador001/attrs/velocidade";
const char* default_TOPICO_PUBLISH_BPM = "/ESP/jogador001/attrs/bpm";
const char* default_TOPICO_PUBLISH_ACEL = "/ESP/jogador001/attrs/acelx";
const char* default_TOPICO_PUBLISH_TEMP = "/ESP/jogador001/attrs/temp";
const char* default_ID_MQTT = "esp32_jogador001";

WiFiClient espClient;
PubSubClient MQTT(espClient);

Adafruit_MPU6050 mpu;

int potVelocidade = 33;
int potBatimento = 32;
int ledBatimento = 2;
int velocidade;
int vel;
int pote;
int tempoBatimento;
int bpm;

void initWiFi() {
  Serial.begin(115200);
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(default_SSID, default_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void initMQTT() {
  MQTT.setServer(default_BROKER_MQTT, default_BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void initMPU() {
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar o chip MPU6050!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 encontrado com sucesso!");
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando conectar ao broker MQTT: ");
    Serial.println(default_BROKER_MQTT);
    if (MQTT.connect(default_ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker!");
      MQTT.subscribe(default_TOPICO_SUBSCRIBE);
    } else {
      Serial.println("Falha na conexão. Tentando novamente em 2s...");
      delay(2000);
    }
  }
}

void VerificaConexoesWiFIEMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    initWiFi();
  }
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);
}

void publicaDados() {
  vel = analogRead(potVelocidade);
  velocidade = map(vel, 0, 4095, 1, 60);

  pote = analogRead(potBatimento);
  tempoBatimento = map(pote, 0, 4095, 400, 1200);
  bpm = tempoBatimento > 0 ? 60000 / tempoBatimento : 0;

  digitalWrite(ledBatimento, HIGH);
  delay(100);
  digitalWrite(ledBatimento, LOW);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.println("=============================");
  Serial.print("Velocidade: "); Serial.print(velocidade); Serial.println(" Km/h");
  Serial.print("Batimentos: "); Serial.print(bpm); Serial.println(" bpm");
  Serial.print("Aceleração X: "); Serial.println(a.acceleration.x);
  Serial.print("Temperatura: "); Serial.print(temp.temperature); Serial.println(" °C");
  Serial.println("Dados publicados via MQTT!");
  Serial.println("=============================\n");

  MQTT.publish(default_TOPICO_PUBLISH_VELOCIDADE, String(velocidade).c_str());
  MQTT.publish(default_TOPICO_PUBLISH_BPM, String(bpm).c_str());
  MQTT.publish(default_TOPICO_PUBLISH_ACEL, String(a.acceleration.x).c_str());
  MQTT.publish(default_TOPICO_PUBLISH_TEMP, String(temp.temperature).c_str());
}

void setup() {
  pinMode(ledBatimento, OUTPUT);
  initWiFi();
  initMQTT();
  initMPU();
}

void loop() {
  VerificaConexoesWiFIEMQTT();
  publicaDados();
  MQTT.loop();
  delay(2000);
}
