# Proyecto Arduino MKR1500: Lectura y Envío de Datos a un Broker MQTT

Este proyecto utiliza un Arduino MKR1500 para leer datos de un sensor de ultrasonido y enviarlos a un broker MQTT a través de una conexión celular. Es ideal para aplicaciones IoT que requieran medir distancias y transmitir los datos de forma inalámbrica.

## Descripción del Proyecto

El programa realiza las siguientes tareas:
1. Establece una conexión a la red celular usando la librería `MKRNB`.
2. Conecta el Arduino MKR1500 a un broker MQTT (Mosquitto en este caso).
3. Lee la distancia medida por un sensor de ultrasonido (HC-SR04).
4. Publica periódicamente el valor leído en un tópico MQTT definido.

## Requisitos de Hardware

- [Arduino MKR1500](https://store.arduino.cc/products/arduino-mkr-nb-1500)
- Sensor de ultrasonido (HC-SR04 o similar)
- Tarjeta SIM con datos móviles activados
- Fuente de alimentación adecuada para el MKR1500
- Cables y conexiones necesarias

## Librerías Utilizadas

Asegúrate de instalar las siguientes librerías en el IDE de Arduino antes de cargar el código:

- `MKRNB` para la conexión NB-IoT
- `MQTT` para la comunicación con el broker
- `ArduinoMqttClient` para manejar la suscripción y publicación MQTT

## Configuración del Código

### Parámetros de la red celular

Asegúrate de configurar los valores correspondientes a tu red celular:

const char pin[]      = " "; // PIN de la SIM (si aplica)
const char apn[]      = "internet.iot"; // APN de tu operador
const char login[]    = "kitiotum"; // Usuario del APN (si aplica)
const char password[] = "k1t10tum"; // Contraseña del APN (si aplica)

==========================================================================================================================================================================
==========================================================================================================================================================================

# Arduino MKR1500 Project: Reading and Sending Data to an MQTT Broker

This project uses an Arduino MKR1500 to read data from an ultrasonic sensor and send it to an MQTT broker via a cellular connection. It is ideal for IoT applications that require measuring distances and transmitting data wirelessly.

## Project Description

The program performs the following tasks:
1. Establishes a cellular network connection using the `MKRNB` library.
2. Connects the Arduino MKR1500 to an MQTT broker (Mosquitto in this case).
3. Reads the distance measured by an ultrasonic sensor (HC-SR04).
4. Periodically publishes the measured value to a defined MQTT topic.

## Hardware Requirements

- [Arduino MKR1500](https://store.arduino.cc/products/arduino-mkr-nb-1500)
- Ultrasonic sensor (HC-SR04 or similar)
- SIM card with active mobile data
- Suitable power supply for the MKR1500
- Necessary cables and connections

## Required Libraries

Ensure you install the following libraries in the Arduino IDE before uploading the code:

- `MKRNB` for the NB-IoT connection
- `MQTT` for broker communication
- `ArduinoMqttClient` to manage MQTT subscription and publishing

## Code Configuration

### Cellular Network Parameters

Make sure to configure the corresponding values for your cellular network:

```cpp
const char pin[]      = " "; // SIM PIN (if applicable)
const char apn[]      = "internet.iot"; // Operator's APN
const char login[]    = "kitiotum"; // APN username (if applicable)
const char password[] = "k1t10tum"; // APN password (if applicable)


