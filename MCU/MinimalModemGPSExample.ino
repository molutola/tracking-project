#include <Arduino.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "utilities.h"

XPowersPMU PMU;

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

#define TINY_GSM_RX_BUFFER 1024

#define TINY_GSM_MODEM_SIM7080
#include <TinyGsmClient.h>
#include "utilities.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// WiFi credentials
const char* ssid = "John3v16";
const char* password = "moyinoluwa";

const char* server = "54.175.245.132";
const int port = 8000;

const char apn[]  = "wholesale";  // APN for Mint Mobile
const char user[] = "";                    // Username (leave blank)
const char pass[] = "";  

String clientId;
double destLat;
double destLng;
String phoneNumber;
const String message = "Hello, this is a test SMS from ESP32!";

// Backend server URL
const char serverUrl[] = "http://localhost:8000/api/location";
const char batteryUrl[] = "http://localhost:8000/api/battery";
const char adminPhoneNumber[] = "+16233209369";
const char clientDetailsUrl[] = "http://localhost:8000/api/getClientDetails";

float lat2 = 0, lon2 = 0, speed2 = 0, alt2 = 0, accuracy2 = 0;
int vsat2 = 0, usat2 = 0, year2 = 0, month2 = 0, day2 = 0, hour2 = 0, min2 = 0, sec2 = 0;
bool level = false;

unsigned long lastLocationSentTime = 0;
unsigned long lastBatterySentTime = 0;

const double SPEED_KMH = 50.0; // Assume a constant speed of 50 km/h
const double THRESHOLD_DISTANCE_KM = (SPEED_KMH / 60.0) * 5.0; // Distance covered in 5 minutes

struct ClientDetails {
    String clientId;
    double destLat;
    double destLng;
    String phoneNumber;
};

ClientDetails getClientDetailsAndLocation() {
    ClientDetails details = {"", 0.0, 0.0};

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(clientDetailsUrl); // Replace with your actual server URL
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            Serial.print("Server response: ");
            Serial.println(payload);

            // Parse the JSON response
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload);
            if (error) {
                Serial.print("Failed to parse JSON: ");
                Serial.println(error.c_str());
                return details;
            }

            details.clientId = doc["clientId"].as<String>();
            details.destLat = doc["destLat"].as<double>();
            details.destLng = doc["destLng"].as<double>();
            details.phoneNumber = doc["phoneNumber"].as<String>();
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }

    return details;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    delay(3000);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Get client details and location from server
    ClientDetails details = getClientDetailsAndLocation();
    clientId = details.clientId;
    destLat = details.destLat;
    destLng = details.destLng;
    phoneNumber = details.phoneNumber;

    if (clientId == "") {
        Serial.println("Failed to get client details from server");
        while (1) {
            delay(5000);
        }
    }
    Serial.print("Client ID: ");
    Serial.println(clientId);
    Serial.print("Destination Latitude: ");
    Serial.println(destLat, 8);
    Serial.print("Destination Longitude: ");
    Serial.println(destLng, 8);

    // Initialize the PMU
    if (!PMU.begin(Wire1, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        Serial.println("Failed to initialize PMU");
        while (1) {
            delay(5000);
        }
    }

    // Enable battery voltage measurement
    PMU.enableBattVoltageMeasure();

    // If it is a power cycle, turn off the modem power. Then restart it
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        PMU.disableDC3();
        // Wait a minute
        delay(200);
    }

    // Set the working voltage of the modem, please do not modify the parameters
    PMU.setDC3Voltage(3000); // SIM7080 Modem main power channel 2700~ 3400V
    PMU.enableDC3();

    // Modem GPS Power channel
    PMU.setBLDO2Voltage(3300);
    PMU.enableBLDO2(); // The antenna power must be turned on to use the GPS function

    // TS Pin detection must be disable, otherwise it cannot be charged
    PMU.disableTSPinMeasure();

    // Start modem
    Serial1.begin(115200, SERIAL_8N1, BOARD_MODEM_RXD_PIN, BOARD_MODEM_TXD_PIN);

    pinMode(BOARD_MODEM_PWR_PIN, OUTPUT);

    digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_MODEM_PWR_PIN, HIGH);
    delay(1000);
    digitalWrite(BOARD_MODEM_PWR_PIN, LOW);

    int retry = 0;
    while (!modem.testAT(1000)) {
        Serial.print(".");
        if (retry++ > 15) {
            // Pull down PWRKEY for more than 1 second according to manual requirements
            digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
            delay(100);
            digitalWrite(BOARD_MODEM_PWR_PIN, HIGH);
            delay(1000);
            digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
            retry = 0;
            Serial.println("Retry start modem .");
        }
    }
    Serial.println();
    Serial.print("Modem started!");

    // Start modem GPS function
    modem.disableGPS();
    delay(500);

    // GPS function needs to be enabled for the first use
    if (modem.enableGPS() == false) {
        Serial.print("Modem enable gps function failed!!");
        while (1) {
            delay(5000);
        }
    }
}

void loop()
{
    // Measure battery voltage
    float batteryVoltage = PMU.getBattVoltage();
    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" mV");

    unsigned long currentTime = millis();

    // Send battery level every 1 minute
    if (currentTime - lastBatterySentTime >= 60000) { // 1 minute = 60000 milliseconds
        sendBatteryToServer(batteryVoltage);
        lastBatterySentTime = currentTime;
    }

    if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                     &year2, &month2, &day2, &hour2, &min2, &sec2)) {
        Serial.println();
        Serial.print("lat:"); Serial.print(String(lat2, 8)); Serial.print("\t");
        Serial.print("lon:"); Serial.print(String(lon2, 8)); Serial.println();

        // Send location every 1 minute
        if (currentTime - lastLocationSentTime >= 60000) { // 1 minute = 60000 milliseconds
            sendLocationToServer(lat2, lon2);
            lastLocationSentTime = currentTime;
        }
        // Calculate distance to destination
        double distanceToDestination = calculateDistance(lat2, lon2, destLat, destLng);
        Serial.print("Distance to destination: ");
        Serial.print(distanceToDestination);
        Serial.println(" km");

        // Check if distance is less than the threshold
        if (distanceToDestination <= THRESHOLD_DISTANCE_KM) {
            sendSMS(phoneNumber, "Your delivery is almost there!");
        }

        // Check if arrived at destination
        if (distanceToDestination <= 0.1) { // Consider arrived if within 100 meters
            sendSMS(phoneNumber, "Your delivery has arrived!");
        }
        // After successful positioning, the PMU charging indicator flashes quickly
        PMU.setChargingLedMode(XPOWERS_CHG_LED_BLINK_4HZ);
    } else {
        // Blinking PMU charging indicator
        PMU.setChargingLedMode(level ? XPOWERS_CHG_LED_ON : XPOWERS_CHG_LED_OFF);
        level ^= 1;
        delay(1000);
    }
}

float calculateBatteryPercentage(float voltage) {
    // Assuming a LiPo battery with a voltage range of 3.0V (0%) to 4.2V (100%)
    float minVoltage = 3.0;
    float maxVoltage = 4.2;
    float percentage = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100;
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    return percentage;
}

void sendLocationToServer(double lat, double lng) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String requestBody = "{\"clientId\":\"" + clientId + "\",\"lat\":" + String(lat, 8) + ",\"lng\":" + String(lng, 8) + "}";
        Serial.print("Sending data to server: ");
        Serial.println(requestBody);

        int httpCode = http.POST(requestBody);

        if (httpCode > 0) {
            String response = http.getString();
            Serial.print("Server response: ");
            Serial.println(response);
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

void sendSMS(const String& phoneNumber, const String& message) {
    // Disable GPS to allow cellular SMS
    modem.disableGPS();
    delay(1000); // Ensure GPS is fully disabled

    Serial.println("Sending SMS...");
    if (modem.sendSMS(phoneNumber, message)) {
        Serial.println("SMS sent successfully!");
    } else {
        Serial.println("Failed to send SMS.");
    }

    // Re-enable GPS after SMS is sent
    if (!modem.enableGPS()) {
        Serial.println("Failed to re-enable GPS!");
    }
}

double calculateDistance(double lat1, double lng1, double lat2, double lng2) {
    // Haversine formula to calculate the distance between two points on the Earth
    double dLat = radians(lat2 - lat1);
    double dLng = radians(lat2 - lng1);
    double a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLng / 2) * sin(dLng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = 6371 * c; // Distance in kilometers
    return distance;
}

void sendBatteryToServer(float voltage) {
    float batteryPercentage = calculateBatteryPercentage(voltage);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(batteryUrl);
        http.addHeader("Content-Type", "application/json");

        String requestBody = "{\"clientId\":\"" + clientId + "\",\"voltage\":" + String(voltage, 2) + ",\"percentage\":" + String(batteryPercentage, 2) + "}";
        Serial.print("Sending battery data to server: ");
        Serial.println(requestBody);

        int httpCode = http.POST(requestBody);

        if (httpCode > 0) {
            String response = http.getString();
            Serial.print("Server response: ");
            Serial.println(response);
        } else {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}
