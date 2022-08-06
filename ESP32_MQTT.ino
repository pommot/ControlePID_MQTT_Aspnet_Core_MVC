#include <OneWire.h> //controle de dados do sensor
#include <DallasTemperature.h> //biblioteca do sensor
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Redmi 9";
const char* password = "maia0481";
const char* mqtt_server = "192.168.21.40";
const int TIP = 15;
const int freq = 25000;
const int ledChannel = 0;
const int resolution = 8;
double kp = 0, ki = 0 , kd = 0;
float setpoint = 35;

WiFiClient espClient;
PubSubClient client(espClient);

// define pino GPIO23 que será conectado ao DS18B20
const int oneWireBus = 23;
// Objeto que tratará da troca de dados com o sensor DS18B20
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void callback(char* topic, byte* message, unsigned int length) {
  // Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  // Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.print(messageTemp);
  if (String(topic) == "controle/kpp") {
    //  Serial.print("Changing output to ");
    if (messageTemp == "kp_dec") {
      kp = kp - 10;
    }
    else if (messageTemp == "kp_inc") {
      kp = kp + 10;
    }
  }
  if (String(topic) == "controle/kii") {
    //  Serial.print("Changing output to ");
    if (messageTemp == "ki_dec") {
      ki = ki - 0.1;
    }
    else if (messageTemp == "ki_inc") {
      ki = ki + 0.1;
    }
  }
  if (String(topic) == "controle/kdd") {
    //  Serial.print("Changing output to ");
    if (messageTemp == "kd_dec") {
      kd = kd - 1;
    }
    else if (messageTemp == "kd_inc") {
      kd = kd + 1;
    }
  }
  if (String(topic) == "controle/spp") {
    //  Serial.print("Changing output to ");
    if (messageTemp == "sp_dec") {
      setpoint = setpoint - 1;
    }
    else if (messageTemp == "sp_inc") {
      setpoint = setpoint + 1;
    }
  }
}
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPClient")) {
      Serial.println("connected");
      /// Subscribe
      client.subscribe("controle/kpp");
      client.subscribe("controle/kii");
      client.subscribe("controle/kdd");
      client.subscribe("controle/spp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup() {
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(TIP, ledChannel);
  Serial.begin(9600);
  // inicia o sensor DS18B20
  sensors.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(100);
}
void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  sensors.requestTemperatures();
  double ultimatemp;
  long lastProcess = 0;
  double controle = 0, PID = 0;
  double modCTRL;
  float temperatureC = sensors.getTempCByIndex(0);
  ultimatemp = temperatureC;
  double erro = 0;
  double P = 0, I = 0, D = 0;
  float deltaTime = millis();
  erro = setpoint - temperatureC;
  P = erro * kp;
  I = (erro * ki) * (deltaTime / 1000.0);
  D = (ultimatemp - temperatureC) * kd * (deltaTime / 1000.0);
  ultimatemp = temperatureC;
  PID = -(P + I + D);
  if (PID < 0)PID = 0;
  if (PID > 255)PID = 255;
  ledcWrite(ledChannel, PID);  

  char kpString[8];
  dtostrf(kp, 1, 2, kpString);
  client.publish("controle/kp", kpString);

  char tempString[8];
  dtostrf(ultimatemp, 1, 2, tempString); 
  client.publish("controle/temperatura", tempString);

  char kiString[8];
  dtostrf(ki, 1, 2, kiString); 
  client.publish("controle/ki", kiString);

  char kdString[8];
  dtostrf(kd, 1, 2, kdString);  
  client.publish("controle/kd", kdString);

  char spString[8];
  dtostrf(setpoint, 1, 2, spString);  
  client.publish("controle/sp", spString);

  char erroString[8];
  dtostrf(erro, 1, 2, erroString);
  client.publish("controle/erro", erroString);

   char pidString[8];
  dtostrf(PID, 1, 2, pidString);
  client.publish("controle/pwm", pidString);
  delay(1000);
}
