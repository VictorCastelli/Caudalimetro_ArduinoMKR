#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

NB modem;
NBClient net;
NB nbAccess;
MQTTClient client;

//____________________________________________________________
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;
const int pressureSensor2Pin = A2;
const int temperatureSensorPin = A1;
const float V_REF = 3.3;
//____________________________________________________________

const char pin[] = " ";
const char apn[] = "internet.iot";
const char login[] = "kitiotum";
const char password[] = "k1t10tum";
const char topic2[] = "/Proy_ALUR_UTEC";

unsigned long lastResetTime = 0;
unsigned long lastSendTime = 0;
#define RESET_INTERVAL 900000  // 15 minutos
#define SEND_INTERVAL 60000    // 1 minuto

//===============================================================
// FUNCIONES DE CONEXIÓN
void connectCellular() {
    Serial.println("\nConectando a red celular...");
    nbAccess.shutdown(); // Reinicio preventivo
    
    int attempts = 0;
    while (nbAccess.status() != NB_READY) {
        if (nbAccess.begin(pin, apn, login, password) == NB_READY) {
            Serial.println("Conexión celular exitosa");
            return;
        }
        
        Serial.print(".");
        if (++attempts >= 15) {
            Serial.println("\nFallo crítico: Reiniciando sistema");
            NVIC_SystemReset();
        }
        delay(5000);
    }
}

void connectMQTT() {
    Serial.println("\nConectando MQTT...");
    //client.stop(); // Limpiar estado previo
    
    int attempts = 0;
    client.begin("kitiot.antel.com.uy", net);
    client.setKeepAlive(30);  // Keep-alive más corto

    while (!client.connect("arduino-" + String(random(1000)), "kitiotum", "k1t10tum")) {
        Serial.print(".");
        if (++attempts >= 10) {
            Serial.println("\nError MQTT: Reiniciando conexiones");
            connectCellular(); // Resetear celular primero
            attempts = 0;
        }
        delay(2000);
    }
    
    Serial.println("\nMQTT conectado!");
    client.subscribe(topic2);
}

//===============================================================
// FUNCIONES DE SENSORES
float readUltrasonic() {
    digitalWrite(trigPin_1, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin_1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin_1, LOW);
    
    long duration = pulseIn(echoPin_1, HIGH);
    return duration * 0.034 / 2;
}

float readPressure(int pin) {
    int raw = analogRead(pin);
    float V_out = raw * (V_REF / 1023.0);
    return (V_out / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + (pin == pressureSensor1Pin ? 1.73 : 0);
}

float readTemperature() {
    int raw = analogRead(temperatureSensorPin);
    float voltage = raw * (V_REF / 1023.0);
    return (voltage / 0.01) - 273.15;
}

//===============================================================
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    pinMode(trigPin_1, OUTPUT);
    pinMode(echoPin_1, INPUT);
    
    connectCellular();
    connectMQTT();
    
    lastResetTime = lastSendTime = millis();
}

void loop() {
    // Mantener conexiones activas
    if (!nbAccess.status() == NB_READY) {
        Serial.println("Conexión celular perdida");
        connectCellular();
    }
    
    if (!client.connected()) {
        Serial.println("MQTT desconectado");
        connectMQTT();
    }
    
    client.loop(); // Procesar mensajes

    // Reinicio programado
    if (millis() - lastResetTime >= RESET_INTERVAL) {
        Serial.println("\nReinicio preventivo");
        NVIC_SystemReset();
    }

    // Envío no bloqueante
    if (millis() - lastSendTime >= SEND_INTERVAL) {
        String mensaje = String::format(
            "[%.2f,%.2f,%.2f,%.2f]",
            readUltrasonic(),
            readPressure(pressureSensor1Pin),
            readPressure(pressureSensor2Pin)),
            readTemperature()
        );
        
        if (client.publish(topic2, mensaje)) {
            Serial.println("Enviado: " + mensaje);
        } else {
            Serial.println("Error enviando");
        }
        
        lastSendTime = millis();
    }

    // Pequeño delay no bloqueante
    delay(100);
}
