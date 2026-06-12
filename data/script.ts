// =========================================================================
// 📐 STRICT TELEMETRY INTERFACES & STRUCTS
// =========================================================================
interface TelemetryData {
    ldr_raw: number;
    is_muted: boolean;
    system_mode: "NOTIFIER" | "ARMED";
    last_action: "RESET" | "BREACH" | "MUTE";
}

// Global Paho MQTT Namespace declaration map targeting CDN scripts
declare namespace Paho {
    namespace MQTT {
        class Client {
            constructor(host: string, port: number, clientId: string);
            connect(options: { onSuccess: () => void; onFailure: (err: { errorMessage: string }) => void; useSSL?: boolean }): void;
            subscribe(topic: string): void;
            send(message: Message): void;
            isConnected(): boolean;
            onMessageArrived: (message: Message) => void;
            onConnectionLost: (responseObject: { errorCode: number; errorMessage: string }) => void;
        }
        class Message {
            constructor(payload: string);
            destinationName: string;
        }
    }
}

document.addEventListener("DOMContentLoaded", () => {
    // 🎛️ DOM Element Selection Matrices with Explicit Casting
    const ldrGauge = document.getElementById("ldr-gauge") as HTMLDivElement;
    const ldrProgress = document.getElementById("ldr-progress") as HTMLDivElement;
    const systemState = document.getElementById("system-state") as HTMLSpanElement;
    const connIndicator = document.getElementById("connection-indicator") as HTMLDivElement;
    const mqttIndicator = document.getElementById("mqtt-indicator") as HTMLDivElement;
    const consoleLog = document.getElementById("console-log") as HTMLDivElement;
    
    const btnReset = document.getElementById("btn-reset") as HTMLButtonElement;
    const btnMute = document.getElementById("btn-mute") as HTMLButtonElement;
    const btnTriggerMqtt = document.getElementById("btn-trigger-mqtt") as HTMLButtonElement;
    
    // Explicitly casting the new profile operational mode switches
    const btnModeNotifier = document.getElementById("btn-mode-notifier") as HTMLButtonElement;
    const btnModeArmed = document.getElementById("btn-mode-armed") as HTMLButtonElement;

    const clientID: string = "sentinel_client_" + Math.floor(Math.random() * 10000);
    const mqttTopic: string = "sgsits/mechanical/laserSecurity/alerts"; 

    let mqttClient: Paho.MQTT.Client | null = null;
    let isLocalMode: boolean = false;

    // 🛑 STATE RUNTIME CACHE: Prevents the background heartbeat loops from flickering the UI flags
    let currentSystemState: TelemetryData = {
        ldr_raw: 450,
        is_muted: false,
        system_mode: "NOTIFIER",
        last_action: "RESET"
    };

    function addLog(message: string): void {
        const timestamp: string = new Date().toLocaleTimeString();
        const logLine: HTMLDivElement = document.createElement("div");
        logLine.className = "log-line";
        logLine.innerText = `[${timestamp}] ${message}`;
        consoleLog.appendChild(logLine);
        consoleLog.scrollTop = consoleLog.scrollHeight;
    }

    // =========================================================================
    // 🗺️ AUTOMATIC NETWORK ENVIRONMENT RESOLVER (AP vs STA Router Mode)
    // =========================================================================
    if (window.location.hostname === "192.168.4.1") {
        isLocalMode = true;
        mqttIndicator.innerText = "MQTT: BYPASSED (LOCAL AP)";
        mqttIndicator.className = "conn-tag tag-amber"; 
        addLog("[NETWORK] Linked via Local ESP32 Hotspot. Polling matrix activated.");
        setInterval(fetchLocalTelemetry, 200);
    } else {
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

        mqttClient.onMessageArrived = (message: any) => {
            try {
                const rawPayload: string = message.payloadString;
                
                // 🛡️ GLITCH GUARD 1: Safely filter out unformatted cross-talk packets from the public broker
                if (!rawPayload || !rawPayload.includes("{") || !rawPayload.includes("}")) return; 
                
                const data: TelemetryData = JSON.parse(rawPayload);
                
                // 🛡️ GLITCH GUARD 2: Ensure critical schema keys exist before modifying visual components
                if (data && typeof data.ldr_raw !== 'undefined') {
                    renderUiData(data, "MQTT WAN");
                }
            } catch(e) {
                // Silently swallow parsing errors from public broker noise streams
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
    // 📊 STABLE UNIFIED UI RENDERING ENGINE
    // =========================================================================
    function renderUiData(data: TelemetryData, sourceLabel: string): void {
        // Feed incoming state to local memory matrix tracking cache
        currentSystemState = data;

        const rawValue: number = data.ldr_raw;
        const systemMuted: boolean = data.is_muted;
        const currentMode: "NOTIFIER" | "ARMED" = data.system_mode || "NOTIFIER";
        const lastAction: "RESET" | "BREACH" | "MUTE" = data.last_action || "RESET";

        // Math padding execution optimization: Zero compilation target error issues
        ldrGauge.innerText = ("0000" + rawValue).slice(-4);
        const percentage: number = (rawValue / 4095) * 100;
        ldrProgress.style.width = `${percentage}%`;

        // Synchronize Active Operational Profile Buttons
        if (currentMode === "NOTIFIER") {
            if (btnModeNotifier) btnModeNotifier.classList.add("active-mode");
            if (btnModeArmed) btnModeArmed.classList.remove("active-mode");
        } else if (currentMode === "ARMED") {
            if (btnModeArmed) btnModeArmed.classList.add("active-mode");
            if (btnModeNotifier) btnModeNotifier.classList.remove("active-mode");
        }

        // Evaluate Alarm Actions dependent on Running Profile Matrix States
        if (lastAction === "BREACH") {
            if (currentMode === "ARMED" && systemState.innerText !== "BREACH DETECTED") {
                systemState.innerText = "BREACH DETECTED";
                systemState.className = "state-tripped";
                ldrGauge.style.color = "#ff0055";
                ldrProgress.style.backgroundColor = "#ff0055";
                addLog(`⚠️ [${sourceLabel}] SECURITY BREACH IN LOCKED LAB AREA!`);
            } else if (currentMode === "NOTIFIER" && systemState.innerText !== "ENTRY NOTICE") {
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
            // Log once, do not build infinite execution blocks
        }

        // Re-render button state label configurations strictly 
        if (systemMuted) {
            btnMute.innerText = "MUTED";
            btnMute.style.borderColor = "#ffaa00";
        } else {
            btnMute.innerText = "MUTE SIREN";
            btnMute.style.borderColor = "";
        }
    }

    function fetchLocalTelemetry(): void {
        fetch('/api/status')
            .then(response => response.json())
            .then((data: TelemetryData) => {
                if(connIndicator.className !== "conn-tag tag-online") {
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

    // =========================================================================
    // 🕹️ INTERACTIVE DISPATCH CONTROLLER LOGIC (Guarantees Full Payload Compliance)
    // =========================================================================
    function dispatchCommand(updates: Partial<TelemetryData>, localEndpoint: string): void {
        if (!isLocalMode && mqttClient && mqttClient.isConnected()) {
            // Build the complete strict payload object so C++ .indexOf parsing doesn't crash
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
        } else {
            // Fallback to local access point web endpoints
            fetch(localEndpoint);
        }
    }

    // 1. Profile Switch Handler: Notifier Chime Mode
    if (btnModeNotifier) {
        btnModeNotifier.addEventListener("click", () => {
            dispatchCommand({ system_mode: "NOTIFIER" }, '/api/action?cmd=mode_notifier');
        });
    }

    // 2. Profile Switch Handler: Armed Lockdown Mode
    if (btnModeArmed) {
        btnModeArmed.addEventListener("click", () => {
            dispatchCommand({ system_mode: "ARMED" }, '/api/action?cmd=mode_armed');
        });
    }

    // 3. Perimeter Alert Simulation Handler
    btnTriggerMqtt.addEventListener("click", () => {
        dispatchCommand({ ldr_raw: 3850, is_muted: false, last_action: "BREACH" }, '/api/action?cmd=sim_breach');
    });

    // 4. Perimeter Recovery Reset Handler
    btnReset.addEventListener("click", () => {
        dispatchCommand({ ldr_raw: 450, is_muted: false, last_action: "RESET" }, '/api/action?cmd=reset');
    });

    // 5. Sound Layer Mute Trigger Handler
    btnMute.addEventListener("click", () => {
        const nextMuteState = !currentSystemState.is_muted;
        dispatchCommand({ is_muted: nextMuteState, last_action: "MUTE" }, '/api/action?cmd=mute');
        addLog(`[CONTROL] Toggle local siren muting profile request dispatched.`);
    });
});