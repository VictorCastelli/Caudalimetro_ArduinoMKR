#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

// ----- Configuración de la red celular y MQTT -----
const char pin[]      = " ";                   // PIN de la SIM
const char apn[]      = "internet.iot";         // APN del operador
//const char apn[]      = "antel.lte";
const char login[]    = "kitiotum";             // Usuario de red
const char password[] = "k1t10tum";             // Contraseña de red
const char broker[]   = "kitiot.antel.com.uy";  // Servidor MQTT
const char topic2[]   = "/Proy_ALUR_UTEC";      // Topic MQTT

// ----- Pines de sensores -----
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;
const int pressureSensor2Pin = A2;
const int temperatureSensorPin = A1;
const float V_REF = 3.3;  

// ----- Objetos NB-IoT y MQTT -----
NB nbAccess;
NBClient net;
MQTTClient client;

// ----- Variables globales -----
long duration_1;
int distance_1;
unsigned long lastResetTime = 0;
#define RESET_INTERVAL 900000  // Reiniciar cada 15 minutos (15 * 60 * 1000)
unsigned long lastLoopTime = 0;

// ===========================================================
// Funcion para medir memoria libre
// extern "C" char *sbrk(int i);
// int freeMemory() {
//     char stack_dummy = 0;
//     return &stack_dummy - sbrk(0);
// }

// ===========================================================
// Funcion de reconexión de red celular
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
            Serial.println("\nReiniciando el modem NB-IoT...");
            nbAccess.shutdown();
            delay(5000);
            retryCount = 0;
        }
        delay(3000);

        if (millis() - lastLoopTime > 300000) {  // Si pasan más de 5 minutos sin respuesta
            Serial.println("Tiempo de espera excedido, reiniciando...");
            NVIC_SystemReset();
        }
    }
    return false;
}

// ===========================================================
// Funcion de reconexión al broker MQTT
bool connectMQTT() {
    int retryCount = 0;
    Serial.println("Conectando al broker MQTT...");

    client.begin(broker, net);
    unsigned long startAttemptTime = millis();
    
    while (!client.connect("arduino", "kitiotum", "k1t10tum")) {
        Serial.print(".");
        retryCount++;
        delay(1000);

        // Timeout de 10 segundos
        if (millis() - startAttemptTime > 10000) {
            Serial.println("\nTimeout al intentar conectar a MQTT.");
            return false;
        }

        // Reiniciar la red celular si no logra conectar al broker
        if (retryCount >= 5) {
            Serial.println("\nFalló la conexión a MQTT. Reiniciando red celular...");
            nbAccess.shutdown();
            delay(5000);
            connectCellular();
            retryCount = 0;
        }
    }
    Serial.println("\nConectado al broker MQTT.");
    client.subscribe(topic2);
    return true;
}

// ===========================================================
// Función de reconexión completa
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
    Serial.begin(300);
    pinMode(trigPin_1, OUTPUT);
    pinMode(echoPin_1, INPUT);
    connect();
    lastResetTime = millis();
    lastLoopTime = millis();
}

// ===========================================================
// Bucle principal
void loop() {
    Serial.println("\nInicio del loop");
    delay(1000);
    lastLoopTime = millis(); // Registro del tiempo actual

    delay(60000);

    // Monitoreo de memoria RAM
    // Serial.print("Memoria libre: ");
    // Serial.println(freeMemory());

    // Verificar conexión MQTT
    if (!client.connected()) {
        Serial.println("MQTT desconectado. Intentando reconectar...");
        if (!connectMQTT()) {
            Serial.println("Fallo en la reconexión MQTT.");
            return;
        }
    }

    client.loop();

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

    // Envío de datos al broker MQTT
    char mensaje[80];
    snprintf(mensaje, sizeof(mensaje), "[%d,%.2f,%.2f,%.2f]", distance_1, P_1, P_2, temperatureC);
    client.publish(topic2, mensaje);
    Serial.print("Enviado: ");
    Serial.println(mensaje);
    delay(2000);
    // Espera de 1 minuto
    unsigned long start = millis();
    while (millis() - start < 60000) {  
        client.loop();
        delay(1000);

        // Si no responde en 5 minutos, reiniciamos
        if (millis() - lastLoopTime > 300000) {
            Serial.println("Tiempo de espera excedido, reiniciando...");
            NVIC_SystemReset();
        }
    }
}
