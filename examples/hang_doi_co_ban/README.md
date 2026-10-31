# Bản console Head / Body / Main

Đây là bản tiếp tục từ đoạn `HangDoiBenhVien` bạn cung cấp, để đọc và thuyết trình trước khi tích hợp các thành phần của dự án web.

| Phần | File | Trách nhiệm |
|---|---|---|
| Head | `HangDoiBenhVien.h` | Khai báo `BenhNhan`, lớp hàng đợi và chữ ký hàm. |
| Body | `HangDoiBenhVien.cpp` | Cấp phát, mở rộng, chèn theo ưu tiên, lấy đầu và in thông báo. |
| Main | `main.cpp` | Đọc dữ liệu, gọi phương thức và xử lý lỗi. |

## Những điểm được hoàn thiện

- Mức 1 ưu tiên cao nhất, cùng mức giữ FIFO. Khác với đoạn ban đầu dùng số lớn trước, quy ước này thống nhất với website và Jira.
- `operator<` tập trung quy tắc so sánh ưu tiên và thứ tự đến.
- Khi đầy, nhân đôi mảng và giữ thứ tự logic; khi rỗng, `goiKham()` ném ngoại lệ.
- Kiểm tra sức chứa, mã, tên và mức ưu tiên; chặn sao chép nông lớp sở hữu bộ nhớ.
- Đọc tên có khoảng trắng và dừng với thông báo khi nhập thiếu/sai kiểu dữ liệu.

## Biên dịch và chạy

Từ thư mục gốc dự án trong PowerShell, với GCC đã có trong PATH:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic examples/hang_doi_co_ban/main.cpp examples/hang_doi_co_ban/HangDoiBenhVien.cpp -o hang_doi_co_ban.exe
.\hang_doi_co_ban.exe
```

Hoặc CMake: `cmake --build build --target hang_doi_co_ban` sau khi cấu hình dự án. Cấu hình CMake gốc vẫn yêu cầu SQLite cho hai target web/console hiện có; lệnh GCC phía trên chạy độc lập, không cần SQLite.

Mỗi bệnh nhân gồm ba dòng: mã, họ tên, mức ưu tiên. Dòng đầu tiên là số bệnh nhân.

```text
4
101
Nguyen Van An
5
102
Tran Thi Binh
1
103
Le Van Cuong
1
104
Pham Thi Dung
3
```

Kết quả gọi theo mã: **102 → 103 → 104 → 101**. Hai bệnh nhân mức 1 giữ thứ tự nhập. STT là thứ tự tiếp nhận, không được đánh lại theo độ ưu tiên.

## Kiểm thử

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic tests/hang_doi_co_ban_check.cpp examples/hang_doi_co_ban/HangDoiBenhVien.cpp -o hang_doi_co_ban_check.exe
.\hang_doi_co_ban_check.exe
```

Kiểm tra hàng rỗng, sức chứa sai, tên trống, mức ưu tiên sai và 10.000 thao tác thêm/lấy xen kẽ đối chiếu với kết quả tham chiếu. Kết thúc không báo lỗi và exit code 0 nghĩa là các assertion đều đạt.

## Phạm vi để trình bày đúng

- Bản này là **hàng đợi ưu tiên bằng mảng vòng có chèn sắp thứ tự**: `xepHang` O(n), `goiKham` O(1), mở rộng O(n). Nó chưa phải Priority Queue dùng heap.
- Bản mẫu xếp mọi mức 1–5 theo ưu tiên. Website hiện dùng heap cho mức 1–2 và FIFO cho các ca thường mức 3–5.
- Chưa có phân khoa, lưu SQLite hoặc Deque lịch sử trong bản ba file. Các thành phần này đang nằm trong `include/`, `src/`, `web/` của dự án chính; không thay thế chúng bằng bản mẫu.
- `goiSoTuDong()` hiện in ra console; không phát giọng nói qua loa.

Hướng dẫn thực hiện từng task: [docs/HUONG_DAN_PHAN_CONG.md](../../docs/HUONG_DAN_PHAN_CONG.md).
