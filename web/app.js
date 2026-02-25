// ==========================================
//  ESP32 Farm Dashboard — JavaScript
// ==========================================

const API_BASE = 'api';
let currentPage = 0;
const PAGE_SIZE = 20;
let soilChart = null;

// ---- Init ----
document.addEventListener('DOMContentLoaded', () => {
  loadFarmers();
  loadReadings();
  loadSmsSettings();
  checkSyncStatus();
  // Poll sync status every 15 seconds
  setInterval(checkSyncStatus, 15000);
});

// ==========================================
//  FARMERS
// ==========================================
async function loadFarmers(search = '') {
  try {
    const params = search ? `?search=${encodeURIComponent(search)}` : '';
    const res = await fetch(`${API_BASE}/farmers.php${params}`);
    const data = await res.json();

    if (data.success) {
      document.getElementById('stat-farmers').textContent = data.total;
      renderFarmersTable(data.farmers);
      populateFarmerFilter(data.farmers);
    }
  } catch (err) {
    console.error('Failed to load farmers:', err);
  }
}

function renderFarmersTable(farmers) {
  const tbody = document.getElementById('farmers-tbody');

  if (farmers.length === 0) {
    tbody.innerHTML = '<tr><td colspan="4" class="loading">No farmers registered yet</td></tr>';
    return;
  }

  tbody.innerHTML = farmers.map(f => `
    <tr onclick="filterByFarmer('${f.farmer_id}')" style="cursor:pointer">
      <td><strong>${f.farmer_id}</strong></td>
      <td>${f.phone_number}</td>
      <td>${f.reading_count || 0}</td>
      <td>${f.last_reading || '—'}</td>
    </tr>
  `).join('');
}

function populateFarmerFilter(farmers) {
  const select = document.getElementById('chart-farmer-filter');
  // Keep the "All Farmers" option
  select.innerHTML = '<option value="">All Farmers</option>';
  farmers.forEach(f => {
    select.innerHTML += `<option value="${f.farmer_id}">ID: ${f.farmer_id}</option>`;
  });
}

function searchFarmers() {
  const search = document.getElementById('farmer-search').value;
  loadFarmers(search);
}

function filterByFarmer(farmerId) {
  document.getElementById('chart-farmer-filter').value = farmerId;
  currentPage = 0;
  loadReadings();
}

// ==========================================
//  READINGS
// ==========================================
async function loadReadings() {
  try {
    const farmerId = document.getElementById('chart-farmer-filter').value;
    const params = new URLSearchParams({
      limit: PAGE_SIZE,
      offset: currentPage * PAGE_SIZE
    });
    if (farmerId) params.append('farmer_id', farmerId);

    const res = await fetch(`${API_BASE}/readings.php?${params}`);
    const data = await res.json();

    if (data.success) {
      document.getElementById('stat-readings').textContent = data.total;

      // Update stat cards
      if (data.stats) {
        const temp = data.stats.avg_temperature;
        const ph = data.stats.avg_ph;
        document.getElementById('stat-temp').textContent =
          temp ? parseFloat(temp).toFixed(1) + '°C' : '--°C';
        document.getElementById('stat-ph').textContent =
          ph ? parseFloat(ph).toFixed(1) : '--';
      }

      renderReadingsTable(data.readings);
      updateChart(data.readings);
      updatePagination(data.total);
    }
  } catch (err) {
    console.error('Failed to load readings:', err);
  }
}

function renderReadingsTable(readings) {
  const tbody = document.getElementById('readings-tbody');

  if (readings.length === 0) {
    tbody.innerHTML = '<tr><td colspan="9" class="loading">No readings yet. Sync data from the ESP32 device.</td></tr>';
    return;
  }

  tbody.innerHTML = readings.map(r => `
    <tr>
      <td><strong>${r.farmer_id}</strong></td>
      <td>${r.reading_timestamp}</td>
      <td>${fmtVal(r.humidity, '%')}</td>
      <td>${fmtVal(r.temperature, '°C')}</td>
      <td>${fmtVal(r.ec, '')}</td>
      <td class="${phClass(r.ph)}">${fmtVal(r.ph, '')}</td>
      <td>${fmtVal(r.nitrogen, '')}</td>
      <td>${fmtVal(r.phosphorus, '')}</td>
      <td>${fmtVal(r.potassium, '')}</td>
    </tr>
  `).join('');
}

function fmtVal(val, unit) {
  if (val === null || val === undefined) return '—';
  return parseFloat(val).toFixed(1) + unit;
}

function phClass(ph) {
  if (ph === null) return '';
  ph = parseFloat(ph);
  if (ph >= 6.0 && ph <= 7.5) return 'val-good';
  if (ph >= 5.5 && ph <= 8.0) return 'val-warn';
  return 'val-bad';
}

function updatePagination(total) {
  const totalPages = Math.max(1, Math.ceil(total / PAGE_SIZE));
  document.getElementById('page-info').textContent =
    `Page ${currentPage + 1} of ${totalPages}`;
}

function prevPage() {
  if (currentPage > 0) {
    currentPage--;
    loadReadings();
  }
}

function nextPage() {
  currentPage++;
  loadReadings();
}

// ==========================================
//  CHART
// ==========================================
function updateChart(readings) {
  const ctx = document.getElementById('soilChart').getContext('2d');

  // Reverse to show chronological order
  const reversed = [...readings].reverse().slice(-20);

  const labels = reversed.map((r, i) => r.reading_timestamp || `#${i + 1}`);

  const datasets = [
    { label: 'pH', data: reversed.map(r => r.ph), borderColor: '#ef4444', backgroundColor: 'rgba(239,68,68,0.1)' },
    { label: 'Humidity %', data: reversed.map(r => r.humidity), borderColor: '#3b82f6', backgroundColor: 'rgba(59,130,246,0.1)' },
    { label: 'Temp °C', data: reversed.map(r => r.temperature), borderColor: '#f59e0b', backgroundColor: 'rgba(245,158,11,0.1)' },
    { label: 'N mg/kg', data: reversed.map(r => r.nitrogen), borderColor: '#10b981', backgroundColor: 'rgba(16,185,129,0.1)' },
    { label: 'P mg/kg', data: reversed.map(r => r.phosphorus), borderColor: '#6366f1', backgroundColor: 'rgba(99,102,241,0.1)' },
    { label: 'K mg/kg', data: reversed.map(r => r.potassium), borderColor: '#06b6d4', backgroundColor: 'rgba(6,182,212,0.1)' },
  ];

  if (soilChart) {
    soilChart.destroy();
  }

  soilChart = new Chart(ctx, {
    type: 'line',
    data: { labels, datasets },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { mode: 'index', intersect: false },
      plugins: {
        legend: {
          position: 'top',
          labels: {
            color: '#94a3b8',
            font: { size: 11, family: 'Inter' },
            usePointStyle: true,
            pointStyle: 'circle',
            padding: 16
          }
        }
      },
      scales: {
        x: {
          ticks: { color: '#64748b', font: { size: 10 }, maxRotation: 45 },
          grid: { color: 'rgba(148,163,184,0.06)' }
        },
        y: {
          ticks: { color: '#64748b', font: { size: 10 } },
          grid: { color: 'rgba(148,163,184,0.06)' }
        }
      },
      elements: {
        line: { tension: 0.35, borderWidth: 2 },
        point: { radius: 3, hoverRadius: 6 }
      }
    }
  });
}

// ==========================================
//  SYNC
// ==========================================
async function checkSyncStatus() {
  try {
    const res = await fetch(`${API_BASE}/trigger_sync.php`);
    const data = await res.json();

    const dot = document.querySelector('.sync-dot');
    const text = document.getElementById('sync-text');

    if (data.sync_pending) {
      dot.className = 'sync-dot pending';
      text.textContent = 'Sync pending...';
    } else if (data.last_sync) {
      dot.className = 'sync-dot active';
      text.textContent = 'Last: ' + data.last_sync.completed_at;
    } else {
      dot.className = 'sync-dot';
      text.textContent = 'No sync yet';
    }
  } catch (err) {
    console.error('Sync status check failed:', err);
  }
}

async function triggerSync() {
  const btn = document.getElementById('sync-btn');
  btn.disabled = true;
  btn.textContent = 'Requesting...';

  try {
    const res = await fetch(`${API_BASE}/trigger_sync.php`, { method: 'POST' });
    const data = await res.json();

    if (data.success) {
      showToast('Sync request sent! Waiting for ESP32 to connect.', 'info');
      checkSyncStatus();
    } else {
      showToast('Failed: ' + (data.message || 'Unknown error'), 'error');
    }
  } catch (err) {
    showToast('Network error: ' + err.message, 'error');
  }

  btn.disabled = false;
  btn.innerHTML = `
    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2.5">
      <path d="M23 4v6h-6"/><path d="M1 20v-6h6"/>
      <path d="M3.51 9a9 9 0 0114.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0020.49 15"/>
    </svg>
    Sync Now`;
}

// ==========================================
//  TOAST NOTIFICATION
// ==========================================
function showToast(message, type = 'info') {
  // Remove existing toast
  const existing = document.querySelector('.toast');
  if (existing) existing.remove();

  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.textContent = message;
  document.body.appendChild(toast);

  // Trigger animation
  requestAnimationFrame(() => toast.classList.add('show'));

  // Auto remove
  setTimeout(() => {
    toast.classList.remove('show');
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// ==========================================
//  SMS SETTINGS
// ==========================================
async function loadSmsSettings() {
  try {
    const res = await fetch(`${API_BASE}/sms_settings.php`);
    const data = await res.json();

    if (data.success && data.settings) {
      const toggle = document.getElementById('sms-enabled-toggle');
      const textarea = document.getElementById('sms-template');
      const statusText = document.getElementById('sms-status-text');

      toggle.checked = data.settings.sms_enabled;
      textarea.value = data.settings.message_template || '';
      statusText.textContent = data.settings.sms_enabled ? 'Enabled' : 'Disabled';
      statusText.style.color = data.settings.sms_enabled ? 'var(--accent-green)' : 'var(--text-muted)';
      updateCharCount();
    }
  } catch (err) {
    console.error('Failed to load SMS settings:', err);
  }
}

async function saveSmsSettings() {
  const btn = document.getElementById('save-sms-btn');
  const enabled = document.getElementById('sms-enabled-toggle').checked;
  const template = document.getElementById('sms-template').value.trim();

  if (!template) {
    showToast('Message template cannot be empty', 'error');
    return;
  }

  btn.disabled = true;
  btn.textContent = 'Saving...';

  try {
    const res = await fetch(`${API_BASE}/sms_settings.php`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        sms_enabled: enabled,
        message_template: template
      })
    });
    const data = await res.json();

    if (data.success) {
      showToast('SMS settings saved! Changes will apply on next ESP32 sync.', 'success');
    } else {
      showToast('Failed: ' + (data.message || 'Unknown error'), 'error');
    }
  } catch (err) {
    showToast('Network error: ' + err.message, 'error');
  }

  btn.disabled = false;
  btn.innerHTML = `
    <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2.5">
      <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/>
      <polyline points="17 21 17 13 7 13 7 21"/>
      <polyline points="7 3 7 8 15 8"/>
    </svg>
    Save Settings`;
}

function toggleSmsStatus() {
  const toggle = document.getElementById('sms-enabled-toggle');
  const statusText = document.getElementById('sms-status-text');
  statusText.textContent = toggle.checked ? 'Enabled' : 'Disabled';
  statusText.style.color = toggle.checked ? 'var(--accent-green)' : 'var(--text-muted)';
}

function insertPlaceholder(name) {
  const textarea = document.getElementById('sms-template');
  const start = textarea.selectionStart;
  const end = textarea.selectionEnd;
  const text = textarea.value;
  const placeholder = `{${name}}`;

  textarea.value = text.substring(0, start) + placeholder + text.substring(end);
  textarea.selectionStart = textarea.selectionEnd = start + placeholder.length;
  textarea.focus();
  updateCharCount();
}

function updateCharCount() {
  const textarea = document.getElementById('sms-template');
  const count = document.getElementById('sms-char-count');
  const len = textarea.value.length;
  count.textContent = `${len} chars`;
  // SMS is 160 chars; warn if template is long (placeholders expand)
  count.style.color = len > 300 ? 'var(--accent-red)' : 'var(--text-muted)';
}

// Attach char count listener
document.addEventListener('DOMContentLoaded', () => {
  const textarea = document.getElementById('sms-template');
  if (textarea) {
    textarea.addEventListener('input', updateCharCount);
  }
});

