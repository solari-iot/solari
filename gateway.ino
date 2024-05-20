/*
 * Gateway para transmisión de datos de nodos a ThingsBoard a través de red de malla y Wi-Fi
 *
 * Este código establece una conexión de red de malla con nodos que envían datos de sensores.
 * Los datos recibidos se transmiten a un servidor ThingsBoard a través de una conexión Wi-Fi.
 *
 * Requiere las bibliotecas painlessMesh, Arduino_JSON, ThingsBoard y Adafruit_SH1106G.
 *
 * Autor: Martin Vasquez, Sara Catalina, Juan Camilo
 */

#include <Arduino.h>
#include <painlessMesh.h>     // Biblioteca para la red de malla
#include <WiFiClient.h>       // Cliente Wi-Fi para la conexión a ThingsBoard
#include <Arduino_JSON.h>     // Biblioteca para manejar datos en formato JSON
#include <ThingsBoard.h>      // Biblioteca para la comunicación con ThingsBoard
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>  // Biblioteca para el controlador de pantalla OLED

#define i2c_Address 0x3c // Dirección I2C del display OLED

#define SCREEN_WIDTH 128  // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 64  // Alto de la pantalla OLED en píxeles
#define OLED_RESET -1     // Pin de reset del display OLED
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

#define TB_SERVER "thingsboard.cloud"  // Servidor ThingsBoard
#define TOKEN "ACCESS_TOKEN"    // Token de acceso a ThingsBoard Solari

// Configuración de la red de malla
#define MESH_PREFIX     "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

// Credenciales del punto de acceso (AP)
#define STATION_SSID     "NOMBRE DE LA RED"
#define STATION_PASSWORD "PASSWORD"


#define HOSTNAME "MQTT_Bridge"

bool conexion = false;       // Estado de la conexión Wi-Fi
bool wifiConectado = false;  // Estado de la conexión Wi-Fi
unsigned int totalReceivedMessages = 0; // Contador de mensajes recibidos

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

// Prototipos de funciones
void receivedCallback( const uint32_t &from, const String &msg );
IPAddress getlocalIP();
IPAddress myIP(0,0,0,0);
IPAddress previousIP = myIP;
IPAddress mqttBroker(172, 20, 10, 5);

painlessMesh  mesh;
WiFiClient wifiClient;
ThingsBoard tb(wifiClient, MAX_MESSAGE_SIZE);

void setup() {
  Serial.begin(115200);
  display.begin(i2c_Address, true); // Inicializar pantalla OLED
  display.clearDisplay();
  display.display();

// Configurar mensajes de depuración para la red de malla
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION ); 
 // Inicializar la red de malla
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6);
  mesh.onReceive(&receivedCallback); // Establecer función de devolución de llamada para mensajes recibidos

  // Configurar conexión Wi-Fi manualmente
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  mesh.setRoot(true);  // Este nodo es el nodo raíz
  mesh.setContainsRoot(true);  // Informar a todos los nodos de que la red contiene un nodo raíz

  // Conectar al servidor ThingsBoard y al punto de acceso Wi-Fi
  connectToThingsBoard();
  conectarWiFi();
}

void loop() {
 mesh.update(); // Actualizar la red de malla

  // Obtener la lista de nodos conectados a la red de malla
  std::list<uint32_t> nodeList = mesh.getNodeList();
  int numberOfNodes = nodeList.size();

  // Verificar si la dirección IP local ha cambiado
  if (myIP != getlocalIP()) {
    myIP = getlocalIP();
    Serial.println("Mi IP es " + myIP.toString());
    conexion = true;
    cartel_Conectado(numberOfNodes); // Mostrar estado de la conexión en la pantalla OLED
  }
  // Verificar el estado de la conexión y mostrar mensajes en la pantalla OLED
  if (conexion == true) {
    if (myIP == IPAddress(0, 0, 0, 0)) {
      display.clearDisplay();
      display.setCursor(5, 5);
      display.println("Conexion perdida ");
      display.println("Reintentando conectar...");
      display.display();
      if (!wifiConectado) {
        Serial.println("Conexion perdida, reintentando");
        conectarWiFi();
      }
    } else {
      cartel_Conectado(numberOfNodes);
    }
  } else {
    cartel_Conectando();
  }
  
  // Verificar si la conexión a ThingsBoard está activa y volver a conectar si es necesario
  if (!tb.connected()) {
    connectToThingsBoard();
  }
  tb.loop(); // Mantener la comunicación con ThingsBoard
}

// Función de devolución de llamada para mensajes recibidos de nodos de la red de malla
void receivedCallback(const uint32_t &from, const String &msg) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  totalReceivedMessages++;
  Serial.print("Total de mensajes recibidos: ");
  Serial.println(totalReceivedMessages);

  // Analizar el mensaje JSON recibido
  JSONVar myObject = JSON.parse(msg.c_str());
  int node = myObject["node"];

  // Procesar los datos recibidos según el tipo de nodo
  if (node == 2) {
    // Nodo del panel solar
    int temp_panel = myObject["temp"];
    double Voltaje_panel = myObject["Voltaje"];
    double Corriente_panel = myObject["Corriente"];
    double Potencia_panel = myObject["Potencia"];
    
    //long Energia_panel = myObject["Energia"];
    // Formar el JSON de datos para ThingsBoard
    String jsonData = "{\"temperature_panel\":" + String(temp_panel) + ", \"Voltaje_panel\":" + String(Voltaje_panel) + "}";

     // Enviar los datos a ThingsBoard
    if(tb.sendTelemetryJson(jsonData.c_str())){
        Serial.println("Datos enviados a ThingsBoard");
    }else{
      Serial.println("ERROR");
    }

    String jsonData2 = "{\"Corriente_panel\":" + String(Corriente_panel) + ", \"Potencia_panel\":" + String(Potencia_panel) + "}";
    // Enviar los datos a ThingsBoard
    if(tb.sendTelemetryJson(jsonData2.c_str())){
        Serial.println("Datos enviados a ThingsBoard");
    }else{
      Serial.println("ERROR");
    }
    
  }else if(node==3){
    // Nodo de la bateria
    double temp = myObject["temp"];
    double Voltaje = myObject["Voltaje"];
    double Corriente = myObject["Corriente"];
    double Potencia_bateria = myObject["Potencia"];
     // Formar el JSON de datos para ThingsBoard
    String jsonData = "{\"temperature_bateria\":" + String(temp) + ", \"Voltaje_bateria\":" + String(Voltaje) + "}";              
    // Enviar los datos a ThingsBoard
    tb.sendTelemetryJson(jsonData.c_str());
    Serial.println("Datos enviados a ThingsBoard");
    
     String jsonData2 = "{\"Corriente_bateria\":" + String(Corriente) +", \"Potencia_bateria\":" + String(Potencia_bateria) + "}";
    // Enviar los datos a ThingsBoard
    if(tb.sendTelemetryJson(jsonData2.c_str())){
        Serial.println("Datos enviados a ThingsBoard");
    }else{
      Serial.println("ERROR");
    }
    

  }else if(node==4){
    // Nodo AC
    double Voltaje = myObject["Voltaje"];
    double Corriente = myObject["Corriente"];
    // Obtener el RSSI de la conexión Wi-Fi
    int rssi = WiFi.RSSI();

     // Formar el JSON de datos para ThingsBoard
    String jsonData = "{\"Voltaje_AC\":" + String(Voltaje) +
                      ", \"Corriente_AC\":" + String(Corriente) + ", \"rssi\":" + String(rssi) + "}";
    // Enviar los datos a ThingsBoard
    tb.sendTelemetryJson(jsonData.c_str());
    Serial.println("Datos enviados a ThingsBoard");
  } 
}



// Función para obtener la dirección IP local
IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}

// Función para conectar al servidor ThingsBoard
void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Conectando al servidor ThingsBoard");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Error al conectar al servidor ThingsBoard");
    } else {
      Serial.println("Conectado al servidor ThingsBoard");
    }
  }
}

// Función para mostrar el cartel de conexión en la pantalla OLED mientras se conecta
void cartel_Conectando() {
  display.clearDisplay();
  display.drawRect(0, 0, display.width(), display.height(), SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  display.setTextSize(2);
  display.print("Conectando");
  display.display();
}

// Función para mostrar el cartel de conexión exitosa en la pantalla OLED
void cartel_Conectado(int numberOfNodes) {
    display.clearDisplay();
    display.drawRect(0, 0, display.width(), display.height(), SH110X_WHITE);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(5,5);
    display.setTextSize(1);
    display.print("Conectado");
    display.setCursor(5,14);
    display.print("Mi IP es " + myIP.toString());
    display.setCursor(5,22);
    display.print("Nodos conectados: ");
    display.println(numberOfNodes);
    display.display();
}

// Función para conectar a la red Wi-Fi
void conectarWiFi() {
  // Intentar conectarse al Wi-Fi
  Serial.println("Conectando a la red Wi-Fi...");
  WiFi.begin(STATION_SSID, STATION_PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado al Wi-Fi");
    wifiConectado = true;
  } else {
    Serial.println("\nError al conectar al Wi-Fi");
    wifiConectado = false;
  }
}

// Función para imprimir el total de mensajes recibidos por el gateway
void printTotalReceivedMessages() {
  Serial.print("Total de mensajes recibidos: ");
  Serial.println(totalReceivedMessages);
}

