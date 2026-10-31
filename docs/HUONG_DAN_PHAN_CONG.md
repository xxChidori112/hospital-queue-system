# Hướng dẫn làm việc theo 25 task Jira

Bản console ba file nằm trong `examples/hang_doi_co_ban/`. Head khai báo, Body cài đặt, Main nhập/xuất và gọi hàm. Website tiếp tục dùng các file trong `include/`, `src/`, `web/`.

## TV1-01 — NNT-32

- File: CMakeLists.txt; .gitignore; examples/hang_doi_co_ban/README.md
- Cách làm: Tạo nhánh cho từng task; giữ Head chỉ khai báo, Body cài đặt, Main nhập/xuất; chạy bản mẫu trước khi tích hợp.
- Tự kiểm tra: Nhờ một thành viên khác làm theo hướng dẫn từ thư mục dự án và ghi lại lệnh biên dịch thành công.

## TV1-02 — NNT-33

- File: include/circular_queue.hpp; examples/hang_doi_co_ban/HangDoiBenhVien.h và .cpp
- Cách làm: Cài đặt Queue FIFO độc lập bằng front/count; tính ô thêm bằng (front + count) % capacity; thêm A,B,C và lấy lần lượt.
- Tự kiểm tra: Không dùng xếp ưu tiên để kiểm tra FIFO: bản HangDoiBenhVien là mảng vòng có sắp ưu tiên; dùng cùng mức 5 trong ví dụ FIFO.

## TV1-04 — NNT-34

- File: include/circular_queue.hpp; examples/hang_doi_co_ban/HangDoiBenhVien.cpp
- Cách làm: Kiểm tra sức chứa >0 trước cấp phát; kiểm tra rỗng trước pop; kiểm tra grow không ghi đè; lớp sở hữu con trỏ không được sao chép nông.
- Tự kiểm tra: Thử sức chứa 0, pop rỗng, hàng đầy và thêm lại sau khi lấy hết; kiểm tra công cụ phát hiện lỗi bộ nhớ nếu môi trường có hỗ trợ.

## TV1-03 — NNT-35

- File: include/circular_queue.hpp::grow; HangDoiBenhVien.cpp::moRong
- Cách làm: Bắt đầu sức chứa 2; thêm A,B; lấy A; thêm C để quay vòng; thêm D để buộc mở rộng; sao chép theo thứ tự logic rồi đặt front=0.
- Tự kiểm tra: Sau mở rộng phải gọi B,C,D theo đúng thứ tự khi cùng mức ưu tiên.

## TV1-05 — NNT-36

- File: tests/queue_check.cpp; tests/hang_doi_co_ban_check.cpp
- Cách làm: Viết kiểm thử trước cho rỗng, một phần tử, quay vòng, mở rộng; đối chiếu nhiều thao tác xen kẽ với kết quả tham chiếu.
- Tự kiểm tra: Biên dịch bản kiểm thử với assertion bật; mọi ca phải kết thúc bằng exit code 0.

## TV2-01 — NNT-37

- File: include/priority_queue.hpp
- Cách làm: Dùng vector làm vùng nhớ heap; push thêm cuối rồi siftUp; pop chuyển phần tử cuối lên đỉnh rồi siftDown; xử lý riêng heap 0 hoặc 1 phần tử.
- Tự kiểm tra: Thử 5 phần tử mức ưu tiên trộn; top luôn trùng phần tử được pop tiếp theo. Bản Body ba file dùng chèn O(n), chưa phải heap.

## TV2-03 — NNT-38

- File: include/hospital_system.hpp::WaitingEntry; examples/hang_doi_co_ban/HangDoiBenhVien.cpp::BenhNhan::operator<
- Cách làm: Thống nhất mức 1 ưu tiên cao nhất; trong max-heap, mức số lớn hơn phải được coi là nhỏ hơn; nếu cùng mức, arrivalOrder/soThuTu lớn hơn có ưu tiên thấp hơn.
- Tự kiểm tra: Nhập A mức 2, B mức 1, C mức 1: phải gọi B,C,A; không thay đổi thứ tự FIFO khi cùng mức.

## TV2-02 — NNT-39

- File: include/priority_queue.hpp::siftUp và siftDown
- Cách làm: Cha=(i-1)/2; con trái=2*i+1; con phải=2*i+2; kiểm tra biên trước truy cập; chọn con có ưu tiên cao hơn để đổi chỗ.
- Tự kiểm tra: Thử cả trường hợp phải đổi với con trái, con phải và không đổi; kiểm tra nhiều lần thêm/xóa liên tiếp.

## TV2-05 — NNT-40

- File: tests/queue_check.cpp; include/priority_queue.hpp; include/deque.hpp
- Cách làm: Bổ sung ca heap cùng mức, khác mức, một phần tử; đối chiếu Deque với danh sách tham chiếu qua thao tác hai đầu.
- Tự kiểm tra: Ghi số ca kiểm thử và kết quả; không chỉ kiểm tra ví dụ 3 phần tử, cần chuỗi xen kẽ để bắt lỗi chỉ số.

## TV3-01 — NNT-41

- File: src/hospital_system.cpp::registerPatient; src/web_server.cpp::/api/register; web/app.js::intake-form
- Cách làm: Kiểm tra dữ liệu ở máy chủ: trim tên, kiểm tra độ dài, năm sinh, triage và chuỗi số đầy đủ; phía giao diện hiển thị lỗi cụ thể.
- Tự kiểm tra: Thử tên chỉ dấu cách, năm tương lai, mức 0/6 và chuỗi số có hậu tố như 23junk; tất cả phải bị từ chối.

## TV2-04 — NNT-42

- File: include/deque.hpp; src/hospital_system.cpp::startConsultation và callHistory
- Cách làm: Cài đặt thêm/xóa hai đầu; push_front bản ghi mới; khi quá 30 lượt thì pop_back bản ghi cũ nhất; phân biệt lịch sử với hàng bệnh nhân đang chờ.
- Tự kiểm tra: Thêm 31 bản ghi, còn đúng 30 và giữ lượt mới nhất; kiểm tra hàng rỗng và mở rộng ở vị trí head khác 0.

## TV3-02 — NNT-43

- File: src/hospital_system.cpp::registerPatient và loadDatabase
- Cách làm: Theo dõi nextTicketNumber và đọc lại số lớn nhất từ mã BN khi khởi động; chỉ cấp mã khi đầu vào hợp lệ; kiểm tra lỗi lưu.
- Tự kiểm tra: Tạo hai phiếu, khởi động lại rồi tạo phiếu thứ ba; không được ghi đè hồ sơ cũ. Bản mẫu ba file chỉ giữ STT trong một lần chạy.

## TV3-03 — NNT-44

- File: src/hospital_system.cpp::inferDepartment và enqueuePatient; web/app.js::rooms
- Cách làm: Dùng danh sách 7 phòng chuẩn; thống nhất từ khóa có/không dấu; kiểm tra khoa nhận được tồn tại trước khi enqueue; mức cấp cứu do người sử dụng chọn.
- Tự kiểm tra: Thử chọn phòng rõ ràng, tự phân luồng và khoa không tồn tại; ghi lại quy tắc, không mô tả quy tắc từ khóa là chẩn đoán y khoa.

## TV4-02 — NNT-45

- File: src/hospital_system.cpp::callSpecificPatient; web/app.js::btn-recall và sound
- Cách làm: Gọi trực tiếp chỉ áp dụng phiếu hợp lệ đang chờ; gọi lại dùng lượt đang khám, không enqueue hoặc tạo thêm hồ sơ.
- Tự kiểm tra: Gọi lại 3 lần: số bệnh nhân và trạng thái không đổi; thử gọi phiếu không tồn tại hoặc đã hoàn tất.

## TV4-01 — NNT-46

- File: src/hospital_system.cpp::callNextPatient và takeNextRegularEntry
- Cách làm: Ưu tiên ca cấp cứu đúng khoa trước; lấy Queue thường nếu không có; bỏ entry hết hiệu lực bằng queueVersion; thống nhất sức chứa phục vụ của phòng.
- Tự kiểm tra: Thử hai khoa độc lập, hàng trống, ca cấp cứu và vé đã hủy; nếu một bác sĩ/phòng thì không gọi thêm khi phòng đang bận.

## TV3-04 — NNT-47

- File: src/hospital_system.cpp::estimatedWaitMinutes và serviceMinutes
- Cách làm: Đếm người chờ phía trước trong cùng khoa theo ưu tiên/FIFO; cộng phần thời gian của lượt đang khám; quy định rõ thời lượng ước tính từng phòng.
- Tự kiểm tra: Kiểm tra phòng trống, một người đang khám, thêm ca cấp cứu và phiếu đã hoàn tất; không báo 0 phút chỉ vì không còn người đang chờ phía trước.

## TV3-05 — NNT-48

- File: web/index.html::intake-form; web/app.js::submit; src/web_server.cpp::/api/register; SQLite
- Cách làm: Khóa nút trong lúc gửi và mở lại khi lỗi; xử lý chống tạo lặp ở máy chủ; sau thành công tải lại trạng thái và xóa biểu mẫu.
- Tự kiểm tra: Giả lập mạng chậm và bấm hai lần: chỉ có một hồ sơ; khởi động lại vẫn giữ đủ trường bệnh nhân.

## TV5-01 — NNT-49

- File: web/waiting.html; tests/waiting_display_check.js; tests/waiting_screen_ui_check.js
- Cách làm: Kiểm tra 7 phòng, dữ liệu công khai chỉ ticket/khoa, polling 5 giây và GIF đổi 6 giây; kiểm tra tạm dừng, chuyển ảnh, mất mạng.
- Tự kiểm tra: Đối chiếu API với giao diện; xem trên 390px và desktop; sửa test đọc hết response trước khi đóng máy chủ nếu Node báo lỗi kết thúc kết nối.

## TV4-04 — NNT-50

- File: src/hospital_system.cpp::transferDepartment và transferToEmergency; web/app.js::emergency
- Cách làm: Xác định rõ nâng mức ưu tiên hay chuyển thật sang P.100; cập nhật khoa/trạng thái/triage cùng nhau; vô hiệu vé cũ bằng queueVersion; kiểm tra lưu lỗi.
- Tự kiểm tra: Sau chuyển chỉ còn một phiếu hợp lệ ở khoa đích; gọi khoa cũ không lấy lại phiếu đó; cấp cứu cùng mức vẫn FIFO.

## TV4-05 — NNT-51

- File: tests/queue_check.cpp; tests mới cho API; src/hospital_system.cpp::startConsultation
- Cách làm: Tái hiện lỗi bằng dữ liệu mẫu riêng; khóa SQLite để giả lập lỗi; kiểm tra bộ nhớ và DB phục hồi cùng trạng thái, không đẩy người đầu xuống cuối.
- Tự kiểm tra: A đến trước B; gọi A khi DB khóa thất bại; mở khóa và gọi lại vẫn phải là A. Thử bấm gọi/hoàn tất liên tiếp.

## TV4-03 — NNT-52

- File: web/app.js::btn-complete, btn-absent, queue-table-body; API hoàn tất/vắng mặt
- Cách làm: Đưa luồng cập nhật và gọi tiếp theo vào một nơi dùng chung; thống nhất nút ở bảng và nút ở panel; tránh hai client cùng tự gọi hai lần.
- Tự kiểm tra: Chuẩn bị A đang khám, B đang chờ; hoàn tất A bằng từng loại nút đều chuyển sang B đúng một lần.

## TV5-02 — NNT-53

- File: web/queue-demo.html; tests/queue_demo_check.js
- Cách làm: Chạy đủ 20 bước; ghi lại front/rear/count/capacity và heap; so sánh với thuật toán C++; xác minh mô phỏng không gửi thay đổi dữ liệu thật.
- Tự kiểm tra: Chụp bước quay vòng, bước mở rộng và cấp cứu; mô phỏng dùng JavaScript riêng, không mô tả nó là mã C++ chạy trong trình duyệt.

## TV5-04 — NNT-54

- File: docs/; báo cáo LaTeX/Overleaf và slide
- Cách làm: Nhận sơ đồ và kết quả từ TV1–TV4; trình bày ba phần Head/Body/Main, quy tắc so sánh, độ phức tạp và giới hạn; phân biệt bản console cơ bản với web C++/SQLite.
- Tự kiểm tra: Mỗi thành viên tự kiểm tra phần thuật toán của mình; báo cáo có ảnh kiểm thử và mô tả đúng tính năng đã hoàn thành.

## TV5-05 — NNT-55

- File: README.md; examples/hang_doi_co_ban/README.md; bảng Jira NNT
- Cách làm: Thử lại lệnh build trên máy khác; thêm dữ liệu mẫu, thứ tự gọi mong đợi và cách chạy tests; gắn commit/ảnh/log vào task tương ứng.
- Tự kiểm tra: Chỉ chuyển Done khi đủ tiêu chí nghiệm thu; xuất ảnh/tệp tổng hợp từ Jira để làm minh chứng, không suy ra hoàn thành chỉ từ việc có mã nguồn.

## TV5-03 — NNT-56

- File: tests/; examples/hang_doi_co_ban/; src/web_server.cpp
- Cách làm: Chạy kiểm thử bản ba file trước, sau đó kiểm thử website với DB tạm; làm luồng tiếp nhận→gọi→chuyển→hoàn tất→khởi động lại.
- Tự kiểm tra: Lưu input, kết quả mong đợi, kết quả thực tế và ảnh; chỉ dùng hồ sơ mẫu trong kiểm thử.

