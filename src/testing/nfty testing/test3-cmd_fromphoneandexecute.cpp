#include <WiFi.h>
#include <HTTPClient.h>

// ======================== CONFIGURATION ========================
// 1. Home Router Configuration (Online Cloud Mode)
const char* routerSSID     = "YOUR_HOME_ROUTER_SSID";
const char* routerPASSWORD = "YOUR_HOME_ROUTER_PASSWORD";
const char* cloudNtfyUrl   = "https://ntfy.sh/local_laser_security_alarm";

// 2. Phone Hotspot Configuration (Open Network - No Password)
const char* hotspotSSID     = "AndroidAP_3211"; 
const char* hotspotPASSWORD = ""; 
const char* localNtfyUrl    = "http://192.168.52.85:8080/local_laser_security_alarm";

// EdgeHax On-Board LED Pins
const int LED_ORANGE = 40;
const int LED_WHITE  = 41;
const int LED_GREEN  = 42;
// ===============================================================

const char* activeNtfyUrl = ""; 
String subscribeUrl = ""; 
WiFiClient client; // Global client connection for persistent streaming

void controlLEDs(String command) {
    command.toUpperCase(); 
    command.trim();

    if (command == "ORANGE") {
        digitalWrite(LED_ORANGE, HIGH);
        Serial.println("🔴 LED_ORANGE turned ON!");
    } 
    else if (command == "WHITE") {
        digitalWrite(LED_WHITE, HIGH);
        Serial.println("⚪ LED_WHITE turned ON!");
    } 
    else if (command == "GREEN") {
        digitalWrite(LED_GREEN, HIGH);
        Serial.println("🟢 LED_GREEN turned ON!");
    } 
    else if (command == "OFF") {
        digitalWrite(LED_ORANGE, LOW);
        digitalWrite(LED_WHITE, LOW);
        digitalWrite(LED_GREEN, LOW);
        Serial.println("🛑 All LEDs turned OFF");
    }
}

// Establishes the continuous streaming connection
void connectToStream() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    // Parse the server IP and port out of activeNtfyUrl
    String serverHost = "192.168.52.85"; 
    int serverPort = 8080;
    
    if (String(activeNtfyUrl).indexOf("ntfy.sh") != -1) {
        serverHost = "ntfy.sh";
        serverPort = 80;
    }

    Serial.println("[Stream] Connecting to live raw events endpoint...");
    
    if (client.connect(serverHost.c_str(), serverPort)) {
        // Send raw HTTP GET request for a continuous stream
        client.println("GET /local_laser_security_alarm/raw HTTP/1.1");
        client.print("Host: "); client.println(serverHost);
        client.println("Connection: keep-alive");
        client.println();
        Serial.println("[Stream] Connected! Listening for phone triggers in real-time...");
    } else {
        Serial.println("[Stream] Connection failed. Retrying in setup loop...");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(LED_ORANGE, OUTPUT);
    pinMode(LED_WHITE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    
    // Quick hardware validation flash
    digitalWrite(LED_ORANGE, HIGH); digitalWrite(LED_WHITE, HIGH); digitalWrite(LED_GREEN, HIGH);
    delay(400);
    digitalWrite(LED_ORANGE, LOW); digitalWrite(LED_WHITE, LOW); digitalWrite(LED_GREEN, LOW);

    Serial.println("\n--- EdgeHax Streaming LED Interface ---");

    WiFi.begin(routerSSID, routerPASSWORD);
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 12) { 
        delay(500); Serial.print("."); counter++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        WiFi.begin(hotspotSSID); 
        counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 12) {
            delay(500); Serial.print("."); counter++;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        activeNtfyUrl = localNtfyUrl; 
        Serial.print("\n[Network] Online. IP Address: ");
        Serial.println(WiFi.localIP());
        
        // Open the persistent live data stream
        connectToStream();
    }
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) return;

    // If the server disconnects us, reconnect instantly
    if (!client.connected()) {
        Serial.println("\n[Stream] Lost connection to server. Reconnecting...");
        connectToStream();
        delay(1000);
        return;
    }

    // Read lines from the server stream as they arrive live
    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        
        // Skip empty keepalive heartbeats sent by ntfy
        if (line.length() > 0) {
            Serial.println("\n-------------------------------------------");
            Serial.printf("📩 NEW LIVE STREAM EVENT: %s\n", line.c_str());
            Serial.println("-------------------------------------------");
            
            // Execute the hardware control directly on the isolated string line
            controlLEDs(line);
        }
    }
}