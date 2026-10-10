function doPost(e) {
  try {
    var data = JSON.parse(e.postData.contents);
    var mailler = data.mailler;
    var konu = "⚠️ DİKKAT: Doğalgaz Kota Uyarısı!";
    
    var htmlBody = "<div style='font-family:Arial,sans-serif; background:#1e1e1e; color:#fff; padding:20px; border-radius:10px;'>" +
      "<h2 style='color:#ff9800;'>🔥 Doğalgaz Kota Sınırı Aşıldı!</h2>" +
      "<p>Merhaba,</p><p>Aylık doğalgaz tüketiminiz belirlenen kota sınırına ulaşmıştır:</p>" +
      "<ul style='line-height:1.8;'>" +
      "<li><b>Güncel Endeks:</b> " + data.endeks + " m³</li>" +
      "<li><b>Bu Ayki Tüketim:</b> " + data.tuketim + " m³</li>" +
      "<li><b>Aylık Kota:</b> " + data.kota + " m³</li>" +
      "<li><b>Uyarı Sınırı (% " + data.yuzde + "):</b> " + data.sinir + " m³</li>" +
      "</ul>";

    var inlineImages = {};
    if (data.imgBase64 && data.imgBase64.length > 100) {
      var imgBlob = Utilities.newBlob(Utilities.base64Decode(data.imgBase64), "image/jpeg", "sayac.jpg");
      inlineImages["sayacFoto"] = imgBlob;
      htmlBody += "<br><h3>📸 Çekilen Sayaç Fotoğrafı:</h3><img src='cid:sayacFoto' style='max-width:100%; border:2px solid #00E676; border-radius:8px;'><br>";
    }
    
    htmlBody += "<br><p style='color:#aaa; font-size:12px;'>ESP32 Akıllı Sayaç Sisteminiz</p></div>";

    MailApp.sendEmail({
      to: mailler,
      subject: konu,
      htmlBody: htmlBody,
      inlineImages: inlineImages
    });

    return ContentService.createTextOutput("Mail ve Resim Gönderildi.");
  } catch(err) {
    return ContentService.createTextOutput("Hata: " + err.toString());
  }
}
