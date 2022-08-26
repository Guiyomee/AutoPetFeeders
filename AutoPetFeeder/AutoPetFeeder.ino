/*
  ==================================================================
  ========            AutoPetFeeders V1.0                   ========
  ======== I started in the development of Arduino project. ========
  ========   I have a plan to add a function with weight.   ========
  ==================================================================

  POOL NTP : https://www.ntppool.org/fr/

*/

#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Stepper.h>


//////////// NTP Setting ////////////
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 30000);

//////////// LED Setting ////////////
#define LED_GREEN D7
#define LED_RED D8

////////// BUTTON Setting ///////////
#define BUTTON D5
int buttonStatePrevious = LOW;                      // previousstate of the switch
unsigned long minButtonLongPressDuration = 5000;    // Time we wait before we see the press as a long press
unsigned long buttonLongPressMillis;                // Time in ms when we the button was pressed
bool buttonStateLongPress = false;                  // True if it is a long press
const int intervalButton = 50;                      // Time between two readings of the button state
unsigned long previousButtonMillis;                 // Timestamp of the latest reading
unsigned long buttonPressDuration;                  // Time the button is pressed in ms
unsigned long currentMillis;          // Variabele to store the number of milleseconds since the Arduino has started

////////// STEPPER Setting //////////
bool stepper;
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

//////////// CONFIG TEXT ////////////
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
const char* PARAM_INPUT_3 = "lang";
const char* langPath = "/config/lang.txt";
const char* ssidPath = "/config/ssid.txt";
const char* passwordPath = "/config/password.txt";
//_____________________________________//
const char* PARAM_INPUT_11 = "morning";
const char* PARAM_INPUT_12 = "evening";
const char* PARAM_INPUT_13 = "portion";
const char* morningPath = "/config/morning.txt";
const char* eveningPath = "/config/evening.txt";
const char* portionPath = "/config/portion.txt";

String ssid;
String password;
String language;
String morning;
String evening;
String portion;

bool restart;

//////////// PORT Server ////////////
AsyncWebServer server(80);

#define DEMI_RESET_STEPPER 40000
long last_reset = 0;
long last_push = 0;

////////// CONFIG STEPPER ///////////
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);


////////// START STEPPER ///////////
void StarStepper() {
  ESP.wdtDisable();
  int portions = portion.toInt();
  digitalWrite(LED_GREEN, HIGH);
  myStepper.step(stepsPerRevolution / 4  * portions);
  digitalWrite(LED_GREEN, LOW);
  Serial.print("Start Stepper / ");
  Serial.print("Portions: ");
  Serial.println(portions);
  stepper = true;
  ESP.wdtEnable(0);
}

//////////// Write file /////////////
void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = LittleFS.open(path, "w");
  if (path) {
    Serial.printf("File edit: %s\r\n", path);
    file.print(message);
    file.close();
  }

}

//////////// Read file //////////////
String readFile(fs::FS &fs, const char * path) {
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("Failed to open file for reading :");
    Serial.println(file);
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}

void setup() {

  Serial.begin(115200);
  pinMode(IN1, OUTPUT);//define pin for ULN2003 in1
  pinMode(IN2, OUTPUT);//define pin for ULN2003 in2
  pinMode(IN3, OUTPUT);//define pin for ULN2003 in3
  pinMode(IN4, OUTPUT);//define pin for ULN2003 in4
  pinMode(LED_GREEN, OUTPUT); //define pin for GREEN LED
  pinMode(LED_RED, OUTPUT); //define pin for RED LED
  pinMode(BUTTON, INPUT); //define pin for BUTTON
  digitalWrite(LED_RED, LOW);
  myStepper.setSpeed(10); //define speed stepper (dont touch)
  stepper = false; // stepper status = FALSE
  restart = false;

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  ssid = readFile(LittleFS, ssidPath);
  password = readFile(LittleFS, passwordPath);
  morning = readFile(LittleFS, morningPath);
  evening = readFile(LittleFS, eveningPath);
  portion = readFile(LittleFS, portionPath);

  Serial.println("Connecting to WiFi.");
  WiFi.begin(ssid, password);
  digitalWrite(LED_RED, HIGH);
  delay(15000);
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    Serial.println("Configuring access point...");
    WiFi.softAP("AutoPerFeeders");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(LittleFS, "/wifi_setting.html", String(), false);
    });
    server.serveStatic("/", LittleFS, "/");
    server.begin();
    server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            if (ssid != NULL) {
              Serial.println(ssid);
              writeFile(LittleFS, ssidPath, ssid.c_str());
            }
          }
          if (p->name() == PARAM_INPUT_2) {
            password = p->value().c_str();
            if (password != NULL) {
              Serial.println(password);
              writeFile(LittleFS, passwordPath, password.c_str());
            }
          }
          if (p->name() == PARAM_INPUT_3) {
            language = p->value().c_str();
            if (language != NULL) {
              Serial.println(language);
              writeFile(LittleFS, langPath, language.c_str());
            }else{
              Serial.println("You dont chose language / FR is default");
              writeFile(LittleFS, langPath, "fr");
            }
          }
        };
      };
      restart = true;
    });

  } else {
    digitalWrite(LED_RED, LOW);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_GREEN, HIGH);
    delay(1000);
    digitalWrite(LED_GREEN, LOW);
    delay(1000);
    digitalWrite(LED_GREEN, HIGH);
    delay(1000);
    digitalWrite(LED_GREEN, LOW);
    timeClient.begin();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(LittleFS, "/index.html", String(), false);
    });
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
      StarStepper();
    });
    server.serveStatic("/", LittleFS, "/");
    server.begin();
    server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == PARAM_INPUT_11) {
            morning = p->value().c_str();
            if (morning != NULL) {
              Serial.println(morning);
              writeFile(LittleFS, morningPath, morning.c_str());
            }
          }
          if (p->name() == PARAM_INPUT_12) {
            evening = p->value().c_str();
            if (evening != NULL) {
              Serial.println(evening);
              writeFile(LittleFS, eveningPath, evening.c_str());
            }
          }
          if (p->name() == PARAM_INPUT_13) {
            portion = p->value().c_str();
            if (portion != NULL) {
              Serial.println(portion);
              writeFile(LittleFS, portionPath, portion.c_str());
            }
          }
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            if (ssid != NULL) {
              Serial.println(ssid);
              writeFile(LittleFS, ssidPath, ssid.c_str());
              restart = true;
            }
          }
          if (p->name() == PARAM_INPUT_2) {
            password = p->value().c_str();
            if (password != NULL) {
              Serial.println(password);
              writeFile(LittleFS, passwordPath, password.c_str());
              restart = true;
            }
          }
          if (p->name() == PARAM_INPUT_3) {
            language = p->value().c_str();
            if (language != NULL) {
              Serial.println(language);
              writeFile(LittleFS, langPath, language.c_str());
            }else{
              Serial.println("You dont chose language / FR is default");
              writeFile(LittleFS, langPath, "fr");
            }
          }
        };
      };
      request->send(LittleFS, "/index.html", String(), false);
    });
  }
}

void readButtonState() {
  currentMillis = millis();
  if (currentMillis - previousButtonMillis > intervalButton) {
    int buttonState = digitalRead(BUTTON);
    if (buttonState == HIGH && buttonStatePrevious == LOW && !buttonStateLongPress) {
      buttonLongPressMillis = currentMillis;
      buttonStatePrevious = HIGH;
      Serial.println("Button pressed");
    }

    buttonPressDuration = currentMillis - buttonLongPressMillis;
    if (buttonState == HIGH && !buttonStateLongPress && buttonPressDuration >= minButtonLongPressDuration) {
      buttonStateLongPress = true;
      Serial.println("Button long pressed");
      LittleFS.remove(passwordPath);
      Serial.println("File Password delete");
      LittleFS.remove(ssidPath);
      Serial.println("File SSID  delete");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, HIGH);
      Serial.println("RESET");
      restart = true;
    }
    if (buttonState == LOW && buttonStatePrevious == HIGH) {
      buttonStatePrevious = LOW;
      buttonStateLongPress = false;
      Serial.println("Button released");
      if (buttonPressDuration < minButtonLongPressDuration) {
        Serial.println("Button pressed shortly");
        Serial.println("START MANUALY");
        StarStepper();
      }
    }

    previousButtonMillis = currentMillis;
  }
}

void loop() {

  readButtonState();

  if (restart){
    delay(3000);
    ESP.restart();
  }

  String evening_hour = evening.substring(0, 2);
  String evening_min = evening.substring(3, 5);
  String morning_hour = morning.substring(0, 2);
  String morning_min = morning.substring(3, 5);
  int ev_hour = evening_hour.toInt();
  int ev_min = evening_min.toInt();
  int mo_hour = morning_hour.toInt();
  int mo_min = morning_min.toInt();

  if ((millis() - last_reset) > DEMI_RESET_STEPPER) {
    last_reset = millis();
    timeClient.update();
    if (stepper) {
      stepper = !stepper;
      Serial.print("YOUR STEPPER STATUS: ");
      Serial.println(stepper);
    } else {
      if (mo_hour == timeClient.getHours() && mo_min == timeClient.getMinutes()) {
        StarStepper();
      } else if (ev_hour == timeClient.getHours() && ev_min == timeClient.getMinutes()) {
        StarStepper();
      }
    }
  }

  


}
