// Firmware Upload Script
let uploadInProgress = false;
let selectedFile = null;

// Initialize event listeners
document.addEventListener('DOMContentLoaded', function() {
    const uploadZone = document.getElementById('uploadZone');
    const firmwareInput = document.getElementById('firmwareInput');

    // Click to select file
    uploadZone.addEventListener('click', function() {
        if (!uploadInProgress) {
            firmwareInput.click();
        }
    });

    // File input change
    firmwareInput.addEventListener('change', function(e) {
        if (e.target.files.length > 0) {
            handleFileSelection(e.target.files[0]);
        }
    });

    // Drag and drop events
    uploadZone.addEventListener('dragover', function(e) {
        e.preventDefault();
        if (!uploadInProgress) {
            uploadZone.classList.add('dragging');
        }
    });

    uploadZone.addEventListener('dragleave', function() {
        uploadZone.classList.remove('dragging');
    });

    uploadZone.addEventListener('drop', function(e) {
        e.preventDefault();
        uploadZone.classList.remove('dragging');

        if (!uploadInProgress && e.dataTransfer.files.length > 0) {
            handleFileSelection(e.dataTransfer.files[0]);
        }
    });
});

// Handle file selection
function handleFileSelection(file) {
    console.log('File selected:', file.name);

    // Client-side validation: check file extension
    if (!file.name.toLowerCase().endsWith('.bin')) {
        showError('Arquivo inválido: apenas arquivos .bin são aceitos');
        return;
    }

    // Check file size (should be < 2MB for typical ESP32)
    if (file.size > 2 * 1024 * 1024) {
        showError('Arquivo muito grande: tamanho máximo é 2MB');
        return;
    }

    if (file.size === 0) {
        showError('Arquivo vazio: selecione um firmware válido');
        return;
    }

    selectedFile = file;

    // Show confirmation
    const confirmMsg = `Confirmar atualização do firmware?\n\n` +
                      `Arquivo: ${file.name}\n` +
                      `Tamanho: ${formatSize(file.size)}\n\n` +
                      `O dispositivo será reiniciado após a atualização.`;

    if (confirm(confirmMsg)) {
        uploadFirmware(file);
    }
}

// Upload firmware
async function uploadFirmware(file) {
    uploadInProgress = true;

    // Update UI
    document.getElementById('uploadZone').classList.add('uploading');
    document.getElementById('progressSection').style.display = 'block';
    document.getElementById('errorSection').style.display = 'none';
    document.getElementById('successSection').style.display = 'none';

    // Update status indicator
    const statusDot = document.getElementById('upload-status');
    const statusText = document.getElementById('status-text');
    statusDot.classList.add('uploading');
    statusText.textContent = 'Enviando...';

    // Prevent navigation during upload
    window.addEventListener('beforeunload', preventNavigation);

    const formData = new FormData();
    formData.append('file', file);

    const xhr = new XMLHttpRequest();

    // Progress tracking
    xhr.upload.addEventListener('progress', function(e) {
        if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            updateProgress(percent, 'uploading');
        }
    });

    // Upload complete
    xhr.addEventListener('load', function() {
        if (xhr.status === 200) {
            // Parse response
            try {
                const response = JSON.parse(xhr.responseText);

                if (response.error) {
                    handleUploadError(response.error);
                } else {
                    handleUploadSuccess();
                }
            } catch (e) {
                console.error('Failed to parse response:', e);
                handleUploadError('Erro ao processar resposta do servidor');
            }
        } else {
            let errorMsg = 'Erro no upload';
            try {
                const response = JSON.parse(xhr.responseText);
                if (response.error) {
                    errorMsg = response.error;
                }
            } catch (e) {
                errorMsg = `Erro no upload: ${xhr.status}`;
            }
            handleUploadError(errorMsg);
        }
    });

    // Upload error
    xhr.addEventListener('error', function() {
        handleUploadError('Erro de conexão durante o upload');
    });

    // Upload timeout
    xhr.addEventListener('timeout', function() {
        handleUploadError('Timeout: o upload demorou muito tempo');
    });

    // Send request
    console.log('Sending firmware to /api/firmware/upload');
    xhr.open('POST', '/api/firmware/upload');
    xhr.timeout = 180000; // 3 minutes timeout
    xhr.send(formData);
}

// Update progress bar
function updateProgress(percent, stage) {
    const progressFill = document.getElementById('progressFill');
    const progressText = document.getElementById('progressText');
    const statusMessage = document.getElementById('statusMessage');

    // Cap at 95% during upload, reserve 95-100% for validation/flashing/reboot
    if (stage === 'uploading') {
        percent = Math.min(percent, 95);
        statusMessage.textContent = 'Enviando firmware...';
    } else if (stage === 'validating') {
        percent = 96;
        statusMessage.textContent = 'Validando arquivo...';
    } else if (stage === 'flashing') {
        percent = 98;
        statusMessage.textContent = 'Gravando na flash...';
    } else if (stage === 'rebooting') {
        percent = 100;
        statusMessage.textContent = 'Reiniciando dispositivo...';
    }

    progressFill.style.width = percent + '%';
    progressText.textContent = percent + '%';
}

// Handle upload success
function handleUploadSuccess() {
    console.log('Firmware upload successful');

    // Update progress stages
    updateProgress(96, 'validating');
    setTimeout(() => {
        updateProgress(98, 'flashing');
        setTimeout(() => {
            updateProgress(100, 'rebooting');

            // Show success message
            setTimeout(() => {
                document.getElementById('progressSection').style.display = 'none';
                document.getElementById('successSection').style.display = 'block';
                document.getElementById('successMessage').textContent =
                    'Firmware gravado com sucesso. O dispositivo está reiniciando...';

                // Start auto-reconnect
                startReconnectPolling();
            }, 1000);
        }, 500);
    }, 500);
}

// Handle upload error
function handleUploadError(errorMsg) {
    console.error('Upload error:', errorMsg);

    uploadInProgress = false;

    // Remove navigation blocker
    window.removeEventListener('beforeunload', preventNavigation);

    // Update UI
    document.getElementById('uploadZone').classList.remove('uploading');
    document.getElementById('progressSection').style.display = 'none';

    // Update status indicator
    const statusDot = document.getElementById('upload-status');
    const statusText = document.getElementById('status-text');
    statusDot.classList.remove('uploading');
    statusText.textContent = 'Erro';

    // Show error
    showError(errorMsg);

    // Reset status after 5 seconds
    setTimeout(() => {
        statusDot.classList.remove('uploading');
        statusText.textContent = 'Pronto';
    }, 5000);
}

// Start reconnect polling after reboot
function startReconnectPolling() {
    console.log('Starting reconnect polling...');

    const reconnectInfo = document.getElementById('reconnectInfo');
    reconnectInfo.textContent = 'Aguardando reconexão...';

    let attempts = 0;
    const maxAttempts = 20;
    const pollInterval = 3000; // 3 seconds

    const pollTimer = setInterval(async () => {
        attempts++;
        console.log(`Reconnect attempt ${attempts}/${maxAttempts}`);

        reconnectInfo.textContent = `Tentativa ${attempts}/${maxAttempts} - Aguardando dispositivo...`;

        try {
            const response = await fetch('/api/health/status', {
                method: 'GET',
                cache: 'no-cache'
            });

            if (response.ok) {
                // Device is back online!
                clearInterval(pollTimer);
                console.log('Device reconnected successfully');

                reconnectInfo.textContent = 'Dispositivo online! Redirecionando...';

                // Remove navigation blocker
                window.removeEventListener('beforeunload', preventNavigation);

                // Reload page after 2 seconds
                setTimeout(() => {
                    window.location.href = '/';
                }, 2000);
            }
        } catch (error) {
            // Device not yet available, continue polling
            console.log('Device not available yet:', error.message);
        }

        if (attempts >= maxAttempts) {
            // Timeout
            clearInterval(pollTimer);
            console.error('Reconnect timeout');

            reconnectInfo.innerHTML =
                'Tempo esgotado aguardando reconexão.<br>' +
                '<button onclick="window.location.reload()" class="reload-btn">Tentar Novamente</button>';

            // Remove navigation blocker
            window.removeEventListener('beforeunload', preventNavigation);
        }
    }, pollInterval);
}

// Show error message
function showError(message) {
    const errorSection = document.getElementById('errorSection');
    const errorBanner = document.getElementById('errorBanner');
    const errorMessage = document.getElementById('errorMessage');

    errorMessage.textContent = message;
    errorSection.style.display = 'block';

    // Determine error severity
    if (message.toLowerCase().includes('inválido') ||
        message.toLowerCase().includes('magic byte')) {
        errorBanner.className = 'error-banner critical';
    } else {
        errorBanner.className = 'error-banner warning';
    }

    // Auto-hide after 10 seconds
    setTimeout(() => {
        closeError();
    }, 10000);
}

// Close error banner
function closeError() {
    document.getElementById('errorSection').style.display = 'none';
}

// Prevent navigation during upload
function preventNavigation(e) {
    e.preventDefault();
    e.returnValue = 'Atualização de firmware em andamento. Sair desta página irá interromper a atualização.';
    return e.returnValue;
}

// Format file size
function formatSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
}
