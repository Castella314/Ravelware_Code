const express = require('express');
const cors = require('cors');

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());

// Simulated sensor states
let sensorStates = {
    dht: 'on',
    ldr: 'on',
    buzzer: 'off',
    ultrasonic: 'on'
};

// Simulated sensor data
let sensorData = {
    dht: {
        temperature: 25.5,
        humidity: 65.2
    },
    ldr: {
        value: 750,
        status: 'TERANG'
    },
    ultrasonic: {
        distance: 45.3
    },
    buzzer: {
        state: 'off'
    }
};

// Generate random sensor data
function generateSensorData() {
    const isDaytime = new Date().getHours() > 6 && new Date().getHours() < 18;
    
    // Update DHT data if sensor is ON
    if (sensorStates.dht === 'on') {
        sensorData.dht.temperature = 23 + Math.random() * 10;
        sensorData.dht.humidity = 55 + Math.random() * 20;
    } else {
        sensorData.dht.temperature = 0;
        sensorData.dht.humidity = 0;
    }
    
    // Update LDR data if sensor is ON
    if (sensorStates.ldr === 'on') {
        sensorData.ldr.value = isDaytime ? 
            Math.floor(Math.random() * 400 + 600) : 
            Math.floor(Math.random() * 400 + 100);
        sensorData.ldr.status = sensorData.ldr.value > 500 ? 'TERANG' : 'GELAP';
    } else {
        sensorData.ldr.value = 0;
        sensorData.ldr.status = 'OFF';
    }
    
    // Update Ultrasonic data if sensor is ON
    if (sensorStates.ultrasonic === 'on') {
        sensorData.ultrasonic.distance = Math.random() * 180 + 10;
        
        // Auto buzzer logic
        if (sensorData.ultrasonic.distance <= 10) {
            sensorData.buzzer.state = 'auto-on';
        } else if (sensorStates.buzzer === 'off') {
            sensorData.buzzer.state = 'off';
        }
    } else {
        sensorData.ultrasonic.distance = 0;
    }
    
    // Update buzzer state
    if (sensorStates.buzzer === 'on') {
        sensorData.buzzer.state = 'on';
    }
    
    return sensorData;
}

// API Endpoints

// GET all sensor data
app.get('/api/sensors', (req, res) => {
    const data = generateSensorData();
    res.json({
        ...data,
        timestamp: new Date().toISOString(),
        states: sensorStates
    });
});

// GET sensor status
app.get('/api/status', (req, res) => {
    res.json({
        states: sensorStates,
        timestamp: new Date().toISOString()
    });
});

// POST control device
app.post('/api/control', (req, res) => {
    const { device, state } = req.body;
    
    if (!device || !state) {
        return res.status(400).json({
            error: 'Device and state are required'
        });
    }
    
    if (!['dht', 'ldr', 'buzzer', 'ultrasonic'].includes(device)) {
        return res.status(400).json({
            error: 'Invalid device. Must be: dht, ldr, buzzer, ultrasonic'
        });
    }
    
    if (!['on', 'off'].includes(state)) {
        return res.status(400).json({
            error: 'Invalid state. Must be: on, off'
        });
    }
    
    // Update sensor state
    sensorStates[device] = state;
    
    // Update buzzer logic if ultrasonic is turned on/off
    if (device === 'ultrasonic' && state === 'off') {
        sensorData.buzzer.state = 'off';
    }
    
    res.json({
        message: `Device ${device} set to ${state}`,
        state: state,
        timestamp: new Date().toISOString()
    });
});

// Health check endpoint
app.get('/api/health', (req, res) => {
    res.json({
        status: 'online',
        timestamp: new Date().toISOString(),
        sensors: sensorStates
    });
});

// Start server
app.listen(PORT, () => {
    console.log(`Mock API Server running at http://localhost:${PORT}`);
    console.log(`Available endpoints:`);
    console.log(`  GET  http://localhost:${PORT}/api/sensors`);
    console.log(`  GET  http://localhost:${PORT}/api/status`);
    console.log(`  POST http://localhost:${PORT}/api/control`);
    console.log(`  GET  http://localhost:${PORT}/api/health`);
});