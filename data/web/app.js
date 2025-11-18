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
        const response = await fetch('/stream', { method: 'HEAD' });
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

console.log('ESP32-CAM Stream Ready');
