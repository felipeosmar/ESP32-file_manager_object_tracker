/**
 * ESP32-CAM Stream - Web Interface JavaScript
 */

// State
let isConnected = false;
let fps = 0;

// Initialize
document.addEventListener('DOMContentLoaded', function() {
    console.log('ESP32-CAM Stream Interface Loaded');

    // Update connection status periodically
    checkConnection();
    setInterval(checkConnection, 2000);

    // Monitor stream for FPS
    monitorStreamFPS();
});

async function checkConnection() {
    try {
        // Use lightweight health check instead of HEAD /stream
        // This prevents 404 errors and doesn't interfere with the active stream
        const response = await fetch('/api/health/status', {
            method: 'GET',
            cache: 'no-cache'
        });
        if (response.ok) {
            if (!isConnected) {
                isConnected = true;
                updateConnectionStatus(true);
            }
        } else {
            if (isConnected) {
                isConnected = false;
                updateConnectionStatus(false);
            }
        }
    } catch (error) {
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

function handleStreamError() {
    console.error('Camera stream error');
    const img = document.getElementById('camera-stream');
    img.style.background = '#333';
    img.alt = 'Stream não disponível';
}

async function monitorStreamFPS() {
    // Display estimated FPS based on camera configuration
    // Monitoring the actual stream would create a second connection and overload the ESP32
    const fpsElement = document.getElementById('info-fps');
    if (fpsElement) {
        fpsElement.textContent = '~16 fps';
    }
}

console.log('ESP32-CAM Stream Ready');
