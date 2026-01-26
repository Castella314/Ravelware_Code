// Configuration
const ESP32_IP = '192.168.4.1';
let isConnected = false;
let connectionRetries = 0;

// DOM Elements
const serverStatusEl = document.getElementById('server-status');

// Function to check if we're on the right network
async function checkNetwork() {
    console.log('Checking network connection...');
    
    // Try to ping the ESP32
    try {
        // Create a timeout promise
        const timeoutPromise = new Promise((_, reject) => {
            setTimeout(() => reject(new Error('Timeout')), 2000);
        });
        
        // Try to fetch from ESP32
        const fetchPromise = fetch(`http://${ESP32_IP}/data`);
        
        const response = await Promise.race([fetchPromise, timeoutPromise]);
        
        if (response.ok) {
            const data = await response.json();
            console.log('✅ Connected to ESP32:', data);
            
            serverStatusEl.className = 'server-status server-online';
            serverStatusEl.innerHTML = '<i class="fas fa-wifi"></i><span>Connected to ESP32</span>';
            
            // Update UI
            document.getElementById('wifi-status-text').textContent = 'TRAINER_LOGIKA';
            document.getElementById('esp32-status-text').textContent = ESP32_IP;
            document.getElementById('esp32-ip').textContent = ESP32_IP;
            
            isConnected = true;
            connectionRetries = 0;
            return true;
        }
    } catch (error) {
        console.log(`❌ Cannot reach ESP32 (Attempt ${connectionRetries + 1}):`, error.message);
        connectionRetries++;
    }
    
    // Not connected
    serverStatusEl.className = 'server-status server-offline';
    
    if (connectionRetries > 3) {
        serverStatusEl.innerHTML = `
            <i class="fas fa-exclamation-triangle"></i>
            <span>
                Not connected to ESP32 WiFi!<br>
                <small>Connect to: TRAINER_LOGIKA (password: 12345678)</small>
            </span>
        `;
    } else {
        serverStatusEl.innerHTML = '<i class="fas fa-sync-alt fa-spin"></i><span>Searching for ESP32...</span>';
    }
    
    isConnected = false;
    return false;
}

// Fetch sensor data
async function fetchSensorData() {
    if (!isConnected) {
        await checkNetwork();
        if (!isConnected) {
            console.log('Skipping sensor update - not connected');
            return null;
        }
    }
    
    try {
        const response = await fetch(`http://${ESP32_IP}/data`);
        if (!response.ok) throw new Error('Response not OK');
        
        const data = await response.json();
        
        // Update UI with real data
        updateSensorUI(data);
        
        return data;
        
    } catch (error) {
        console.error('Failed to fetch sensor data:', error);
        isConnected = false;
        
        // Fallback to simulation
        updateSimulatedData();
        return null;
    }
}

// Update UI with real data
function updateSensorUI(data) {
    console.log('Updating UI with real data:', data);
    
    // Temperature
    const temp = parseFloat(data.t);
    document.getElementById('temperature').textContent = isNaN(temp) ? '--' : temp.toFixed(1);
    
    // Humidity
    const hum = parseFloat(data.h);
    document.getElementById('humidity').textContent = isNaN(hum) ? '--' : hum.toFixed(1);
    
    // LDR
    const ldr = parseInt(data.l);
    document.getElementById('ldr-raw').textContent = ldr || '--';
    
    if (ldr > 0) {
        const ldrStatus = ldr > 500 ? 'TERANG' : 'GELAP';
        document.getElementById('ldr-value').textContent = ldrStatus;
        document.getElementById('ldr-value').className = `status-text ${ldrStatus === 'TERANG' ? 'status-terang' : 'status-gelap'}`;
    } else {
        document.getElementById('ldr-value').textContent = '--';
        document.getElementById('ldr-value').className = 'status-text';
    }
    
    // Distance
    const dist = parseFloat(data.d);
    document.getElementById('distance').textContent = dist <= 0 ? '--' : dist.toFixed(1);
    
    // Update relay status
    updateRelayStatus('dht', data.r1 == 1);
    updateRelayStatus('ldr', data.r2 == 1);
    updateRelayStatus('buzzer', data.r3 == 1);
    updateRelayStatus('ultrasonic', data.r4 == 1);
    
    // Update timestamp
    updateTimestamp();
}

// Fallback simulation
function updateSimulatedData() {
    console.log('Using simulated data');
    
    // Only simulate if relays are "on"
    const isDhtOn = document.getElementById('dht-status').textContent.includes('ON');
    const isLdrOn = document.getElementById('ldr-status').textContent.includes('ON');
    const isUsonicOn = document.getElementById('ultrasonic-status').textContent.includes('ON');
    
    document.getElementById('temperature').textContent = isDhtOn ? (Math.random() * 10 + 22).toFixed(1) : '--';
    document.getElementById('humidity').textContent = isDhtOn ? (Math.random() * 20 + 50).toFixed(1) : '--';
    
    const ldrValue = isLdrOn ? Math.floor(Math.random() * 4095) : 0;
    document.getElementById('ldr-raw').textContent = ldrValue || '--';
    
    if (ldrValue > 0) {
        const ldrStatus = ldrValue > 500 ? 'TERANG' : 'GELAP';
        document.getElementById('ldr-value').textContent = ldrStatus;
        document.getElementById('ldr-value').className = `status-text ${ldrStatus === 'TERANG' ? 'status-terang' : 'status-gelap'}`;
    }
    
    document.getElementById('distance').textContent = isUsonicOn ? (Math.random() * 100 + 1).toFixed(1) : '--';
    
    updateTimestamp();
}

// Control relay
async function controlRelay(device, action) {
    const endpointMap = {
        dht: { on: '/r1on', off: '/r1off' },
        ldr: { on: '/r2on', off: '/r2off' },
        buzzer: { on: '/r3on', off: '/r3off' },
        ultrasonic: { on: '/r4on', off: '/r4off' }
    };
    
    const endpoint = endpointMap[device][action];
    
    console.log(`Attempting to control ${device} to ${action}...`);
    
    // Update UI immediately (for better UX)
    updateRelayStatus(device, action === 'on');
    
    try {
        const response = await fetch(`http://${ESP32_IP}${endpoint}`, {
            timeout: 5000
        });
        
        if (response.ok) {
            const result = await response.json();
            console.log(`✅ Relay ${device} ${action}:`, result);
            
            // Update connection status
            isConnected = true;
            serverStatusEl.className = 'server-status server-online';
            serverStatusEl.innerHTML = '<i class="fas fa-wifi"></i><span>Connected to ESP32</span>';
            
            // Refresh sensor data after a delay
            setTimeout(fetchSensorData, 300);
            
        } else {
            throw new Error('Response not OK');
        }
        
    } catch (error) {
        console.error(`❌ Failed to control relay ${device}:`, error);
        
        // Show error to user
        alert(`Failed to control ${device}. Make sure:\n1. Connected to TRAINER_LOGIKA WiFi\n2. ESP32 is powered on\n3. Relays are connected`);
        
        // Update status
        isConnected = false;
        serverStatusEl.className = 'server-status server-offline';
        serverStatusEl.innerHTML = '<i class="fas fa-exclamation-triangle"></i><span>ESP32 not reachable</span>';
    }
}

// Update relay UI
function updateRelayStatus(device, isOn) {
    const statusElement = document.getElementById(`${device}-status`);
    const powerElement = document.getElementById(`${device}-power-status`) || 
                         document.getElementById(`${device}-relay-status`);
    
    if (isOn) {
        statusElement.className = 'status-badge status-on';
        statusElement.textContent = 'RELAY ON';
        if (powerElement) {
            powerElement.textContent = 'ON';
            powerElement.style.color = 'var(--success)';
        }
    } else {
        statusElement.className = 'status-badge status-off';
        statusElement.textContent = 'RELAY OFF';
        if (powerElement) {
            powerElement.textContent = 'OFF';
            powerElement.style.color = 'var(--danger)';
        }
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

// Initialize dashboard
async function initDashboard() {
    console.log('🚀 IoT Dashboard Starting...');
    console.log('ESP32 IP:', ESP32_IP);
    
    // Initial timestamp
    updateTimestamp();
    
    // Check connection
    await checkNetwork();
    
    // Start update loop if connected
    if (isConnected) {
        fetchSensorData();
        setInterval(fetchSensorData, 1000);
    } else {
        // Try to reconnect periodically
        setInterval(checkNetwork, 3000);
        
        // Still update simulated data
        setInterval(updateSimulatedData, 2000);
    }
    
    console.log('Dashboard initialized');
}

// Start when page loads
document.addEventListener('DOMContentLoaded', initDashboard);

// Export for debugging
window.dashboard = {
    checkNetwork,
    fetchSensorData,
    controlRelay,
    isConnected: () => isConnected
};

// Add cache-busting to fetch requests
async function fetchWithCacheBust(url) {
    const timestamp = new Date().getTime();
    const urlWithCache = `${url}?_=${timestamp}`;
    
    console.log(`Fetching: ${urlWithCache}`);
    
    try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 3000);
        
        const response = await fetch(urlWithCache, {
            signal: controller.signal,
            mode: 'cors',
            cache: 'no-cache'
        });
        
        clearTimeout(timeoutId);
        return response;
    } catch (error) {
        console.error(`Fetch error for ${url}:`, error);
        throw error;
    }
}