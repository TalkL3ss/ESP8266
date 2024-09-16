#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define API_KEY ""
#define DATABASE_URL "https://dbname.firebaseio.com"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define USER_EMAIL ""
#define USER_PASSWORD ""
// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool debugPer = true;                //set flag to true if you want all the debug to the serial console
String myHostname = "FAN-CTL-MSTR";  //the name of the client name in the router
#define FAN_PWM_PIN D1               // PWM pin connected to the fan's PWM control wire
#define FAN_ON_Off_PIN D2            // control wire on the relay to turn on or off the FAN

int MaxVal = 255;  //FAN Support max Max Value of
String Speed = "0";
String NewSpeed;
String RestartNeed = "0";
String Power = "0";

boolean isValidNumber(String str);  //Just to check if the char that enterd its a a valid number
void WriteFanVal(String Val);       //change the pwm aignal )in my case its blue,last pin)
int intPres(int pres);              //Calucate from the presntage the value for the fan load

boolean isValidNumber(String str) {
  for (byte i = 0; i < str.length(); i++) {
    if (isDigit(str.charAt(i))) return true;
  }
  return false;
}

void WriteFanVal(String Val) {
  int iVal;
  if (isValidNumber(Val)) {
    iVal = Val.toInt();
    if (iVal >= 0 && iVal <= 100) {
      if (debugPer) {

        Serial.print("Setting Load: ");
        Serial.print(iVal);
        Serial.print("!");
        analogWrite(FAN_PWM_PIN, intPres(iVal));
      }
    } else {
      if (debugPer) {
        Serial.print("myVal: ");
        Serial.print(iVal);
        Serial.println(" too high");
      }
    }
  }
}

int intPres(int pres) {
  int PWN_Presentage;
  PWN_Presentage = (MaxVal * pres) / 100;
  return PWN_Presentage;
}


void setup() {
  pinMode(FAN_ON_Off_PIN, OUTPUT);
  Serial.begin(115200);
  WiFi.hostname(myHostname.c_str());
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readString();  // Read the input from Serial monitor
    input.trim();                        // Remove any leading/trailing whitespace

    if (input == "debug") {
      debugPer = true;  // Set the flag
      Serial.println("Flag is set! to debug on");
    } else if (input == "getip") {
      Serial.println(WiFi.localIP());
    } else if (input == "help") {
      Serial.println("Use debug or getip");
    } else if (input == "firebase") {
      Serial.println(Firebase.ready());
    } else if (input == "80") {
       WriteFanVal("80");
    }
  }
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    Power = Firebase.RTDB.getString(&fbdo, F("/PowerOn")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str();  //Need to add PowerOnTheFan
    if (Power == "1") {
      digitalWrite(FAN_ON_Off_PIN, HIGH);
    } else {
      digitalWrite(FAN_ON_Off_PIN, LOW);
    }

    NewSpeed = Firebase.RTDB.getString(&fbdo, F("/Speed")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
    if (Speed != NewSpeed) { Speed = NewSpeed; WriteFanVal(Speed); }
    RestartNeed = Firebase.RTDB.getString(&fbdo, F("/Restart")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
    if (RestartNeed == "1") {
    Serial.printf("Restart %s\n", Firebase.RTDB.setString(&fbdo, F("/Restart"), F("0")) ? "ok" : fbdo.errorReason().c_str());
    delay(2000);
        ESP.restart();
      }
    Serial.printf("Get Speed... %s\n", Speed);
    Serial.println();
  }
}
