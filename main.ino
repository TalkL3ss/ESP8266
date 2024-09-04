//NodeMCU 1.0 (ESP-12E Module)
//Didn't Test it yet But Convert Val to String all the way

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimedAction.h>
#define FAN_PWM_PIN D1  // PWM pin connected to the fan's PWM control wire

const char *ssid = "";//ssid name
const char *password = "";//ssid password
bool debugPer = true;//set flag to true if you want all the debug to the serial console

void blinkIT();//bypass/timebase
boolean isValidNumber(String str);//Just to check if the char that enterd its a a valid number
void WriteFanVal(String Val);//change the pwm aignal )in my case its blue,last pin)
String myHostname = "FAN-CTL-MSTR";//the name of the client name in the router
int MaxVal = 255;//my FAN Support max Max Value of 
int intPres(int pres);//Calucate from the presntage the value for the fan load

int intPres(int pres) {
  int PWN_Presentage;
  PWN_Presentage =  (MaxVal * pres) / 100;
  return PWN_Presentage;
}


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");//Set NTP Client to fix the time

TimedAction timedAction = TimedAction(60000, blinkIT);//Create a time action
WiFiServer server(80);//webserver port 

// Variable to store the HTTP request
String header;
boolean daylightSaving = true;  //false for winter, true for summer
String Val = "0";//Starting Load of the fan

// Auxiliar variables to store the current output state
int output12State = 0;
String opt;
// Assign output variables to GPIO pins
const int output12 = 12;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(output12, OUTPUT);
  //pinMode(14, INPUT);

  // Set outputs to LOW
  digitalWrite(output12, LOW);

  // Connect to Wi-Fi network with SSID and password
  if (debugPer) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.hostname(myHostname.c_str());
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debugPer) {
      Serial.print(".");
    }
  }
  if (debugPer) {

    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  server.begin();
  timeClient.begin();
  timeClient.setTimeOffset(7200);
}

void loop() {
    if (Serial.available() > 0) {
    String input = Serial.readString(); // Read the input from Serial monitor
    input.trim(); // Remove any leading/trailing whitespace

    if (input == "debug") {
      debugPer = true; // Set the flag
      Serial.println("Flag is set! to debug on");
    } else if (input == "getip") {
      Serial.println(WiFi.localIP());
    } else if (input == "help") {
      Serial.println("Use debug or getip");
    }
  }
  if (daylightSaving) {
    timeClient.setTimeOffset(7200);
  } else {
    timeClient.setTimeOffset(10800);
  }
  WiFiClient client = server.available();  // Listen for incoming clients
  timeClient.update();
  timedAction.check();
  String myTime = timeClient.getFormattedTime();

  if (client) {
    if (debugPer) {
      // If a new client connects,
      Serial.println("New Client.");  // print a message out in the serial port
    }
    String currentLine = "";  // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        if (debugPer) {
          Serial.write(c);  // print it out the serial monitor
        }
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /12/on") >= 0) {
              if (debugPer) {
                Serial.println("GPIO 12 on");
              }
              output12State = 1;
              digitalWrite(output12, HIGH);
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /12/off") >= 0) {
              if (debugPer) {

                Serial.println("GPIO 12 off");
              }
              output12State = 0;
              digitalWrite(output12, LOW);
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /Restart") >= 0) {
              if (debugPer) {
                Serial.println("Restart!");
              }
              client.println("ok");
              client.println();
              delay(3000);
              ESP.restart();
              break;
            } else if (header.indexOf("GET /DayLight/off") >= 0) {
              if (debugPer) {
                Serial.println("DayLight off");
              }
              daylightSaving = true;
              client.println("ok");
              client.println();
              break;
            } else if (header.indexOf("GET /DayLight/on") >= 0) {
              if (debugPer) {
                Serial.println("DayLight on");
              }
              daylightSaving = false;
              client.println("ok");
              client.println();
              break;
              //WriteFanVal
            } else if (header.indexOf("GET /Control") >= 0) {
              if (header.indexOf("Load=")) {
                int startParam = header.indexOf("Load=");
                String strStartParam = header.substring((startParam + 5), (startParam + 8));
                strStartParam.replace(" ", "");
                strStartParam.replace("H", "");
                int intStartParam = int(strStartParam.toInt());
                if (debugPer) {
                  Serial.println("Control on: " + strStartParam + "!");
                }
                Val = strStartParam;
                WriteFanVal(strStartParam);
                client.println("ok");
                client.println();
                break;
              }
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            //client.println("<meta http-equiv=\"refresh\" content=\"1800\" >");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;} .button3 {background-color: #6c0000;}</style></head>");

            // Web Page Heading
            client.println("<script>function ControlLinks(e,t){var n=new XMLHttpRequest;document.getElementById(t).innerHTML;n.onreadystatechange=function(){4==this.readyState&&this.status},n.open(\"GET\",e,!0),n.send();setTimeout(() => {console.log(\"Bam! 5 seconds have passed.\");location.reload();}, 2000);}function addtt(){for(var e=document.getElementById(\"myTime\").innerHTML,t=[0,0,0],n=t.length,a=(e||\"\").split(\":\"),i=\"00:00:01\".split(\":\"),s=0;s<n;s++)a[s]=isNaN(parseInt(a[s]))?0:parseInt(a[s]),i[s]=isNaN(parseInt(i[s]))?0:parseInt(i[s]);for(s=0;s<n;s++)t[s]=a[s]+i[s];var r=t[0],o=t[1],d=t[2];if(d>=60){var l=d/60<<0;o+=l,d-=60*l}if(o>=60){var c=o/60<<0;r+=c,o-=60*c}document.getElementById(\"myTime\").innerHTML=(\"0\"+r).slice(-2)+\":\"+(\"0\"+o).slice(-2)+\":\"+(\"0\"+d).slice(-2)}setInterval((()=>{addtt()}),1e3);</script>");
            client.println("<script>function RestartFNC(){!0==confirm(\"Are you sure that you want to restart?\")?ControlLinks(\"/Restart\",\"rstBTN\"):alert(\"You canceled!\")}function sendload(){ControlLinks(\"/Control?Load=\"+document.getElementById(\"Load\").value,\"Load\")}</script>");
            client.println("<body bgcolor=#006468><h1>Room Fan Control!!</h1>");
            client.println("<p><h2>Time: <div id=\"myTime\">" + myTime + "</div></h2></p>");
            if (output12State == 1) {
              opt = "On";
            } else {
              opt = "OFF";
            };
            client.println("<p>Fan Now " + opt + "!</p>");
            // If the output12State is off, it displays the ON button
            if (output12State == 0) {
              client.println("<button class=\"button\" onclick=\"ControlLinks('/12/on','BypassBTN');\"><div id=\"BypassBTN\">Power ON</div></button></p>");
            } else {
              client.println("<button class=\"button button2\" onclick=\"ControlLinks('/12/off','BypassBTN');\"><div id=\"BypassBTN\">Power OFF</div></button></p>");
            }
            client.println("<button class=\"button button3\" onclick=\"RestartFNC();\"><div id=\"rstBTN\">Restart Device</div></button></p>");
            if (daylightSaving) {
              client.println("<p>DayLight:</p><p><button class=\"button\" onclick=\"ControlLinks('/DayLight/on','DayBTN');\"><div id=\"DayBTN\">ON</div></button></a></p>");
            } else {
              client.println("<p>DayLight:</p><p><button class=\"button\" onclick=\"ControlLinks('/DayLight/off','DayBTN');\"><div id=\"DayBTN\">OFF</div></button></p>");
            }
            client.println("<p>Change Fan Speed</p>");
            client.println("<p>Example: Value Ranges 0-100</p>");
            client.println("<p>Quiter < 59");
            client.println(" Recommended ~ 64");
            client.println(" Nice ~ 84</p></div>");
            client.println("<p>Press The Buttons to Set the fan to...</p>");
            client.println("<p><button class=\"button\" onclick=\"ControlLinks(\'/Control?Load=59\',\'59BTN\');\"><div id=\"59BTN\">59</div></button>");
            client.println("<button class=\"button\" onclick=\"ControlLinks(\'/Control?Load=64\',\'64BTN\');\"><div id=\"64BTN\">64</div></button>");
            client.println("<button class=\"button\" onclick=\"ControlLinks(\'/Control?Load=80\',\'80BTN\');\"><div id=\"80BTN\">80</div></button></p>");
            client.println("<p><label for=\"Load\">Set the Value to:</label></p>");
            client.println("<input type=\"text\" size=3 maxlength=3 id=\"Load\" name=\"Load\" value=\"" + Val + "\"><br><br>");
            client.println("<button class=\"button\" onclick=\"sendload();\"><div id=\"BTNLoad\">Send</div></button></p>");
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    if (debugPer) {

      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}

void blinkIT() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  if (output12State == 1) {
    digitalWrite(output12, HIGH);
  } else {
      digitalWrite(output12, LOW);
  }
   
  if (debugPer) {

    Serial.print("PowerOn Is in: ");
    Serial.print(opt);
    Serial.println(" State");
  }
}

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

        Serial.print("myVal: ");
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
