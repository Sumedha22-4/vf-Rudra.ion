#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ---------- PINS & CONFIG ----------
#define SDA_PIN 21
#define SCL_PIN 22
#define TOUCH_PIN 27
Adafruit_SH1106G display(128, 64, &Wire, -1);

// WiFi & API
const char* ssid = "Redmi 9A";
const char* password = "e93a4dbce8e6";
String city = "Kolkata";
String apiKey = "8b0086a06d830253bc8cbecf519863e8";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

// ---------- STATES ----------
enum State { BOOT_SNORE, WAKE_UP, GREETING, EMOTION_SHOWCASE, IDLE, PET_JOY, ANNOYED, SLEEP_SNORE, CLOCK_DISPLAY };
State currentState = BOOT_SNORE;

// ---------- VARIABLES ----------
unsigned long stateTimer = 0;
unsigned long lastTouchStart = 0;
unsigned long touchDuration = 0;
unsigned long lastInteraction = 0;
int emotionIndex = 0; // To cycle through Happy, Sad, Angry, Shy
float temperature = 0;
String weatherMain = "Clear";
int zzzY = 50;
bool greetingDone = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(TOUCH_PIN, INPUT);

  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Initial Boot Screen
  display.setCursor(10, 25);
  display.print("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  timeClient.begin();
  fetchWeather();
  
  stateTimer = millis();
  lastInteraction = millis();
}

void loop() {
  timeClient.update();
  bool touching = (digitalRead(TOUCH_PIN) == HIGH);

  // --- TOUCH LOGIC (PETTING VS ANNOYED) ---
  if (touching) {
    if (lastTouchStart == 0) lastTouchStart = millis();
    touchDuration = millis() - lastTouchStart;
    lastInteraction = millis();

    if (touchDuration > 5000) currentState = ANNOYED;
    else if (currentState != ANNOYED) currentState = PET_JOY;
  } else {
    if (lastTouchStart != 0) { // Touch just ended
      lastTouchStart = 0;
      if (currentState == PET_JOY || currentState == ANNOYED) {
        currentState = IDLE;
      }
    }
  }

  // --- STATE ENGINE ---
  display.clearDisplay();
  unsigned long elapsed = millis() - stateTimer;

  switch (currentState) {
    case BOOT_SNORE:
      drawSnoring("Zzz...");
      if (elapsed > 4000) { currentState = WAKE_UP; stateTimer = millis(); }
      break;

    case WAKE_UP:
      drawYawn(elapsed);
      if (elapsed > 3000) { currentState = GREETING; stateTimer = millis(); }
      break;

    case GREETING:
      drawGreeting();
      if (elapsed > 4000) { currentState = EMOTION_SHOWCASE; stateTimer = millis(); emotionIndex = 0; }
      break;

    case EMOTION_SHOWCASE:
      handleEmotionCycle(elapsed);
      break;

    case IDLE:
      drawMochiEyes(false);
      if (millis() - lastInteraction > 25000) { currentState = SLEEP_SNORE; stateTimer = millis(); }
      break;

    case PET_JOY:
      drawJoyFace();
      break;

    case ANNOYED:
      drawAnnoyedFace();
      break;

    case SLEEP_SNORE:
      drawSnoring("Snore...");
      if (elapsed > 5000) { currentState = CLOCK_DISPLAY; stateTimer = millis(); }
      break;

    case CLOCK_DISPLAY:
      drawFullClock();
      if (touching) { currentState = WAKE_UP; stateTimer = millis(); lastInteraction = millis(); }
      break;
  }

  display.display();
  delay(30);
}

// ---------- ANIMATIONS & DRAWING ----------

void drawSnoring(String txt) {
  display.fillRect(25, 35, 20, 3, SH110X_WHITE);
  display.fillRect(83, 35, 20, 3, SH110X_WHITE);
  display.setCursor(95, zzzY);
  display.print(txt);
  zzzY--; if (zzzY < 10) zzzY = 50;
}

void drawYawn(unsigned long t) {
  int r = map(t, 0, 3000, 2, 16);
  display.fillCircle(36, 32, r, SH110X_WHITE);
  display.fillCircle(92, 32, r, SH110X_WHITE);
  display.fillCircle(64, 55, r/3, SH110X_WHITE); // Yawn mouth
}

void drawGreeting() {
  int hr = timeClient.getHours();
  String wish = "Good Day!";
  if (hr >= 5 && hr < 12) wish = "Good Morning!";
  else if (hr >= 12 && hr < 17) wish = "Good Afternoon!";
  else if (hr >= 17 && hr < 21) wish = "Good Evening!";
  else wish = "Good Night!";

  display.setTextSize(1);
  display.setCursor(10, 15);
  display.print("Hii I'm Rudra");
  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print(wish);
}

void handleEmotionCycle(unsigned long t) {
  int cycle = (t / 5000); 
  if (cycle == 0) drawMochiEyes(false); // Happy
  else if (cycle == 1) drawSadEyes();
  else if (cycle == 2) drawAngryEyes();
  else if (cycle == 3) drawShyFace();
  else { currentState = IDLE; lastInteraction = millis(); }
}

void drawFullClock() {
  display.setTextSize(2);
  display.setCursor(15, 5);
  display.print(timeClient.getFormattedTime()); // HH:MM:SS
  
  display.setTextSize(1);
  display.setCursor(15, 35);
  display.printf("Temp: %.1fC", temperature);
  display.setCursor(15, 50);
  display.print("Weather: " + weatherMain);
}

// --- BASIC EYE SHAPES ---
void drawMochiEyes(bool closed) {
  if (closed) { display.fillRect(25,32,25,4,SH110X_WHITE); display.fillRect(78,32,25,4,SH110X_WHITE); }
  else { display.fillCircle(36,32,15,SH110X_WHITE); display.fillCircle(92,32,15,SH110X_WHITE); 
         display.fillCircle(36,32,5,SH110X_BLACK); display.fillCircle(92,32,5,SH110X_BLACK); }
}

void drawSadEyes() {
  display.fillCircle(36, 32, 15, SH110X_WHITE);
  display.fillCircle(92, 32, 15, SH110X_WHITE);
  display.fillRect(0, 0, 128, 28, SH110X_BLACK); // Droopy lids
}

void drawAngryEyes() {
  display.fillCircle(36, 32, 15, SH110X_WHITE);
  display.fillCircle(92, 32, 15, SH110X_WHITE);
  display.fillTriangle(64, 30, 0, 0, 128, 0, SH110X_BLACK); // Angry Brows
}

void drawAnnoyedFace() {
  int shake = random(-2, 3);
  display.fillCircle(36 + shake, 32, 15, SH110X_WHITE);
  display.fillCircle(92 + shake, 32, 15, SH110X_WHITE);
  display.fillTriangle(64, 30, 0, 0, 128, 0, SH110X_BLACK);
}

void drawJoyFace() {
  display.drawCircleHelper(36, 35, 15, 1, SH110X_WHITE);
  display.drawCircleHelper(36, 35, 15, 2, SH110X_WHITE);
  display.drawCircleHelper(92, 35, 15, 1, SH110X_WHITE);
  display.drawCircleHelper(92, 35, 15, 2, SH110X_WHITE);
}

void drawShyFace() {
  display.fillCircle(36, 35, 12, SH110X_WHITE);
  display.fillCircle(92, 35, 12, SH110X_WHITE);
  display.fillCircle(20, 45, 3, SH110X_WHITE); // Blush
  display.fillCircle(108, 45, 3, SH110X_WHITE);
}

void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=metric&appid=" + apiKey;
    http.begin(url);
    if (http.GET() == 200) {
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, http.getString());
      temperature = doc["main"]["temp"];
      weatherMain = doc["weather"][0]["main"].as<String>();
    }
    http.end();
  }
}
