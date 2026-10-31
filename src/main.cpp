#include "hospital_system.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace {
int readInt(const std::string& prompt, int minimum, int maximum) {
    for (;;) {
        std::cout << prompt;
        int value;
        if (std::cin >> value && value >= minimum && value <= maximum) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "Gia tri khong hop le. Vui long nhap lai.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void printPatient(const Patient& patient, const HospitalSystem& system, bool includeWait = false) {
    std::cout << patient.ticket << " | " << patient.name << " | " << patient.age << " tuoi | "
              << patient.department << " | uu tien " << patient.triageLevel << " | "
              << HospitalSystem::statusText(patient.status);
    if (patient.emergency) std::cout << " | CAP CUU";
    if (includeWait) {
        const auto wait = system.estimatedWaitMinutes(patient.ticket);
        if (wait) std::cout << " | cho uoc tinh " << *wait << " phut";
    }
    std::cout << '\n';
}

void printMenu() {
    std::cout << "\n========== HE THONG XEP HANG KHAM BENH =========="
              << "\n1. Tiep nhan benh nhan"
              << "\n2. Goi benh nhan tiep theo"
              << "\n3. Hoan tat luot kham"
              << "\n4. Chuyen benh nhan dang cho sang cap cuu"
              << "\n5. Xem danh sach dang cho"
              << "\n6. Tra cuu benh nhan / thoi gian cho"
              << "\n7. Xem lich su goi kham"
              << "\n8. Xem tat ca benh nhan"
              << "\n0. Thoat"
              << "\n==================================================\n";
}
}

int main() {
    HospitalSystem system;
    std::cout << "HE THONG XEP HANG KHAM BENH & CAP CUU\n"
              << "Muc do 1 la nguy kich nhat; muc 5 la thong thuong.\n";

    for (;;) {
        printMenu();
        const int choice = readInt("Chon chuc nang: ", 0, 8);
        try {
            if (choice == 0) break;
            if (choice == 1) {
                auto name = readLine("Ho ten: ");
                const int age = readInt("Tuoi (0-120): ", 0, 120);
                auto symptom = readLine("Trieu chung: ");
                const int triage = readInt("Muc uu tien (1 nguy kich - 5 thong thuong): ", 1, 5);
                std::cout << "Chon khoa (0 tu dong):\n";
                for (std::size_t i = 0; i < system.departments().size(); ++i)
                    std::cout << i + 1 << ". " << system.departments()[i] << '\n';
                const int departmentChoice = readInt("Khoa: ", 0, static_cast<int>(system.departments().size()));
                std::string department;
                if (departmentChoice > 0) department = system.departments()[static_cast<std::size_t>(departmentChoice - 1)];
                const auto patient = system.registerPatient(name, age, symptom, triage, department);
                std::cout << "Da tiep nhan: ";
                printPatient(patient, system, true);
            } else if (choice == 2) {
                const auto patient = system.callNextPatient();
                if (patient) std::cout << "Moi benh nhan " << patient->ticket << " - " << patient->name
                                       << " den khoa " << patient->department << ".\n";
                else std::cout << "Hien khong co benh nhan dang cho.\n";
            } else if (choice == 3) {
                const auto ticket = readLine("Ma phieu: ");
                std::cout << (system.completeConsultation(ticket) ? "Da cap nhat hoan tat luot kham.\n"
                                                                    : "Khong tim thay luot kham dang thuc hien.\n");
            } else if (choice == 4) {
                const auto ticket = readLine("Ma phieu can chuyen cap cuu: ");
                const int level = readInt("Muc cap cuu (1 nguy kich, 2 rat khan): ", 1, 2);
                std::cout << (system.transferToEmergency(ticket, level) ? "Da chuyen vao hang uu tien cap cuu.\n"
                                                                          : "Khong the chuyen: kiem tra ma phieu va trang thai cho.\n");
            } else if (choice == 5) {
                const auto waiting = system.waitingPatients();
                std::cout << "So benh nhan dang cho: " << waiting.size() << '\n';
                for (const auto& patient : waiting) printPatient(patient, system, true);
            } else if (choice == 6) {
                const auto ticket = readLine("Ma phieu: ");
                const auto* patient = system.findPatient(ticket);
                if (!patient) std::cout << "Khong tim thay ma phieu.\n";
                else printPatient(*patient, system, true);
            } else if (choice == 7) {
                const auto records = system.callHistory();
                if (records.empty()) std::cout << "Chua co luot goi kham.\n";
                for (const auto& record : records)
                    std::cout << record.calledAt << " | " << record.ticket << " | " << record.patientName
                              << " | " << record.department << (record.emergency ? " | CAP CUU" : "") << '\n';
            } else {
                for (const auto& patient : system.allPatients()) printPatient(patient, system);
            }
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
    std::cout << "Da dong he thong.\n";
    return 0;
}
