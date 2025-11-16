/**
 * ESP32 Object Tracker - Web Interface JavaScript
 */

// State
let isTracking = true;
let currentPan = 90;
let currentTilt = 90;
let isConnected = false;
let lastFrameTime = Date.now();
let fps = 0;

// Initialize
document.addEventListener('DOMContentLoaded', function() {
    console.log('ESP32 Object Tracker Interface Loaded');

    // Setup event listeners
    setupEventListeners();

    // Start status updates
    updateStatus();
    setInterval(updateStatus, 1000);

    // Monitor stream for FPS
    monitorStreamFPS();
});

function setupEventListeners() {
    // Auto-tracking toggle
    const trackingToggle = document.getElementById('auto-tracking');
    trackingToggle.addEventListener('change', function() {
        isTracking = this.checked;
        toggleTracking(this.checked);
        updateTrackingLabel(this.checked);
    });

    // Pan/Tilt sliders
    document.getElementById('pan-slider').addEventListener('input', function() {
        currentPan = parseInt(this.value);
        document.getElementById('pan-value').textContent = currentPan;
        sendManualControl();
    });

    document.getElementById('tilt-slider').addEventListener('input', function() {
        currentTilt = parseInt(this.value);
        document.getElementById('tilt-value').textContent = currentTilt;
        sendManualControl();
    });

    // Settings
    document.getElementById('motion-threshold').addEventListener('input', function() {
        document.getElementById('threshold-value').textContent = this.value;
        // Send to ESP32 (would need additional API endpoint)
    });

    document.getElementById('tracking-speed').addEventListener('input', function() {
        document.getElementById('speed-value').textContent = this.value;
        // Send to ESP32 (would need additional API endpoint)
    });
}

async function updateStatus() {
    try {
        const response = await fetch('/api/status');
        if (response.ok) {
            const data = await response.json();

            // Update connection status
            if (!isConnected) {
                isConnected = true;
                updateConnectionStatus(true);
            }

            // Update UI with current values
            currentPan = data.pan || 90;
            currentTilt = data.tilt || 90;

            document.getElementById('info-pan').textContent = currentPan + '°';
            document.getElementById('info-tilt').textContent = currentTilt + '°';
            document.getElementById('info-tracking').textContent = data.tracking ? 'Ativo' : 'Inativo';

            // Update sliders without triggering events
            document.getElementById('pan-slider').value = currentPan;
            document.getElementById('tilt-slider').value = currentTilt;
            document.getElementById('pan-value').textContent = currentPan;
            document.getElementById('tilt-value').textContent = currentTilt;

        } else {
            if (isConnected) {
                isConnected = false;
                updateConnectionStatus(false);
            }
        }
    } catch (error) {
        console.error('Status update error:', error);
        if (isConnected) {
            isConnected = false;
            updateConnectionStatus(false);
        }
    }
}

function updateConnectionStatus(connected) {
    const statusDot = document.getElementById('connection-status');
    const statusText = document.getElementById('status-text');

    if (connected) {
        statusDot.classList.add('connected');
        statusText.textContent = 'Conectado';
    } else {
        statusDot.classList.remove('connected');
        statusText.textContent = 'Desconectado';
    }
}

function updateTrackingLabel(enabled) {
    const label = document.getElementById('tracking-label');
    const infoTracking = document.getElementById('info-tracking');

    label.textContent = enabled ? 'Ativo' : 'Inativo';
    infoTracking.textContent = enabled ? 'Ativo' : 'Inativo';
}

async function toggleTracking(enabled) {
    try {
        const formData = new FormData();
        formData.append('enabled', enabled);

        const response = await fetch('/api/tracking', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            console.log('Tracking toggled:', enabled);
        }
    } catch (error) {
        console.error('Toggle tracking error:', error);
    }
}

async function centerServos() {
    try {
        const response = await fetch('/api/center', {
            method: 'POST'
        });

        if (response.ok) {
            console.log('Servos centered');
            currentPan = 90;
            currentTilt = 90;

            // Update UI
            document.getElementById('pan-slider').value = 90;
            document.getElementById('tilt-slider').value = 90;
            document.getElementById('pan-value').textContent = '90';
            document.getElementById('tilt-value').textContent = '90';
        }
    } catch (error) {
        console.error('Center servos error:', error);
    }
}

function moveServo(type, delta) {
    if (type === 'pan') {
        currentPan = Math.max(0, Math.min(180, currentPan + delta));
        document.getElementById('pan-slider').value = currentPan;
        document.getElementById('pan-value').textContent = currentPan;
    } else if (type === 'tilt') {
        currentTilt = Math.max(0, Math.min(180, currentTilt + delta));
        document.getElementById('tilt-slider').value = currentTilt;
        document.getElementById('tilt-value').textContent = currentTilt;
    }

    sendManualControl();
}

function updatePanTilt(type, value) {
    if (type === 'pan') {
        currentPan = parseInt(value);
    } else if (type === 'tilt') {
        currentTilt = parseInt(value);
    }

    sendManualControl();
}

let manualControlTimeout;
async function sendManualControl() {
    // Debounce rapid changes
    clearTimeout(manualControlTimeout);
    manualControlTimeout = setTimeout(async () => {
        try {
            const formData = new FormData();
            formData.append('pan', currentPan);
            formData.append('tilt', currentTilt);

            const response = await fetch('/api/manual', {
                method: 'POST',
                body: formData
            });

            if (response.ok) {
                console.log(`Manual control - Pan: ${currentPan}, Tilt: ${currentTilt}`);
            }
        } catch (error) {
            console.error('Manual control error:', error);
        }
    }, 100);
}

function updateThreshold(value) {
    console.log('Motion threshold:', value);
    // Would send to ESP32 via API
}

function updateSpeed(value) {
    console.log('Tracking speed:', value);
    // Would send to ESP32 via API
}

function handleStreamError() {
    console.error('Camera stream error');
    const img = document.getElementById('camera-stream');
    img.style.background = '#333';
    img.alt = 'Stream não disponível';
}

function monitorStreamFPS() {
    const img = document.getElementById('camera-stream');
    let frameCount = 0;

    img.addEventListener('load', function() {
        frameCount++;
    });

    setInterval(() => {
        fps = frameCount;
        frameCount = 0;
        document.getElementById('info-fps').textContent = fps + ' fps';
    }, 1000);
}

// Keyboard shortcuts
document.addEventListener('keydown', function(e) {
    switch(e.key) {
        case 'ArrowUp':
            moveServo('tilt', 5);
            e.preventDefault();
            break;
        case 'ArrowDown':
            moveServo('tilt', -5);
            e.preventDefault();
            break;
        case 'ArrowLeft':
            moveServo('pan', -5);
            e.preventDefault();
            break;
        case 'ArrowRight':
            moveServo('pan', 5);
            e.preventDefault();
            break;
        case 'c':
        case 'C':
            centerServos();
            break;
        case 't':
        case 'T':
            const toggle = document.getElementById('auto-tracking');
            toggle.checked = !toggle.checked;
            toggle.dispatchEvent(new Event('change'));
            break;
    }
});

console.log('Keyboard shortcuts:');
console.log('  Arrow keys: Manual control');
console.log('  C: Center servos');
console.log('  T: Toggle tracking');
