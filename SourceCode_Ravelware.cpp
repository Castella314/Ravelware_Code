/* Created by Ravelware x Sigproc Brawijaya */

#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

/* ================= Mapping PIN Sensor================= */
#define RELAY1_DHT     13
#define RELAY2_LDR     14
#define RELAY3_BUZZER  27
#define RELAY4_USONIC  26

#define DHTPIN   25
#define DHTTYPE  DHT22
#define LDRPIN   33

#define TRIGPIN  18
#define ECHOPIN  19

#define BUZZERPIN 23

#define RELAY_ACTIVE_LOW true

/* ================= Variabel Sensor ================= */
float threshold_cm = 50.0;
float jarak = 0;

WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

/* ================= Relay ================= */
void relayOn(int pin) {
  digitalWrite(pin, RELAY_ACTIVE_LOW ? LOW : HIGH);
}

void relayOff(int pin) {
  digitalWrite(pin, RELAY_ACTIVE_LOW ? HIGH : LOW);
}

/* ================= Ultrasonic ================= */
float readUltrasonic() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  long dur = pulseIn(ECHOPIN, HIGH, 30000);
  if (dur == 0) return -1;
  return dur * 0.0343 / 2.0;
}

/* ================= Buzzer Logic ================= */
void updateBuzzer(float d, bool rUsonic, bool rBuzzer) {
  if (rUsonic && rBuzzer && d <= threshold_cm) digitalWrite(BUZZERPIN, HIGH);
  else digitalWrite(BUZZERPIN, LOW);
}

/* ================= HTML ================= */
String page() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Relay Control Dashboard</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        :root {
            --primary: #2563eb;
            --success: #10b981;
            --danger: #ef4444;
            --warning: #f59e0b;
            --dark: #0f172a;
            --darker: #020617;
            --light: #f8fafc;
            --gray: #64748b;
            --card-bg: #1e293b;
            --border: #334155;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', system-ui, sans-serif;
        }

        body {
            background: linear-gradient(135deg, var(--darker) 0%, var(--dark) 100%);
            color: var(--light);
            min-height: 100vh;
            padding: 20px;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
            position: relative;
        }

        .logo-container {
            display: flex;  
            background-color: var(--light);
            width: 280px;
            border-radius: 16px;
        }

        .logo {
            height: 50px;
            padding: 5px;
            padding-left: 10px;
        }

        .logo:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
        }

        header {
            text-align: center;
            padding: 2rem;
            background: linear-gradient(135deg,#1e293b 0%, #0f172a 100%);
            border-radius: 16px;
            margin-bottom: 2rem;
            border: 1px solid var(--border);
            box-shadow: 0 10px 25px rgba(0, 0, 0, 0.3);
            position: relative;
            padding-top: 4rem;
        }

        header h1 {
            font-size: 2.5rem;
            background: linear-gradient(90deg, var(--primary), #8b5cf6);
            -webkit-background-clip: text;
            background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 0.5rem;
        }

        header .subtitle {
            color: var(--gray);
            font-size: 1.1rem;
            margin-bottom: 1.5rem;
        }

        .relay-info {
            background: rgba(30, 41, 59, 0.8);
            padding: 1.5rem;
            border-radius: 10px;
            margin-top: 1rem;
            border-left: 4px solid var(--warning);
        }

        .relay-info h3 {
            margin-bottom: 1rem;
            color: var(--warning);
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .relay-info h4 {
            margin-top: 1.5rem;
            margin-bottom: 0.8rem;
            color: var(--primary);
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .relay-channels {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
            gap: 1rem;
            margin-bottom: 1.5rem;
        }

        .channel {
            background: rgba(0, 0, 0, 0.2);
            padding: 1rem;
            border-radius: 8px;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 0.5rem;
            transition: all 0.3s ease;
            border: 1px solid rgba(59, 130, 246, 0.3);
        }

        .channel:hover {
            background: rgba(0, 0, 0, 0.4);
            transform: translateY(-2px);
            border-color: var(--primary);
        }

        .channel-number {
            background: var(--primary);
            color: white;
            padding: 0.3rem 0.8rem;
            border-radius: 20px;
            font-weight: bold;
            font-size: 0.9rem;
            transition: all 0.3s ease;
        }

        .channel-number.active {
            background: var(--success);
            box-shadow: 0 0 15px rgba(16, 185, 129, 0.4);
            transform: scale(1.05);
        }

        .channel-device {
            font-weight: 600;
            color: var(--light);
            text-align: center;
        }

        .channel-pin {
            color: var(--warning);
            font-size: 0.85rem;
            font-family: 'Courier New', monospace;
            background: rgba(245, 158, 11, 0.1);
            padding: 0.2rem 0.5rem;
            border-radius: 4px;
        }

        .sensor-pins {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 0.8rem;
            margin-top: 0.5rem;
        }

        .pin-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0.6rem;
            background: rgba(30, 41, 59, 0.5);
            border-radius: 6px;
            border-left: 3px solid var(--primary);
        }

        .pin-label {
            color: var(--gray);
            font-size: 0.9rem;
        }

        .pin-number {
            color: #60a5fa;
            font-family: 'Courier New', monospace;
            font-size: 0.85rem;
            background: rgba(59, 130, 246, 0.1);
            padding: 0.2rem 0.5rem;
            border-radius: 4px;
        }

        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 1.5rem;
            margin-bottom: 2rem;
        }

        .card {
            background: linear-gradient(145deg, var(--card-bg) 0%, #111827 100%);
            border-radius: 16px;
            padding: 1.5rem;
            border: 1px solid var(--border);
            transition: all 0.3s ease;
            display: flex;
            flex-direction: column;
            box-shadow: 0 8px 20px rgba(0, 0, 0, 0.2);
        }

        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 15px 30px rgba(0, 0, 0, 0.3);
            border-color: var(--primary);
        }

        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
            padding-bottom: 1rem;
            border-bottom: 1px solid var(--border);
        }

        .card-title {
            display: flex;
            align-items: center;
            gap: 0.75rem;
            font-size: 1.4rem;
            font-weight: 600;
            flex-wrap: wrap;
        }

        .sensor-channel {
            display: flex;
            flex-direction: column;
            align-items: flex-start;
            gap: 5px;
        }

        .card-title i {
            font-size: 1.6rem;
        }

        .relay-indicator {
            color: #60a5fa;
            border-radius: 20px;
            font-size: 0.8rem;
        }

        .status-badge {
            padding: 0.4rem 1rem;
            border-radius: 20px;
            font-size: 0.85rem;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .status-on {
            background: rgba(16, 185, 129, 0.2);
            color: var(--success);
            border: 1px solid var(--success);
        }

        .status-off {
            background: rgba(239, 68, 68, 0.2);
            color: var(--danger);
            border: 1px solid var(--danger);
        }

        .sensor-data {
            flex-grow: 1;
            margin-bottom: 1.5rem;
        }

        #ldr-value {
            margin: 8pt 0%;
        }

        #dht-bottom, #ldr-bottom {
            margin-bottom: 0;
            height: 160px;
        }

        .data-item {
            margin-bottom: 1.25rem;
            padding: 1rem;
            background: rgba(30, 41, 59, 0.5);
            border-radius: 10px;
            transition: all 0.3s ease;
        }

        .data-item:hover {
            background: rgba(30, 41, 59, 0.8);
        }

        .data-label {
            display:flex;
            justify-content: space-between;
            width: 100%;
            gap: 0.5rem;
            color: var(--gray);
            margin-bottom: 0.5rem;
            font-size: 0.95rem;
            flex-wrap: wrap;
        }

        .pin-label-power, .pin-label-data {
            background: rgba(59, 130, 246, 0.1);
            color: var(--warning);
            padding: 0.2rem 0.5rem;
            border-radius: 4px;
            font-size: 0.75rem;
            font-family: 'Courier New', monospace;
            margin-left: auto;
        }

        .pin-label-data{
            color: #60a5fa;; 
        }

        .pin-label-small-group{
            display:grid;
            gap:2px
        }

        .data-value {
            font-size: 2rem;
            font-weight: 700;
            color: var(--light);
            display: block;
        }

        .data-unit {
            font-size: 1rem;
            color: var(--gray);
            margin-left: 0.25rem;
        }

        .sensor-raw {
            margin-top: 0.5rem;
            font-size: 0.85rem;
            color: var(--gray);
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .sensor-raw small {
            display: flex;
            align-items: center;
            gap: 0.25rem;
        }

        .raw-value {
            font-family: 'Courier New', monospace;
            color: var(--warning);
            background: rgba(245, 158, 11, 0.1);
            padding: 0.1rem 0.3rem;
            border-radius: 3px;
        }

        .relay-status {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            margin-bottom: 0.8rem;
            padding: 0.5rem 0.945rem 0.5rem 0.9rem;
            background: rgba(0, 0, 0, 0.1);
            border-radius: 6px;
            font-size: 0.9rem;
            flex-wrap: wrap;
        }

        .gpio-info-small {
            background: rgba(245, 158, 11, 0.1);
            color: var(--warning);
            padding: 0.2rem 0.5rem;
            border-radius: 4px;
            font-size: 0.75rem;
            font-family: 'Courier New', monospace;
            margin-left: auto;
        }

        .status-text {
            font-size: 1.2rem;
            font-weight: 600;
            padding: 0.5rem 1rem;
            border-radius: 8px;
            display: inline-block;
            margin: 0.5rem 0;
        }

        .status-terang {
            color: white;
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid white;
        }

        .status-sedang {
            color: var(--warning);
            background: rgba(11, 190, 53, 0.1);
            border: 1px solid var(--warning);
        }

        .status-gelap {
            color: #a78bfa;
            background: rgba(168, 85, 247, 0.1);
            border: 1px solid #a78bfa;
        }

        .status-offline {
            color: var(--gray);
            background: rgba(100, 116, 139, 0.1);
            border: 1px solid var(--gray);
        }

        .distance-warning {
            background: rgba(245, 158, 11, 0.1);
            border: 1px solid var(--warning);
            color: #fcd34d;
            padding: 0.8rem;
            border-radius: 8px;
            margin-top: 1rem;
            display: flex;
            align-items: center;
            gap: 0.5rem;
            flex-wrap: wrap;
        }

        .distance-warning input {
            background: rgba(0, 0, 0, 0.3);
            border: 1px solid var(--border);
            color: var(--light);
            padding: 0.3rem 0.5rem;
            border-radius: 4px;
            width: 60px;
            text-align: center;
        }

        .btn-small {
            background: var(--primary);
            color: white;
            border: none;
            padding: 0.3rem 0.8rem;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.8rem;
            margin-left: auto;
        }

        .btn-small:hover {
            background: #1d4ed8;
        }

        .buzzer-visual {
            text-align: center;
            padding: 1rem;
            background: rgba(0, 0, 0, 0.2);
            border-radius: 12px;
            margin: 1rem 0;
        }

        .buzzer-icon {
            font-size: 3rem;
            margin-bottom: 0.5rem;
            color: var(--gray);
        }

        .buzzer-icon.active {
            color: var(--warning);
            animation: pulse 1.5s infinite;
        }

        .buzzer-logic {
            font-size: 0.85rem;
            color: var(--gray);
            line-height: 1.4;
            text-align: left;
            padding: 0.5rem;
            background: rgba(0, 0, 0, 0.1);
            border-radius: 6px;
            margin-top: 0.5rem;
        }

        .controls {
            display: flex;
            gap: 1rem;
        }

        .btn {
            flex: 1;
            padding: 0.9rem 1.5rem;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 0.5rem;
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-on {
            background: linear-gradient(135deg, var(--success), #059669);
            color: white;
        }

        .btn-on:hover {
            background: linear-gradient(135deg, #0da271, #047857);
        }

        .btn-off {
            background: linear-gradient(135deg, var(--danger), #dc2626);
            color: white;
        }

        .btn-off:hover {
            background: linear-gradient(135deg, #dc2626, #b91c1c);
        }

        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none !important;
        }

        .connection-status {
            display: flex;
            justify-content: space-around;
            padding: 1rem;
            border-radius: 10px;
        }

        .connection-item {
            display: flex;
            align-items: center;
            gap: 0.5rem;
            font-size: 0.9rem;
        }

        .connection-item i {
            color: var(--primary);
        }

        .server-status {
            position: fixed;
            bottom: 20px;
            right: 20px;
            padding: 0.75rem 1.5rem;
            border-radius: 25px;
            font-weight: 600;
            display: flex;
            align-items: center;
            gap: 0.5rem;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
            z-index: 1000;
        }

        .server-online {
            background: var(--success);
            color: white;
        }

        .server-offline {
            background: var(--danger);
            color: white;
        }

        footer {
            text-align: center;
            padding: 2rem;
            color: var(--gray);
            border-top: 1px solid var(--border);
        }

        footer code {
            background: rgba(0, 0, 0, 0.3);
            padding: 0.2rem 0.5rem;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            color: var(--warning);
        }

        .connection-test-btn {
            position: fixed;
            top: 20px;
            right: 20px;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 1001;
            transition: all 0.3s ease;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
        }

        .connection-test-btn:hover {
            background: #1d4ed8;
            transform: scale(1.1);
        }

        .connection-test-panel {
            position: fixed;
            top: 70px;
            right: 20px;
            background: var(--card-bg);
            padding: 15px;
            border-radius: 10px;
            border: 2px solid var(--border);
            z-index: 1000;
            width: 300px;
            display: none;
            box-shadow: 0 8px 24px rgba(0, 0, 0, 0.3);
        }

        .connection-test-panel.show {
            display: block;
        }

        .connection-test-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
            padding-bottom: 10px;
            border-bottom: 1px solid var(--border);
        }

        .connection-test-header h4 {
            color: #60a5fa;
            margin: 0;
        }

        .close-btn {
            background: none;
            border: none;
            color: var(--gray);
            cursor: pointer;
            font-size: 1.2rem;
            padding: 0;
            width: 24px;
            height: 24px;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 4px;
        }

        .close-btn:hover {
            background: rgba(255, 255, 255, 0.1);
            color: var(--light);
        }

        .test-buttons {
            display: flex;
            gap: 10px;
            margin-bottom: 10px;
        }

        .test-btn {
            flex: 1;
            padding: 8px 12px;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 0.9rem;
            transition: background 0.3s ease;
        }

        .test-btn:hover {
            background: #1d4ed8;
        }

        .test-result {
            font-size: 0.85rem;
            color: var(--gray);
            min-height: 40px;
            padding: 8px;
            background: rgba(0, 0, 0, 0.2);
            border-radius: 5px;
            word-break: break-word;
        }

        @keyframes pulse {
            0%, 100% { transform: scale(1); opacity: 1; }
            50% { transform: scale(1.1); opacity: 0.8; }
        }

        @media (max-width: 768px) {
            .logo-container {
                justify-content: center;
                margin-bottom: 1rem;
            }
            
            header {
                padding-top: 2rem;
            }
            
            .dashboard-grid {
                grid-template-columns: 1fr;
            }
            
            header h1 {
                font-size: 2rem;
            }
            
            .relay-channels, .sensor-pins {
                grid-template-columns: 1fr;
            }
            
            .connection-status {
                flex-direction: column;
                gap: 1rem;
            }
            
            .controls {
                flex-direction: column;
            }
            
            .server-status {
                position: static;
                margin-top: 1rem;
                width: 100%;
                justify-content: center;
            }
            
            .connection-test-panel {
                width: calc(100% - 40px);
                right: 20px;
                left: 20px;
            }
        }
    </style>
</head>

<body>
    <div class="container">
        <!-- Tombol Tes Konesksi -->
        <button class="connection-test-btn" id="connectionTestBtn" title="Test Koneksi">
            <i class="fas fa-ellipsis-v"></i>
        </button>

        <!-- Panel Tes Konesksi -->
        <div class="connection-test-panel" id="connectionTestPanel">
            <div class="connection-test-header">
                <h4>Test Koneksi</h4>
                <button class="close-btn" id="closeTestPanel" title="Tutup">
                    <i class="fas fa-times"></i>
                </button>
            </div>
            <div class="test-buttons">
                <button class="test-btn" onclick="testConnection('data')">Test data</button>
                <button class="test-btn" onclick="testConnection('r1on')">Test Relay</button>
            </div>
            <div class="test-result" id="test-result">Klik tombol untuk melakukan test</div>
        </div>

        <header>
            <h1>IoT Control Dashboard</h1>
            <p class="subtitle">ESP32 + 4-Channel Relay ACTIVE LOW</p>
            
            <div class="relay-info">
                <h3>Relay Pin Mapping:</h3>
                <div class="relay-channels">
                    <div class="channel">
                        <span class="channel-number" id="ch1-indicator">CH1</span>
                        <span class="channel-device">DHT22 Power</span>
                        <span class="channel-pin">GPIO 13</span>
                    </div>
                    <div class="channel">
                        <span class="channel-number" id="ch2-indicator">CH2</span>
                        <span class="channel-device">LDR Power</span>
                        <span class="channel-pin">GPIO 14</span>
                    </div>
                    <div class="channel">
                        <span class="channel-number" id="ch3-indicator">CH3</span>
                        <span class="channel-device">Buzzer Control</span>
                        <span class="channel-pin">GPIO 27</span>
                    </div>
                    <div class="channel">
                        <span class="channel-number" id="ch4-indicator">CH4</span>
                        <span class="channel-device">HC-SR04 Power</span>
                        <span class="channel-pin">GPIO 26</span>
                    </div>
                </div>
                
                <div class="sensor-info">
                    <h4><i class="fas fa-sensor"></i> Sensor Pin Mapping:</h4>
                    <div class="sensor-pins">
                        <div class="pin-item">
                            <span class="pin-label">DHT22 Data:</span>
                            <span class="pin-number">GPIO 25</span>
                        </div>
                        <div class="pin-item">
                            <span class="pin-label">LDR Analog:</span>
                            <span class="pin-number">GPIO 34 (ADC1_CH6)</span>
                        </div>
                        <div class="pin-item">
                            <span class="pin-label">Buzzer Direct:</span>
                            <span class="pin-number">GPIO 23</span>
                        </div>
                        <div class="pin-item">
                            <span class="pin-label">HC-SR04 Trig:</span>
                            <span class="pin-number">GPIO 18</span>
                        </div>
                        <div class="pin-item">
                            <span class="pin-label">HC-SR04 Echo:</span>
                            <span class="pin-number">GPIO 19</span>
                        </div>
                    </div>
                </div>
            </div>
        </header>

        <div class="dashboard-grid">
            <!-- DHT22 Card -->
            <div class="card" id="dht-card">
                <div class="card-header">
                    <div class="card-title">
                        <i class="fas fa-thermometer-half"></i>
                        <div class="sensor-channel">
                            <span>DHT22</span>
                            <span class="relay-indicator" id="dht-relay-indicator">CH1</span>
                        </div>
                    </div>
                    <div class="status-badge status-off" id="dht-status">RELAY OFF</div>
                </div>
                
                <div class="sensor-data">
                    <div class="data-item">
                        <div class="data-label">
                            <span>Suhu</span>
                            <div class="pin-label-small-group">
                                <span class="pin-label-power">GPIO 13</span>
                                <span class="pin-label-data">GPIO 25</span>
                            </div>
                        </div>
                        <span class="data-value" id="temperature">--</span>
                        <span class="data-unit">°C</span>
                    </div>
                    
                    <div class="data-item" id="dht-bottom">
                        <div class="data-label">
                            <span>Kelembapan</span>
                        </div>
                        <span class="data-value" id="humidity">--</span>
                        <span class="data-unit">%</span>
                    </div>
                </div>
                
                <div class="controls">
                    <button class="btn btn-on" onclick="controlRelay('dht', 'on')">
                        <i class="fas fa-toggle-on"></i> POWER ON
                    </button>
                    <button class="btn btn-off" onclick="controlRelay('dht', 'off')">
                        <i class="fas fa-toggle-off"></i> POWER OFF
                    </button>
                </div>
            </div>

            <!-- LDR Card -->
            <div class="card" id="ldr-card">
                <div class="card-header">
                    <div class="card-title">
                        <i class="fas fa-sun"></i>
                        <div class="sensor-channel">
                            <span>LDR</span>
                            <span class="relay-indicator" id="ldr-relay-indicator">CH2</span>
                        </div>
                    </div>
                    <div class="status-badge status-off" id="ldr-status">RELAY OFF</div>
                </div>
                
                <div class="sensor-data">
                    <div class="data-item">
                        <div class="data-label">
                            <span>Status Cahaya</span>
                            <div class="pin-label-small-group">
                                <span class="pin-label-power">GPIO 14</span>
                                <span class="pin-label-data">GPIO 34/ADC1_CH6</span>
                            </div>

                        </div>
                        <div class="status-text status-offline" id="ldr-value">OFFLINE</div>
                    </div>
                    
                    <div class="data-item" id="ldr-bottom">
                        <div class="data-label">
                            <span>Intensitas Cahaya</span>
                        </div>
                        <span class="data-value" id="ldr-lux">--</span>
                        <span class="data-unit">lux</span>
                        <div class="sensor-raw">
                            <small>Raw ADC: <span class="raw-value" id="ldr-raw">--</span> (0-4095)</small>
                        </div>
                    </div>
                </div>
                
                <div class="controls">
                    <button class="btn btn-on" onclick="controlRelay('ldr', 'on')">
                        <i class="fas fa-toggle-on"></i> POWER ON
                    </button>
                    <button class="btn btn-off" onclick="controlRelay('ldr', 'off')">
                        <i class="fas fa-toggle-off"></i> POWER OFF
                    </button>
                </div>
            </div>

            <!-- Buzzer Card -->
            <div class="card" id="buzzer-card">
                <div class="card-header">
                    <div class="card-title">
                        <i class="fas fa-volume-up"></i>
                        <div class="sensor-channel">
                            <span>Buzzer</span>
                            <span class="relay-indicator" id="buzzer-relay-indicator">CH3</span>
                        </div>
                    </div>
                    <div class="status-badge status-off" id="buzzer-status">RELAY OFF</div>
                </div>
                
                <div class="sensor-data">
                    <div class="data-item">
                        <div class="data-label">
                            <span>Status Buzzer</span>
                            <div class="pin-label-small-group">
                                <span class="pin-label-power">GPIO 27</span>
                                <span class="pin-label-data">GPIO 23</span>
                            </div>

                        </div>
                        <div class="buzzer-visual">
                            <div class="buzzer-icon" id="buzzer-icon">
                                <i class="fas fa-volume-mute"></i>
                            </div>
                        </div>
                    </div>
                </div>
                
                <div class="controls">
                    <button class="btn btn-on" onclick="controlRelay('buzzer', 'on')">
                        <i class="fas fa-toggle-on"></i> RELAY ON
                    </button>
                    <button class="btn btn-off" onclick="controlRelay('buzzer', 'off')">
                        <i class="fas fa-toggle-off"></i> RELAY OFF
                    </button>
                </div>
            </div>

            <!-- HC-SR04 Card -->
            <div class="card" id="ultrasonic-card">
                <div class="card-header">
                    <div class="card-title">
                        <i class="fas fa-ruler-vertical"></i>
                        <div class="sensor-channel">
                            <span>HC-SR04</span>
                            <span class="relay-indicator" id="ultrasonic-relay-indicator">CH4</span>
                        </div>
                    </div>
                    <div class="status-badge status-off" id="ultrasonic-status">RELAY OFF</div>
                </div>
                
                <div class="sensor-data">
                    <div class="data-item">
                        <div class="data-label">
                            <span>Jarak</span>
                            <div class="pin-label-small-group">
                                <span class="pin-label-power">GPIO 26</span>
                                <span class="pin-label-data">GPIO 18/19</span>
                            </div>
                            
                        </div>
                        <span class="data-value" id="distance">--</span>
                        <span class="data-unit">cm</span>
                    </div>
                    
                    <div class="distance-warning" id="distance-warning">
                        <i class="fas fa-info-circle"></i> Threshold: 
                        <input type="number" id="threshold" value="50" min="2" max="400"> cm
                        <button class="btn-small" onclick="setThreshold()">Set</button>
                    </div>
                </div>
                
                <div class="controls">
                    <button class="btn btn-on" onclick="controlRelay('ultrasonic', 'on')">
                        <i class="fas fa-toggle-on"></i> POWER ON
                    </button>
                    <button class="btn btn-off" onclick="controlRelay('ultrasonic', 'off')">
                        <i class="fas fa-toggle-off"></i> POWER OFF
                    </button>
                </div>
            </div>
        </div>



        <div class="connection-status">
            <div class="connection-item" id="last-update">
                <i class="fas fa-clock"></i>
                <span>Terakhir update: <b>--:--:--</b></span>
            </div>
        </div>

        <div class="server-status server-online" id="server-status">
            <i class="fas fa-server"></i>
            <span>Mode: REAL ESP32 CONNECTED</span>
        </div>
    </div>

    <footer>
        <p>Ravelware × Sigproc Brawijaya | ESP32 Relay Control System</p>
        <p>WiFi: <code>TRAINER_LOGIKA</code> | IP ESP32: <code id="esp32-ip">192.168.4.1</code> | Password: <code>12345678</code></p>
    </footer>
    
    <script>
        // Konfigurasi
        const ESP32_IP = '192.168.4.1';
        let isConnected = false;
        let updateInterval = null;

        // Map endpoint untuk relay
        const ENDPOINT_MAP = {
            dht: { on: '/r1on', off: '/r1off' },
            ldr: { on: '/r2on', off: '/r2off' },
            buzzer: { on: '/r3on', off: '/r3off' },
            ultrasonic: { on: '/r4on', off: '/r4off' }
        };

        // Map channel indicator
        const CHANNEL_MAP = {
            dht: 'ch1-indicator',
            ldr: 'ch2-indicator',
            buzzer: 'ch3-indicator',
            ultrasonic: 'ch4-indicator'
        };

        // DOM Elements
        const serverStatusEl = document.getElementById('server-status');
        const connectionTestPanel = document.getElementById('connectionTestPanel');
        const connectionTestBtn = document.getElementById('connectionTestBtn');
        const closeTestPanel = document.getElementById('closeTestPanel');

        // Evaluasi status buzzer
        function evaluateBuzzerStatus() {
            const buzzerRelayStatus = document.getElementById('buzzer-status').textContent.includes('ON');
            const ultrasonicRelayStatus = document.getElementById('ultrasonic-status').textContent.includes('ON');
            const distanceText = document.getElementById('distance').textContent;
            const threshold = parseFloat(document.getElementById('threshold').value) || 50;
    
            const buzzerIcon = document.getElementById('buzzer-icon');
            const buzzerIconI = buzzerIcon.querySelector('i');
    
            // Cek apakah distance bukan "--" dan merupakan angka yang valid
            const isValidDistance = distanceText !== '--' && !isNaN(parseFloat(distanceText)) && parseFloat(distanceText) > 0;
            const isObjectClose = isValidDistance && parseFloat(distanceText) <= threshold;
    
            if (buzzerRelayStatus && ultrasonicRelayStatus && isValidDistance && isObjectClose) {
                buzzerIcon.classList.add('active');
                buzzerIconI.className = 'fas fa-volume-up';
            } else {
                buzzerIcon.classList.remove('active');
                buzzerIconI.className = 'fas fa-volume-mute';
            }
        }

        // Periksa koneksi jaringan
        async function checkNetwork() {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 2000);
                
                const response = await fetch(`http://${ESP32_IP}/data`, {
                    signal: controller.signal
                });
                
                clearTimeout(timeoutId);
                
                if (response.ok) {
                    isConnected = true;
                    serverStatusEl.className = 'server-status server-online';
                    serverStatusEl.innerHTML = '<i class="fas fa-wifi"></i><span>Terhubung ke ESP32</span>';
                    return true;
                }
            } catch (error) {
                console.log("Network check failed:", error.message);
            }
            
            isConnected = false;
            serverStatusEl.className = 'server-status server-offline';
            serverStatusEl.innerHTML = '<i class="fas fa-exclamation-triangle"></i><span>Tidak terhubung ke ESP32</span>';
            
            return false;
        }

        // Ambil data sensor
        async function fetchSensorData() {
            try {
                const response = await fetch(`http://${ESP32_IP}/data`);
                if (!response.ok) throw new Error(`HTTP ${response.status}`);
    
                const data = await response.json();
                console.log("Received data from ESP32:", data);
    
                updateSensorUI(data);
                return data;
    
            } catch (error) {
                console.error("Error fetching sensor data:", error);
                isConnected = false;
                serverStatusEl.className = 'server-status server-offline';
                serverStatusEl.innerHTML = '<i class="fas fa-exclamation-triangle"></i><span>Gagal mengambil data</span>';
                return null;
            }
        }

        // Update UI dengan data aktual
        function updateSensorUI(data) {
            // Update status relay terlebih dahulu
            if (data.r1 !== undefined) updateRelayStatus('dht', data.r1 == 1);
            if (data.r2 !== undefined) updateRelayStatus('ldr', data.r2 == 1);
            if (data.r3 !== undefined) updateRelayStatus('buzzer', data.r3 == 1);
            if (data.r4 !== undefined) updateRelayStatus('ultrasonic', data.r4 == 1);

            // Update suhu dan kelembapan
            const temp = parseFloat(data.t);
            const hum = parseFloat(data.h);
            const dhtRelayOn = data.r1 == 1;
    
            if (dhtRelayOn && !isNaN(temp) && temp > -100) {
                document.getElementById('temperature').textContent = temp.toFixed(1);
            } else {
                document.getElementById('temperature').textContent = '--';
            }
    
            if (dhtRelayOn && !isNaN(hum) && hum > -100) {
                document.getElementById('humidity').textContent = hum.toFixed(1);
            } else {
                document.getElementById('humidity').textContent = '--';
            }

            // Update data LDR
            const ldrADC = parseInt(data.l);
            const luxValue = parseFloat(data.lux);
            const ldrRelayOn = data.r2 == 1;
    
            if (ldrRelayOn) {
                document.getElementById('ldr-raw').textContent = 
                    (isNaN(ldrADC) || ldrADC < 0) ? '--' : ldrADC;
        
                if (!isNaN(luxValue) && luxValue >= 0) {
                    document.getElementById('ldr-lux').textContent = luxValue.toFixed(1);
                    updateLDRStatus(ldrADC, luxValue);
                } else {
                    document.getElementById('ldr-lux').textContent = '--';
                    // Tampilkan "--" jika nilai lux invalid
                    const ldrValueEl = document.getElementById('ldr-value');
                    ldrValueEl.textContent = '--';
                    ldrValueEl.className = 'status-text';
                }
            } else {
                document.getElementById('ldr-raw').textContent = '--';
                document.getElementById('ldr-lux').textContent = '--';
                const ldrValueEl = document.getElementById('ldr-value');
                ldrValueEl.textContent = 'OFFLINE';
                ldrValueEl.className = 'status-text status-offline';
            }

            // Update jarak
            const dist = parseFloat(data.d);
            const ultrasonicRelayOn = data.r4 == 1;
    
            if (ultrasonicRelayOn && !isNaN(dist) && dist > 0 && dist <= 400) {
                document.getElementById('distance').textContent = dist.toFixed(1);
            } else {
                document.getElementById('distance').textContent = '--';
            }

            // Update indikator channel
            updateChannelIndicators(data);
            // Evaluasi status buzzer
            evaluateBuzzerStatus();
            // Update timestamp
            updateTimestamp();
        }

        // Update status LDR
        function updateLDRStatus(ldrADC, luxValue) {
            const ldrValueEl = document.getElementById('ldr-value');

            if (!isNaN(luxValue) && luxValue > 0) {
                let ldrStatus;
                let statusClass;
        
                if (luxValue < 80) {
                    ldrStatus = 'GELAP';
                    statusClass = 'status-gelap';
                } else if (luxValue < 150) {
                    ldrStatus = 'SEDANG';
                    statusClass = 'status-sedang';
                } else if (luxValue < 500) {
                    ldrStatus = 'TERANG';
                    statusClass = 'status-terang';
                } else {
                    ldrStatus = 'SANGAT TERANG';
                    statusClass = 'status-terang';
                }
        
                ldrValueEl.textContent = ldrStatus;
                ldrValueEl.className = `status-text ${statusClass}`;
            } else {
                ldrValueEl.textContent = '--';
                ldrValueEl.className = 'status-text';
            }
        }

        // Update indikator channel
        function updateChannelIndicators(data) {
            document.getElementById('ch1-indicator').classList.toggle('active', data.r1 == 1);
            document.getElementById('ch2-indicator').classList.toggle('active', data.r2 == 1);
            document.getElementById('ch3-indicator').classList.toggle('active', data.r3 == 1);
            document.getElementById('ch4-indicator').classList.toggle('active', data.r4 == 1);
        }

        // Kontrol relay
        async function controlRelay(device, action) {
            const endpoint = ENDPOINT_MAP[device][action];
            const isOn = action === 'on';

            // Disable tombol saat ada yang diklik
            const buttons = document.querySelectorAll(`#${device}-card .btn`);
            buttons.forEach(btn => btn.disabled = true);

            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 5000);

                const response = await fetch(`http://${ESP32_IP}${endpoint}`, {
                    signal: controller.signal
                });

                clearTimeout(timeoutId);

                if (response.ok) {
                    const result = await response.text();
                    console.log(`Relay ${device} ${action} response:`, result);
                    isConnected = true;
                    serverStatusEl.className = 'server-status server-online';
                    serverStatusEl.innerHTML = '<i class="fas fa-wifi"></i><span>Terhubung ke ESP32</span>';
            
                    // Update status di interface
                    updateRelayStatus(device, isOn);
            
                    // Update indikator channel relay
                    const channelElement = document.getElementById(CHANNEL_MAP[device]);
                    if (channelElement) {
                        channelElement.classList.toggle('active', isOn);
                    }
            
                    // Update display sensor sesuai kondisi channel relay
                    if (device === 'dht' && !isOn) {
                        // DHT turned OFF
                        document.getElementById('temperature').textContent = '--';
                        document.getElementById('humidity').textContent = '--';
                    } else if (device === 'ldr') {
                        if (!isOn) {
                            // Jika LDR OFF
                            document.getElementById('ldr-raw').textContent = '--';
                            document.getElementById('ldr-lux').textContent = '--';
                            const ldrValueEl = document.getElementById('ldr-value');
                            ldrValueEl.textContent = 'OFFLINE';
                            ldrValueEl.className = 'status-text status-offline';
                        } else {
                            // Jika LDR baru ON, tampilkan "--" sementara hingga ada data
                            document.getElementById('ldr-raw').textContent = '--';
                            document.getElementById('ldr-lux').textContent = '--';
                            const ldrValueEl = document.getElementById('ldr-value');
                            ldrValueEl.textContent = '--';
                            ldrValueEl.className = 'status-text';
                        }
                    } else if (device === 'ultrasonic') {
                        if (!isOn) {
                            document.getElementById('distance').textContent = '--';
                        }
                        evaluateBuzzerStatus();
                    }
            
                    // Delay 300ms setelah channel aktif, baru request data dari ESP32
                    if (isOn && device !== 'buzzer') {
                        setTimeout(fetchSensorData, 300);
                    }
                } else {
                    throw new Error(`HTTP ${response.status}`);
                }

            } catch (error) {
                console.error(`Error controlling ${device}:`, error);
                isConnected = false;
                serverStatusEl.className = 'server-status server-offline';
                serverStatusEl.innerHTML = '<i class="fas fa-exclamation-triangle"></i><span>Gagal mengontrol relay</span>';

                alert(`Gagal mengendalikan ${device}. Periksa koneksi WiFi.`);
            } finally {
                // Re-enable buttons
                buttons.forEach(btn => btn.disabled = false);
            }
        }

        // Update status relay UI
        function updateRelayStatus(device, isOn) {
            const statusElement = document.getElementById(`${device}-status`);
            
            if (isOn) {
                statusElement.className = 'status-badge status-on';
                statusElement.textContent = 'RELAY ON';
            } else {
                statusElement.className = 'status-badge status-off';
                statusElement.textContent = 'RELAY OFF';
            }
        }

        // Update timestamp
        function updateTimestamp() {
            const now = new Date();
            const timeString = now.toLocaleTimeString('id-ID', { hour12: false });
            const updateElement = document.getElementById('last-update');
            if (updateElement) {
                updateElement.querySelector('b').textContent = timeString;
            }
        }

        // Set threshold
        function setThreshold() {
            const thresholdInput = document.getElementById('threshold');
            
            if (thresholdInput.value < 2 || thresholdInput.value > 400) {
                alert('Threshold harus antara 2 dan 400 cm');
                thresholdInput.value = 50;
                return;
            }
            
            fetch(`http://${ESP32_IP}/set?th=${thresholdInput.value}`)
                .then(response => {
                    if (response.ok) {
                        console.log(`Threshold set to ${thresholdInput.value} cm`);
                    }
                })
                .catch(error => {
                    console.error('Failed to set threshold:', error);
                });
            evaluateBuzzerStatus();
        }

        // Test koneksi
        async function testConnection(endpoint) {
            const testResult = document.getElementById('test-result');
            testResult.innerHTML = 'Menguji...';
            testResult.style.color = '#f59e0b';
    
            try {
                const startTime = Date.now();
                const response = await fetch(`http://${ESP32_IP}/${endpoint}`, {
                    signal: AbortSignal.timeout(5000)
                });
                const responseTime = Date.now() - startTime;
        
                if (response.ok) {
                    const contentType = response.headers.get('content-type');
            
                    if (contentType && contentType.includes('application/json')) {
                        // Handle JSON response
                        const data = await response.json();
                        testResult.innerHTML = `Berhasil (${responseTime}ms)<br>${JSON.stringify(data)}`;
                        testResult.style.color = '#10b981';
                    } else {
                        // Handle plain text response (like "OK" from relay endpoints)
                        const text = await response.text();
                        testResult.innerHTML = `Berhasil (${responseTime}ms)<br>Response: ${text}`;
                        testResult.style.color = '#10b981';
                    }
                } else {
                    testResult.innerHTML = `HTTP ${response.status}`;
                    testResult.style.color = '#ef4444';
                }
            } catch (error) {
                testResult.innerHTML = `Error: ${error.message}`;
                testResult.style.color = '#ef4444';
            }
        }

        // Toggle panel test koneksi
        function toggleConnectionTestPanel() {
            connectionTestPanel.classList.toggle('show');
        }

        // Inisialisasi dashboard
        async function initDashboard() {
            updateTimestamp();
            await checkNetwork();
            
            if (isConnected) {
                fetchSensorData();
                updateInterval = setInterval(fetchSensorData, 1000);
            }
            
            // Event listeners untuk panel test koneksi
            connectionTestBtn.addEventListener('click', toggleConnectionTestPanel);
            closeTestPanel.addEventListener('click', toggleConnectionTestPanel);
            
            // Tutup panel saat klik di luar
            document.addEventListener('click', (event) => {
                if (!connectionTestPanel.contains(event.target) && 
                    !connectionTestBtn.contains(event.target) && 
                    connectionTestPanel.classList.contains('show')) {
                    connectionTestPanel.classList.remove('show');
                }
            });
        }

        // Start saat page load
        document.addEventListener('DOMContentLoaded', initDashboard);
    </script>
</body>
</html>
)rawliteral";
}

/* ================= Handler Web ================= */
void handleRoot(){ 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/html",page()); 
}

void handleData(){
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    double l = analogRead(LDRPIN);
    jarak = readUltrasonic();

    float lux = 0;
    
    //Konversi voltage ke lux
    float V_ldr = (l / 4095.0) * 3.3; //ADC to voltage (0-3.3V)
    lux = abs(15.673 * exp(1.19*V_ldr));
    if (lux<0 || V_ldr==0){
        lux=0;
    }

    // Membaca kondisi channel-channel relay
    bool r1 = (digitalRead(RELAY1_DHT) == (RELAY_ACTIVE_LOW ? LOW : HIGH));
    bool r2 = (digitalRead(RELAY2_LDR) == (RELAY_ACTIVE_LOW ? LOW : HIGH));
    bool r3 = (digitalRead(RELAY3_BUZZER) == (RELAY_ACTIVE_LOW ? LOW : HIGH));
    bool r4 = (digitalRead(RELAY4_USONIC) == (RELAY_ACTIVE_LOW ? LOW : HIGH));

    // Debug output di Serial Monitor
    Serial.print("DHT Temp: "); Serial.print(t);
    Serial.print(" Humidity: "); Serial.print(h);
    Serial.print(" LDR: "); Serial.print(l);
    Serial.print(" Lux: "); Serial.print(lux);
    Serial.print(" Distance: "); Serial.println(jarak);

    Serial.print("Relays: R1="); Serial.print(r1);
    Serial.print(" R2="); Serial.print(r2);
    Serial.print(" R3="); Serial.print(r3);
    Serial.print(" R4="); Serial.println(r4);

    String json = "{";
    json += "\"t\":" + (isnan(t) ? "null" : String(t)) + ",";
    json += "\"h\":" + (isnan(h) ? "null" : String(h)) + ",";
    json += "\"l\":" + String(l) + ",";
    json += "\"lux\":" + (lux <= 0 ? "null" : String(lux, 1)) + ",";
    json += "\"d\":" + String(jarak) + ",";
    json += "\"r1\":" + String(r1 ? 1 : 0) + ",";
    json += "\"r2\":" + String(r2 ? 1 : 0) + ",";
    json += "\"r3\":" + String(r3 ? 1 : 0) + ",";
    json += "\"r4\":" + String(r4 ? 1 : 0);
    json += "}";

    // DEBUG: Print JSON yang akan dikirim ke interface
    Serial.println("Sending JSON: " + json);
    
    // Mengirim JSON ke interface
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"application/json",json);
}

/* ================= Setup Web + Sensor ================= */
void setup() {
  Serial.begin(115200);

  pinMode(RELAY1_DHT, OUTPUT);
  pinMode(RELAY2_LDR, OUTPUT);
  pinMode(RELAY3_BUZZER, OUTPUT);
  pinMode(RELAY4_USONIC, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  relayOff(RELAY1_DHT);
  relayOff(RELAY2_LDR);
  relayOff(RELAY3_BUZZER);
  relayOff(RELAY4_USONIC);
  digitalWrite(BUZZERPIN, LOW);

  dht.begin();

  WiFi.softAP("TRAINER_LOGIKA", "12345678");
  Serial.println("WiFi AP Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.on("/r1on", [](){ 
    relayOn(RELAY1_DHT); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r1off", [](){ 
    relayOff(RELAY1_DHT); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r2on", [](){ 
    relayOn(RELAY2_LDR); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r2off", [](){ 
    relayOff(RELAY2_LDR); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r3on", [](){ 
    relayOn(RELAY3_BUZZER); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r3off", [](){ 
    relayOff(RELAY3_BUZZER); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r4on", [](){ 
    relayOn(RELAY4_USONIC); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });
  server.on("/r4off", [](){ 
    relayOff(RELAY4_USONIC); 
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK"); 
  });

  server.on("/set", [](){
    if(server.hasArg("th")) threshold_cm = server.arg("th").toFloat();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200,"text/plain","OK");
  });

  server.begin();
}

/* ================= Loop Relay + Sensor ================= */
void loop() {
  server.handleClient();

  bool rU = (digitalRead(RELAY4_USONIC) == (RELAY_ACTIVE_LOW ? LOW : HIGH));
  bool rB = (digitalRead(RELAY3_BUZZER) == (RELAY_ACTIVE_LOW ? LOW : HIGH));

  jarak = readUltrasonic();
  updateBuzzer(jarak, rU, rB);

  delay(200);
}

