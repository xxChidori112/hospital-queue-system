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
}
