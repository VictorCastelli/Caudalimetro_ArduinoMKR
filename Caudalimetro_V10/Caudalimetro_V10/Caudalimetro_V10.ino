#include <MKRNB.h>
#include <MQTT.h>
#include <ArduinoMqttClient.h>

// ----- Configuración -----
const char pin[] = " ";
const char apn[] = "internet.iot";  // Prueba también con "antel.lte"
const char login[] = "kitiotum";
const char password[] = "k1t10tum";
const char broker[] = "kitiot.antel.com.uy";
const char topic2[] = "/Proy_ALUR_UTEC";

// ----- Pines de sensores -----
#define trigPin_1 2
#define echoPin_1 3
const int pressureSensor1Pin = A0;
const int pressureSensor2Pin = A2;
const int temperatureSensorPin = A1;
const float V_REF = 3.3;

// ----- Objetos -----
NB nbAccess;
NBClient net;
MQTTClient client;

// ----- Variables globales -----
#define RESET_INTERVAL 900000    // 15 minutos
#define SEND_INTERVAL 300000     // 5 minutos
unsigned long lastResetTime = 0;
unsigned long lastSendTime = 0;

// ===========================================================
// Función de conexión celular mejorada
bool connectCellular() {
    Serial.println("[CEL] Iniciando conexión celular...");
    
    // Reinicio completo del módem
    nbAccess.shutdown();
    Serial.println("[CEL] Módem apagado, esperando 10 segundos...");
    delay(10000);
    
    Serial.println("[CEL] Iniciando módem...");
    for(int i = 1; i <= 10; i++) {  // Más intentos
        Serial.print("[CEL] Intento ");
        Serial.print(i);
        Serial.print(" de 10... ");
        
        int status = nbAccess.begin(pin, apn, login, password);
        
        if(status == NB_READY) {
            Serial.println("¡Éxito!");
            Serial.println("[CEL] Conectado a la red celular");
            return true;
        } else {
            Serial.print("Fallo (Estado: ");
            Serial.print(status);
            Serial.println(")");
            
            // Mostrar descripción del estado
            switch(status) {
                case IDLE: Serial.println("[CEL] Estado: Inactivo"); break;
                case CONNECTING: Serial.println("[CEL] Estado: Conectando"); break;
                case NB_READY: Serial.println("[CEL] Estado: Listo"); break; // Aunque ya lo capturamos
                case GPRS_READY: Serial.println("[CEL] Estado: GPRS Listo"); break;
                case TRANSPARENT_CONNECTED: Serial.println("[CEL] Estado: Conectado Transparente"); break;
                case NB_ERROR: Serial.println("[CEL] Estado: Error"); break;
                default: Serial.println("[CEL] Estado: Desconocido");
            }
        }
        
        delay(10000);  // Espera más larga entre intentos
    }
    
    Serial.println("[CEL] Error crítico: No se pudo conectar después de 10 intentos");
    return false;
}

// ===========================================================
// Función de conexión MQTT mejorada
bool connectMQTT() {
    Serial.println("[MQTT] Conectando al broker...");
    
    client.begin(broker, net);
    
    for(int i = 1; i <= 5; i++) {
        Serial.print("[MQTT] Intento ");
        Serial.println(i);
        
        if(client.connect("arduino", "kitiotum", "k1t10tum")) {
            Serial.println("[MQTT] ¡Conectado al broker!");
            client.subscribe(topic2);
            return true;
        }
        
        Serial.println("[MQTT] Fallo en conexión, reintentando en 5 segundos...");
        delay(5000);
    }
    
    Serial.println("[MQTT] Error de conexión permanente");
    return false;
}

// ===========================================================
// Configuración inicial con más diagnóstico
void setup() {
    Serial.begin(115200);
    while(!Serial) {
        delay(100);  // Esperar a que el puerto serial esté listo
    }
    
    Serial.println("\n\n[SISTEMA] INICIANDO SETUP");
    Serial.println("[SISTEMA] Inicializando pines...");
    
    pinMode(trigPin_1, OUTPUT);
    pinMode(echoPin_1, INPUT);
    
    // Inicializar tiempos
    lastResetTime = millis();
    lastSendTime = millis();
    
    // Conexiones iniciales con más diagnóstico
    Serial.println("[SISTEMA] Iniciando conexión celular...");
    if (!connectCellular()) {
        Serial.println("[SISTEMA] Falla crítica en conexión celular - Reiniciando");
        delay(1000);
        NVIC_SystemReset();
    }
    
    Serial.println("[SISTEMA] Iniciando conexión MQTT...");
    if (!connectMQTT()) {
        Serial.println("[SISTEMA] Falla crítica en conexión MQTT - Reiniciando");
        delay(1000);
        NVIC_SystemReset();
    }
    
    Serial.println("[SISTEMA] Setup completado con éxito");
}

// ===========================================================
// Bucle principal simplificado
void loop() {
    unsigned long currentMillis = millis();
    
    // 1. Mantener conexión MQTT
    if(!client.connected()) {
        Serial.println("[MQTT] Desconectado - Reconectando...");
        if(!connectMQTT()) {
            Serial.println("[MQTT] Fallo reconexión - Reiniciando red celular");
            if(!connectCellular() || !connectMQTT()) {
                Serial.println("[SISTEMA] Falla crítica - Reiniciando");
                delay(1000);
                NVIC_SystemReset();
            }
        }
    }
    
    // Mantener actividad MQTT
    client.loop();

    // 2. Reinicio programado cada 15 minutos
    if(currentMillis - lastResetTime >= RESET_INTERVAL) {
        Serial.println("[SISTEMA] Reinicio programado por tiempo");
        delay(100);
        NVIC_SystemReset();
    }

    // 3. Enviar keep-alive cada 30 segundos
    static unsigned long lastActivity = 0;
    if(currentMillis - lastActivity >= 30000) {
        Serial.println("[MQTT] Enviando keep-alive");
        if(client.publish(topic2, "{}")) {
            lastActivity = currentMillis;
            Serial.println("[MQTT] Keep-alive enviado");
        } else {
            Serial.println("[MQTT] Error en keep-alive");
        }
    }

    // 4. Control de envío de datos cada 5 minutos
    if(currentMillis - lastSendTime >= SEND_INTERVAL) {
        Serial.println("[SENSOR] Iniciando lectura de sensores");
        
        // Lectura ultrasónica simplificada
        digitalWrite(trigPin_1, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin_1, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin_1, LOW);
        
        long duration = pulseIn(echoPin_1, HIGH, 50000);  // Timeout de 50ms
        int distance = (duration > 0) ? duration * 0.034 / 2 : -1;
        Serial.print("[SENSOR] Distancia: ");
        Serial.println(distance);

        // Lecturas analógicas
        int pressureValue1 = analogRead(pressureSensor1Pin);
        int pressureValue2 = analogRead(pressureSensor2Pin);
        int tempValue = analogRead(temperatureSensorPin);
        
        // Cálculos
        float V_out1 = pressureValue1 * (V_REF / 1023.0);  
        float V_out2 = pressureValue2 * (V_REF / 1023.0);  
        float P_1 = (V_out1 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09) + 1.73; 
        float P_2 = (V_out2 / (0.09 * V_REF)) - (0.04 * V_REF / 0.09); 
        float temperatureC = (tempValue * (V_REF / 1023.0) / 0.01) - 273.15;
        
        Serial.print("[SENSOR] Presión 1: ");
        Serial.println(P_1);
        Serial.print("[SENSOR] Presión 2: ");
        Serial.println(P_2);
        Serial.print("[SENSOR] Temperatura: ");
        Serial.println(temperatureC);

        // Construir y enviar mensaje
        char mensaje[80];
        snprintf(mensaje, sizeof(mensaje), "[%d,%.2f,%.2f,%.2f]", 
                distance, P_1, P_2, temperatureC);
        
        Serial.print("[MQTT] Enviando: ");
        Serial.println(mensaje);
        
        if(client.publish(topic2, mensaje)) {
            Serial.println("[MQTT] Envío exitoso");
            lastSendTime = currentMillis;
            lastActivity = currentMillis;
        } else {
            Serial.println("[MQTT] Error en envío!");
        }
    }

    // Pequeña pausa para evitar sobrecarga
    delay(1000);
    Serial.println("[SISTEMA] Sistema activo");
}