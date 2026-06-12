"use strict";
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
    const btnModeNotifier = document.getElementById("btn-mode-notifier");
    const btnModeArmed = document.getElementById("btn-mode-armed");
    const clientID = "sentinel_client_" + Math.floor(Math.random() * 10000);
    const mqttTopic = "sgsits/mechanical/laserSecurity/alerts";
    let mqttClient = null;
    let isLocalMode = false;
    let currentSystemState = {
        ldr_raw: 450,
        is_muted: false,
        system_mode: "NOTIFIER",
        last_action: "RESET"
    };
    function addLog(message) {
        const timestamp = new Date().toLocaleTimeString();
        const logLine = document.createElement("div");
        logLine.className = "log-line";
        logLine.innerText = `[${timestamp}] ${message}`;
        consoleLog.appendChild(logLine);
        consoleLog.scrollTop = consoleLog.scrollHeight;
    }
    if (window.location.hostname === "192.168.4.1") {
        isLocalMode = true;
        mqttIndicator.innerText = "MQTT: BYPASSED (LOCAL AP)";
        mqttIndicator.className = "conn-tag tag-amber";
        addLog("[NETWORK] Linked via Local ESP32 Hotspot. Polling matrix activated.");
        setInterval(fetchLocalTelemetry, 200);
    }
    else {
        isLocalMode = false;
        addLog("[NETWORK] Linked via Router Gateway. Initializing Cloud transceiver link...");
        mqttClient = new Paho.MQTT.Client("broker.hivemq.com", 8000, clientID);
        mqttClient.connect({
            onSuccess: () => {
                if (mqttClient) {
                    mqttIndicator.innerText = "MQTT: CLOUD CONNECTED";
                    mqttIndicator.className = "conn-tag tag-online";
                    addLog(`[MQTT] Securely bound to pipeline: ${mqttTopic}`);
                    mqttClient.subscribe(mqttTopic);
                }
            },
            onFailure: (message) => {
                addLog(`[MQTT ERROR] Cloud link dropped: ${message.errorMessage}`);
            },
            useSSL: false
        });
        mqttClient.onMessageArrived = (message) => {
            try {
                const rawPayload = message.payloadString;
                if (!rawPayload || !rawPayload.includes("{") || !rawPayload.includes("}"))
                    return;
                const data = JSON.parse(rawPayload);
                if (data && typeof data.ldr_raw !== 'undefined') {
                    renderUiData(data, "MQTT WAN");
                }
            }
            catch (e) {
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
    function renderUiData(data, sourceLabel) {
        currentSystemState = data;
        const rawValue = data.ldr_raw;
        const systemMuted = data.is_muted;
        const currentMode = data.system_mode || "NOTIFIER";
        const lastAction = data.last_action || "RESET";
        ldrGauge.innerText = ("0000" + rawValue).slice(-4);
        const percentage = (rawValue / 4095) * 100;
        ldrProgress.style.width = `${percentage}%`;
        if (currentMode === "NOTIFIER") {
            if (btnModeNotifier)
                btnModeNotifier.classList.add("active-mode");
            if (btnModeArmed)
                btnModeArmed.classList.remove("active-mode");
        }
        else if (currentMode === "ARMED") {
            if (btnModeArmed)
                btnModeArmed.classList.add("active-mode");
            if (btnModeNotifier)
                btnModeNotifier.classList.remove("active-mode");
        }
        if (lastAction === "BREACH") {
            if (currentMode === "ARMED" && systemState.innerText !== "BREACH DETECTED") {
                systemState.innerText = "BREACH DETECTED";
                systemState.className = "state-tripped";
                ldrGauge.style.color = "#ff0055";
                ldrProgress.style.backgroundColor = "#ff0055";
                addLog(`⚠️ [${sourceLabel}] SECURITY BREACH IN LOCKED LAB AREA!`);
            }
            else if (currentMode === "NOTIFIER" && systemState.innerText !== "ENTRY NOTICE") {
                systemState.innerText = "ENTRY NOTICE";
                systemState.className = "state-warn";
                ldrGauge.style.color = "#ffaa00";
                ldrProgress.style.backgroundColor = "#ffaa00";
                addLog(`🔔 [${sourceLabel}] Proximity Entry Notice registered.`);
            }
        }
        else if (lastAction === "RESET" && systemState.innerText !== "SECURE") {
            systemState.innerText = "SECURE";
            systemState.className = "state-safe";
            ldrGauge.style.color = "#39ff14";
            ldrProgress.style.backgroundColor = "#39ff14";
            addLog(`💚 [${sourceLabel}] System cleared. Perimeter structural latch secure.`);
        }
        else if (lastAction === "MUTE" && systemState.innerText !== "MUTED TRACK") {
        }
        if (systemMuted) {
            btnMute.innerText = "MUTED";
            btnMute.style.borderColor = "#ffaa00";
        }
        else {
            btnMute.innerText = "MUTE SIREN";
            btnMute.style.borderColor = "";
        }
    }
    function fetchLocalTelemetry() {
        fetch('/api/status')
            .then(response => response.json())
            .then((data) => {
            if (connIndicator.className !== "conn-tag tag-online") {
                connIndicator.innerText = "HTTP: ONLINE";
                connIndicator.className = "conn-tag tag-online";
            }
            renderUiData(data, "LOCAL LAN");
        })
            .catch(() => {
            connIndicator.innerText = "HTTP: OFFLINE";
            connIndicator.className = "conn-tag tag-offline";
        });
    }
    function dispatchCommand(updates, localEndpoint) {
        if (!isLocalMode && mqttClient && mqttClient.isConnected()) {
            const compiledPayload = JSON.stringify({
                ldr_raw: updates.ldr_raw !== undefined ? updates.ldr_raw : currentSystemState.ldr_raw,
                is_muted: updates.is_muted !== undefined ? updates.is_muted : currentSystemState.is_muted,
                system_mode: updates.system_mode !== undefined ? updates.system_mode : currentSystemState.system_mode,
                last_action: updates.last_action !== undefined ? updates.last_action : currentSystemState.last_action
            });
            const message = new Paho.MQTT.Message(compiledPayload);
            message.destinationName = mqttTopic;
            mqttClient.send(message);
            addLog(`[TX WAN] Outbound update package sent over broker gateway...`);
        }
        else {
            fetch(localEndpoint);
        }
    }
    if (btnModeNotifier) {
        btnModeNotifier.addEventListener("click", () => {
            dispatchCommand({ system_mode: "NOTIFIER" }, '/api/action?cmd=mode_notifier');
        });
    }
    if (btnModeArmed) {
        btnModeArmed.addEventListener("click", () => {
            dispatchCommand({ system_mode: "ARMED" }, '/api/action?cmd=mode_armed');
        });
    }
    btnTriggerMqtt.addEventListener("click", () => {
        dispatchCommand({ ldr_raw: 3850, is_muted: false, last_action: "BREACH" }, '/api/action?cmd=sim_breach');
    });
    btnReset.addEventListener("click", () => {
        dispatchCommand({ ldr_raw: 450, is_muted: false, last_action: "RESET" }, '/api/action?cmd=reset');
    });
    btnMute.addEventListener("click", () => {
        const nextMuteState = !currentSystemState.is_muted;
        dispatchCommand({ is_muted: nextMuteState, last_action: "MUTE" }, '/api/action?cmd=mute');
        addLog(`[CONTROL] Toggle local siren muting profile request dispatched.`);
    });
});
