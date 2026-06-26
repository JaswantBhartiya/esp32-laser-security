// V_0.2
// Topic: testing the esp ntfy with massage input from serial monitor
// Discription: testing the esp ntfy with massage input from serial monitor


#include <WiFi.h>
#include <HTTPClient.h>

// ======================== CONFIGURATION ========================
// 1. Home Router Configuration (Online Cloud Mode)
const char* routerSSID     = "YOUR_HOME_ROUTER_SSID";
const char* routerPASSWORD = "YOUR_HOME_ROUTER_PASSWORD";
const char* cloudNtfyUrl   = "https://ntfy.sh/laser_security_alarm";

// 2. Phone Hotspot Configuration (Open Network - No Password)
const char* hotspotSSID     = "YOUR_OPEN_HOTSPOT_NAME"; 
const char* hotspotPASSWORD = ""; // Empty string for open network
const char* localNtfyUrl    = "http://192.168.52.85:8080/laser_security_alarm";
// ===============================================================

const char* activeNtfyUrl = ""; 
String subscribeUrl = ""; // To store the json stream endpoint

// Function to Publish Messages typed in Serial Monitor
void publishNtfyMessage(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(activeNtfyUrl);
        http.addHeader("Title", "ESP32 Terminal Console");
        http.addHeader("Tags", "computer,speech_balloon");
        
        int httpResponseCode = http.POST(message);
        if (httpResponseCode > 0) {
            Serial.printf("[SENT] Message published successfully! (Code: %d)\n", httpResponseCode);
        } else {
            Serial.printf("[ERROR] Failed to publish: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[ERROR] WiFi disconnected. Cannot publish.");
    }
}

// Function to check and print incoming messages from the server
void checkIncomingMessages() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    // Using a 1-second timeout so it doesn't block your serial input typing loop
    http.begin(subscribeUrl + "?poll=1"); 
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        if (payload.length() > 0) {
            // Simple parsing to pull out the raw message string from ntfy JSON formatting
            int msgIdx = payload.indexOf("\"message\":\"");
            if (msgIdx != -1) {
                int start = msgIdx + 11;
                int end = payload.indexOf("\"", start);
                String receivedMsg = payload.substring(start, end);
                
                Serial.println("\n-------------------------------------------");
                Serial.printf("📩 RECEIVED FROM PHONE/WEB UI: %s\n", receivedMsg.c_str());
                Serial.println("-------------------------------------------");
            }
        }
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Greet the user at the very beginning
    Serial.println("\n==================================================");
    Serial.println("   🚀 WELCOME TO THE ESP32-S3 CONSOLE TERMINAL   ");
    Serial.println("==================================================");
    Serial.println("Initializing system modules...\n");

    // Network Fallback Logic
    Serial.printf("Connecting to Home Router: %s...\n", routerSSID);
    WiFi.begin(routerSSID, routerPASSWORD);
    
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 16) { 
        delay(500);
        Serial.print(".");
        counter++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Network] Connected to Home Router!");
        activeNtfyUrl = cloudNtfyUrl;
    } 
    else {
        Serial.println("\n[Network] Router not found. Switching to Open Phone Hotspot...");
        WiFi.disconnect();
        
        WiFi.begin(hotspotSSID); // Connect to open network
        
        counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 16) {
            delay(500);
            Serial.print(".");
            counter++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[Network] Connected to Open Phone Hotspot!");
            activeNtfyUrl = localNtfyUrl;
        } else {
            Serial.println("\n[Network] Critical Error: Network setup failed.");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[Network] Local IP allocated: ");
        Serial.println(WiFi.localIP());
        
        // Build the polling subscription string layout
        subscribeUrl = String(activeNtfyUrl) + "/json";
        
        Serial.println("\n==================================================");
        Serial.println("👉 READY! Type anything below and hit Enter to send:");
        Serial.println("==================================================\n");
    }
}

void loop() {
    // 1. Check if user typed anything into the Serial Monitor to publish
    if (Serial.available() > 0) {
        String outgoingMessage = Serial.readStringUntil('\n');
        outgoingMessage.trim(); // Clean up trailing spaces/newlines
        
        if (outgoingMessage.length() > 0) {
            Serial.printf("[LOCAL] Transmitting message: \"%s\"\n", outgoingMessage.c_str());
            publishNtfyMessage(outgoingMessage);
        }
    }

    // 2. Listen to check if any incoming messages are arriving from the phone
    checkIncomingMessages();
    
    delay(200); // Small breathing window for the ESP32 network stack
}