document.addEventListener("DOMContentLoaded", () => {
    const ldrGauge = document.getElementById("ldr-gauge");
    const ldrProgress = document.getElementById("ldr-progress");
    const systemState = document.getElementById("system-state");
    const connIndicator = document.getElementById("connection-indicator");
    const mqttIndicator = document.getElementById("mqtt-indicator");
    const consoleLog = document.getElementById("console-log");
    
    const btnReset = document.getElementById("btn-reset");
    const btnMute = document.getElementById("btn-mute");
    const btnTriggerMqtt = document.getElementById("btn-trigger-mqtt");

    const ALARM_THRESHOLD = 1500; 
    const clientID = "sentinel_client_" + Math.floor(Math.random() * 10000);
    const mqttTopic = "sgsits/mechanical/laserSecurity/alerts"; 

    let mqttClient = null;
    let isLocalMode = false;

    function addLog(message) {
        const timestamp = new Date().toLocaleTimeString();
        const logLine = document.createElement("div");
        logLine.className = "log-line";
        logLine.innerText = `[${timestamp}] ${message}`;
        consoleLog.appendChild(logLine);
        consoleLog.scrollTop = consoleLog.scrollHeight;
    }

    // =========================================================================
    // 🗺️ DYNAMIC AUTOMATIC NETWORK ROUTING SELECTOR
    // =========================================================================
    if (window.location.hostname === "192.168.4.1") {
        isLocalMode = true;
        mqttIndicator.innerText = "MQTT: BYPASSED (LOCAL AP)";
        mqttIndicator.className = "conn-tag tag-amber"; 
        addLog("[NETWORK] Linked via Local ESP32 Hotspot. Polling matrix activated.");

        // High-speed polling loop to pull synchronized states from the hardware core
        setInterval(fetchLocalTelemetry, 200);

    } else {
        isLocalMode = false;
        addLog("[NETWORK] Linked via Router Gateway. Initializing Cloud transceiver link...");
        
        mqttClient = new Paho.MQTT.Client("broker.hivemq.com", 8000, clientID);

        mqttClient.connect({
            onSuccess: () => {
                mqttIndicator.innerText = "MQTT: CLOUD CONNECTED";
                mqttIndicator.className = "conn-tag tag-online";
                addLog(`[MQTT] Securely bound to pipeline: ${mqttTopic}`);
                mqttClient.subscribe(mqttTopic);
            },
            onFailure: (message) => {
                addLog(`[MQTT ERROR] Cloud link dropped: ${message.errorMessage}`);
            },
            useSSL: false
        });

        mqttClient.onMessageArrived = (message) => {
            try {
                const data = JSON.parse(message.payloadString);
                renderUiData(data, "MQTT WAN");
            } catch(e) {
                addLog("[MQTT ERROR] Failed parsing corrupted payload JSON.");
            }
        };

        mqttClient.onConnectionLost = (responseObject) => {
            if (responseObject.errorCode !== 0) {
                mqttIndicator.innerText = "MQTT: DISCONNECTED";
                mqttIndicator.className = "conn-tag tag-offline";
                addLog(`[MQTT ALERT] Connection severed: ${responseObject.errorMessage}`);
            }
        };
    }

    // =========================================================================
    // 📊 UNIFIED UI DATA RENDERING ENGINE
    // =========================================================================
    function renderUiData(data, sourceLabel) {
        const rawValue = data.ldr_raw;
        const systemMuted = data.is_muted;
        const lastAction = data.last_action || "";

        // 1. Update gauge numbers and progress bar lines
        ldrGauge.innerText = String(rawValue).padStart(4, '0');
        const percentage = (rawValue / 4095) * 100;
        ldrProgress.style.width = `${percentage}%`;

        // 2. Synchronize logs based on execution actions sent by the hardware core
        if (lastAction === "BREACH" && systemState.innerText !== "BREACH DETECTED") {
            systemState.innerText = "BREACH DETECTED";
            systemState.className = "state-tripped";
            ldrGauge.style.color = "#ff0055";
            ldrProgress.style.backgroundColor = "#ff0055";
            addLog(`⚠️ [${sourceLabel}] PERIMETER VIOLATION DETECTED!`);
        } 
        else if (lastAction === "RESET" && systemState.innerText !== "SECURE") {
            systemState.innerText = "SECURE";
            systemState.className = "state-safe";
            ldrGauge.style.color = "#39ff14";
            ldrProgress.style.backgroundColor = "#39ff14";
            addLog(`💚 [${sourceLabel}] System cleared. Latch structural reset verified.`);
        }
        else if (lastAction === "MUTE") {
            if (btnMute.innerText !== "MUTED") {
                btnMute.innerText = "MUTED";
                btnMute.style.borderColor = "#ffaa00";
                addLog(`🔕 [${sourceLabel}] Audio warning array silenced by remote request.`);
            }
        }

        // Revert mute button styles if backend clears the mute condition
        if (!systemMuted && btnMute.innerText === "MUTED") {
            btnMute.innerText = "MUTE SIREN";
            btnMute.style.borderColor = "";
        }
    }

    // Direct HTTP fetching routing for standalone local AP tabs
    function fetchLocalTelemetry() {
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                if(connIndicator.className !== "conn-tag tag-online") {
                    connIndicator.innerText = "HTTP: ONLINE";
                    connIndicator.className = "conn-tag tag-online";
                }
                // Process data through unified UI renderer
                renderUiData(data, "LOCAL LAN");
            })
            .catch(() => {
                connIndicator.innerText = "HTTP: OFFLINE";
                connIndicator.className = "conn-tag tag-offline";
            });
    }

    // =========================================================================
    // 🕹️ INTERACTIVE SYNCHRONIZED ACTION BUTTON HANDLERS
    // =========================================================================
    
    btnTriggerMqtt.addEventListener("click", () => {
        if (!isLocalMode && mqttClient && mqttClient.isConnected()) {
            // Router Path: Publish directly via web sockets
            const payload = JSON.stringify({ ldr_raw: 3850, is_muted: false, last_action: "BREACH" });
            const message = new Paho.MQTT.Message(payload);
            message.destinationName = mqttTopic;
            mqttClient.send(message);
        } else {
            // Local Path: Push command to ESP32 core API over local airwaves
            fetch('/api/action?cmd=sim_breach');
        }
    });

    btnReset.addEventListener("click", () => {
        if (!isLocalMode && mqttClient && mqttClient.isConnected()) {
            // Router Path: Publish directly via web sockets
            const payload = JSON.stringify({ ldr_raw: 450, is_muted: false, last_action: "RESET" });
            const message = new Paho.MQTT.Message(payload);
            message.destinationName = mqttTopic;
            mqttClient.send(message);
        } else {
            // Local Path: Push command to ESP32 core API over local airwaves
            fetch('/api/action?cmd=reset');
        }
    });

    btnMute.addEventListener("click", () => {
        if (!isLocalMode && mqttClient && mqttClient.isConnected()) {
            // Router Path: Publish directly via web sockets
            const payload = JSON.stringify({ ldr_raw: 450, is_muted: true, last_action: "MUTE" });
            const message = new Paho.MQTT.Message(payload);
            message.destinationName = mqttTopic;
            mqttClient.send(message);
        } else {
            // Local Path: Push command to ESP32 core API over local airwaves
            fetch('/api/action?cmd=mute');
        }
    });
});