#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

NB modem;

//____________________________________________________________
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;
const int pressureSensor2Pin = A2;
const int temperatureSensorPin = A1;
const float V_REF = 3.3;
long duration_1;
int distance_1;
//____________________________________________________________

const char pin[]      = " ";
const char apn[]      = "internet.iot";
const char login[]    = "kitiotum";
const char password[] = "k1t10tum";
const char topic2[]   = "/Proy_ALUR_UTEC";

NBClient net;
NB nbAccess;
MQTTClient client;

unsigned long lastSendTime = 0;  // Tiempo del último envío de datos
unsigned long startTime = 0;     // Tiempo en el que inició el equipo

//===============================================================
void connect() {
  Serial.println("Iniciando conexión a la red celular...");

  bool connected = false;
  int intentos = 0;

  while (!connected) {
    Serial.print("Intento ");
    Serial.println(intentos + 1);

    if (nbAccess.begin(pin, apn, login, password) == NB_READY) {
      connected = true;
    } else {
      Serial.println("Error conectando a la red. Reintentando...");
      delay(2000);
    }

    intentos++;
    if (intentos >= 5) {  // Si después de 5 intentos falla, reinicia
      Serial.println("No se pudo conectar a la red. Reiniciando...");
      resetDevice();
    }
  }

  Serial.println("Conectado a la red celular!");

  Serial.print("Conectando a broker MQTT...");
  while (!client.connect("arduino", "kitiotum", "k1t10tum")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nConectado al broker MQTT!");

  client.subscribe(topic2);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Mensaje recibido: " + topic + " - " + payload);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  Serial.println("Iniciando configuración...");
  
  pinMode(trigPin_1, OUTPUT);
  pinMode(echoPin_1, INPUT);

  Serial.println("Configurando cliente MQTT...");
  client.begin("kitiot.antel.com.uy", net);
  client.onMessage(messageReceived);

  connect();

  startTime = millis();  // Guarda el tiempo de inicio
  lastSendTime = millis(); // Inicializa el contador de envíos
}

void loop() {
  unsigned long currentTime = millis();

  // Verifica si han pasado 5 minutos para enviar datos
  if (currentTime - lastSendTime >= 300000) {  // 5 minutos = 300000 ms
    sendData();
    lastSendTime = millis();  // Actualiza el tiempo del último envío
  }

  // Verifica si han pasado 15 minutos para reiniciar
  if (currentTime - startTime >= 900000) {  // 15 minutos = 900000 ms
    Serial.println("Reiniciando dispositivo en 5 segundos...");
    delay(5000);
    resetDevice();
  }
}

void sendData() {
  Serial.println("Inicio de ciclo LOOP...");

  // Leer distancia del sensor de ultrasonido
  Serial.println("Midiendo distancia...");
  digitalWrite(trigPin_1, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin_1, HIGH);
  delayMicroseconds(150);
  digitalWrite(trigPin_1, LOW);
  duration_1 = pulseIn(echoPin_1, HIGH);
  distance_1 = duration_1 * 0.034 / 2;
  Serial.print("Distancia medida: ");
  Serial.println(distance_1);

  // Leer los sensores de presión
  //Serial.println("Leyendo sensores de presión...");
  int pressureValue1 = analogRead(pressureSensor1Pin);
  int pressureValue2 = analogRead(pressureSensor2Pin);
  
  float V_out1 = pressureValue1 * (V_REF / 1023.0);
  float V_out2 = pressureValue2 * (V_REF / 1023.0);

  float P_1 = (V_out1 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + 1.73;
  float P_2 = (V_out2 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09);

  // Serial.print("Presión 1: ");
  // Serial.println(P_1, 2);
  // Serial.print("Presión 2: ");
  // Serial.println(P_2, 2);

  // Leer el sensor de temperatura
  //Serial.println("Midiendo temperatura...");
  int tempValue = analogRead(temperatureSensorPin);
  float V_out_temp = tempValue * (V_REF / 1023.0);
  float temperatureK = V_out_temp / 0.01;
  float temperatureC = temperatureK - 273.15;

  Serial.print("Temperatura: ");
  Serial.println(temperatureC, 2);

  // Preparar mensaje MQTT
  String mensaje = "[" + String(distance_1) + "," + String(P_1, 2) + "," + String(P_2, 2) + "," + String(temperatureC, 2) + "]";
  Serial.print("Enviando mensaje MQTT: ");
  Serial.println(mensaje);

  // Verificar conexión MQTT y enviar datos
  client.loop();
  if (!client.connected()) {
    Serial.println("Conexión MQTT perdida. Reconectando...");
    connect();
  }

  if (client.publish(topic2, mensaje)) {
    Serial.println("Mensaje enviado correctamente!");
  } else {
    Serial.println("Error al enviar el mensaje MQTT.");
  }
}

void resetDevice() {
  Serial.println("Limpiando memoria...");
  
  // Cerrar conexiones antes de reiniciar
  client.disconnect();  
  nbAccess.shutdown();

  // Esperar un poco antes del reset
  delay(1000);

  Serial.println("Reiniciando Arduino...");
  NVIC_SystemReset();  // Reinicia el microcontrolador
}
