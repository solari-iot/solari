// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"             // Include Emon Library
#include "painlessMesh.h"     // Biblioteca para la red de malla
#include <Arduino_JSON.h>     // Biblioteca para manejar datos en formato JSON

// Definiciones de red de malla
#define MESH_PREFIX     "whateverYouLike"  // Prefijo de la red de malla
#define MESH_PASSWORD   "somethingSneaky"  // Contraseña de la red de malla
#define MESH_PORT       5555               // Puerto de comunicación de la red de malla

EnergyMonitor emon1;             // Create an instance
int nodeNumber = 4;  

String readings;                             // Lecturas de sensores a enviar

unsigned int totalSentMessages = 0;          // Contador de mensajes enviados

Scheduler userScheduler;    // Programador de tareas personalizado
painlessMesh mesh;          // Instancia de la red de malla

// Prototipo de funciones
void sendMessage();   // Envía mensajes de lectura
String getReadings(); // Obtiene lecturas de sensores

// Tarea para enviar mensajes periódicamente
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

// Función para obtener las lecturas de los sensores y formatearlas como JSON
String getReadings() {
  JSONVar jsonReadings; // Objeto JSON para almacenar las lecturas
  // Asignación de valores al objeto JSON
  jsonReadings["node"] = nodeNumber;
  jsonReadings["Voltaje"] = emon1.Vrms;  // Lectura del voltaje del bus
  jsonReadings["Corriente"] = emon1.Irms;    // Lectura de la corriente
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
  taskSendMessage.setInterval(TASK_SECOND * 1); // Programar próxima ejecución
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
                        // Número de este nodo en la red de malla

void setup()
{  
  Serial.begin(115200); // Iniciar comunicación serial
  
  emon1.voltage(32, 254.35, 1);  // Voltage: input pin, calibration, phase_shift
  emon1.current(33, 0.69);       // Current: input pin, calibration.
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
}

void loop()
{
   mesh.update(); // Actualizar la red de malla
  emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
         // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)

  float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  float Irms            = emon1.Irms;             //extract Irms into Variable
  // Imprimir todas las variables (potencia real, potencia aparente, Vrms, Irms, factor de potencia)
  Serial.print("Supply Voltage: "); Serial.print(emon1.Vrms); Serial.println(" V");
  Serial.print("Irms: "); Serial.print(emon1.Irms); Serial.println(" A");
  //delay(1000);
  
}
