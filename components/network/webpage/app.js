let API_URL = "http://192.168.0.1"; // AP adress


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

    otaStatus.textContent = `Wysyłanie pliku... ${percent}%`;

    if (percent === 100) {
      otaStatus.textContent = "Upload zakończony. Trwa weryfikacja...";
    }

    getUpdateStatus();
  }
}


// ===== TABS =====
document.querySelectorAll(".tab-btn").forEach(btn => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(tab => tab.classList.remove("active"));
    document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
    document.getElementById(btn.dataset.tab).classList.add("active");
    btn.classList.add("active");
  });
});


// ===== OTA UPDATE =====
let seconds=null, otaTimerVar=null;

fileInput.addEventListener("change", () => {
  if(fileInput.files.length>0){
    const file = fileInput.files[0];
    fileInfo.innerHTML = `<h4>Plik: ${file.name}<br>Rozmiar: ${file.size} bajtów</h4>`;
  }
});

btnUpdate.addEventListener("click", ()=>{
  if(fileInput.files.length===0){ alert("Wybierz plik najpierw"); return; }
  const file = fileInput.files[0];
  const formData = new FormData();
  formData.set("file", file, file.name);

  otaStatus.textContent = `Uploading ${file.name}, Firmware Update in Progress...`;
  const xhr = new XMLHttpRequest();
  xhr.upload.addEventListener("progress", updateProgress);
  xhr.open("POST", `${API_URL}/api/OTA/update`);
  xhr.responseType="blob";
  xhr.send(formData);
});


function getUpdateStatus(){
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


function otaRebootTimer(){
  otaProgress.style.display = "none"; 
  otaStatus.textContent=`OTA Firmware Update Complete. Rebooting in: ${seconds}`;
  if(--seconds===0){ clearTimeout(otaTimerVar); window.location.reload(); }
  else{ otaTimerVar=setTimeout(otaRebootTimer,1000); }
}

// ===== NETWORK CONFIG =====
const staIpContainer = document.createElement("div"); // container for STA IP
staIpContainer.id = "sta-ip-container";
staIpContainer.style.marginBottom = "10px"; // optional styling
const networkSection = document.querySelector(".network-section");
networkSection.prepend(staIpContainer); // add at the top


btnGetNetwork.addEventListener("click", async () => {
  try{
    const res = await fetch(`${API_URL}/api/config/network`);
    if(res.ok){
      const data = await res.json();
      ssidInput.value=data.ssid||"";
      passwordInput.value=data.password||"";
      networkStatus.textContent="Dane pobrane ✅";
    }
  } catch(e){ networkStatus.textContent="Błąd pobrania ❌"; console.error(e); }
});

btnSaveNetwork.addEventListener("click", async ()=>{
  const ssid = ssidInput.value;
  const password = passwordInput.value;
  try{
    const res = await fetch(`${API_URL}/api/config/network`, {
      method:"POST",
      headers:{ "Content-Type":"application/json" },
      body: JSON.stringify({ssid,password})
    });
    if(res.ok) networkStatus.textContent="Zapisano ✅";
    else networkStatus.textContent="Błąd zapisu ❌";
  } catch(e){ networkStatus.textContent="Błąd zapisu ❌"; console.error(e); }
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
          return hit;    // END – device found
        }
      }
    }

    console.log("Not found — retrying...");
    await new Promise(r => setTimeout(r, retryDelay));
  }
}


// run at startup
discoverStaApiUrl();

