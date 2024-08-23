//DEV by Manuel
//Version 1.2

#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include "FS.h"
#include <SD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

// SD-Karten Pin-Definition (je nach Ihrem Setup anpassen)
const int chipSelect = 5;
File golfImageFile;

// Bluetooth und OBD-II
BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

// Variablen zum Speichern der Konfigurationsdaten
String DEVICE_NAME;
String OBDII_NAME;
String BLUETOOTH_PIN;
int BUTTON1PIN;
int BUTTON2PIN;
int BACKLIGHT_PIN;
String WIFI_SSID;
String WIFI_PASSWORD;

// TFT und OBD-II
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
ELM327 myELM327;

uint32_t pidSoC = 0x028C; // PID for SoC
uint32_t pidTemp = 0x2A0B; // PID for Battery Temperature
uint8_t service = 0x22; // "Show current data" Service
uint8_t num_responses = 1; // Number of expected responses
uint8_t numExpectedBytesSoC = 2; // Number of expected bytes in response for SoC
uint8_t numExpectedBytesTemp = 2; // Number of expected bytes in response for Temperature
float scaleFactorSoC = 1.0; // Scaling factor for SoC response
float scaleFactorTemp = 1.0; // Scaling factor for Temp response
float biasSoC = 0.0; // Bias for SoC response
float biasTemp = 0.0; // Bias for Temp response

// WiFi and WebServer variables
AsyncWebServer server(80);
Preferences preferences;

void setup() {
    Serial.begin(115200);

    if (!SD.begin(chipSelect)) {
        Serial.println("SD-Karte konnte nicht initialisiert werden.");
        return;
    }
    Serial.println("SD-Karte initialisiert.");

    // Konfigurationsdatei laden
    loadConfig("/config.txt");

    // TFT-Initialisierung
    tft.init();
    Serial.println("TFT initialized");

    // Set rotation
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(true);

    // Hintergrundbeleuchtung ein
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);

    // Starten des Access Points oder Verbinden mit WLAN
    if (WIFI_SSID != "" && WIFI_PASSWORD != "") {
        connectToWiFi();
    } else {
        setupAccessPoint();
    }

    // Bild anzeigen
    displayImageFromSD();
}

void loadConfig(const char* filename) {
    File configFile = SD.open(filename);
    if (!configFile) {
        Serial.println("Fehler beim Öffnen der Konfigurationsdatei");
        return;
    }

    while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        line.trim();

        int delimiterIndex = line.indexOf('=');
        if (delimiterIndex == -1) continue;

        String key = line.substring(0, delimiterIndex);
        String value = line.substring(delimiterIndex + 1);

        if (key == "DEVICE_NAME") DEVICE_NAME = value;
        else if (key == "OBDII_NAME") OBDII_NAME = value;
        else if (key == "BLUETOOTH_PIN") BLUETOOTH_PIN = value;
        else if (key == "BUTTON1PIN") BUTTON1PIN = value.toInt();
        else if (key == "BUTTON2PIN") BUTTON2PIN = value.toInt();
        else if (key == "BACKLIGHT_PIN") BACKLIGHT_PIN = value.toInt();
        else if (key == "WIFI_SSID") WIFI_SSID = value;
        else if (key == "WIFI_PASSWORD") WIFI_PASSWORD = value;
    }
    configFile.close();
}

void connectToWiFi() {
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    Serial.print("Verbinde mit dem WLAN-Netzwerk: ");
    Serial.println(WIFI_SSID);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nErfolgreich verbunden!");
        Serial.print("IP Adresse: ");
        Serial.println(WiFi.localIP());
        // Hier können Sie alle Aktionen starten, die nach der WLAN-Verbindung erfolgen sollen
        initializeOBDConnection();  // OBD-Verbindung initialisieren
    } else {
        Serial.println("\nVerbindung fehlgeschlagen. Starte den Access Point.");
        setupAccessPoint();
    }
}

void setupAccessPoint() {
    const char* apSSID = "ESP32_HOTSPOT";
    const char* apPassword = "12345678";

    WiFi.softAP(apSSID, apPassword);
    Serial.println("Access Point gestartet");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Webserver einrichten
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<form method='POST' action='/save'>";
        html += "SSID: <input type='text' name='ssid'><br>";
        html += "Password: <input type='password' name='password'><br>";
        html += "<input type='submit' value='Save'>";
        html += "</form>";
        request->send(200, "text/html", html);
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        preferences.begin("wifi", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();

        String html = "SSID und Passwort gespeichert. ESP32 wird nun mit dem Netzwerk verbinden.";
        request->send(200, "text/html", html);
        
        // Neustart nach dem Speichern der Konfiguration
        delay(3000);
        ESP.restart();
    });

    server.begin();
}

void initializeOBDConnection() {
    DEBUG_PORT.begin(38400);
    SerialBT.setPin(BLUETOOTH_PIN.c_str());
    ELM_PORT.begin(DEVICE_NAME.c_str(), true);

    // Verbindung mit OBD-II Adapter
    bool connected = false;
    for (int i = 0; i < 3; i++) {  // 3 Verbindungsversuche
        if (ELM_PORT.connect(OBDII_NAME.c_str())) {
            connected = true;
            break;
        } else {
            DEBUG_PORT.println("Verbindungsversuch mit OBD-II Adapter fehlgeschlagen, versuche erneut...");
            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_RED);
            tft.drawString("FEHLER", 10, 10, 4);
            tft.drawString("OBDII Verbindung ", 10, 45, 4);
            tft.drawString("Versuche erneut ", 10, 80, 4);
            delay(2000);  // Warte 2 Sekunden vor erneutem Versuch
        }
    }

    if (!connected) {
        DEBUG_PORT.println("Konnte keine Verbindung mit dem OBD-II Adapter herstellen - Phase 1");
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("FEHLER 1", 10, 10, 4);
        tft.drawString("OBDII Verbindung", 10, 45, 4);
        while (1);
    }

    if (!myELM327.begin(ELM_PORT, true, 2000)) {
        Serial.println("Konnte keine Verbindung mit dem OBD-II Scanner herstellen - Phase 2");
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("FEHLER 2", 10, 10, 4);
        tft.drawString("OBDII Verbindung", 10, 45, 4);
        while (1);
    }

    Serial.println("Connected to ELM327");
    tft.fillScreen(TFT_BLACK);
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

void displayImageFromSD() {
    golfImageFile = SD.open("/golf.bmp");
    if (golfImageFile) {
        tft.setCursor(0, 0);
        int byteRead;
        while ((byteRead = golfImageFile.read()) != -1) {
            tft.pushColor(byteRead);
        }
        golfImageFile.close();
    } else {
        Serial.println("Bilddatei nicht geöffnet oder ungültig.");
    }
}

void loop() {
    // Ihr Hauptprogrammcode
    // Button logic for rotating the display
    static int rotation = 1; // Variable to store the current rotation
    if (digitalRead(BUTTON1PIN) == LOW) {
        tft.fillScreen(TFT_BLACK); // Clear screen before rotating

        rotation = (rotation + 1) % 4; // Increase rotation
        tft.setRotation(rotation); // Update display rotation
        delay(500); // Short delay to prevent multiple triggers

        // Re-display static text after rotation
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.drawString("SOC: ", 10, 20, 4);
        tft.drawString("Temp: ", 10, 60, 4);
    }

    // Button logic for turning the display on/off
    static bool displayOn = true; // Variable to store display state
    if (digitalRead(BUTTON2PIN) == LOW) {
        if (displayOn) {
            tft.writecommand(TFT_DISPOFF); // Turn off display
            digitalWrite(BACKLIGHT_PIN, LOW); // Turn off backlight
            displayOn = false;
        } else {
            tft.writecommand(TFT_DISPON); // Turn on display
            digitalWrite(BACKLIGHT_PIN, HIGH); // Turn on backlight
            displayOn = true;
        }
        delay(500); // Short delay to prevent multiple triggers
    }

    // Read SoC value
    float soc = myELM327.processPID(service, pidSoC, num_responses, numExpectedBytesSoC, scaleFactorSoC, biasSoC);
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        // Read the response byte and process SoC value
        uint8_t responseByte_0 = myELM327.responseByte_0;
        soc = (responseByte_0 - 16) / 2.25;

        // Update SoC value on display
        tft.fillRect(80, 20, 200, 30, TFT_BLACK); // Clear the area where the value will be displayed
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1); // Adjust text size as needed
        tft.setCursor(80, 20); // Set cursor for value
        tft.printf("%.1f%%", soc); // Display result with 1 decimal place
    } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        // Error handling
        Serial.println("Fehler beim Abrufen des SoC");
    }

    // Read Battery Temperature
    float batteryTemp = myELM327.processPID(service, pidTemp, num_responses, numExpectedBytesTemp, scaleFactorTemp, biasTemp);
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        // Extract response bytes for temperature
        uint8_t responseByte_0 = myELM327.responseByte_0;
        uint8_t responseByte_1 = myELM327.responseByte_1;
        batteryTemp = ((responseByte_0 * 256) + responseByte_1) / 64.0;

        // Update temperature value on display
        tft.fillRect(80, 60, 200, 30, TFT_BLACK); // Clear the area where the value will be displayed
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1); // Adjust text size as needed
        tft.setCursor(80, 60); // Set cursor for value
        tft.printf("%.1f C", batteryTemp); // Display result with 1 decimal place
    } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        // Error handling
        Serial.println("Fehler beim Abrufen der Batterietemperatur");
    }

    delay(200); // Delay between reads
}
