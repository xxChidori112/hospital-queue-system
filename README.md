# CareQueue V2 — Hệ thống xếp hàng khám bệnh & cấp cứu

Phiên bản giao diện V2 có bảng điều phối mới, thống kê hàng đợi, bộ lọc hồ sơ, tiếp nhận nhanh, gọi lượt theo khoa và lịch sử gọi gần đây. Giao diện chạy trên trình duyệt và kết nối máy chủ C++17 hiện có.

Ứng dụng web chạy cục bộ hoặc triển khai bằng Docker. Giao diện HTML/CSS/JavaScript gọi máy chủ C++17; logic tiếp tục dùng Queue mảng vòng tự cài đặt, Priority Queue dạng heap và Deque cho lịch sử gọi.

## Chức năng

- Tiếp nhận bệnh nhân, tự cấp mã phiếu và phân luồng khoa theo lựa chọn hoặc từ khóa triệu chứng.
- Gọi bệnh nhân theo khoa đã chọn; ca cấp cứu trong khoa được gọi trước, các ca còn lại giữ FIFO.
- Tìm kiếm theo mã, tên, khoa hoặc triệu chứng; lọc theo khoa, mức ưu tiên và trạng thái.
- Lưu hồ sơ, trạng thái khám và lịch sử gọi vào SQLite. Dữ liệu được nạp lại khi máy chủ khởi động lại.
- Ước tính thời gian chờ theo hàng đợi của khoa.
- Gọi trực tiếp một phiếu đang chờ, gọi lại bằng âm báo, đánh dấu vắng mặt, hủy phiếu chờ hoặc chuyển bệnh nhân đang khám sang hàng đợi khoa khác.
- Hiển thị bảy phòng khám và tên bác sĩ theo mẫu, cùng số bệnh nhân đang chờ. Danh sách bác sĩ là thông tin tĩnh theo ảnh, không phản ánh ca trực thực tế.
- Tiếp nhận theo năm sinh, giới tính, đối tượng, phòng khám và lý do khám; hiển thị bác sĩ theo phòng đã chọn. Tuổi là ước tính theo năm sinh vì chưa thu thập ngày và tháng sinh.
- Nút Màn hình chờ mở bảng chờ ngay trong trang điều phối, không mở tab mới. Bảng hiển thị mã đang khám, tối đa năm mã tiếp theo và tổng số lượt chờ theo phòng; tự cập nhật mỗi 5 giây. Đóng bảng hoặc nhấn Escape để quay lại thao tác. Dữ liệu riêng của màn hình này không bao gồm tên hay triệu chứng; đường dẫn `/waiting` vẫn dùng được khi cần mở trực tiếp trên TV.
- Khu vực GIF meme trong màn hình chờ chọn ngẫu nhiên mỗi 6 giây, không lặp ngay ảnh vừa phát, có nút tạm dừng và chuyển ảnh. Danh sách `memes` trong `web/waiting.html` chứa bốn đường dẫn GIF tải trực tiếp từ GIPHY, không cần API key; cần Internet để tải ảnh. Đây là danh sách cố định, không tự tìm GIF mới trong kho GIPHY. Thiết bị bật giảm chuyển động sẽ bắt đầu ở trạng thái tạm dừng.

Không tiếp nhận hoặc chuyển bệnh nhân mới vào hai khoa cũ Tim mạch và Nội tiết. Nếu dữ liệu còn hồ sơ thuộc hai khoa này, bộ lọc và gọi lượt vẫn hiển thị chúng để xử lý hồ sơ cũ; các phòng mới chưa có thời lượng riêng dùng mức ước tính mặc định 15 phút.
Khi khởi động với cơ sở dữ liệu cũ, hệ thống bổ sung cột năm sinh, giới tính và đối tượng; hồ sơ cũ không có các thông tin này sẽ để trống.

Giao diện MedCare dùng `web/index.html` và `web/app.js`, kết nối trực tiếp với máy chủ C++ và SQLite. Nút gọi trực tiếp do người điều phối chọn nên có thể bỏ qua thứ tự ưu tiên; nút gọi lượt tiếp theo vẫn theo thứ tự cấp cứu và FIFO. Sau khi hoàn tất, đánh dấu vắng mặt hoặc chuyển khoa, hệ thống gọi lượt tiếp theo của khoa cũ nếu có người chờ. Gọi lại phát âm báo trên thiết bị hiện tại và hiển thị thông báo; không phát tiếng nói qua hệ thống loa bệnh viện.

Giao diện tải Tailwind CSS và phông chữ từ CDN nên cần kết nối Internet để hiển thị đầy đủ. Đây là mô hình học tập; không dùng dữ liệu bệnh nhân thật.

## Giải thích Queue khi thuyết trình

Queue trong `include/circular_queue.hpp` là hàng đợi FIFO bằng mảng vòng:

- `data_` là mảng lưu phần tử; `front_` là vị trí đầu hàng; `count_` là số phần tử.
- `push()` thêm vào vị trí `(front_ + count_) % data_.size()`; `front()` xem phần tử đầu; `pop()` lấy phần tử đầu và dịch `front_` sang vị trí kế tiếp.
- Khi đầy, `grow()` tạo mảng lớn gấp đôi và chuyển các phần tử theo đúng thứ tự chờ. Phép `%` giúp quay lại đầu mảng khi đến cuối.
- Ví dụ: thêm A, B, C rồi lấy một lần sẽ nhận A; thêm D thì thứ tự còn lại là B, C, D.
- Mỗi khoa có một Queue thông thường. Bệnh nhân cấp cứu dùng Priority Queue riêng; `operator<` quyết định mức ưu tiên và thứ tự đến.

`std::vector` chỉ dùng cấp phát mảng động; các thao tác Queue được tự cài đặt. `push()` thường là O(1), O(n) khi mở rộng; `pop()` và `front()` là O(1).

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
