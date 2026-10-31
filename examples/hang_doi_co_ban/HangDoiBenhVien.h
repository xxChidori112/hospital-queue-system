#ifndef HANG_DOI_BENH_VIEN_H
#define HANG_DOI_BENH_VIEN_H

#include <string>

struct BenhNhan {
    int maBenhNhan = 0;
    std::string ten;
    int doUuTien = 5;
    int soThuTu = 0;

    bool operator<(const BenhNhan& khac) const;
};

class HangDoiBenhVien {
private:
    BenhNhan* danhSach;
    int sucChua;
    int dau;
    int cuoi;
    int soLuong;
    int sttTuDong;

    void moRong();

public:
    explicit HangDoiBenhVien(int sc);
    ~HangDoiBenhVien();
    HangDoiBenhVien(const HangDoiBenhVien&) = delete;
    HangDoiBenhVien& operator=(const HangDoiBenhVien&) = delete;

    void xepHang(int ma, std::string ten, int doUuTien);
    BenhNhan goiKham();
    void goiSoTuDong();
    bool laRong() const;
};

#endif
