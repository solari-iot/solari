/*
 * Sistema de Adquisición de Variables de Voltaje y Corriente DC con PainlessMesh
 *
 * Este código adquiere las variables de voltaje y corriente DC utilizando el sensor Adafruit INA228.
 * Luego envía estas variables al gateway a través de la red PainlessMesh.
 * El nodo también mantiene un contador de los mensajes enviados.
 *
 * Requiere las bibliotecas painlessMesh, Arduino_JSON y Adafruit_INA228.
 *
 * Autor: Martin Vasquez, Juan Camilo Moreno, Sara Gonzalez
 */

#include "painlessMesh.h"     // Biblioteca para la red de malla
#include <Arduino_JSON.h>     // Biblioteca para manejar datos en formato JSON
#include <Adafruit_INA228.h>  // Biblioteca para el sensor INA228
#include <OneWire.h>
#include <DallasTemperature.h>

// Definiciones de red de malla
#define MESH_PREFIX     "whateverYouLike"  // Prefijo de la red de malla
#define MESH_PASSWORD   "somethingSneaky"  // Contraseña de la red de malla
#define MESH_PORT       5555               // Puerto de comunicación de la red de malla
// Pin donde está conectado el bus OneWire
#define ONE_WIRE_BUS 2 // Por ejemplo, para ESP32, el pin D5

Adafruit_INA228 ina228 = Adafruit_INA228();  // Instancia del sensor INA228
int nodeNumber = 2;                          // Número de este nodo en la red de malla
String readings;                             // Lecturas de sensores a enviar

unsigned int totalSentMessages = 0;          // Contador de mensajes enviados

Scheduler userScheduler;    // Programador de tareas personalizado
painlessMesh mesh;          // Instancia de la red de malla

// Inicializar el bus OneWire
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Prototipo de funciones
void sendMessage();   // Envía mensajes de lectura
String getReadings(); // Obtiene lecturas de sensores

// Tarea para enviar mensajes periódicamente
Task taskSendMessage(TASK_SECOND * 10, TASK_FOREVER, &sendMessage);

// Función para obtener las lecturas de los sensores y formatearlas como JSON
String getReadings() {
  JSONVar jsonReadings; // Objeto JSON para almacenar las lecturas
  // Asignación de valores al objeto JSON
  sensors.requestTemperatures(); // Solicitar las temperaturas al sensor
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = sensors.getTempCByIndex(0); // Obtener la temperatura en grados Celsius
  jsonReadings["Voltaje"] = ina228.readBusVoltage(); // Lectura del voltaje del bus
  jsonReadings["Corriente"] = ina228.readCurrent();   // Lectura de la corriente
  jsonReadings["Potencia"] = ina228.readPower();       // Lectura de la potencia
  jsonReadings["Energia"] = ina228.readEnergy();       // Lectura de la energía
  readings = JSON.stringify(jsonReadings); // Convertir objeto JSON a cadena JSON
  return readings; // Devolver las lecturas formateadas como JSON
}

// Función para enviar mensajes de lectura a través de la red de malla
void sendMessage() {
  String msg = getReadings(); // Obtener lecturas
  mesh.sendBroadcast(msg);    // Enviar mensaje a todos los nodos en la red de malla
  totalSentMessages++;        // Incrementar contador de mensajes enviados
  Serial.print("Total de mensajes enviados: ");
  Serial.println(totalSentMessages);
  taskSendMessage.setInterval(TASK_SECOND * 3); // Programar próxima ejecución
}

// Función de devolución de llamada para manejar mensajes recibidos
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

// Función de devolución de llamada para manejar nuevas conexiones
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

// Función de devolución de llamada para manejar cambios en las conexiones
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

// Función de devolución de llamada para manejar ajustes en el tiempo del nodo
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

// Función para imprimir el total de mensajes enviados
void printTotalSentMessages() {
  Serial.print("Total de mensajes enviados: ");
  Serial.println(totalSentMessages);
}

// Configuración inicial
void setup() {
  Serial.begin(115200); // Iniciar comunicación serial
  // Inicializar el sensor INA228
  if (!ina228.begin()) {
    Serial.println("Couldn't find INA228 chip");
    while (1);
  }
  Serial.println("Found INA228 chip");
  // Configurar parámetros del sensor INA228
  ina228.setShunt(0.015, 3.0);
  ina228.setAveragingCount(INA228_COUNT_16);
  ina228.setVoltageConversionTime(INA228_TIME_150_us);
  ina228.setCurrentConversionTime(INA228_TIME_280_us);

  // Configurar mensajes de depuración para la red de malla
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  // Inicializar la red de malla
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  // Asignar funciones de devolución de llamada
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  // Agregar tarea para enviar mensajes
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable(); // Habilitar la tarea de envío de mensajes
  sensors.begin(); // Iniciar la comunicación con el sensor
}

// Bucle principal
void loop() {
  mesh.update(); // Actualizar la red de malla
  //printTotalSentMessages(); // Imprimir el total de mensajes enviados (descomentar si es necesario)
}
