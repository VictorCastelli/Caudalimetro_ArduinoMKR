#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

NB modem;
NBClient net;
NB nbAccess;
MQTTClient client;

extern "C" char *sbrk(int i);
int freeMemory() {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}


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

const char pin[] = " ";
const char apn[] = "internet.iot";
const char login[] = "kitiotum";
const char password[] = "k1t10tum";
const char topic2[] = "/Proy_ALUR_UTEC";

unsigned long lastResetTime = 0;
#define RESET_INTERVAL 900000  // 15 minutos (15 * 60 * 1000)
unsigned long lastPingTime = 0;
#define PING_INTERVAL 30000  // 30 segundos

//===============================================================
// FUNCIONES DE CONEXIÓN
void connectCellular() {
    int attempts = 0;
    Serial.print("Conectando a la red celular...");
    
    while (nbAccess.status() != NB_READY) {
        Serial.println("Intentando conectar a la red...");
        
        if (nbAccess.begin(pin, apn, login, password) == NB_READY) {
            Serial.println("Conexión exitosa a la red celular.");
            return;
        } else {
            Serial.println("Error al conectar a la red celular.");
        }

        attempts++;
        if (attempts >= 10) {
            Serial.println("\nReiniciando módulo NB...");
            nbAccess.shutdown();  // Apagar la red y volver a intentar
            delay(5000);
            attempts = 0;
        }

        delay(5000);
    }
}

void connectMQTT() {
    int attempts = 0;
    Serial.print("Conectando al broker MQTT...");

    client.begin("kitiot.antel.com.uy", net);
    client.setKeepAlive(60);  // Aumentamos keep-alive a 60 segundos

    while (!client.connect("arduino", "kitiotum", "k1t10tum")) {
        Serial.print(".");
        attempts++;
        delay(1000);
        
        if (attempts >= 10) {
            Serial.println("\nMQTT sigue fallando. Reiniciando conexión celular...");
            nbAccess.shutdown();
            delay(5000);
            connectCellular();
            attempts = 0;
        }
    }

    Serial.println("\n¡Conectado a MQTT!");
    client.subscribe(topic2);
}

void setup() {
    Serial.begin(115200);
    pinMode(trigPin_1, OUTPUT);
    pinMode(echoPin_1, INPUT);
    
    connectCellular();
    connectMQTT();
    
    lastResetTime = millis();
    lastPingTime = millis();
}

void loop() {
    Serial.println("Inicio");

    // Verificar conexión MQTT
    if (!client.connected()) {
        Serial.println("⚠ MQTT desconectado. Intentando reconectar...");
        connectMQTT();
    }

    client.loop();

    // Enviar un mensaje vacío cada 30 segundos para evitar desconexión
    if (millis() - lastPingTime >= PING_INTERVAL) {
        Serial.println("Manteniendo conexión MQTT...");
        client.publish(topic2, "");  // Enviar un mensaje vacío
        lastPingTime = millis();
    }

    // Reinicio programado cada 15 minutos
    if (millis() - lastResetTime >= RESET_INTERVAL) {
        Serial.println("Reinicio programado...");
        delay(1000);
        NVIC_SystemReset();
    }

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

    // Envío de datos
    String mensaje = "[" + String(distance_1) + "," + String(P_1, 2) + "," + String(P_2, 2) + "," + String(temperatureC, 2) + "]";
    Serial.print("Memoria libre: ");
    Serial.println(freeMemory());
    client.publish(topic2, mensaje);
    Serial.println("Enviado: " + mensaje);

    delay(60000);
    Serial.println("Esperando 1 min...");
    Serial.print("Memoria libre: ");
    Serial.println(freeMemory());

}
