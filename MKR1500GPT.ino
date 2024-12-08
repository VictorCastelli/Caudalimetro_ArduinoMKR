#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

NB modem;

//____________________________________________________________
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;  // Primer sensor de presión
const int pressureSensor2Pin = A2;  // Segundo sensor de presión
const int temperatureSensorPin = A1; // Sensor de temperatura LM335Z
const float V_REF = 3.3;             // Voltaje de referencia de la placa (3.3V para Arduino MKR1500)
long duration_1;
int distance_1;
//____________________________________________________________

int Segundo = 0;
const char pin[]      = " ";
const char apn[]      = "internet.iot";
const char login[]    = "kitiotum";
const char password[] = "k1t10tum";
const char topic2[]   = "/Proy_ALUR_UTEC";

NBClient net;
NB nbAccess;
MQTTClient client;

unsigned long lastMillis = 0;

//===============================================================
void connect() {
  // connection state
  bool connected = false;

  Serial.print("Connecting to cellular network ...");

  while (!connected) {
    if ((nbAccess.begin(pin, apn, login, password) == NB_READY)) {
      connected = true;
    } else {
      Serial.print(".");
      delay(1000);
    }
  }

  Serial.print("\nConnecting to MQTT broker ...");
  while (!client.connect("arduino", "kitiotum", "k1t10tum")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nConnected to MQTT broker!");

  client.subscribe(topic2);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming message: " + topic + " - " + payload);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin_1, OUTPUT);
  pinMode(echoPin_1, INPUT);

  client.begin("kitiot.antel.com.uy", net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  // Leer distancia del sensor de ultrasonido
  digitalWrite(trigPin_1, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin_1, HIGH);
  delayMicroseconds(150);
  digitalWrite(trigPin_1, LOW);
  duration_1 = pulseIn(echoPin_1, HIGH);
  distance_1 = duration_1 * 0.034 / 2;  // Calcula la distancia en centímetros

  // Leer los sensores de presión
  int pressureValue1 = analogRead(pressureSensor1Pin);
  int pressureValue2 = analogRead(pressureSensor2Pin);
  
  float V_out1 = pressureValue1 * (V_REF / 1023.0);  // Voltaje del primer sensor
  float V_out2 = pressureValue2 * (V_REF / 1023.0);  // Voltaje del segundo sensor

  float P_1 = (V_out1 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + 1.73; // Presión del primer sensor
  float P_2 = (V_out2 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + 1.73; // Presión del segundo sensor

  // Leer el sensor de temperatura LM335Z
  int tempValue = analogRead(temperatureSensorPin);
  float V_out_temp = tempValue * (V_REF / 1023.0);   // Voltaje del sensor LM335Z
  float temperatureK = V_out_temp / 0.01;            // Temperatura en Kelvin (10 mV/°K)
  float temperatureC = temperatureK - 273.15;        // Convertir a Celsius

  // Preparar mensaje MQTT en formato JSON simplificado para Ignition
  String mensaje = "[" + String(distance_1) + "," + String(P_1, 2) + "," + String(P_2, 2) + "," + String(temperatureC, 2) + "]";

  // Enviar datos al broker MQTT
  client.loop();
  if (!client.connected()) {
    Serial.println("Lost connection to MQTT broker. Reconnecting...");
    connect();
  }

  client.publish(topic2, mensaje);

  // Imprimir datos en el monitor serial
  Serial.println("Enviando datos: " + mensaje);

  // Esperar antes de la próxima iteración
  delay(10000);
}

