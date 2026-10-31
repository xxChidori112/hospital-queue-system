#include "HangDoiBenhVien.h"

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

bool BenhNhan::operator<(const BenhNhan& khac) const {
    // Mức 1 ưu tiên cao nhất; cùng mức thì người đến trước được gọi trước.
    if (doUuTien != khac.doUuTien) return doUuTien > khac.doUuTien;
    return soThuTu > khac.soThuTu;
}

HangDoiBenhVien::HangDoiBenhVien(int sc)
    : danhSach(nullptr), sucChua(sc), dau(0), cuoi(-1), soLuong(0), sttTuDong(0) {
    if (sc <= 0) throw std::invalid_argument("Suc chua phai lon hon 0.");
    danhSach = new BenhNhan[sucChua];
}

HangDoiBenhVien::~HangDoiBenhVien() {
    delete[] danhSach;
}

void HangDoiBenhVien::moRong() {
    if (sucChua > std::numeric_limits<int>::max() / 2)
        throw std::length_error("Hang doi qua lon.");
    const int sucChuaMoi = sucChua * 2;
    auto mangMoi = std::make_unique<BenhNhan[]>(sucChuaMoi);
    for (int i = 0; i < soLuong; ++i)
        mangMoi[i] = danhSach[(dau + i) % sucChua];
    delete[] danhSach;
    danhSach = mangMoi.release();
    sucChua = sucChuaMoi;
    dau = 0;
    cuoi = soLuong - 1;
}

void HangDoiBenhVien::xepHang(int ma, std::string ten, int doUuTien) {
    if (ma <= 0) throw std::invalid_argument("Ma benh nhan phai lon hon 0.");
    if (ten.find_first_not_of(" \t\r\n") == std::string::npos)
        throw std::invalid_argument("Ten benh nhan khong duoc de trong.");
    if (doUuTien < 1 || doUuTien > 5)
        throw std::invalid_argument("Muc uu tien phai tu 1 den 5.");
    if (sttTuDong == std::numeric_limits<int>::max())
        throw std::overflow_error("Da het so thu tu.");
    if (soLuong == sucChua) moRong();

    BenhNhan bn{ma, std::move(ten), doUuTien, sttTuDong + 1};
    int i = cuoi;
    int viTriTiepTheo = (cuoi + 1) % sucChua;
    int soPhanTuDaKiemTra = 0;

    while (soPhanTuDaKiemTra < soLuong && danhSach[i] < bn) {
        danhSach[viTriTiepTheo] = std::move(danhSach[i]);
        viTriTiepTheo = i;
        i = (i - 1 + sucChua) % sucChua;
        ++soPhanTuDaKiemTra;
    }
    danhSach[viTriTiepTheo] = std::move(bn);
    cuoi = (cuoi + 1) % sucChua;
    ++soLuong;
    ++sttTuDong;
}

BenhNhan HangDoiBenhVien::goiKham() {
    if (laRong()) throw std::out_of_range("Khong the goi kham khi hang doi rong.");
    BenhNhan bn = std::move(danhSach[dau]);
    dau = (dau + 1) % sucChua;
    --soLuong;
    return bn;
}

void HangDoiBenhVien::goiSoTuDong() {
    if (laRong()) {
        std::cout << "[LOA THONG BAO] Hien tai khong co benh nhan trong hang doi.\n";
        return;
    }
    BenhNhan bn = goiKham();
    std::cout << "[LOA THONG BAO] Xin moi benh nhan STT " << bn.soThuTu
              << " - " << bn.ten << " (Ma BN: " << bn.maBenhNhan << ")"
              << " den Phong Kham!\n";
}

bool HangDoiBenhVien::laRong() const {
    return soLuong == 0;
}
