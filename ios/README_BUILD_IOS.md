# Hướng Dẫn Build & Cài Đặt Client MU Online iOS (.ipa)

Tài liệu này hướng dẫn cách build ứng dụng MU Online cho iPhone / iPad. Có 2 phương thức:
1. **Qua GitHub Actions (Khuyên dùng khi ngồi máy Windows)**: 100% tự động, miễn phí, không cần máy Mac, build trực tiếp trên cloud Apple Silicon runner.
2. **Qua máy tính macOS cục bộ (MacBook / Mac Mini / Hackintosh)**: Dùng script `Build_iOS.sh` hoặc mở dự án trực tiếp bằng Xcode.

---

## PHƯƠNG THỨC 1: Build Trực Tiếp Qua GitHub Actions (Khuyên Dùng Từ Windows)

Dự án đã tích hợp sẵn workflow CI/CD tại [`.github/workflows/build_ios.yml`](file:///g:/mu_project/.github/workflows/build_ios.yml).

### Các bước thực hiện:
1. Push code lên GitHub:
   ```bash
   git add .
   git commit -m "feat(ios): deploy multiplatform build pipeline"
   git push origin main
   ```
2. Mở trình duyệt vào trang repository trên GitHub:
   - Nhấn vào tab **Actions**.
   - Ở cột bên trái, chọn workflow **Build iOS Client**.
   - Nhấn nút **Run workflow** (bên phải):
     - **Build Target**: Chọn `device` (để tạo file `.ipa` cho điện thoại thật) hoặc `simulator`.
     - **Bundle Client/Data into IPA**: Chọn `true` (mặc định đã nhúng sẵn dữ liệu Data).
   - Bấm **Run workflow**.
3. Sau khoảng 5-8 phút, workflow hoàn tất.
4. Cuộn xuống mục **Artifacts** trong kết quả run -> Tải file `MUOnline_iOS_device.zip` về máy tính.
5. Giải nén file zip sẽ được file `MUOnline_Client.ipa`.

---

## PHƯƠNG THỨC 2: Build Cục Bộ Trên Máy Mac (macOS)

### Yêu cầu:
- macOS 14+ (Sonoma hoặc mới hơn).
- Xcode 15+ (`xcode-select --install`).
- CMake (`brew install cmake ninja molten-vk`).

### Cách 1: Chạy Script Tự Động 1-Click
```bash
cd ios
chmod +x Build_iOS.sh
./Build_iOS.sh
```
- Chọn `[1]` để biên dịch Release cho iPhone/iPad thật và đóng gói ra `ios/outputs/MUOnline_Client.ipa`.
- Chọn `[2]` để build cho máy ảo iOS Simulator.
- Chọn `[3]` để sinh file `Main.xcodeproj` và mở bằng Xcode.

### Cách 2: Mở & Debug Bằng Xcode GUI
```bash
cd ios
./Build_iOS.sh 3
open build-xcode/Main.xcodeproj
```
- Chọn Scheme **Main** -> Chọn thiết bị iPhone của bạn.
- Vào tab **Signing & Capabilities** -> Chọn Apple ID của bạn để tự động ký chứng chỉ.
- Bấm `Cmd + R` để Run & Debug trực tiếp trên điện thoại.

---

## HƯỚNG DẪN CÀI ĐẶT FILE `.ipa` LÊN IPHONE / IPAD

Sau khi đã có file `MUOnline_Client.ipa`, bạn có thể cài đặt theo một trong các cách sau:

### 1. TrollStore (iOS 14.0 - 17.0) - Tối Ưu Nhất
- Dành cho các máy tương thích TrollStore.
- Bắn file `.ipa` qua AirDrop hoặc gửi qua Telegram/iCloud.
- Mở file `.ipa` bằng TrollStore và nhấn **Install**.
- *Đặc điểm*: Không bao giờ bị thu hồi chứng chỉ (vĩnh viễn, no revoke), không giới hạn 7 ngày, tốc độ khởi động nhanh nhất.

### 2. Sideloadly (Cài Từ Windows / Mac Qua Cáp USB)
1. Tải phần mềm Sideloadly tại: [https://sideloadly.io](https://sideloadly.io)
2. Kết nối iPhone với máy tính qua cáp USB.
3. Kéo file `MUOnline_Client.ipa` thả vào giao diện Sideloadly.
4. Nhập Apple ID của bạn và nhấn **Start**.
5. Sau khi cài xong, trên iPhone vào: **Cài đặt -> Cài đặt chung -> Quản lý VPN & Thiết bị** -> Bấm **Tin cậy (Trust)** chứng chỉ nhà phát triển.

### 3. AltStore / SideStore
- Cài qua Wi-Fi bằng Apple ID miễn phí, tự động gia hạn 7 ngày một lần.

### 4. Apple Developer Account (Ad-Hoc / TestFlight)
- Ký bằng chứng chỉ nhà phát triển Apple Developer Program để phân phối thử nghiệm cho người chơi khác qua link TestFlight hoặc cài đặt OTA.

---

## ĐẶC ĐIỂM KỸ THUẬT CLIENT TRÊN iOS
- **Renderer**: Vulkan / Metal qua SDL3 GPU Subsystem và MoltenVK runtime.
- **Font**: FreeType 2.13.2 nhúng trong ứng dụng, hỗ trợ hiển thị tiếng Việt UTF-8 có dấu hoàn chỉnh và tự động fallback sang system font PingFang / Helvetica.
- **Tần số quét**: Hỗ trợ 120Hz ProMotion trên iPhone Pro / iPad Pro.
- **Giao diện**: Tự động nhận diện Safe Area tránh tai thỏ (Notch) và Dynamic Island.
