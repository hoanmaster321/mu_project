# Hướng Dẫn Triển Khai & Build iOS Client - MU Online

Tài liệu này hướng dẫn chi tiết cách biên dịch, đóng gói và triển khai Game Client MU Online lên thiết bị iOS (iPhone / iPad) và iOS Simulator.

---

## 1. Kiến Trúc Client Trên iOS

Client MU Online mobile được xây dựng trên nền tảng hiện đại:
- **Ngôn ngữ**: C++17 / C++20.
- **Windowing & Input & Audio**: **SDL3** (`release-3.2.4`) tích hợp UIKit, hỗ trợ cảm ứng đa điểm, virtual joystick, bàn phím ảo iOS.
- **Đồ họa**: **Vulkan / Metal** (thông qua MoltenVK trên iOS và SDL3 GPU subsystem).
- **Text Rendering**: FreeType 2 (`VER-2-13-2`) biên dịch trực tiếp qua CMake.
- **Hệ thống điều khiển**: Cụm phím ảo Joystick + Skill cluster (`MobileControls.cpp`, `NewUIMainFrameMobile.cpp`).
- **Nền tảng tương thích**: Cung cấp đầy đủ các API Windows mô phỏng qua `Platform/PlatformDefs.h` và `Platform/MobilePlatform.h` với cờ `MU_IOS`.

---

## 2. Cấu Trúc Thư Mục iOS

```text
mu_project/
├── ios/
│   ├── Info.plist               # Cấu hình Bundle ID, quyền hạn, xoay màn hình ngang
│   ├── LaunchScreen.storyboard  # Màn hình Splash khởi động tối ưu Dark Theme
│   ├── ios.toolchain.cmake      # Toolchain CMake chuẩn cho iOS (arm64, Simulator)
│   ├── Build_iOS.sh             # Script 1-click tự động sinh Xcode và đóng gói IPA
│   ├── PORTING_TAKUMI_IOS.md    # Tài liệu hướng dẫn này
│   └── outputs/                 # Nơi chứa file MUOnline_Client.ipa sau khi build
```

---

## 3. Yêu Cầu Môi Trường (Prerequisites)

Để build ứng dụng iOS, bạn cần:
1. **Máy tính macOS** (MacBook, Mac Mini, iMac hoặc máy ảo macOS / Hackintosh).
2. **Xcode 15+** (đã cài đặt Command Line Tools: `xcode-select --install`).
3. **CMake 3.24+** (`brew install cmake`).
4. **Vulkan SDK cho macOS/iOS** (Tùy chọn nếu muốn dùng MoltenVK framework từ LunarG):
   - Tải tại: [https://vulkan.lunarg.com/sdk/home](https://vulkan.lunarg.com/sdk/home) (chọn macOS SDK, có sẵn MoltenVK cho iOS).

---

## 4. Các Bước Build Client

### Cách 1: Tự Động 1-Click Bằng `Build_iOS.sh` (Khuyên dùng)

Mở Terminal trên máy Mac tại thư mục `ios/`:

```bash
cd /path/to/mu_project/ios
chmod +x Build_iOS.sh
./Build_iOS.sh
```

Menu sẽ hiển thị 3 tùy chọn:
- **`[1] Real Device (arm64)`**: Biên dịch Release cho iPhone/iPad thật và tự động đóng gói ra file `ios/outputs/MUOnline_Client.ipa`.
- **`[2] Simulator`**: Biên dịch Debug/Release cho máy ảo iOS Simulator (chạy test nhanh không cần chứng chỉ Apple).
- **`[3] Generate Xcode project only`**: Tạo dự án `Main.xcodeproj` để mở bằng Xcode GUI.

---

### Cách 2: Mở & Build Trực Tiếp Trong Xcode (Dành cho Debug & Lập trình)

1. Tạo file dự án Xcode:
   ```bash
   cd /path/to/mu_project/ios
   ./Build_iOS.sh 3
   ```
2. Mở file `build-xcode/Main.xcodeproj` bằng Xcode:
   ```bash
   open build-xcode/Main.xcodeproj
   ```
3. Trong Xcode:
   - Chọn scheme **Main** -> Target Device (iPhone của bạn hoặc Simulator).
   - Vào tab **Signing & Capabilities** -> Chọn Team Apple ID của bạn để tự động ký chứng chỉ.
   - Nhấn `Cmd + R` để Run & Debug trực tiếp trên máy.

---

## 5. Dữ Liệu Game (Client Data)

Client game đọc dữ liệu từ thư mục `Data/`:
- **Chế độ Bundle (Offline)**: Script `Build_iOS.sh` tự động sao chép toàn bộ thư mục `Client/Data` vào `MUOnline.app/Data`. Khi cài đặt app, toàn bộ map, model 3D, âm thanh sẽ có sẵn trong game.
- **Chế độ Update Online (Preload)**: Client hỗ trợ tải và giải nén dữ liệu từ máy chủ update từ xa (tương tự như Android qua `http://update.daybreak.id.vn/update/data.zip`) vào thư mục Documents của sandbox iOS (`SDL_GetPrefPath`).

---

## 6. Hướng Dẫn Cài Đặt File `.ipa` Lên iPhone / iPad

Sau khi build ra file `ios/outputs/MUOnline_Client.ipa`, bạn có các cách cài đặt sau:

1. **TrollStore (iOS 14.0 - 17.0)**:
   - Chuyển file `.ipa` qua AirDrop hoặc iCloud Drive.
   - Mở bằng TrollStore -> Nhấn **Install**.
   - *Ưu điểm*: Cài vĩnh viễn, không bao giờ bị thu hồi chứng chỉ (no revoke), không giới hạn 7 ngày.

2. **AltStore / SideStore**:
   - Sử dụng Apple ID miễn phí để ký (chứng chỉ 7 ngày, tự động làm mới qua Wi-Fi).

3. **Sideloadly**:
   - Kết nối iPhone với máy tính (Windows hoặc Mac).
   - Kéo file `MUOnline_Client.ipa` vào Sideloadly, nhập Apple ID và bấm **Start**.

4. **Apple Developer Account (Ad-Hoc / TestFlight)**:
   - Dành cho nhà phát triển muốn phân phối cho nhiều người chơi thử nghiệm qua TestFlight hoặc chứng chỉ Enterprise.

---

## 7. Các Lưu Ý Kỹ Thuật

- **Màn hình tai thỏ & Dynamic Island**: Đã được cấu hình full màn hình ngang, tự động nhận diện safe-area qua SDL3.
- **ProMotion 120Hz**: `CADisableMinimumFrameDurationOnPhone = true` trong `Info.plist` cho phép render mượt mà lên tới 120 FPS trên các dòng iPhone Pro / iPad Pro.
