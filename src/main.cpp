#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <PubSubClient.h>

// =========================================================================
// 📡 NETWORK & HARDWARE CONFIGURATION PARAMETERS
// =========================================================================
const char* ap_ssid = "ESP32S3-Security-Hub";
const char* ap_pass = "secure1234";

const char* home_ssid = "WireShark-2.4Ghz"; 
const char* home_pass = "$p!D3y-PasS";    

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883; 
const char* mqtt_topic = "sgsits/mechanical/laserSecurity/alerts";

// =========================================================================
// 🎛️ OBJECT INSTANTIATIONS & GLOBAL SYSTEM VARIABLES
// =========================================================================
AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Shared Global Memory Layout Matrix
int globalLdrRaw = 450;
bool globalIsMuted = false;
String globalSystemMode = "NOTIFIER"; 
String globalLastAction = "RESET";

unsigned long lastMqttRetry = 0;
unsigned long lastHeartbeat = 0;

// =========================================================================
// 🔄 UNIFIED STATE BROADCAST COMPILER
// =========================================================================
void broadcastSystemStateMQTT() {
    if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
        String payload = "{\"ldr_raw\":" + String(globalLdrRaw) + 
                         ",\"is_muted\":" + (globalIsMuted ? String("true") : String("false")) + 
                         ",\"system_mode\":\"" + globalSystemMode + "\"" +
                         ",\"last_action\":\"" + globalLastAction + "\"}";
                         
        mqttClient.publish(mqtt_topic, payload.c_str());
        Serial.println("[MQTT WAN] Dispatched synchronization packet: " + payload);
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) { message += (char)payload[i]; }
    Serial.println("[MQTT INCOMING] Cloud packet intercepted: " + message);

    // Strict string parsing rules to handle remote UI toggles smoothly
    if (message.indexOf("\"system_mode\":\"NOTIFIER\"") > 0) globalSystemMode = "NOTIFIER";
    else if (message.indexOf("\"system_mode\":\"ARMED\"") > 0) globalSystemMode = "ARMED";

    if (message.indexOf("\"is_muted\":true") > 0) globalIsMuted = true;
    else if (message.indexOf("\"is_muted\":false") > 0) globalIsMuted = false;

    if (message.indexOf("\"last_action\":\"BREACH\"") > 0) {
        globalLdrRaw = 3850;
        globalLastAction = "BREACH";
    } 
    else if (message.indexOf("\"last_action\":\"RESET\"") > 0) {
        globalLdrRaw = 450;
        globalLastAction = "RESET";
    } 
    else if (message.indexOf("\"last_action\":\"MUTE\"") > 0) {
        globalLastAction = "MUTE";
    }
}

void handleMqttReconnection() {
    // Only attempt MQTT operations if the background Wi-Fi link has successfully connected
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastMqttRetry > 5000) { 
            lastMqttRetry = now;
            Serial.print("[MQTT LOG] Home router connected. Binding to HiveMQ broker matrix...");
            
            String clientId = "ESP32S3_Gateway_" + String(random(0, 9999));
            if (mqttClient.connect(clientId.c_str())) {
                Serial.println(" CONNECTED!");
                mqttClient.subscribe(mqtt_topic);
                broadcastSystemStateMQTT();
            } else {
                Serial.print(" FAILED, rc=");
                Serial.println(mqttClient.state());
            }
        }
    }
}

// =========================================================================
// 🚀 CORE SETUP INITIALIZATION INTERFACE (NON-BLOCKING)
// =========================================================================
void setup() {
    Serial.begin(115200);
    delay(1000); 
    
    Serial.println("\n==============================================");
    Serial.println("[SYSTEM CORE] INITIALIZING ASYNCHRONOUS NETWORK LAYER...");
    Serial.println("==============================================");

    if (!LittleFS.begin()) {
        Serial.println("[ERROR] LittleFS Mounting Failure! Process halted.");
        return;
    }

    // Set dual-mode station + access point configurations
    WiFi.mode(WIFI_AP_STA);

    // 1. Spawn Local Hotspot Instantly (Zero Waiting Time)
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.print("[AP] Local Sentinel Portal Spawned. Gateway IP: ");
    Serial.println(WiFi.softAPIP());

    // 2. Bind all Async Web Server endpoints immediately 
    // This ensures 192.168.4.1 works instantly even if there's no router nearby!
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonResponse = "{\"ldr_raw\":" + String(globalLdrRaw) + 
                              ",\"is_muted\":" + (globalIsMuted ? String("true") : String("false")) + 
                              ",\"system_mode\":\"" + globalSystemMode + "\"" +
                              ",\"last_action\":\"" + globalLastAction + "\"}";
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/api/action", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("cmd")) {
            String command = request->getParam("cmd")->value();
            
            if (command == "mode_notifier") globalSystemMode = "NOTIFIER";
            else if (command == "mode_armed") globalSystemMode = "ARMED";
            else if (command == "sim_breach") {
                globalLdrRaw = 3850;
                globalIsMuted = false;
                globalLastAction = "BREACH";
            } 
            else if (command == "reset") {
                globalLdrRaw = 450;
                globalIsMuted = false;
                globalLastAction = "RESET";
            } 
            else if (command == "mute") {
                globalIsMuted = !globalIsMuted; // Toggles mute state cleanly
                globalLastAction = "MUTE";
            }
            
            broadcastSystemStateMQTT(); 
            request->send(200, "text/plain", "ACK");
        } else {
            request->send(400, "text/plain", "BAD REQUEST");
        }
    });

    server.begin();
    Serial.println("[SYSTEM CORE] Local Server Engine Activated on Port 80.");

    // 3. Initiate background handshake with home router
    Serial.print("[STA] Registering background connection request to: ");
    Serial.println(home_ssid);
    WiFi.begin(home_ssid, home_pass); 
    
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
}

// =========================================================================
// 🔄 REAL-TIME MAIN PROCESS MAINTENANCE LOOP
// =========================================================================
void loop() {
    // Check connection status and manage the MQTT subscription state completely in the background
    if (WiFi.status() == WL_CONNECTED) {
        handleMqttReconnection();
        mqttClient.loop();
    }

    // Low Frequency Background Telemetry Sync Loop
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeat > 4000) {
        lastHeartbeat = currentMillis;
        if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
            broadcastSystemStateMQTT();
        }
    }

    // Your physical LDR analog acquisition hook layout maps down here later (analogRead(4))...
}