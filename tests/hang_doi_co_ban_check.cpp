#include "../examples/hang_doi_co_ban/HangDoiBenhVien.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <stdexcept>
#include <vector>

int main() {
    bool caught = false;
    try { HangDoiBenhVien invalid(0); } catch (const std::invalid_argument&) { caught = true; }
    assert(caught);
    HangDoiBenhVien hd(2);
    caught = false;
    try { hd.goiKham(); } catch (const std::out_of_range&) { caught = true; }
    assert(caught);
    caught = false;
    try { hd.xepHang(1, "   ", 5); } catch (const std::invalid_argument&) { caught = true; }
    assert(caught);
    caught = false;
    try { hd.xepHang(1, "A", 0); } catch (const std::invalid_argument&) { caught = true; }
    assert(caught);

    std::vector<BenhNhan> expected;
    std::mt19937 random(1234);
    int arrival = 0;
    for (int i = 1; i <= 10000; ++i) {
        if (expected.empty() || random() % 2 == 0) {
            int level = 1 + static_cast<int>(random() % 5);
            hd.xepHang(i, "Benh nhan mau", level);
            expected.push_back({i, "Benh nhan mau", level, ++arrival});
            std::stable_sort(expected.begin(), expected.end(), [](const auto& a, const auto& b) {
                return a.doUuTien < b.doUuTien;
            });
        } else {
            const auto patient = hd.goiKham();
            assert(patient.maBenhNhan == expected.front().maBenhNhan);
            assert(patient.soThuTu == expected.front().soThuTu);
            expected.erase(expected.begin());
        }
        assert(hd.laRong() == expected.empty());
    }
    for (const auto& patient : expected) assert(hd.goiKham().maBenhNhan == patient.maBenhNhan);
    assert(hd.laRong());
}
