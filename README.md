# Hệ thống xếp hàng khám bệnh & cấp cứu

Ứng dụng web chạy cục bộ trên máy tính. Giao diện HTML/CSS/JavaScript chạy trong trình duyệt; máy chủ C++17 xử lý nghiệp vụ và dùng Queue, Deque, Priority Queue tự cài đặt. Đây là mô hình phục vụ bài tập cấu trúc dữ liệu; không thay thế quy trình phân loại hoặc quyết định lâm sàng của nhân viên y tế.

## Cấu trúc dữ liệu

- `CircularQueue<T>`: Queue FIFO cài bằng mảng vòng động, tự tăng gấp đôi dung lượng khi đầy.
- `PriorityQueue<T>`: max-heap tự cài đặt; `WaitingEntry::operator<` giúp mức ưu tiên 1 được gọi trước, sau đó giữ thứ tự người đến trước.
- `Deque<T>`: deque vòng động lưu 30 lượt gọi gần nhất theo thứ tự mới nhất trước; thêm ở đầu deque và loại lượt cũ ở cuối deque.
- `std::map` và `std::unordered_map` chỉ dùng để nhóm hàng đợi theo khoa và tra cứu hồ sơ.

## Chạy trên Windows với MSYS2 GCC

1. Cài GCC trong môi trường MSYS2 UCRT64 nếu máy chưa có.
2. Mở thư mục dự án trong VS Code rồi chọn **Terminal → New Terminal**. Terminal cần đứng ở thư mục gốc có `CMakeLists.txt`, `src` và `include` (không đứng trong `include`).
3. Chạy các lệnh trong PowerShell:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/web_server.cpp src/hospital_system.cpp -o hospital_web.exe -lws2_32
.\hospital_web.exe
```

4. Để cửa sổ Terminal mở và truy cập [http://127.0.0.1:8080](http://127.0.0.1:8080) trên trình duyệt.
5. Dừng máy chủ bằng `Ctrl+C`. Dữ liệu hiện chỉ lưu trong bộ nhớ và mất khi tắt chương trình.

Ở chế độ chạy trực tiếp, máy chủ mặc định chỉ lắng nghe trên `127.0.0.1:8080`. Nếu cổng 8080 đã được sử dụng, máy chủ sẽ báo lỗi.

## Đưa lên internet để có link công khai

Dự án đã có `Dockerfile` để triển khai như một Web Service. Ví dụ với Render:

1. Đẩy mã nguồn lên một repository GitHub.
2. Tạo **New → Web Service** trên Render và kết nối repository đó.
3. Chọn runtime **Docker**, để Render build `Dockerfile`, sau đó bấm deploy.
4. Khi deploy thành công, Render cấp một URL công khai dạng `https://ten-dich-vu.onrender.com`. Có thể gắn tên miền riêng trong phần cài đặt domain.

Để chạy bên ngoài máy cá nhân, máy chủ đọc cổng từ biến môi trường `PORT` và bind vào `HOST`; Dockerfile đặt `HOST=0.0.0.0`. Nền tảng sẽ chuyển HTTPS tới cổng của dịch vụ. Tên miền `onrender.com` do Render cấp miễn phí; tên miền riêng thường cần mua.

**Lưu ý:** ứng dụng hiện chưa có đăng nhập, cơ sở dữ liệu bền vững hoặc phân quyền. Ai có link có thể xem và sửa dữ liệu trong phiên chạy; dữ liệu mất khi dịch vụ khởi động lại. Chỉ dùng dữ liệu giả để trình diễn, không nhập thông tin bệnh nhân thật.

## Biên dịch bằng CMake

```sh
cmake -S . -B build
cmake --build build --target hospital_web
```

LaTeX dùng để soạn báo cáo PDF, không chạy máy chủ web. Ứng dụng web chạy bằng GCC hoặc CMake như hướng dẫn trên.
