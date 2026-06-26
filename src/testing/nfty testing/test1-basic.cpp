// V_0.1
// Topic: Basic testing nfty with esp
// Discription: This code is for testing nfty notification with esp


#include <WiFi.h>
#include <HTTPClient.h>

// ======================== CONFIGURATION ========================
// 1. Home Router Configuration (Online Cloud Mode)
const char* routerSSID     = "YOUR_HOME_ROUTER_SSID";
const char* routerPASSWORD = "YOUR_HOME_ROUTER_PASSWORD";
const char* cloudNtfyUrl   = "https://ntfy.sh/laser_security_alarm";

// 2. Phone Hotspot Configuration (Offline Local Docker Mode)
const char* hotspotSSID     = "AndroidAP_3211";
const char* hotspotPASSWORD = "";
const char* localNtfyUrl    = "http://192.168.52.85:8080/local_laser_security_alarm";
// ===============================================================

// Global variable to store whichever URL is successfully selected
const char* activeNtfyUrl = ""; 

void sendNtfyAlert(String message, String title, String priority, String tags) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        Serial.print("[ntfy] Sending alert to: ");
        Serial.println(activeNtfyUrl);
        
        http.begin(activeNtfyUrl);
        
        // Setup headers using clean strings to avoid terminal-like parsing bugs
        http.addHeader("Title", title);
        http.addHeader("Priority", priority); // 5 = Max Urgency
        http.addHeader("Tags", tags);           // Emojis
        
        int httpResponseCode = http.POST(message);
        
        if (httpResponseCode > 0) {
            Serial.printf("[ntfy] Alert sent successfully! Response code: %d\n", httpResponseCode);
        } else {
            Serial.printf("[ntfy] Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[ntfy] Cannot send alert: WiFi not connected.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- Laser Security Network Init ---");

    // Try Connecting to Home Router first
    Serial.printf("Attempting to connect to Home Router: %s...\n", routerSSID);
    WiFi.begin(routerSSID, routerPASSWORD);
    
    int counter = 0;
    // Wait for 10 seconds maximum
    while (WiFi.status() != WL_CONNECTED && counter < 20) { 
        delay(500);
        Serial.print(".");
        counter++;
    }

    // Check if successfully connected to the Router
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Network] Connected to Home Router successfully!");
        Serial.print("[Network] IP Address: ");
        Serial.println(WiFi.localIP());
        activeNtfyUrl = cloudNtfyUrl; // Target the public cloud server
    } 
    else {
        // Fallback: Router not found, switch over to Phone Hotspot
        Serial.println("\n[Network] Home Router not found. Switching to Fallback Mode...");
        WiFi.disconnect(); // Clear failed connection attempt
        
        Serial.printf("[Network] Connecting to Phone Hotspot: %s...\n", hotspotSSID);
        WiFi.begin(hotspotSSID, hotspotPASSWORD);
        
        counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 20) {
            delay(500);
            Serial.print(".");
            counter++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[Network] Connected to Phone Hotspot!");
            Serial.print("[Network] IP Address: ");
            Serial.println(WiFi.localIP());
            activeNtfyUrl = localNtfyUrl; // Target your local laptop Docker IP
        } else {
            Serial.println("\n[Network] Critical Error: Both network connections failed.");
        }
    }

    // Fire a confirmation startup test alert if any network is up
    if (WiFi.status() == WL_CONNECTED) {
        sendNtfyAlert(
            "ESP32-S3 firmware online and armed.", 
            "🔒 SYSTEM BOOT", 
            "3", 
            "gear,white_check_mark"
        );
    }
}

void loop() {
    // This standalone file is dedicated purely to testing your network initialization setup.
    // Once verified, we will drop this structure into your core state machine file.
}