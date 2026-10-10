let lastFileName = "";
let lastPhotoTime = 0; 
let isFetchingAI = false; 

const AYLAR = ["Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran", "Temmuz", "Ağustos", "Eylül", "Ekim", "Kasım", "Aralık"];
let currentKotalar = [0,0,0,0,0,0,0,0,0,0,0,0];
let currentEmails = [];

// ==========================================
// ARAYÜZ VE MENÜ YÖNETİMİ
// ==========================================

function aylariCiz() {
    let listDiv = document.getElementById("aylar-listesi"); 
    listDiv.innerHTML = "";
    AYLAR.forEach((ay, i) => {
        listDiv.innerHTML += `
        <div style="background:#222; padding:8px; border-radius:5px; border:1px solid #444;">
            <div style="color:#00E676; font-size:12px; margin-bottom:3px; font-weight:bold;">${ay}</div>
            <input type="number" id="kota-${i}" value="${currentKotalar[i]}" style="margin:0; padding:5px; width:100%;">
        </div>`;
    });
}

function mailListesiniCiz() {
    let listDiv = document.getElementById("mail-listesi"); 
    listDiv.innerHTML = "";
    if(currentEmails.length === 0) { 
        listDiv.innerHTML = "<div style='color:#777; font-size:12px;'>E-Posta adresi eklenmedi.</div>"; 
        return; 
    }
    currentEmails.forEach((mail, i) => {
        listDiv.innerHTML += `
        <div style="display:flex; gap:5px; margin-bottom:5px;">
            <input type="email" value="${mail}" onchange="mailGuncelle(${i}, this.value)" placeholder="ornek@gmail.com" style="margin:0; padding:8px; flex:1;">
            <button class="btn-red btn-sm" onclick="mailSil(${i})" style="margin:0; width:50px;">Sil</button>
        </div>`;
    });
}

function yeniMailEkle() { 
    currentEmails.push(""); 
    mailListesiniCiz(); 
}

function mailGuncelle(index, val) { 
    currentEmails[index] = val; 
}

function mailSil(index) { 
    currentEmails.splice(index, 1); 
    mailListesiniCiz(); 
}

// ==========================================
// YAPAY ZEKA VE ANA EKRAN GÜNCELLEYİCİ
// ==========================================

function sonucuEkranaBas(data) {
    if(data.zaman) { 
        document.getElementById("foto-tarih-etiket").innerText = data.zaman; 
    }
    
    if(data.hata && data.hata !== "false" && data.hata !== false) {
        document.getElementById("okunan-rakam").innerText = "HATA";
        document.getElementById("aylik-tuketim").innerText = data.hata;
        document.getElementById("sonuc-kutusu").style.display = "block";
        return;
    }
    
    document.getElementById("okunan-rakam").innerText = data.deger + " m³";
    document.getElementById("aylik-tuketim").innerText = (data.tuketim !== "") ? "+ " + data.tuketim + " m³" : "Fatura Girilmemiş";
    
    let detayHtml = "<div style='display:flex; justify-content:center; gap:10px; margin-top:15px; flex-wrap:wrap;'>";
    if(data.detaylar) {
        data.detaylar.forEach(d => {
            detayHtml += `
            <div style="text-align:center; background:#111; padding:8px; border-radius:5px; border:1px solid #444;">
                <img src="${d.img}" style="width:40px; height:auto; border:1px solid #777; image-rendering: pixelated;">
                <div style="font-size:20px; font-weight:bold; color:#00E676; margin-top:5px;">${d.d}</div>
                <div style="font-size:11px; color:${d.p > 85 ? '#aaa' : '#f44336'};">%${d.p}</div>
            </div>`;
        });
    }
    detayHtml += "</div>";
    
    let detayKutu = document.getElementById("ai-detaylar");
    if(!detayKutu) { 
        detayKutu = document.createElement("div"); 
        detayKutu.id = "ai-detaylar"; 
        document.getElementById("sonuc-kutusu").appendChild(detayKutu); 
    }
    detayKutu.innerHTML = detayHtml; 
    document.getElementById("sonuc-kutusu").style.display = "block";
}

setInterval(() => {
    fetch('/status')
    .then(r => r.json())
    .then(data => {
        document.getElementById('cpu-temp').innerText = data.cpu_temp + " °C"; 
        document.getElementById('wifi-rssi').innerText = data.rssi + " dBm";
        
        if(data.photo_time !== lastPhotoTime && lastPhotoTime !== 0 && !isFetchingAI) {
            document.getElementById("ana-foto").src = "/latest.jpg?t=" + new Date().getTime();
            tabloyuGetir(); 
            fetch('/config/last_result.json?t=' + new Date().getTime())
                .then(r => r.json())
                .then(aiData => { sonucuEkranaBas(aiData); })
                .catch(e=>{});
        }
        lastPhotoTime = data.photo_time;
    }).catch(e => {});
}, 5000);

window.onload = () => { 
    ayarlariGetir(); 
    fetch('/config/last_result.json?t=' + new Date().getTime())
        .then(r => r.json())
        .then(aiData => { sonucuEkranaBas(aiData); })
        .catch(e=>{});
};

function showTab(tabId) {
    document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(el => el.classList.remove('active'));
    document.getElementById(tabId).classList.add('active'); 
    event.target.classList.add('active');
    
    if(tabId === 'ayarlar' || tabId === 'kotalar') ayarlariGetir(); 
    if(tabId === 'gecmis') tabloyuGetir(); 
    if(tabId === 'kalibrasyon') kalibrasyonResmiYenile(); 
}

function fotografCek() {
    let btn = document.getElementById("cek-btn"); 
    btn.disabled = true; 
    btn.innerText = "⏳ Çekiliyor..."; 
    document.getElementById("okut-btn").style.display = "none"; 
    document.getElementById("sonuc-kutusu").style.display = "none";
    
    fetch('/capture')
    .then(res => res.json())
    .then(data => {
        let img = document.getElementById("ana-foto");
        img.onload = () => { 
            document.getElementById("foto-tarih-etiket").innerText = data.zaman; 
            lastFileName = data.filename; 
            btn.disabled = false; 
            btn.innerText = "📸 Tekrar Çek"; 
            document.getElementById("okut-btn").style.display = "block"; 
        };
        img.src = data.url + "?t=" + new Date().getTime();
    }).catch(e => { alert("Kamera hatası!"); btn.disabled = false; btn.innerText = "📸 Fotoğraf Çek"; });
}

function yapayZekayaYolla() {
    isFetchingAI = true; 
    let btn = document.getElementById("okut-btn"); 
    btn.innerText = "🧠 Motor Analiz Ediyor... (Bekleyin)"; 
    btn.disabled = true; 
    document.getElementById("cek-btn").disabled = true;
    
    fetch('/ask_ai?file=' + lastFileName)
    .then(res => res.json())
    .then(data => {
        btn.innerText = "🤖 Yapay Zekaya Okut"; 
        btn.disabled = false; 
        document.getElementById("cek-btn").disabled = false;
        sonucuEkranaBas(data); 
        isFetchingAI = false;
    }).catch(e => { 
        alert("İletişim koptu!"); 
        btn.disabled = false; 
        document.getElementById("cek-btn").disabled = false; 
        isFetchingAI = false; 
    });
}

// ==========================================
// TABLO VE GEÇMİŞ YÖNETİMİ
// ==========================================

function tabloyuGetir() {
    document.getElementById("tablo-govde").innerHTML = "<tr><td colspan='5'>Yükleniyor...</td></tr>";
    
    fetch('/history')
    .then(res => res.text())
    .then(csv => {
        let html = ""; 
        let lines = csv.split("\n");
        let lastFatura = "";
        
        for(let i = lines.length - 1; i >= 0; i--) {
            if(lines[i].trim() === "") continue; 
            
            let parts = lines[i].split(",");
            if(parts.length >= 4) {
                let faturaEnd = (parts.length >= 5) ? parts[3].trim() : "-";
                let imgPath = (parts.length >= 5) ? parts[4].trim() : parts[3].trim();
                
                let rowStyle = "";
                if(lastFatura !== "" && faturaEnd !== "-" && lastFatura !== faturaEnd) {
                    rowStyle = "style='border-top: 3px solid #00E676;'";
                }
                lastFatura = faturaEnd;

                html += `
                <tr ${rowStyle}>
                    <td>${parts[0]}</td>
                    <td style="font-weight:bold; color:#00E676;">${parts[1]}</td>
                    <td style="color:#aaa;">${faturaEnd !== "" ? faturaEnd + " m³" : "-"}</td>
                    <td style="font-weight:bold; color:#ff9800;">${parts[2]!=="-" ? "+"+parts[2] : "-"}</td>
                    <td>
                        <button onclick="resimGoster('${imgPath}', '${parts[0]} - ${parts[1]} m³')" class="btn-sm btn-blue">🖼️ Gör</button>
                        <button onclick="tabloDuzenle(${i})" class="btn-sm">✏️</button>
                        <button onclick="tabloSil(${i})" class="btn-sm btn-del">🗑️</button>
                    </td>
                </tr>`;
            }
        }
        if(html === "") html = "<tr><td colspan='5'>Henüz okuma yok.</td></tr>"; 
        document.getElementById("tablo-govde").innerHTML = html;
    });
}

function resimGoster(url, title) { 
    document.getElementById("modal-img").src = url + "?t=" + new Date().getTime(); 
    document.getElementById("modal-title").innerText = title; 
    document.getElementById("img-modal").style.display = "flex"; 
}

function modalKapat() { 
    document.getElementById("img-modal").style.display = "none"; 
}

function tabloDuzenle(index) { 
    let yeniDeger = prompt("Hatalı değeri düzeltin:"); 
    if(yeniDeger && !isNaN(yeniDeger)) { 
        fetch('/update_history?index=' + index + '&val=' + yeniDeger).then(r => tabloyuGetir()); 
    } 
}

function tabloSil(index) { 
    if(confirm("Silmek istediğinize emin misiniz?")) { 
        fetch('/delete_history?index=' + index).then(r => tabloyuGetir()); 
    } 
}

// ==========================================
// AYARLARIN KAYDEDİLMESİ VE OKUNMASI
// ==========================================

function ayarlariGetir() {
    fetch('/get_config').then(res => res.json()).then(data => {
        document.getElementById('set-ai-engine').value = data.aiEngine || "gemini"; 
        document.getElementById('set-api').value = data.apiKey || ""; 
        document.getElementById('set-autotime').value = data.autoTime || "14:00"; 
        document.getElementById('set-quality').value = data.quality || 12; 
        document.getElementById('set-flash').value = data.useFlash || 0; 
        document.getElementById('set-pwm').value = data.flashPWM || 10; 
        document.getElementById('set-wait').value = data.waitTime || 1000; 
        document.getElementById('fatura-endeks').value = data.faturaEndeks || ""; 
        document.getElementById('fatura-tarih').value = data.faturaTarih || "";
        
        document.getElementById('set-auto-fatura').checked = (data.autoUpdateFatura === 1);
        
        currentRotation = parseFloat(data.rotation || 0.0); 
        document.getElementById('rotation-slider').value = currentRotation; 
        document.getElementById('rotation-val').innerText = currentRotation.toFixed(1) + "°";
        
        document.getElementById("ana-fatura-bilgi").innerText = (data.faturaEndeks || "Girilmedi") + (data.faturaTarih ? ` (${data.faturaTarih})` : "");
        
        document.getElementById('set-webhook').value = data.webhookUrl || "";
        document.getElementById('set-mail-yuzde').value = data.mailYuzde || 90;
        
        if(data.kotalar && Array.isArray(data.kotalar)) {
            currentKotalar = data.kotalar; 
        }
        aylariCiz();
        
        if(data.mailListesi) {
            currentEmails = data.mailListesi.split(",").filter(m => m !== ""); 
        } else {
            currentEmails = []; 
        }
        mailListesiniCiz();
    });
}

function ayarlariKaydet() {
    for(let i=0; i<12; i++) { 
        currentKotalar[i] = parseInt(document.getElementById('kota-'+i).value) || 0; 
    }
    
    let temizMailler = currentEmails.filter(m => m.trim() !== "").join(",");
    
    let config = {
        aiEngine: document.getElementById('set-ai-engine').value, 
        apiKey: document.getElementById('set-api').value, 
        autoTime: document.getElementById('set-autotime').value, 
        framesize: 6, 
        quality: parseInt(document.getElementById('set-quality').value), 
        useFlash: parseInt(document.getElementById('set-flash').value), 
        flashPWM: parseInt(document.getElementById('set-pwm').value), 
        waitTime: parseInt(document.getElementById('set-wait').value), 
        rotation: currentRotation, 
        faturaEndeks: document.getElementById('fatura-endeks').value, 
        faturaTarih: document.getElementById('fatura-tarih').value,
        autoUpdateFatura: document.getElementById('set-auto-fatura').checked ? 1 : 0,
        kotalar: currentKotalar, 
        webhookUrl: document.getElementById('set-webhook').value, 
        mailYuzde: parseInt(document.getElementById('set-mail-yuzde').value), 
        mailListesi: temizMailler
    };
    
    fetch('/set_config', { method: 'POST', body: JSON.stringify(config) })
    .then(res => { 
        alert("Tüm Ayarlar ve Kotalar Kaydedildi!"); 
        ayarlariGetir(); 
    });
}

// ==========================================
// KALİBRASYON (SÜRÜKLE & BIRAK VE IZGARA)
// ==========================================

let canvas = document.getElementById('roi-canvas'); 
let ctx = canvas.getContext('2d'); 
let bgImage = new Image(); 
let rois = []; 
let draggingIndex = -1; 
let dragOffsetX = 0; 
let dragOffsetY = 0; 
let currentRotation = 0.0; 
let showGrid = true;

function aciDegisti(val) { 
    currentRotation = parseFloat(val); 
    document.getElementById('rotation-val').innerText = currentRotation.toFixed(1) + "°"; 
    cizimleriEkranaBas(); 
}

function aciSifirla() { 
    currentRotation = 0.0; 
    document.getElementById('rotation-slider').value = 0; 
    document.getElementById('rotation-val').innerText = "0.0°"; 
    cizimleriEkranaBas(); 
}

function gridGosterAçKapa() { 
    showGrid = !showGrid; 
    cizimleriEkranaBas(); 
}

function kalibrasyonResmiYenile() { 
    bgImage.onload = () => { 
        canvas.width = bgImage.naturalWidth; 
        canvas.height = bgImage.naturalHeight; 
        roisGetir(); 
    }; 
    bgImage.src = "/config/reference.jpg?t=" + new Date().getTime(); 
}

function kalibrasyonReferansCek() { 
    let btn = document.getElementById("ref-btn"); 
    btn.innerText = "⏳ Referans Sabitleniyor..."; 
    btn.disabled = true; 
    fetch('/capture_reference')
    .then(res => res.json())
    .then(data => { 
        kalibrasyonResmiYenile(); 
        btn.innerText = "📸 Yeni Referans Sabitlendi"; 
        btn.disabled = false; 
    }).catch(e => { 
        alert("Hata!"); 
        btn.innerText = "📸 Yeni Referans Fotoğrafı Sabitle"; 
        btn.disabled = false; 
    }); 
}

function roisGetir() { 
    fetch('/config/roi.json?t='+new Date().getTime())
    .then(res => res.json())
    .then(data => { 
        if(Array.isArray(data)) rois = data; 
        kutuListesiniCiz(); 
        cizimleriEkranaBas(); 
    }).catch(e => { 
        rois = []; 
        kutuListesiniCiz(); 
        cizimleriEkranaBas(); 
    }); 
}

function getGercekKoordinat(evt) { 
    let rect = canvas.getBoundingClientRect(); 
    let scaleX = canvas.width / rect.width; 
    let scaleY = canvas.height / rect.height; 
    let clientX = evt.touches ? evt.touches[0].clientX : evt.clientX; 
    let clientY = evt.touches ? evt.touches[0].clientY : evt.clientY; 
    return { 
        x: (clientX - rect.left) * scaleX, 
        y: (clientY - rect.top) * scaleY 
    }; 
}

canvas.addEventListener('mousedown', dragBasla); 
canvas.addEventListener('touchstart', dragBasla, {passive: false}); 
canvas.addEventListener('mousemove', dragDevam); 
canvas.addEventListener('touchmove', dragDevam, {passive: false}); 
canvas.addEventListener('mouseup', dragBitir); 
canvas.addEventListener('touchend', dragBitir); 
canvas.addEventListener('mouseleave', dragBitir);

function dragBasla(e) { 
    if(e.cancelable) e.preventDefault(); 
    let pos = getGercekKoordinat(e); 
    draggingIndex = -1; 
    for(let i = rois.length - 1; i >= 0; i--) { 
        let r = rois[i]; 
        if(pos.x >= r.x && pos.x <= r.x + r.w && pos.y >= r.y && pos.y <= r.y + r.h) { 
            draggingIndex = i; 
            dragOffsetX = pos.x - r.x; 
            dragOffsetY = pos.y - r.y; 
            canvas.style.cursor = "grabbing"; 
            break; 
        } 
    } 
}

function dragDevam(e) { 
    if (draggingIndex === -1) return; 
    if(e.cancelable) e.preventDefault(); 
    let pos = getGercekKoordinat(e); 
    rois[draggingIndex].x = Math.round(pos.x - dragOffsetX); 
    rois[draggingIndex].y = Math.round(pos.y - dragOffsetY); 
    cizimleriEkranaBas(); 
    kutuListesiniCiz(); 
}

function dragBitir(e) { 
    draggingIndex = -1; 
    canvas.style.cursor = "grab"; 
}

function cizimleriEkranaBas() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Resim Döndürme
    if(bgImage.src) { 
        ctx.save(); 
        ctx.translate(canvas.width / 2, canvas.height / 2); 
        ctx.rotate(currentRotation * Math.PI / 180); 
        ctx.drawImage(bgImage, -canvas.width / 2, -canvas.height / 2, canvas.width, canvas.height); 
        ctx.restore(); 
    }
    
    // Izgara (Grid) Çizimi
    if(showGrid) { 
        ctx.strokeStyle = "rgba(255, 255, 255, 0.15)"; 
        ctx.lineWidth = 1; 
        ctx.setLineDash([2, 2]); 
        for(let x = 0; x < canvas.width; x += 30) { 
            ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, canvas.height); ctx.stroke(); 
        } 
        for(let y = 0; y < canvas.height; y += 30) { 
            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(canvas.width, y); ctx.stroke(); 
        } 
        ctx.setLineDash([]); 
    }
    
    // ROI Kutularının Çizilmesi
    rois.forEach((roi, index) => { 
        let isSelected = (index === draggingIndex);
        
        // Dış Kırmızı Kutu
        ctx.beginPath(); 
        ctx.lineWidth = isSelected ? 3 : 2; 
        ctx.strokeStyle = isSelected ? "#00E676" : "red"; 
        ctx.rect(roi.x, roi.y, roi.w, roi.h); 
        ctx.stroke(); 
        
        // İç Yeşil Kesik Çizgili Kutu (Kılavuz)
        ctx.beginPath(); 
        ctx.lineWidth = 1; 
        ctx.strokeStyle = "rgba(0, 230, 118, 0.9)"; 
        ctx.setLineDash([4, 4]); 
        let innerX = roi.x + (roi.w * 0.15); 
        let innerY = roi.y + (roi.h * 0.20); 
        let innerW = roi.w * 0.70; 
        let innerH = roi.h * 0.60; 
        ctx.rect(innerX, innerY, innerW, innerH); 
        ctx.stroke(); 
        ctx.setLineDash([]); 
        
        // Metin (R1, R2 vb.)
        ctx.font = "bold 18px Arial"; 
        ctx.fillStyle = "black"; 
        ctx.fillText("R" + (index + 1), roi.x + 2, roi.y - 4); 
        ctx.fillStyle = isSelected ? "#00E676" : "red"; 
        ctx.fillText("R" + (index + 1), roi.x, roi.y - 6); 
    });
}

function kutuListesiniCiz() {
    let listDiv = document.getElementById("kutu-listesi"); 
    listDiv.innerHTML = ""; 
    
    if(rois.length === 0) { 
        listDiv.innerHTML = "<div style='color:#777; font-size:12px;'>Kutu yok. Ekle butonuna basın.</div>"; 
        return; 
    }
    
    rois.forEach((roi, index) => { 
        listDiv.innerHTML += `
        <div style="display:flex; gap:5px; align-items:center; margin-bottom:8px; background:#222; padding:8px; border-radius:5px;">
            <strong style="color:#00E676; width:25px;">R${index+1}</strong> 
            X:<input type="number" value="${roi.x}" onchange="roiGuncelle(${index}, 'x', this.value)" style="width:55px; margin:0; padding:5px; font-size:12px;"> 
            Y:<input type="number" value="${roi.y}" onchange="roiGuncelle(${index}, 'y', this.value)" style="width:55px; margin:0; padding:5px; font-size:12px;"> 
            W:<input type="number" value="${roi.w}" onchange="roiGuncelle(${index}, 'w', this.value)" style="width:55px; margin:0; padding:5px; font-size:12px;"> 
            H:<input type="number" value="${roi.h}" onchange="roiGuncelle(${index}, 'h', this.value)" style="width:55px; margin:0; padding:5px; font-size:12px;"> 
            <button class="btn-red" onclick="kutuSil(${index})" style="padding:5px 10px; margin:0;">Sil</button>
        </div>`; 
    });
}

function yeniKutuEkle() { 
    let w = 34; 
    let h = 54; 
    let x = 100 + (rois.length * 40); 
    let y = 100; 
    rois.push({x: x, y: y, w: w, h: h}); 
    cizimleriEkranaBas(); 
    kutuListesiniCiz(); 
}

function roiGuncelle(index, prop, val) { 
    let parsed = parseInt(val); 
    if(!isNaN(parsed)) { 
        rois[index][prop] = parsed; 
        cizimleriEkranaBas(); 
    } 
}

function kutuSil(index) { 
    rois.splice(index, 1); 
    cizimleriEkranaBas(); 
    kutuListesiniCiz(); 
}

function koordinatlariKaydet() { 
    if(rois.length === 0) { 
        alert("Kaydedilecek kutu yok!"); 
        return; 
    } 
    fetch('/set_roi', { method: 'POST', body: JSON.stringify(rois) })
    .then(res => { 
        ayarlariKaydet(); 
    }); 
}