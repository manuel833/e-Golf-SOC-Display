//MANUEL833

#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include "golf.h"

BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

// Define
const char* DEVICE_NAME = "Golf"; //Gerätenamen des ESP
const char* OBDII_NAME = "OBDII"; //OBDII Namen
const char* BLUETOOTH_PIN = "1234"; //OBDII Bluetooth pin


// Define button pins
#define BUTTON1PIN 35 // Pin for the rotate button
#define BUTTON2PIN 0  // Pin for the on/off button
#define BACKLIGHT_PIN 4  // Hintergrundbeleuchtung

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
ELM327 myELM327;

uint32_t pid = 0x028C; // PID for SoC
uint8_t service = 0x22; // "Show current data" Service
uint8_t num_responses = 1; // Number of expected responses
uint8_t numExpectedBytes = 2; // Number of expected bytes in response
float scaleFactor = 1.0; // Scaling factor for response
float bias = 0.0; // Bias for response

void setup() {
    // Initialize the hardware, the BMA423 sensor has been initialized internally
    tft.init();

    // Set rotation
    tft.setRotation(1);

    // Background
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);

    // Image
    tft.pushImage(0, 0, 275, 183, golf);

    // Text Size
    tft.setTextSize(1);

    // Set "cursor" at top left corner of display (0, 0) and select font 4
    tft.setCursor(0, 0, 4);

    // Set the font colour to be white with a black background
    tft.setTextColor(TFT_BLUE);

    // Display text

    tft.drawString("Verbinde mit OBD-II", 10, 10, 4);
    tft.setTextColor(TFT_ORANGE);
    tft.setTextSize(1);
    tft.drawString("Manuel833", 35, 45, 4);
    tft.setTextSize(1);
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("Version", 70, 80, 4);
    tft.drawString("1.1", 95, 100, 4);

    // Initialize buttons
    pinMode(BUTTON1PIN, INPUT_PULLUP);
    pinMode(BUTTON2PIN, INPUT_PULLUP);
    pinMode(BACKLIGHT_PIN, OUTPUT);  // Setzen Sie den Beleuchtungspin als Ausgang
    digitalWrite(BACKLIGHT_PIN, HIGH); // Schaltet die Hintergrundbeleuchtung ein


#if LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    DEBUG_PORT.begin(38400);
    SerialBT.setPin(BLUETOOTH_PIN);
    ELM_PORT.begin(DEVICE_NAME, true);

    // Attempt to connect to OBD scanner
    if (!ELM_PORT.connect(OBDII_NAME)) {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        tft.pushImage(0, 0, 275, 183, golf);
        tft.setTextColor(TFT_RED);
        tft.drawString("FEHLER 1", 10, 10, 4);
        tft.drawString("OBDII Verbindung", 10, 45, 4);
        while (1);
    }

    if (!myELM327.begin(ELM_PORT, true, 2000)) {
        Serial.println("Couldn't connect to OBD scanner - Phase 2");
        tft.pushImage(0, 0, 275, 183, golf);
        tft.setTextColor(TFT_RED);
        tft.drawString("FEHLER 2", 10, 10, 4);
        tft.drawString("OBDII Verbindung", 10, 45, 4);
        while (1);
    }

    Serial.println("Connected to ELM327");
    tft.pushImage(0, 0, 275, 183, golf);
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("OBD 2 verbunden", 10, 30, 4);
    delay(2500);
    tft.fillScreen(TFT_BLACK);

    // Send AT command to set header
    myELM327.sendCommand("AT SH 7e5");
    myELM327.sendCommand("7e5");
    Serial.println("Sent AT command to set header to 7E0");
}

void loop() {


    // Button logic for rotating the display
    static int rotation = 1; // Variable
    // Variable to store the current rotation
    if (digitalRead(BUTTON1PIN) == LOW) {
        tft.fillScreen(TFT_BLACK); // Löscht den Bildschirm vor dem Drehen

        rotation = (rotation + 1) % 4; // Increase rotation
        tft.setRotation(rotation); // Update display rotation
        delay(500); // Short delay to prevent multiple triggers
    }

    // Button logic for turning the display on/off
    static bool displayOn = true; // Variable to store display state
    if (digitalRead(BUTTON2PIN) == LOW) {
        if (displayOn) {
            tft.writecommand(TFT_DISPOFF); // Turn off display
            digitalWrite(BACKLIGHT_PIN, LOW); // Schaltet die Hintergrundbeleuchtung aus
            displayOn = false;
        } else {
            tft.writecommand(TFT_DISPON); // Turn on display
            digitalWrite(BACKLIGHT_PIN, HIGH); // Schaltet die Hintergrundbeleuchtung ein
            displayOn = true;
        }
        delay(500); // Short delay to prevent multiple triggers
    }




  uint8_t responseByte_0 = 0; 
    
  float soc = myELM327.processPID(service, pid, num_responses, numExpectedBytes, scaleFactor, bias);
if (myELM327.nb_rx_state == ELM_SUCCESS)
{


    // Auslesen des Wertes von responseByte_0
    uint8_t responseByte_0 = myELM327.responseByte_0;
    Serial.print("Original responseByte_0: ");
    Serial.println(responseByte_0, HEX); // Zeigt den originalen Wert von responseByte_0 in Hexadezimal an

    // Manipulation des Wertes
    float soc = (responseByte_0 - 16) / 2.25;
    Serial.print("Modifizierter Wert: ");
    Serial.println(soc);

    tft.fillRect(80, 10, 200, 80, TFT_BLACK); // Breite und Höhe anpassen
    tft.setTextSize(2); // Textgröße 
    tft.setCursor(80, 10); // Cursor für die Zeile setzen
    tft.printf("%.1f%%", soc); // Zeigt das Ergebnis mit 1 Dezimalstellen

   }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
      {
    // Fehlerbehandlung
    Serial.println("Fehler beim Abrufen des SoC");
  }

  delay(200); // Delay between reads

       // Ergebnis auf dem TFT-Display anzeigen
       tft.setTextColor(TFT_WHITE); // Textfarbe setzen
       tft.setTextSize(1); // Textgröße 
       tft.setCursor(10, 10); // Cursor setzen
       tft.drawString("SOC: ",  10, 20, 4);



} 
