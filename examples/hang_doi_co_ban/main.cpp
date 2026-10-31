#include "HangDoiBenhVien.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

void xuLy() {
    int n;
    if (!(std::cin >> n) || n < 0)
        throw std::invalid_argument("So benh nhan phai la so nguyen khong am.");
    HangDoiBenhVien hd(n == 0 ? 1 : n);
    for (int i = 0; i < n; ++i) {
        int ma, doUuTien;
        std::string ten;
        if (!(std::cin >> ma)) throw std::invalid_argument("Ma benh nhan khong hop le.");
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (!std::getline(std::cin, ten)) throw std::invalid_argument("Thieu ten benh nhan.");
        if (!(std::cin >> doUuTien)) throw std::invalid_argument("Muc uu tien khong hop le.");
        hd.xepHang(ma, ten, doUuTien);
    }
    while (!hd.laRong()) hd.goiSoTuDong();
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    try {
        xuLy();
    } catch (const std::exception& loi) {
        std::cerr << "Loi: " << loi.what() << '\n';
        return 1;
    }
    return 0;
}
