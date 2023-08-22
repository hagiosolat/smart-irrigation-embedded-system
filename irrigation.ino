#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "time.h" 

//Provide the token generation process info
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and the other helper function
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Faloyin4"
#define WIFI_PASSWORD "Faloyin4"

//Insert Firebase project API Key
#define API_KEY "AIzaSyA4oFOC1ZvCvrsT76xVOAwDuAj3DIBDsrU"


//Define the ID token for client or device to send the messsage
//#define DEVICE_REGISTRATION_ID_TOKEN "cqmeFHIlRvuh9XDG4_rCGk:APA91bGPCv71Sacs1T44aCW0-P0K2PB402A87bK4hqCOlGbqaitLPh6gtM400qGifyK8vR_uaHl-NRZYSQGzISj20fsBjLhcd6DM-t8S1he96e7rQHkUbVrpJcfKhLUsg05yuZjoi-7n"

#define FIREBASE_FCM_SERVER_KEY "server Key"

 

//Insert Authorized Email and Corresponding Password
#define USER_EMAIL "samuelt4christ@gmail.com"
#define USER_PASSWORD "princeolaty01#"


// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "url_to the serverKey"


#define DHTPIN 4

#define DHTTYPE DHT11

DHT dht(DHTPIN,DHTTYPE);


// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
String listDataPath;

// Database child nodes
String tempPath = "/temperature/kelvin";
String humPath = "/humidity";
String soilPath = "/soilMoisture";
String timePath = "/timestamp";
String buttonPath = "/buttonstate";

// Parent Node (to be updated in every loop)
String parentPath;
String secondParentPath;

int timestamp;
FirebaseJson json;
FirebaseJson tojson;


const char* ntpServer = "pool.ntp.org";
const int soil_sensor = 39;
int pumping_machine = 33;

// DHT11 SENSOR
float temperature;
float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;// delay atleast 3mins.
int THRESHOLD = 2000;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void sendMessage(String token);

void setup() {
    Serial.begin(9600);
    Serial.println(F("DHTxx test!"));
    pinMode(pumping_machine, OUTPUT);
    pinMode(soil_sensor, INPUT);
    
   
  initWiFi();
  configTime(0, 0, ntpServer);
  
  dht.begin();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  // databasePath = "/UsersData/" + uid + "/readings";
   databasePath = "/currentcondition";
   listDataPath = "/weather";


    // required for legacy HTTP API
    Firebase.FCM.setServerKey(FIREBASE_FCM_SERVER_KEY);

    //sendMessage();
 

}

void loop() {
    // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);


    int sensorValue = analogRead(soil_sensor);
    Serial.println(sensorValue);
    delay(100);

      
      // if(isnan(dht.readHumidity()) || isnan(dht.readTemperature())){
      //   Serial.println("Failed to read from DHT sensor!");
      //   return;
      // }

      // Serial.printf("%f\n%f\n", dht.readHumidity(), dht.readTemperature());

     //parentPath= databasePath + "/" + String(timestamp);
       

     if((int)analogRead(soil_sensor) > THRESHOLD) {
       if(Firebase.RTDB.getString(&fbdo,"token")){
          Serial.println(fbdo.to<String>());
         sendMessage(fbdo.to<String>());
       }
       parentPath = databasePath;
     tojson.set(buttonPath.c_str(), "ON");
     tojson.set(humPath.c_str(), 10);
     tojson.set(soilPath.c_str(), (int)analogRead(soil_sensor));
     tojson.set(tempPath.c_str(), 0.01);
     tojson.set(timePath.c_str(), int(timestamp));
     Serial.printf("Set json... %s\n",Firebase.RTDB.setJSON(&fbdo,parentPath.c_str(), &tojson) ? "ok" : fbdo.errorReason().c_str());
      
       } 
       else{
           if(Firebase.RTDB.getString(&fbdo,"token")){
             Serial.println("The Token is....");
             Serial.println(fbdo.to<String>());
       //sendMessage(fbdo.to<String>());
      }
         parentPath = databasePath;
     tojson.set(buttonPath.c_str(), "OFF");
     tojson.set(humPath.c_str(), 10);
     tojson.set(soilPath.c_str(), (int)analogRead(soil_sensor));
     tojson.set(tempPath.c_str(), 0.01);
     tojson.set(timePath.c_str(), int(timestamp));
     Serial.printf("Set json... %s\n",Firebase.RTDB.setJSON(&fbdo,parentPath.c_str(), &tojson) ? "ok" : fbdo.errorReason().c_str());
     }

     secondParentPath = listDataPath + "/" + String(timestamp);
    json.set(humPath.c_str(),  10);
    json.set(soilPath.c_str(), (int)analogRead(soil_sensor));
    json.set(tempPath.c_str(), 0.01);
    json.set(timePath.c_str(), int(timestamp));
    Serial.printf("Set json... %s\n",Firebase.RTDB.setJSON(&fbdo,secondParentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

    //Read From the buttonState Node to determine the action of the water_Pump.
    if(Firebase.RTDB.getString(&fbdo,"currentcondition/buttonstate")){
       Serial.println(fbdo.to<String>());
      if((fbdo.to<String>()) == "ON"){
        Serial.println(fbdo.to<String>());
        Serial.println("This is my name here");
        digitalWrite(pumping_machine, HIGH);
        delay(100);
      }
      else {
        digitalWrite(pumping_machine,LOW);
        delay(100);
      }  
    }
  }

}




void sendMessage(String token){

Serial.println("Send firebase Cloud Messaging...");

FCM_Legacy_HTTP_Message msg;

msg.targets.to = token; 
//DEVICE_REGISTRATION_ID_TOKEN;


msg.options.time_to_live = "1000";
msg.options.priority = "high";

msg.payloads.notification.title = "IRRIGATION SYSTEM";
msg.payloads.notification.body = "The System needs an attention now. Kindly click and check the current condition for user intervention";
msg.payloads.notification.icon = "myicon";
msg.payloads.notification.click_action = "FLUTTER_NOTIFICATION_CLICK";

FirebaseJson payload;

payload.add("title", "SOIL_ALERT");
payload.add("body","The soil_moisture level needs attention");
payload.add("image","This is the Image");
payload.add("click_action","FLUTTER_NOTIFICATION_CLICK");
msg.payloads.data = payload.raw();


  if(Firebase.FCM.send(&fbdo, &msg)) // send message to recipient
   Serial.printf("ok\n%s\n\n", Firebase.FCM.payload(&fbdo).c_str());

   else 
   Serial.println(fbdo.errorReason());

}


