#pragma once

#include "circular_queue.hpp"
#include "deque.hpp"
#include "priority_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct sqlite3;

enum class PatientStatus { Waiting, InConsultation, Completed, Absent, Cancelled };

struct Patient {
    std::string ticket;
    std::string name;
    int age = 0;
    int birthYear = 0; // 0: hồ sơ cũ chưa có năm sinh.
    std::string gender;
    std::string category;
    std::string symptom;
    std::string department;
    int triageLevel = 5; // 1: nguy kich, 5: thong thuong
    std::uint64_t arrivalOrder = 0;
    std::uint64_t queueVersion = 0;
    std::string arrivalTime;
    PatientStatus status = PatientStatus::Waiting;
    bool emergency = false;
};

// operator<: mức 1 đứng trước; nếu cùng mức, người đến trước đứng trước.
struct WaitingEntry {
    std::string ticket;
    std::string department;
    int triageLevel = 5;
    std::uint64_t arrivalOrder = 0;
    std::uint64_t queueVersion = 0;

    bool operator<(const WaitingEntry& other) const noexcept {
        if (triageLevel != other.triageLevel) return triageLevel > other.triageLevel;
        return arrivalOrder > other.arrivalOrder;
    }
};

struct CallRecord {
    std::string ticket;
    std::string patientName;
    std::string department;
    std::string calledAt;
    bool emergency = false;
};

class HospitalSystem {
public:
    explicit HospitalSystem(const std::string& databasePath = "hospital.db");
    ~HospitalSystem();
    HospitalSystem(const HospitalSystem&) = delete;
    HospitalSystem& operator=(const HospitalSystem&) = delete;

    Patient registerPatient(std::string name, int age, std::string symptom,
                            int triageLevel, std::string department = "", int birthYear = 0,
                            std::string gender = "", std::string category = "");
    bool transferToEmergency(const std::string& ticket, int triageLevel = 1);
    std::optional<Patient> callNextPatient(const std::string& department = "");
    std::optional<Patient> callSpecificPatient(const std::string& ticket);
    bool completeConsultation(const std::string& ticket);
    bool markAbsent(const std::string& ticket);
    bool cancelPatient(const std::string& ticket);
    bool transferDepartment(const std::string& ticket, const std::string& department);

    [[nodiscard]] std::vector<Patient> waitingPatients() const;
    [[nodiscard]] std::vector<Patient> allPatients() const;
    [[nodiscard]] std::vector<CallRecord> callHistory() const;
    [[nodiscard]] std::optional<int> estimatedWaitMinutes(const std::string& ticket) const;
    [[nodiscard]] const std::vector<std::string>& departments() const noexcept { return departments_; }
    [[nodiscard]] const Patient* findPatient(const std::string& ticket) const;
    [[nodiscard]] std::size_t waitingCount() const;

    static std::string statusText(PatientStatus status);
    static std::string currentTime();

private:
    bool isCurrentWaitingEntry(const WaitingEntry& entry) const;
    void enqueuePatient(Patient& patient);
    std::optional<Patient> startConsultation(const std::string& ticket);
    std::optional<WaitingEntry> takeNextRegularEntry(const std::string& department);
    void initializeDatabase(const std::string& databasePath);
    void loadDatabase();
    void savePatient(const Patient& patient);
    void saveCall(const CallRecord& record);
    void executeSql(const char* sql);
    std::string inferDepartment(const std::string& symptom) const;
    int serviceMinutes(const std::string& department) const;

    std::vector<std::string> departments_;
    std::map<std::string, CircularQueue<WaitingEntry>> regularQueues_;
    PriorityQueue<WaitingEntry> emergencyQueue_;
    std::unordered_map<std::string, Patient> patients_;
    Deque<CallRecord> callHistory_;
    std::uint64_t nextTicketNumber_ = 1;
    std::uint64_t nextArrivalOrder_ = 1;
    std::size_t historyLimit_ = 30;
    sqlite3* database_ = nullptr;
};
