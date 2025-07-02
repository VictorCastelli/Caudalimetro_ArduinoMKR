#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

// Configuración de la red celular y MQTT
const char pin[]      = "";                     // PIN de la SIM
const char apn[]      = "internet.iot";         // APN del operador
const char login[]    = "kitiotum";             // Usuario de red
const char password[] = "k1t10tum";             // Contraseña de red
const char broker[]   = "kitiot.antel.com.uy";  // Servidor MQTT
const char topic2[]   = "/Proy_ALUR_UTEC";      // Topic principal

// Pines de sensores
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;
const int pressureSensor2Pin = A2;
const int temperatureSensorPin = A1;
const float V_REF = 3.3;

// Objetos NB-IoT y MQTT
NB nbAccess;
NBClient net;
MQTTClient client;

// Variables globales
long duration_1;
int distance_1;
unsigned long lastResetTime = 0;
#define RESET_INTERVAL 900000  // Reiniciar cada 15 minutos
unsigned long lastPingTime = 0; // Para keep-alive
unsigned long lastSendTime = 0; // Para control de envío principal

// ===========================================================
// Reconexión de red celular
bool connectCellular() {
  int retryCount = 0;
  Serial.println("Iniciando conexión a la red celular...");

  while (nbAccess.status() != NB_READY) {
    Serial.println("Intentando conectar a la red celular...");
    if (nbAccess.begin(pin, apn, login, password) == NB_READY) {
      Serial.println("Conexión exitosa a la red celular.");
      return true;
    } else {
      Serial.println("Error al conectar a la red celular.");
    }

    retryCount++;
    if (retryCount >= 5) {
      Serial.println("Reiniciando el modem NB-IoT...");
      nbAccess.shutdown();
      delay(5000);
      retryCount = 0;
    }
    delay(3000);
  }
  return false;
}

// ===========================================================
// Reconexión al broker MQTT
bool connectMQTT() {
  Serial.println("Conectando al broker MQTT...");

  client.begin(broker, net);
  if (!client.connect("arduino", "kitiotum", "k1t10tum")) {
    Serial.print("Error MQTT: ");
    Serial.println(client.lastError());
    return false;
  }
  Serial.println("Conectado al broker MQTT.");
  return true;
}

// ===========================================================
// Conexión completa
void connect() {
  if (!connectCellular()) {
    Serial.println("No se pudo conectar a la red celular.");
    return;
  }
  if (!connectMQTT()) {
    Serial.println("No se pudo conectar a MQTT.");
    return;
  }
}

// ===========================================================
// Configuración inicial
void setup() {
  Serial.begin(115200);
  pinMode(trigPin_1, OUTPUT);
  pinMode(echoPin_1, INPUT);
  connect();
  lastResetTime = millis();
  lastSendTime = millis();
  lastPingTime = millis();
}

// ===========================================================
// Bucle principal
void loop() {
  client.loop();  // Mantiene viva la conexión MQTT

  // Verificar conexión MQTT
  if (!client.connected()) {
    Serial.println("MQTT desconectado. Intentando reconectar...");
    if (!connectMQTT()) {
      Serial.println("Fallo en la reconexión MQTT.");
      delay(5000);
      return;
    }
  }

  unsigned long currentMillis = millis();

  // 1️⃣ Enviar mensaje real cada 5 minutos
  if (currentMillis - lastSendTime >= 300000) {  // 5 * 60 * 1000
    Serial.println("\nEnviando datos de sensores...");

    // Lectura de sensores
    digitalWrite(trigPin_1, LOW);
    delayMicroseconds(5);
    digitalWrite(trigPin_1, HIGH);
    delayMicroseconds(150);
    digitalWrite(trigPin_1, LOW);
    duration_1 = pulseIn(echoPin_1, HIGH);
    distance_1 = duration_1 * 0.034 / 2;

    int pressureValue1 = analogRead(pressureSensor1Pin);
    int pressureValue2 = analogRead(pressureSensor2Pin);
    float V_out1 = pressureValue1 * (V_REF / 1023.0);
    float V_out2 = pressureValue2 * (V_REF / 1023.0);
    float P_1 = (V_out1 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + 1.73;
    float P_2 = (V_out2 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09);

    int tempValue = analogRead(temperatureSensorPin);
    float temperatureC = (tempValue * (V_REF / 1023.0) / 0.01) - 273.15;

    char mensaje[80];
    snprintf(mensaje, sizeof(mensaje), "[%d,%.2f,%.2f,%.2f]", distance_1, P_1, P_2, temperatureC);

    client.publish(topic2, mensaje);
    Serial.print("Enviado: ");
    Serial.println(mensaje);

    lastSendTime = currentMillis;  // Actualizar timestamp de envío
  }

  // 2️⃣ Enviar mensaje keep-alive vacío cada 60 s para evitar cierre del módem
  if (currentMillis - lastPingTime >= 60000) {
    client.publish(topic2, "");
    Serial.println("Keep-alive enviado.");
    lastPingTime = currentMillis;
  }

  // 3️⃣ Reinicio programado cada 15 minutos
  if (currentMillis - lastResetTime >= RESET_INTERVAL) {
    Serial.println("Reinicio programado...");
    delay(1000);
    NVIC_SystemReset();
  }

  delay(100);  // Pequeño delay para evitar sobrecargar CPU
}
