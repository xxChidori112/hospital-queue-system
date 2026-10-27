#include "circular_queue.hpp"
#include "hospital_system.hpp"

#include <cassert>
#include <stdexcept>

int main() {
    CircularQueue<int> queue(2);
    queue.push(1);
    queue.push(2);
    assert(queue.pop() == 1);
    queue.push(3); // Quay vòng về đầu mảng.
    queue.push(4); // Mở rộng khi đầu hàng không ở vị trí 0.
    assert(queue.front() == 2 && queue.size() == 3);
    assert(queue.pop() == 2 && queue.pop() == 3 && queue.pop() == 4);
    assert(queue.empty());
    bool threw = false;
    try { queue.pop(); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    HospitalSystem hospital(":memory:");
    auto first = hospital.registerPatient("A", 30, "", 5, "Tim mach");
    auto second = hospital.registerPatient("B", 30, "", 5, "Tim mach");
    auto emergency = hospital.registerPatient("C", 30, "", 1, "Tim mach");
    assert(hospital.callNextPatient("Tim mach")->ticket == emergency.ticket);
    assert(hospital.callNextPatient("Tim mach")->ticket == first.ticket);
    assert(hospital.callNextPatient("Tim mach")->ticket == second.ticket);
    assert(!hospital.callNextPatient("Tim mach"));

    auto transferred = hospital.registerPatient("D", 40, "", 5, "Tim mach");
    auto cancelled = hospital.registerPatient("E", 20, "", 5, "Tim mach");
    assert(hospital.cancelPatient(cancelled.ticket));
    assert(hospital.callSpecificPatient(transferred.ticket));
    assert(hospital.transferDepartment(transferred.ticket, "Nhi"));
    assert(!hospital.callNextPatient("Tim mach"));
    auto called = hospital.callNextPatient("Nhi");
    assert(called && called->ticket == transferred.ticket);
    assert(hospital.markAbsent(transferred.ticket));
    assert(!hospital.markAbsent(transferred.ticket));

    for (const auto& dept : {"Kham tong quat", "Chan thuong", "Nhi", "Mat", "Tai mui hong", "Chan doan hinh anh", "Cap cuu UTC2", "Tim mach", "Noi tiet"}) {
        auto patient = hospital.registerPatient("Kiem tra", 25, "", 5, dept);
        auto next = hospital.callNextPatient(dept);
        assert(next && next->ticket == patient.ticket && next->department == dept);
    }
}
