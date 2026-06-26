// V_0.3
// Topic: Final code that will be add to final code of this project
// Discription: Final code that will be add to final code of this project

#include <WiFi.h>
#include <HTTPClient.h>

// ======================== CONFIGURATION ========================
// 1. Home Router Configuration (Online Cloud Mode)
const char* routerSSID     = "YOUR_HOME_ROUTER_SSID";
const char* routerPASSWORD = "YOUR_HOME_ROUTER_PASSWORD";
const char* cloudNtfyUrl   = "https://ntfy.sh/local_laser_security_alarm";

// 2. Phone Hotspot Configuration (Open Network - No Password)
const char* hotspotSSID     = "AndroidAP_3211"; 
const char* hotspotPASSWORD = ""; // Empty string for unsecured hotspot
const char* localNtfyUrl    = "http://192.168.52.85:8080/local_laser_security_alarm";
// ===============================================================

// Global string to hold the automatically selected endpoint target
const char* activeNtfyUrl = ""; 

// Optimized function to push notifications out
void sendNtfyAlert(String message, String title, String priority, String tags) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        Serial.print("[ntfy] Routing alert to: ");
        Serial.println(activeNtfyUrl);
        
        http.begin(activeNtfyUrl);
        
        // Setup ntfy formatting metadata headers
        http.addHeader("Title", title);
        http.addHeader("Priority", priority); // 1 = Min, 5 = Max (Urgent)
        http.addHeader("Tags", tags);           // Notification Emojis
        
        int httpResponseCode = http.POST(message);
        
        if (httpResponseCode > 0) {
            Serial.printf("[ntfy] Alert delivered. Server Response Code: %d\n", httpResponseCode);
        } else {
            Serial.printf("[ntfy] Network post failed: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[ntfy] Broadcast blocked: WiFi link is dead.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n==================================================");
    Serial.println("  🔒 LASER SECURITY ALARM - TRANSMITTER MODULE   ");
    Serial.println("==================================================");

    // Phase 1: Try targeting your Home Router network
    Serial.printf("\n[1/2] Scanning for Home Router: %s...\n", routerSSID);
    WiFi.begin(routerSSID, routerPASSWORD);
    
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 16) { 
        delay(500);
        Serial.print(".");
        counter++;
    }

    // Phase 2: Handle Route Validation or Hotspot Fallback
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Network] Connected to Home Router (Cloud Mode active).");
        activeNtfyUrl = cloudNtfyUrl; 
    } 
    else {
        Serial.println("\n[Network] Router offline. Pivoting to local backup routing...");
        WiFi.disconnect();
        
        Serial.printf("[2/2] Linking to Open Phone Hotspot: %s...\n", hotspotSSID);
        WiFi.begin(hotspotSSID); // Instantiates connection to unsecured network
        
        counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 16) {
            delay(500);
            Serial.print(".");
            counter++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[Network] Connected to Phone Hotspot (Local Docker Mode active).");
            activeNtfyUrl = localNtfyUrl; 
        } else {
            Serial.println("\n[Network] CRITICAL ERROR: Transmitter could not establish any network profile.");
        }
    }

    // Send a boot greeting to your phone app/dashboard to confirm transmission works
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[Network] Assigned Local IP: ");
        Serial.println(WiFi.localIP());
        
        Serial.println("\n👉 READY! Type a message in Serial Monitor and press Enter to transmit.");
        Serial.println("==================================================\n");

        sendNtfyAlert(
            "Transmitter Node initialized and standing by.", 
            "System Online", 
            "3", 
            "shield,white_check_mark"
        );
    }
}

void loop() {
    // Read from Serial Monitor so you can still manually trigger test alerts
    if (Serial.available() > 0) {
        String outgoingMessage = Serial.readStringUntil('\n');
        outgoingMessage.trim(); 
        
        if (outgoingMessage.length() > 0) {
            Serial.printf("[Local Command] Triggering transmission: \"%s\"\n", outgoingMessage.c_str());
            
            // Send the serial message text straight to your phone
            sendNtfyAlert(outgoingMessage, "Manual Console Alert", "4", "warning,bell");
        }
    }
}