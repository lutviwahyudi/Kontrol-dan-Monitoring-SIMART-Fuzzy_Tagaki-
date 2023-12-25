#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#define RELAY_PIN D3

LiquidCrystal_I2C lcd(0x27,16,2);
//input ssid dan password wifi
const char* ssid = "Google.Net";
const char* password = "muhammad";
//input server dan port mqtt broker
const char* host = "privatelutvi.cloud.shiftr.io";
const int port = 1883;

//koneksi ke perpustakaan WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

unsigned long lastMsgTime = 0;
const unsigned long publishInterval = 2000;
const char* sensorValueTopic = "sensorValue";
const char* moisturePercentageTopic = "moisturePercentage";
const char* statusTopic = "status";

const int soilMoisturePin = A0;

//derajat keanggotaan untuk fuzzy
float N_sangat_kering = 800;
float N_kering = 600; 
float N_netral = 425; 
float N_lembab = 300; 
float N_sangat_lembab = 100;

int sensorValue;
int moisturePercentage;
float hasil = 0;

void setup() {

    Serial.begin(9600);

    WiFi.begin(ssid, password);


    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("connecting to wifi...");
    }

    Serial.println("WiFi connected");
    Serial.print("ssid: ");
    Serial.println(ssid);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //lcd 
    lcd.init();                      
    lcd.backlight();
    lcd.clear();
    //mqtt
    client.setServer(host, port);
    client.setCallback(callback);
    //relay
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {

    mqttBroker();
    tagakiSugenoKang();
    tampilan1("N_Sensor: ", "Kelembapan: ", 1000);
    tampilan2("Durasi Fuzzy :");
    lcd.setCursor(0, 1);
    lcd.print(hasil);
    lcd.print(" ms");
    delay(1000);
    
    kendaliRelay();
}


void tampilan1(const char *teks1, const char *teks2, int delayTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(teks1);
  lcd.print(sensorValue); 
  lcd.setCursor(0, 1);
  lcd.print(teks2);
  lcd.print(moisturePercentage);
  lcd.print("%");
  delay(delayTime);
}

void tampilan2(const char *teks2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(teks2);
}

void kendaliRelay(){
    if (sensorValue > 800) {

      digitalWrite(RELAY_PIN, HIGH);
      delay(10000);

      digitalWrite(RELAY_PIN, LOW);
      delay(hasil);
  
    } else if (sensorValue < 800 && sensorValue > 600) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000);

      digitalWrite(RELAY_PIN, LOW);
      delay(hasil);

    } else if (sensorValue < 600 && sensorValue > 425){
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000);

      digitalWrite(RELAY_PIN, LOW);
      delay(hasil);
  
    } 
    else if (sensorValue < 425 && sensorValue > 300){
      digitalWrite(RELAY_PIN, HIGH);
    }
    else if(sensorValue < 300 && sensorValue > 200){
      digitalWrite(RELAY_PIN, HIGH);
    }
    else {
      digitalWrite(RELAY_PIN, HIGH);
    }
}

void mqttBroker(){
   if (!client.connected()) {
  reconnect();
  }

  unsigned long now = millis();
  if (now - lastMsgTime > publishInterval) {
    lastMsgTime = now;

    sensorValue = 430;
    moisturePercentage = 58;
    // sensorValue = analogRead(soilMoisturePin);
    // moisturePercentage = map(sensorValue, 1023, 0, 0, 100);

    char sensorValueStr[10];
    char moisturePercentageStr[10];

    itoa(sensorValue, sensorValueStr, 10);
    itoa(moisturePercentage, moisturePercentageStr, 10);

    const char* status;
    if (sensorValue > 800) {
          status = "sangat kering";
    } else if (sensorValue < 800 && sensorValue > 600) {
          status = "kering";
    } else if (sensorValue < 600 && sensorValue > 425){
          status = "OnGoing";
    } 
    else if (sensorValue < 425 && sensorValue > 300){
          status = "netral";
    }else if(sensorValue < 300 && sensorValue > 200){
      status = "basah";
    }else {
      status = "sangat basah";
    }

    Serial.print("status kelembapan : ");
    Serial.println(status);

    client.publish(sensorValueTopic, sensorValueStr);
    client.publish(moisturePercentageTopic, moisturePercentageStr);
    client.publish(statusTopic, status);   

    Serial.print("Mempublish NilaiSensor: ");
    Serial.println(sensorValue);
    Serial.print("Mempublish Kelembapan: ");
    Serial.println(moisturePercentage);
    Serial.print("Mempublish status: ");
    Serial.println(status);
  }

  client.loop();
}

void tagakiSugenoKang(){
  float sangat_lembab;
    if(N_lembab <= sensorValue){
        
      sangat_lembab = 0;

    }else if(N_sangat_lembab <= sensorValue && sensorValue <= N_lembab){

    float hasil_sangat_lembab = (float)(N_lembab - sensorValue) / (N_lembab - N_sangat_lembab);
      sangat_lembab = hasil_sangat_lembab;

    }else if(sensorValue <= N_sangat_lembab){
      sangat_lembab = 1;

    }

    float lembab;
    if(N_netral <= sensorValue || sensorValue <= N_sangat_lembab){
        
      lembab = 0;

    }else if(N_sangat_lembab <= sensorValue && sensorValue <= N_lembab){

    float hasil_lembab = (float)(sensorValue - N_sangat_lembab) / (N_lembab - N_sangat_lembab);
      lembab = hasil_lembab;

    }else if(sensorValue <= N_netral && sensorValue >= N_lembab){

    float hasil_lembab = (float)(N_netral - sensorValue) / (N_netral - N_lembab);
    lembab = hasil_lembab;

    }

    float netral;
    if(N_kering <= sensorValue || sensorValue <= N_lembab){
        
      netral = 0;

    }else if(N_lembab <= sensorValue && sensorValue <= N_netral){

    float hasil_netral = (float)(sensorValue - N_lembab) / (N_netral - N_lembab);
      netral = hasil_netral;

    }else if(sensorValue >= N_netral && sensorValue <= N_kering){
    float hasil_netral = (float)(N_kering - sensorValue) / (N_kering - N_netral);
      netral = hasil_netral;

    }

    float kering; 
    if(sensorValue <= N_netral || N_sangat_kering <= sensorValue){
        
      kering = 0;

    }else if(N_netral <= sensorValue && sensorValue <= N_kering){

    float hasil_kering = (float)(sensorValue - N_netral) / (N_kering - N_netral);
      kering = hasil_kering;

    }else if(N_kering < sensorValue && sensorValue < N_sangat_kering){
    float hasil_kering = (float)(N_sangat_kering - sensorValue) / (N_sangat_kering - N_kering);
      kering = hasil_kering;

    }

    float sangat_kering; 
    if(sensorValue <= N_kering){
        
      sangat_kering = 0;

    }else if(N_kering <= sensorValue && sensorValue <= N_sangat_kering){

    float hasil_sangat_kering = (float)(sensorValue - N_kering) / (N_sangat_kering - N_kering);
      sangat_kering = hasil_sangat_kering;

    }else if(N_sangat_kering <= sensorValue){
      sangat_kering = 1;

    }
 
    float w1 = 300000;
    float w2 = 210000;
    float w3 = 150000;
    float w4 = 60000;
    float w5 = 0;
    
    float z = ((sangat_lembab * w5) + (lembab * w4) + (netral * w3) + (kering * w2) + (sangat_kering * w1)) / (sangat_lembab + lembab + netral + kering + sangat_kering);
    int hasil = z;

    Serial.print("Jika Nilai Sensor ");
    Serial.print(sensorValue); // Mencetak nilai dari variabel sensor
    Serial.print(" dan kelembapannya ");
    Serial.print(moisturePercentage); // Mencetak nilai dari variabel kelembapan
    Serial.print(" maka waktu yang diperlukan adalah : ");
    Serial.println(hasil);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "privatelutvi", "3auTMdqoX8b3D4XE")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("suhu", "selamat anda berhasil");
      // ... and resubscribe
      client.subscribe("terima");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}