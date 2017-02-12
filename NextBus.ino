#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

ESP8266WiFiMulti WiFiMulti;

const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

const unsigned long postingInterval = 60L * 1000L;
unsigned long lastConnectionTime = 0;
int numberOfCalls = 0;

const int buttonPin = 2;
int buttonState = 0;
bool activatedDisplay = false;

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  pinMode(buttonPin, INPUT);
  
  WiFiMulti.addAP(ssid, password);
  displayMessage(1, "Connecting to wifi...");
  
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(1000);
  }
  displayLine(1, "Connected!");
  delay(3000);

  resetDisplay();
}

void loop() {
  buttonState = digitalRead(buttonPin);
 
  if (buttonState == LOW && activatedDisplay == false) {
    if(WiFiMulti.run() == WL_CONNECTED) {
      doRequest();
      activateDisplay();
    }
  }
  if (activatedDisplay && (millis() - lastConnectionTime > postingInterval)) {
    if (numberOfCalls < 5) {
      doRequest();
      numberOfCalls++;
    }
    else {
      resetDisplay();
    }
  }
}

void activateDisplay() {
  activatedDisplay = true;
  numberOfCalls = 0;
}

void resetDisplay() {
  displayMessage(2, "Ska du med");
  displayLine(2, "bussen?");
  displayLine(2, "Tryck!");
  activatedDisplay = false;
}

void doRequest() {
  lastConnectionTime = millis();
  HTTPClient http;
  http.begin("http://api.sl.se/api2/realtimedeparturesV4.json?key=YOUR_API_KEY&siteid=1275&timewindow=30");
  int httpCode = http.GET();

  if(httpCode == HTTP_CODE_OK) {
    String jsonString = http.getString();
    char *jsonChars = &jsonString[0];
    Serial.println(jsonString);
    parseJson(jsonChars);
  } 
  else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void parseJson(char* jsonPayload) {
  DynamicJsonBuffer jsonBuffer;
  
  JsonObject& root = jsonBuffer.parseObject(jsonPayload);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  
  JsonArray& buses = root["ResponseData"]["Buses"];
  if (!buses.success()) {
    Serial.println("Creating JsonArray failed");
    return;
  }

  displayMessage(1, "Lilla Essingen\n");
  
  for(JsonArray::iterator it = buses.begin(); it != buses.end(); ++it) 
  {
    const JsonObject& bus = it -> as <const JsonObject&>();
    const int direction = bus["JourneyDirection"];
    const String lineNumber = bus["LineNumber"];
    const String displayTime = bus["DisplayTime"];
    if (direction == 2) {
      displayLine(2, lineNumber + ": " + displayTime);
    }
  }
}

void displayMessage(int textSize, String text) {
  display.clearDisplay();
  display.display();
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(text);
  display.display();
}

void displayLine(int textSize, String text) {
  display.setTextSize(textSize);
  display.println(text);
  display.display();
}
