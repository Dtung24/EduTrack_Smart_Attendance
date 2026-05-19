#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <time.h> 

// ── Thư viện WiFiManager (Tự động kết nối/Cài đặt WiFi) ────
#include <WiFiManager.h>

// ── Pin config ─────────────────────────────────────────────
#define SCK  7
#define MISO 9
#define MOSI 11
#define SS   5
#define RST  33
#define BUZZ 35

// LINK Firebase
#define FIREBASE_URL "https://edutrack-6db11-default-rtdb.firebaseio.com/attendance_logs.json"

// ── Cấu hình Thời gian thực (NTP) cho Việt Nam ─────────────
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 (Việt Nam)
const int   daylightOffset_sec = 0;

// ══════════════════════════════════════════════════════════
// ANTI-PASSBACK
// ══════════════════════════════════════════════════════════
#define ANTI_TIME_MS 3000UL   // 3 giây (3000 ms)

enum CardState { OUTSIDE, INSIDE };
struct CardRecord {
  CardState     state       = OUTSIDE;
  unsigned long checkinTime = 0;
};
std::map<String, CardRecord> cardDB;

MFRC522 mfrc522(SS, RST);

// ══════════════════════════════════════════════════════════
// Buzzer & Helpers
// ══════════════════════════════════════════════════════════
void tick(int count, int ms) {
  while (count--) {
    digitalWrite(BUZZ, HIGH); delay(ms);
    digitalWrite(BUZZ, LOW);  delay(ms);
  }
}
void buzz_checkin()    { tick(1,  80); } 
void buzz_checkout()   { tick(2,  80); } 
void buzz_apb()        { tick(5,  60); } 

String uidToString(byte* buf, byte len) {
  String s = "";
  for (byte i = 0; i < len; i++) {
    if (buf[i] < 0x10) s += "0";
    s += String(buf[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// ══════════════════════════════════════════════════════════
// Lấy thời gian thực dạng chuỗi (VD: 15:30:45 18/05/2026)
// ══════════════════════════════════════════════════════════
String get_current_time_string() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Khong_xac_dinh";
  }
  char timeStringBuff[50];
  // Định dạng: Giờ:Phút:Giây Ngày/Tháng/Năm
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S %d/%m/%Y", &timeinfo);
  return String(timeStringBuff);
}

// ══════════════════════════════════════════════════════════
// WiFi & WiFiManager
// ══════════════════════════════════════════════════════════
void wifi_connect() {
  WiFiManager wm;

  // BỎ COMMENT dòng dưới đây nếu bạn muốn MẠCH QUÊN WIFI ĐÃ LƯU để test cài đặt lại
   wm.resetSettings();

  Serial.println("[WiFi] Dang ket noi hoac tao mang Setup (EduTrack_Setup)...");

  // Tạo trạm phát Wi-Fi tên là "EduTrack_Setup" với mật khẩu "12345678"
  // Lệnh autoConnect sẽ cố vào Wi-Fi cũ trước. Nếu không được, nó mở AP chờ bạn cấu hình.
  bool res = wm.autoConnect("EduTrack_Setup", "12345678"); 

  if(!res) {
    Serial.println("[WiFi] Loi: Khong the ket noi! Dang khoi dong lai...");
    delay(3000);
    ESP.restart(); // Khởi động lại mạch nếu kết nối thất bại
  } 
  else {
    Serial.println("\n[WiFi] Da ket noi WiFi thanh cong!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  }
}

// ══════════════════════════════════════════════════════════
// FIREBASE HTTP POST (Gửi kèm thời gian thực dạng chữ)
// ══════════════════════════════════════════════════════════
void send_event(const String& uid, const char* event, unsigned long duration_s) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[FIREBASE] Lỗi: Chưa kết nối WiFi");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");

  // Lấy giờ thực tế
  String thoi_gian_thuc = get_current_time_string();

  JsonDocument doc;
  doc["ma_the"]     = uid;
  doc["trang_thai"] = event; 
  doc["phong_hoc"]  = "A101";
  
  if (strcmp(event, "CHECK_IN") == 0) {
    doc["thoi_gian_check_in"] = thoi_gian_thuc;
  } 
  else if (strcmp(event, "CHECK_OUT") == 0) {
    doc["thoi_gian_check_out"] = thoi_gian_thuc;
    doc["thoi_luong_hoc(giay)"] = duration_s;
  }
  else {
    doc["thoi_gian_vi_pham"] = thoi_gian_thuc;
  }

  String body; 
  serializeJson(doc, body);
  
  int code = http.POST(body);
  
  if (code > 0) {
    Serial.printf("[FIREBASE] Gửi thành công: %s → %s | HTTP %d\n", uid.c_str(), event, code);
  } else {
    Serial.printf("[FIREBASE] Lỗi gửi dữ liệu: %s\n", http.errorToString(code).c_str());
  }
  
  http.end();
}

// ══════════════════════════════════════════════════════════
// Xử lý Cổng VÀO / RA
// ══════════════════════════════════════════════════════════
void handle_gate_in(const String& uid) {
  CardRecord& rec = cardDB[uid];
  if (rec.state == OUTSIDE) {
    rec.state       = INSIDE;
    rec.checkinTime = millis();
    buzz_checkin();
    Serial.println("[IN] CHECK-IN OK: " + uid);
    send_event(uid, "CHECK_IN", 0);
  } else {
    buzz_apb();
    Serial.println("[IN] APB VIOLATION — Vao 2 lan: " + uid);
    send_event(uid, "APB_VIOLATION", 0);
  }
}

void handle_gate_out(const String& uid) {
  CardRecord& rec = cardDB[uid];
  if (rec.state == OUTSIDE) {
    buzz_apb();
    Serial.println("[OUT] APB VIOLATION — Ra khi chua Vao: " + uid);
    send_event(uid, "APB_VIOLATION", 0);
  } else {
    unsigned long elapsed = millis() - rec.checkinTime;
    if (elapsed < ANTI_TIME_MS) {
      unsigned long con_lai = (ANTI_TIME_MS - elapsed) / 1000;
      buzz_apb();
      Serial.printf("[APB] VIOLATION: %s | Quet 2 lan! Tu choi diem danh\n", uid.c_str(), con_lai);
      send_event(uid, "APB_VIOLATION", 0);
    } else {
      unsigned long dur_s = elapsed / 1000;
      rec.state = OUTSIDE;
      buzz_checkout();
      Serial.printf("[OUT] CHECK-OUT OK: %s | %lu giay\n", uid.c_str(), dur_s);
      send_event(uid, "CHECK_OUT", dur_s);
    }
  }
}

void handle_single_reader(const String& uid) {
  if (cardDB[uid].state == OUTSIDE) handle_gate_in(uid);
  else handle_gate_out(uid);
}

// ══════════════════════════════════════════════════════════
// Setup & Loop
// ══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  pinMode(BUZZ, OUTPUT);
  tick(1, 100);
  delay(1000);

  SPI.begin(SCK, MISO, MOSI, SS);
  mfrc522.PCD_Init();
  delay(4);

  // Kích hoạt kết nối WiFi thông minh (WiFiManager)
  wifi_connect();

  // Khởi tạo và đồng bộ thời gian từ Internet
  Serial.println("[SYS] Dang dong bo thoi gian thuc tu Internet...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000); // Chờ 2 giây để mạch lấy giờ

  Serial.println("[SYS] EduTrack System + NTP Ready.");
  Serial.println("[SYS] Che do: 1 READER (Tu dong Vao/Ra)");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String uid = uidToString(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println("\n[RFID] UID: " + uid);

  handle_single_reader(uid);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}