# EduTrack - Hệ Thống Điểm Danh Thông Minh Chống Gian Lận (Edge IoT & AI)

EduTrack là một giải pháp quản lý điểm danh thông minh toàn diện, ứng dụng nền tảng điện toán biên (Edge IoT) với thuật toán chống gian lận FSM để kiểm soát phần cứng thời gian thực, kết hợp cùng cơ sở dữ liệu đám mây Firebase và Trí tuệ nhân tạo (mạng học sâu LSTM, trợ lý ảo Gemini) nhằm tự động hóa quy trình giám sát, phân tích hành vi và trực quan hóa dữ liệu chuyên cần của sinh viên một cách minh bạch, hiệu quả.

---

## 🚀 Tính Năng Cốt Lõi

### 1. Edge Hardware & Anti-Fraud Logic
* **Điểm danh Real-time:** Nhận diện sinh viên qua thẻ RFID RC522 nhanh chóng thông qua giao tiếp SPI trên chip ESP32-S2 Mini.
* **Thuật toán Chống gian lận (Anti-passback):** Sử dụng Máy trạng thái hữu hạn (FSM) cài đặt trực tiếp tại biên để ngăn chặn hành vi quẹt hộ hoặc quẹt thẻ liên tục.
* **Cảnh báo tại chỗ:** Hệ thống phản hồi âm thanh linh hoạt qua còi Buzzer (Bíp ngắn khi hợp lệ, còi hú dài khi phát hiện vi phạm APB).
* **Đồng bộ thời gian:** Tự động đồng bộ mốc thời gian thực chuẩn quốc tế thông qua giao thức NTP qua kết nối Wi-Fi.

### 2. Cloud Backend & Web Dashboard
* **Firebase Realtime Database:** Lưu trữ dữ liệu thô và đồng bộ hóa không độ trễ giữa phần cứng và phần mềm.
* **Firebase Authentication:** Xác thực tài khoản an toàn cho Giảng viên/Quản trị viên khi đăng nhập hệ thống.
* **Quản trị Toàn diện:** 
  * Định danh trực tiếp (gắn mã thẻ vật lý UID với thông tin Sinh viên).
  * Lọc lịch sử điểm danh thông minh theo mốc thời gian tùy chọn.
  * Nhập (Import) danh sách lớp và Xuất (Export) báo cáo chuyên cần trực tiếp ra file Excel (`.xlsx`) phục vụ học vụ.

### 3. AI Insights & Assistant
* **Phát hiện dị thường (LSTM):** Server Python chạy mô hình mạng học sâu LSTM liên tục quét dữ liệu chuỗi thời gian để phát hiện tự động các mẫu hành vi đi học bất thường.
* **Trợ lý ảo thông minh (Gemini 2.5 API):** Tích hợp khung Chatbox ứng dụng kỹ thuật RAG cơ bản, cho phép giảng viên truy vấn dữ liệu lớp học bằng ngôn ngữ tự nhiên (VD: *"Liệt kê các sinh viên vắng quá 3 buổi"*).
### 4. Kiến Trúc Hệ Thống (System Architecture)
```text
    [ Thẻ RFID ] ──▶ [ Module RFID ] ──▶ [ ESP32-S2 Mini ] ──▶ [ Còi Buzzer ]
                                              ↕️ (Wi-Fi)
                                     [ Cơ sở dữ liệu Firebase ]
                                       ↕️                ↕️ (Xác thực & Truy xuất)
```
### 5. Tầng Hệ Thống,Công Nghệ / Thành Phần
* Phần cứng (Hardware): ESP32-S2 Mini, Module RFID RC522, Active Buzzer
* Phần mềm: Antigravity / VS Code (PlatformIO)
* Database: Firebase Realtime Database, Firebase Auth
* Giao diện (Frontend): HTML5, CSS3 (TailwindCSS/Bootstrap), JavaScript (ES6)
* Phân tích AI: Python, TensorFlow/Keras (LSTM Model), Google Gemini 2.5 API

### 6. Cấu trúc File

```text
├── EduTrack_Firmware/       # Mã nguồn C++ cho vi điều khiển ESP32
│   ├── src/
│   └── platformio.ini
├── EduTrack_Web/            # Giao diện Web Dashboard quản lý
│   ├── Login.html           # Trang đăng nhập bảo mật
│   ├── Dashboard.html       # Bảng điều khiển và Chatbox AI
│   ├── css/
│   └── js/                  # Logic xử lý Firebase & Gemini SDK
├── EduTrack_AI_Server/      # Lõi xử lý AI (LSTM Anomaly Detection)
│   ├── model/               # File mô hình đã huấn luyện (.h5)
│   └── server.py            # Script lắng nghe Firebase và chạy predict
└── README.md                # Tài liệu hướng dẫn dự án
```
### 7. Hướng Dẫn Cài Đặt (Setup & Installation)
git clone hoặc tải trực tiếp file về máy
1. Cấu hình Phần cứng (Sơ đồ nối dây gợi ý)
* RC522 -> ESP32: SS(SDA) -> GPIO14, SCK -> GPIO12, MOSI -> GPIO11, MISO -> GPIO13, RST -> GPIO9.
* Buzzer -> ESP32: VCC -> 3V3, GND -> GND, I/O -> GPIO5.
2. Cấu hình Firebase & Web
* Tạo một dự án mới trên Firebase Console.
* Bật dịch vụ Realtime Database và Authentication (Email/Password provider).
* Copy đoạn mã cấu hình Firebase (firebaseConfig) dán vào các file JavaScript trong thư mục EduTrack_Web/js/config.js.
