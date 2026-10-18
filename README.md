# CareQueue V2 — Hệ thống xếp hàng khám bệnh & cấp cứu

Phiên bản giao diện V2 có bảng điều phối mới, thống kê hàng đợi, bộ lọc hồ sơ, tiếp nhận nhanh, gọi lượt theo khoa và lịch sử gọi gần đây. Giao diện chạy trên trình duyệt và kết nối máy chủ C++17 hiện có.

Ứng dụng web chạy cục bộ hoặc triển khai bằng Docker. Giao diện HTML/CSS/JavaScript gọi máy chủ C++17; logic tiếp tục dùng Queue mảng vòng tự cài đặt, Priority Queue dạng heap và Deque cho lịch sử gọi.

## Chức năng

- Tiếp nhận bệnh nhân, tự cấp mã phiếu và phân luồng khoa theo lựa chọn hoặc từ khóa triệu chứng.
- Gọi bệnh nhân theo khoa đã chọn; ca cấp cứu trong khoa được gọi trước, các ca còn lại giữ FIFO.
- Tìm kiếm theo mã, tên, khoa hoặc triệu chứng; lọc theo khoa, mức ưu tiên và trạng thái.
- Lưu hồ sơ, trạng thái khám và lịch sử gọi vào SQLite. Dữ liệu được nạp lại khi máy chủ khởi động lại.
- Ước tính thời gian chờ theo hàng đợi của khoa.

## Chạy trên Windows với MSYS2 GCC

Cài thư viện SQLite trong terminal **MSYS2 UCRT64**:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-sqlite3
```

Gói này cung cấp header và thư viện SQLite cho GCC UCRT64. [Thông tin gói MSYS2](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-sqlite3)

Trong PowerShell ở thư mục gốc dự án (thư mục có `CMakeLists.txt`, `src` và `include`), biên dịch và chạy:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/web_server.cpp src/hospital_system.cpp -o hospital_web.exe -lws2_32 -lsqlite3
.\hospital_web.exe
```

Mở [http://127.0.0.1:8080](http://127.0.0.1:8080). SQLite tạo file `hospital.db` tại thư mục chạy. Đặt biến `HOSPITAL_DB` nếu muốn lưu database ở vị trí khác. Dừng bằng `Ctrl+C`.

Ứng dụng console có thể biên dịch tương tự, thay `src/web_server.cpp` bằng `src/main.cpp` và bỏ `-lws2_32`.

## CMake

```sh
cmake -S . -B build
cmake --build build --target hospital_web
```

Yêu cầu SQLite development library (`libsqlite3-dev` trên Debian/Ubuntu, gói MSYS2 phía trên trên Windows).

## Deploy bằng Docker

Repository có `Dockerfile`. Khi tạo Web Service, chọn runtime Docker. Ứng dụng đọc `PORT`, bind theo `HOST` và dùng `HOSPITAL_DB` làm vị trí database. Trên hosting, gắn persistent disk tại `/var/data` để SQLite không mất dữ liệu khi instance được thay thế; cấu hình disk phụ thuộc gói hosting.

## Giới hạn và quyền riêng tư

Đây là mô hình học tập, chưa có đăng nhập hoặc phân quyền. Không đưa thông tin bệnh nhân thật vào dịch vụ công khai. Mức phân loại cần do nhân viên y tế quyết định; phần mềm không tự chẩn đoán hoặc thay thế quy trình lâm sàng.
