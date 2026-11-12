#define BLYNK_TEMPLATE_ID "TMPL3v_hpjtgD"
#define BLYNK_TEMPLATE_NAME "New Template"
#define BLYNK_AUTH_TOKEN "8SHB431AxmPalqQuF_k_zbKzfT2L4oSP"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 5
#define RST_PIN 22
#define LED_PIN 25
#define BUZZER_PIN 26

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 17
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

MFRC522 rfid(SS_PIN, RST_PIN);

Adafruit_SSD1306 display(
  OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET
);

struct Student {
  const char* uid;
  const char* name;
};

Student students[] = {
  {"01 02 03 04", "Student 1"},
  {"11 22 33 44", "Student 2"},
  {"55 66 77 88", "Student 3"},
  {"AA BB CC DD", "Student 4"}
};

const int totalStudents =
  sizeof(students) / sizeof(students[0]);

void showOLED(const char* line1,
              const char* line2,
              const char* line3) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 5);
  display.println(line1);

  display.setCursor(0, 25);
  display.println(line2);

  display.setCursor(0, 45);
  display.println(line3);

  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED initialization failed!");
  } else {
    showOLED("RFID ATTENDANCE",
             "Starting system",
             "Please wait...");
  }

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  Serial.println("RFID Attendance Logger");
  Serial.println("Connecting to Blynk...");

  showOLED("RFID ATTENDANCE",
           "Connecting to",
           "Blynk...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Blynk.virtualWrite(V1, "RFID Attendance ESP32");
  Blynk.virtualWrite(V2, "Ready");

  showOLED("RFID ATTENDANCE",
           "System Ready",
           "Scan your card");

  Serial.println("Blynk connected!");
  Serial.println("Scan your RFID card...");
}

void loop() {
  Blynk.run();

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      uid += " ";
    }
  }

  uid.toUpperCase();

  Serial.print("Card UID: ");
  Serial.println(uid);

  // V0 stores the most recently scanned UID
  Blynk.virtualWrite(V0, uid);

  bool authorized = false;

  // Check registered student cards
  for (int i = 0; i < totalStudents; i++) {
    if (uid == students[i].uid) {
      authorized = true;

      Serial.print("Welcome, ");
      Serial.println(students[i].name);

      // V3, V4, V5, V6 store each student's latest UID
      Blynk.virtualWrite(V3 + i, uid);

      showOLED("ATTENDANCE MARKED",
               students[i].name,
               "Access Granted");

      Blynk.virtualWrite(V2, "Attendance Marked");

      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000);
      delay(300);

      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

      delay(3000);
      break;
    }
  }

  if (!authorized) {
    Serial.println("Access Denied");

    showOLED("ACCESS DENIED",
             "Unknown Card",
             "Not Registered");

    Blynk.virtualWrite(V2, "Access Denied");

    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);

    delay(3000);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  showOLED("RFID ATTENDANCE",
           "System Ready",
           "Scan your card");
}
