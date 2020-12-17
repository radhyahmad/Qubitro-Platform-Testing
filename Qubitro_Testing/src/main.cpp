#include <Arduino.h>
#include <WiFiNINA.h>
#include <SPI.h>
#include <WiFi.h>
#include <QubitroMqttClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>


void receivedMessage(int messageSize); 

struct dataSens
{
  float temperature = 0.0f;
  float humidity = 0.0f;
};

static char dataEnv[256];
static dataSens data;
StaticJsonDocument<256> doc;


#define PERIOD 10000

WiFiClient wifiClient;
QubitroMqttClient mqttClient(wifiClient);

// WiFi Credentials
char ssid[] = "";   
char pass[] = "";

char deviceID[] = "";
char deviceToken[] = "";
char host[] = "broker.qubitro.com";
int port = 1883;

unsigned long next = 0;

#define DHTPIN 6
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() 
{
  // Initialize serial port
  Serial.begin(9600);
  dht.begin();
  while (!Serial) {;} 
  
  // connect to Wifi network:
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\tConnected to the WiFi !");

  // You need to provide device id and device token
   mqttClient.setId(deviceID);
   mqttClient.setDeviceIdToken(deviceID, deviceToken);

  Serial.println("Connecting to Qubitro...");

  if (!mqttClient.connect(host, port)) 
  {
    Serial.println("Connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    Serial.println("Visit docs.qubitro.com or create a new issue on github.com/qubitro");
    while (1);
  }

  Serial.println("Connected to the Qubitro !");

  mqttClient.onMessage(receivedMessage);    
                                      
  mqttClient.subscribe(deviceID);

}

void loop() 
{
  mqttClient.poll();

  if(millis() > next) {

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h))
    {
      Serial.println("Failed");
      return;
    }

    data.temperature = t;
    data.humidity = h;

    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    serializeJsonPretty(doc, dataEnv);
    

    next = millis() + PERIOD;

    // Change if possible to have a message over 256 characters
    //static char payload[256];
    /*snprintf(payload, sizeof(payload)-1, "{\"temp\":%f}", t);
    snprintf(payload, sizeof(payload)-1, "{\"hum\":%f}", h);*/

   //snprintf(payload, sizeof(payload)-1, dataEnv);

    mqttClient.beginMessage(deviceID);

    // Send value
    mqttClient.print(dataEnv); 
    mqttClient.endMessage();  

  }
}

void receivedMessage(int messageSize) 
{
  Serial.print("New message received:");

  while (mqttClient.available()) 
  {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();
}
