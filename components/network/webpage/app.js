let API_URL = "http://192.168.0.1"; // AP address


// OTA elements
const fileInput = document.getElementById("selected_file");
const btnUpdate = document.getElementById("btnUpdate");
const fileInfo = document.getElementById("file_info");
const otaStatus = document.getElementById("ota_update_status");

// Network elements
const btnGetNetwork = document.getElementById("btnGetNetwork");
const btnSaveNetwork = document.getElementById("btnSaveNetwork");
const ssidInput = document.getElementById("ssid");
const passwordInput = document.getElementById("password");
const networkStatus = document.getElementById("network_status");

const otaProgress = document.getElementById("ota_progress");

function updateProgress(event) {
  if (event.lengthComputable) {
    const percent = Math.round((event.loaded / event.total) * 100);
    otaProgress.style.display = "block";
    otaProgress.value = percent;

    otaStatus.textContent = `Uploading file... ${percent}%`;

    if (percent === 100) {
      otaStatus.textContent = "Upload complete. Verifying...";
    }

    getUpdateStatus();
  }
}


// ===== TABS =====
document.querySelectorAll(".tab-btn").forEach(btn => {
  btn.addEventListener("click", () => {
    const tabId = btn.dataset.tab;

    document.querySelectorAll(".tab").forEach(tab => tab.classList.remove("active"));
    document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));

    document.getElementById(tabId).classList.add("active");
    btn.classList.add("active");

    if (tabId === "logbook") {
      loadLogBook();
    }

	if (tabId === "identity") {
	  loadIdentity();

	  // force default subtab
	  document.querySelectorAll(".subtab").forEach(t => t.classList.remove("active"));
	  document.querySelectorAll(".subtab-btn").forEach(b => b.classList.remove("active"));

	  document.querySelector('[data-subtab="crew"]')?.classList.add("active");
	  document.getElementById("crew-tab")?.classList.add("active");
	}
  });
});

// ===== OTA UPDATE =====
let seconds = null, otaTimerVar = null;

fileInput.addEventListener("change", () => {
  if (fileInput.files.length > 0) {
    const file = fileInput.files[0];
    fileInfo.innerHTML = `<h4>File: ${file.name}<br>Size: ${file.size} bytes</h4>`;
  }
});

btnUpdate.addEventListener("click", () => {
  if (fileInput.files.length === 0) { 
    alert("Select a file first"); 
    return; 
  }

  const file = fileInput.files[0];
  const formData = new FormData();
  formData.set("file", file, file.name);

  otaStatus.textContent = `Uploading ${file.name}, Firmware Update in Progress...`;

  const xhr = new XMLHttpRequest();
  xhr.upload.addEventListener("progress", updateProgress);
  xhr.open("POST", `${API_URL}/api/OTA/update`);
  xhr.responseType = "blob";
  xhr.send(formData);
});


function getUpdateStatus() {
  const xhr = new XMLHttpRequest();
  xhr.open("POST", `${API_URL}/api/OTA/status`);

  xhr.onload = () => {
    if (xhr.status === 200) {
      const resp = JSON.parse(xhr.responseText);

      if (resp.ota_update_status === 1) {
        seconds = 10;
        otaRebootTimer();
      }
      else if (resp.ota_update_status === -1) {
        otaStatus.textContent = "!!! Upload Error !!!";
      }
    }
  };

  xhr.send("ota_update_status");
}


function otaRebootTimer() {
  otaProgress.style.display = "none"; 
  otaStatus.textContent = `OTA Firmware Update Complete. Rebooting in: ${seconds}`;
  
  if (--seconds === 0) { 
    clearTimeout(otaTimerVar); 
    window.location.reload(); 
  }
  else { 
    otaTimerVar = setTimeout(otaRebootTimer, 1000); 
  }
}


// ===== NETWORK CONFIG =====
const staIpContainer = document.createElement("div"); // container for STA IP
staIpContainer.id = "sta-ip-container";
staIpContainer.style.marginBottom = "10px";

const networkSection = document.querySelector(".network-section");
networkSection.prepend(staIpContainer);


btnGetNetwork.addEventListener("click", async () => {
  try {
    const res = await fetch(`${API_URL}/api/config/network`);
    if (res.ok) {
      const data = await res.json();
      ssidInput.value = data.ssid || "";
      passwordInput.value = data.password || "";
      networkStatus.textContent = "Data loaded ✅";
    }
  } catch (e) { 
    networkStatus.textContent = "Load error ❌"; 
    console.error(e); 
  }
});

btnSaveNetwork.addEventListener("click", async () => {
  const ssid = ssidInput.value;
  const password = passwordInput.value;

  try {
    const res = await fetch(`${API_URL}/api/config/network`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ssid, password })
    });

    if (res.ok) networkStatus.textContent = "Saved ✅";
    else networkStatus.textContent = "Save error ❌";

  } catch (e) { 
    networkStatus.textContent = "Save error ❌"; 
    console.error(e); 
  }
});


async function updateStaIpLink() {
  // Only show STA IP link if we're currently on AP IP
  if (API_URL === "http://192.168.0.1") {
    try {
      const res = await fetch(`${API_URL}/api/config/ip_addr`);
      if (res.ok) {
        const data = await res.json();
        if (data.ip && data.ip !== "0.0.0.0") {
          staIpContainer.innerHTML = `
            <label>Device STA IP: </label>
            <a href="http://${data.ip}" target="_blank">${data.ip}</a>
          `;
          return;
        }
      }
      staIpContainer.textContent = "STA IP unavailable";
    } catch (e) {
      staIpContainer.textContent = "";
      console.error(e);
    }
  } else {
    // Hide if we're not on AP
    staIpContainer.innerHTML = "";
  }
}

// Run on startup
updateStaIpLink();


// ===== IMPROVED & MORE STABLE FAST STA DISCOVERY =====
async function discoverStaApiUrl() {
  const subnets = [
    { base: "192.168.0.", start: 1, end: 200 },
    { base: "172.20.10.", start: 2, end: 14 }
  ];

  const timeout = 600;
  const batchSize = 50;
  const retryDelay = 300;

  console.log("Starting IMPROVED FAST STA discovery...");

  async function probeIp(testIp) {
    try {
      const controller = new AbortController();
      const timer = setTimeout(() => controller.abort(), timeout);

      const res = await fetch(`http://${testIp}/api/config/ip_addr`, {
        signal: controller.signal
      }).catch(() => null);

      clearTimeout(timer);

      if (res && res.ok) {
        const data = await res.json().catch(() => null);
        if (data && data.ip && data.ip !== "0.0.0.0") {
          return testIp;
        }
      }
    } catch (e) {}

    return null;
  }

  // run until found
  while (true) {
    for (const subnet of subnets) {
      console.log("Scanning subnet...");

      for (let start = subnet.start; start <= subnet.end; start += batchSize) {
        const batch = [];

        for (let i = start; i < start + batchSize && i <= subnet.end; i++) {
          batch.push(probeIp(`${subnet.base}${i}`));
        }

        const results = await Promise.all(batch);
        const hit = results.find(ip => ip !== null);

        if (hit) {
          console.log("FOUND STA device at:", hit);
          API_URL = `http://${hit}`;
          return hit;
        }
      }
    }

    console.log("Not found — retrying...");
    await new Promise(r => setTimeout(r, retryDelay));
  }
}

// run at startup
discoverStaApiUrl();

// ===== DEFAULT TAB (Identity) =====
window.addEventListener("DOMContentLoaded", () => {
  const defaultTab = document.querySelector('[data-tab="identity"]');
  const defaultSection = document.getElementById("identity");

  document.querySelectorAll(".tab").forEach(tab => tab.classList.remove("active"));
  document.querySelectorAll(".tab-btn").forEach(btn => btn.classList.remove("active"));

  if (defaultTab && defaultSection) {
    defaultTab.classList.add("active");
    defaultSection.classList.add("active");
  }
});

async function loadIdentity() {
  try {
    const res = await fetch(`${API_URL}/api/config/identity`);
    if (!res.ok) return;

    const data = await res.json();

    // CREW
    document.getElementById("pilot").value = data.pilot || "";
    document.getElementById("crew").value = data.crew || "";
    document.getElementById("reg").value = data.reg || "";
    document.getElementById("base").value = data.base || "";

    // PLANE
    document.getElementById("manuf").value = data.manuf || "";
    document.getElementById("model").value = data.model || "";
    document.getElementById("type").value = data.type || "";
    document.getElementById("sn").value = data.sn || "";

  } catch (e) {
    console.error("Identity load failed", e);
  }
}
async function loadLogBook() {
  const container = document.getElementById("logbook_entries");
  container.innerHTML = "Loading logbook...";

  try {
    const res = await fetch(`${API_URL}/api/logbook`);
    if (!res.ok) {
      container.innerHTML = "Failed to load logbook";
      return;
    }

    const data = await res.json();

    if (!data.logbook || data.logbook.length === 0) {
      container.innerHTML = "No flights recorded";
      return;
    }

    container.innerHTML = "";

    data.logbook.forEach((flight) => {
      const entry = document.createElement("div");
      entry.className = "log-entry";

      entry.innerHTML = `
        <div class="log-row">
          <span class="log-date">📅 ${flight.date}</span>
          <span class="log-duration">⏱ ${flight.duration}</span>
        </div>

        <div class="log-aircraft">🛩 ${flight.aircraft}</div>
        <div class="log-remarks">"${flight.remarks}"</div>
      `;

      container.appendChild(entry);
    });

  } catch (e) {
    console.error(e);
    container.innerHTML = "Error loading logbook";
  }
}