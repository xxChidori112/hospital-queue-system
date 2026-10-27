#include "hospital_system.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

HospitalSystem::HospitalSystem(const std::string& databasePath)
    : departments_{"Kham tong quat", "Chan thuong", "Nhi", "Mat", "Tai mui hong",
                   "Chan doan hinh anh", "Cap cuu UTC2", "Tim mach", "Noi tiet"} {
    for (const auto& department : departments_) regularQueues_.try_emplace(department);
    initializeDatabase(databasePath);
}

HospitalSystem::~HospitalSystem() {
    if (database_ != nullptr) sqlite3_close(database_);
}

void HospitalSystem::initializeDatabase(const std::string& databasePath) {
    if (sqlite3_open(databasePath.c_str(), &database_) != SQLITE_OK) {
        const std::string message = database_ == nullptr ? "Khong mo duoc co so du lieu SQLite."
                                                         : sqlite3_errmsg(database_);
        if (database_ != nullptr) sqlite3_close(database_);
        database_ = nullptr;
        throw std::runtime_error("SQLite: " + message);
    }
    sqlite3_busy_timeout(database_, 3000);
    executeSql("CREATE TABLE IF NOT EXISTS patients ("
               "ticket TEXT PRIMARY KEY, name TEXT NOT NULL, age INTEGER NOT NULL, symptom TEXT NOT NULL, "
               "department TEXT NOT NULL, triage_level INTEGER NOT NULL, arrival_order INTEGER NOT NULL, "
               "queue_version INTEGER NOT NULL, arrival_time TEXT NOT NULL, status INTEGER NOT NULL, emergency INTEGER NOT NULL)");
    executeSql("CREATE TABLE IF NOT EXISTS call_history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, ticket TEXT NOT NULL, patient_name TEXT NOT NULL, "
               "department TEXT NOT NULL, called_at TEXT NOT NULL, emergency INTEGER NOT NULL)");
    loadDatabase();
}

void HospitalSystem::executeSql(const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(database_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error == nullptr ? sqlite3_errmsg(database_) : error;
        sqlite3_free(error);
        throw std::runtime_error("SQLite: " + message);
    }
}

void HospitalSystem::loadDatabase() {
    sqlite3_stmt* statement = nullptr;
    const char* patientQuery = "SELECT ticket,name,age,symptom,department,triage_level,arrival_order,arrival_time,status,emergency "
                               "FROM patients ORDER BY arrival_order";
    if (sqlite3_prepare_v2(database_, patientQuery, -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("SQLite: khong doc duoc danh sach benh nhan.");
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto text = [statement](int column) {
            const auto* value = sqlite3_column_text(statement, column);
            return value == nullptr ? std::string{} : std::string(reinterpret_cast<const char*>(value));
        };
        Patient patient;
        patient.ticket = text(0);
        patient.name = text(1);
        patient.age = sqlite3_column_int(statement, 2);
        patient.symptom = text(3);
        patient.department = text(4);
        patient.triageLevel = sqlite3_column_int(statement, 5);
        patient.arrivalOrder = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 6));
        patient.arrivalTime = text(7);
        patient.status = static_cast<PatientStatus>(sqlite3_column_int(statement, 8));
        patient.emergency = sqlite3_column_int(statement, 9) != 0;
        patient.queueVersion = 0;
        nextArrivalOrder_ = std::max(nextArrivalOrder_, patient.arrivalOrder + 1);
        if (patient.ticket.size() > 2 && patient.ticket.rfind("BN", 0) == 0) {
            try { nextTicketNumber_ = std::max(nextTicketNumber_, static_cast<std::uint64_t>(std::stoull(patient.ticket.substr(2)) + 1)); }
            catch (const std::exception&) { /* Keep the generated sequence when legacy tickets are malformed. */ }
        }
        auto inserted = patients_.emplace(patient.ticket, std::move(patient));
        if (inserted.first->second.status == PatientStatus::Waiting) enqueuePatient(inserted.first->second);
    }
    sqlite3_finalize(statement);

    const char* historyQuery = "SELECT ticket,patient_name,department,called_at,emergency FROM call_history ORDER BY id DESC LIMIT 30";
    if (sqlite3_prepare_v2(database_, historyQuery, -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("SQLite: khong doc duoc lich su goi.");
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto text = [statement](int column) {
            const auto* value = sqlite3_column_text(statement, column);
            return value == nullptr ? std::string{} : std::string(reinterpret_cast<const char*>(value));
        };
        callHistory_.push_back(CallRecord{text(0), text(1), text(2), text(3), sqlite3_column_int(statement, 4) != 0});
    }
    sqlite3_finalize(statement);
}

void HospitalSystem::savePatient(const Patient& patient) {
    const char* sql = "INSERT INTO patients(ticket,name,age,symptom,department,triage_level,arrival_order,queue_version,arrival_time,status,emergency) "
                      "VALUES(?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(ticket) DO UPDATE SET "
                      "name=excluded.name,age=excluded.age,symptom=excluded.symptom,department=excluded.department,"
                      "triage_level=excluded.triage_level,arrival_order=excluded.arrival_order,queue_version=excluded.queue_version,"
                      "arrival_time=excluded.arrival_time,status=excluded.status,emergency=excluded.emergency";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_, sql, -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("SQLite: khong chuan bi duoc lenh luu benh nhan.");
    sqlite3_bind_text(statement, 1, patient.ticket.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, patient.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, patient.age);
    sqlite3_bind_text(statement, 4, patient.symptom.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, patient.department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 6, patient.triageLevel);
    sqlite3_bind_int64(statement, 7, static_cast<sqlite3_int64>(patient.arrivalOrder));
    sqlite3_bind_int64(statement, 8, static_cast<sqlite3_int64>(patient.queueVersion));
    sqlite3_bind_text(statement, 9, patient.arrivalTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 10, static_cast<int>(patient.status));
    sqlite3_bind_int(statement, 11, patient.emergency ? 1 : 0);
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) throw std::runtime_error("SQLite: khong luu duoc benh nhan.");
}

void HospitalSystem::saveCall(const CallRecord& record) {
    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT INTO call_history(ticket,patient_name,department,called_at,emergency) VALUES(?,?,?,?,?)";
    if (sqlite3_prepare_v2(database_, sql, -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("SQLite: khong chuan bi duoc lenh luu lich su goi.");
    sqlite3_bind_text(statement, 1, record.ticket.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, record.patientName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, record.department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, record.calledAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 5, record.emergency ? 1 : 0);
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) throw std::runtime_error("SQLite: khong luu duoc lich su goi.");
}

Patient HospitalSystem::registerPatient(std::string name, int age, std::string symptom,
                                        int triageLevel, std::string department) {
    if (name.empty()) throw std::invalid_argument("Ten benh nhan khong duoc de trong.");
    if (age < 0 || age > 120) throw std::invalid_argument("Tuoi phai nam trong khoang 0-120.");
    if (triageLevel < 1 || triageLevel > 5) throw std::invalid_argument("Muc do uu tien phai tu 1 den 5.");
    if (department.empty()) department = inferDepartment(symptom);
    if (std::find(departments_.begin(), departments_.end(), department) == departments_.end())
        throw std::invalid_argument("Khoa khong ton tai trong danh sach.");

    std::ostringstream ticket;
    ticket << "BN" << std::setw(4) << std::setfill('0') << nextTicketNumber_++;

    Patient patient;
    patient.ticket = ticket.str();
    patient.name = std::move(name);
    patient.age = age;
    patient.symptom = std::move(symptom);
    patient.department = std::move(department);
    patient.triageLevel = triageLevel;
    patient.arrivalOrder = nextArrivalOrder_++;
    patient.arrivalTime = currentTime();
    patient.emergency = triageLevel <= 2;
    auto inserted = patients_.emplace(patient.ticket, std::move(patient));
    enqueuePatient(inserted.first->second);
    try { savePatient(inserted.first->second); }
    catch (...) { patients_.erase(inserted.first); throw; }
    return inserted.first->second;
}

void HospitalSystem::enqueuePatient(Patient& patient) {
    ++patient.queueVersion;
    WaitingEntry entry{patient.ticket, patient.department, patient.triageLevel,
                       patient.arrivalOrder, patient.queueVersion};
    if (patient.emergency) emergencyQueue_.push(std::move(entry));
    else regularQueues_.at(patient.department).push(std::move(entry));
}

bool HospitalSystem::transferToEmergency(const std::string& ticket, int triageLevel) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::Waiting || triageLevel < 1 || triageLevel > 2)
        return false;
    const Patient previous = it->second;
    it->second.triageLevel = triageLevel;
    it->second.emergency = true;
    enqueuePatient(it->second); // queueVersion làm vô hiệu bản ghi cũ trong hàng đợi khoa.
    try { savePatient(it->second); }
    catch (...) { it->second = previous; return false; }
    return true;
}

bool HospitalSystem::isCurrentWaitingEntry(const WaitingEntry& entry) const {
    const auto it = patients_.find(entry.ticket);
    return it != patients_.end() && it->second.status == PatientStatus::Waiting &&
           it->second.queueVersion == entry.queueVersion && it->second.emergency == (entry.triageLevel <= 2);
}

std::optional<WaitingEntry> HospitalSystem::takeNextRegularEntry(const std::string& department) {
    if (!department.empty()) {
        const auto found = regularQueues_.find(department);
        if (found == regularQueues_.end()) return std::nullopt;
        auto& queue = found->second;
        while (!queue.empty() && !isCurrentWaitingEntry(queue.front())) queue.pop();
        if (queue.empty()) return std::nullopt;
        return queue.pop();
    }

    CircularQueue<WaitingEntry>* selectedQueue = nullptr;
    std::uint64_t oldestArrival = UINT64_MAX;
    for (auto& [department, queue] : regularQueues_) {
        (void)department;
        while (!queue.empty() && !isCurrentWaitingEntry(queue.front())) queue.pop();
        if (!queue.empty() && queue.front().arrivalOrder < oldestArrival) {
            oldestArrival = queue.front().arrivalOrder;
            selectedQueue = &queue;
        }
    }
    if (selectedQueue == nullptr) return std::nullopt;
    return selectedQueue->pop();
}

std::optional<Patient> HospitalSystem::callNextPatient(const std::string& department) {
    std::optional<WaitingEntry> entry;
    std::vector<WaitingEntry> deferred;
    const std::size_t emergencyCount = emergencyQueue_.size();
    for (std::size_t i = 0; i < emergencyCount; ++i) {
        auto candidate = emergencyQueue_.pop();
        if (!isCurrentWaitingEntry(candidate)) continue;
        if (department.empty() || candidate.department == department) {
            entry = std::move(candidate);
            break;
        }
        deferred.push_back(std::move(candidate));
    }
    for (auto& candidate : deferred) emergencyQueue_.push(std::move(candidate));
    if (!entry) entry = takeNextRegularEntry(department);
    if (!entry) return std::nullopt;

    return startConsultation(entry->ticket);
}

std::optional<Patient> HospitalSystem::callSpecificPatient(const std::string& ticket) {
    const auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::Waiting) return std::nullopt;
    return startConsultation(ticket);
}

std::optional<Patient> HospitalSystem::startConsultation(const std::string& ticket) {
    auto& patient = patients_.at(ticket);
    const Patient previous = patient;
    patient.status = PatientStatus::InConsultation;
    CallRecord record{patient.ticket, patient.name, patient.department, currentTime(), patient.emergency};
    try {
        savePatient(patient);
        saveCall(record);
    } catch (...) {
        patient.status = PatientStatus::Waiting;
        enqueuePatient(patient);
        try { savePatient(patient); } catch (...) { patient = previous; }
        throw;
    }
    callHistory_.push_front(std::move(record));
    while (callHistory_.size() > historyLimit_) callHistory_.pop_back();
    return patient;
}

bool HospitalSystem::completeConsultation(const std::string& ticket) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::InConsultation) return false;
    it->second.status = PatientStatus::Completed;
    try { savePatient(it->second); }
    catch (...) { it->second.status = PatientStatus::InConsultation; return false; }
    return true;
}

bool HospitalSystem::markAbsent(const std::string& ticket) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::InConsultation) return false;
    it->second.status = PatientStatus::Absent;
    try { savePatient(it->second); }
    catch (...) { it->second.status = PatientStatus::InConsultation; return false; }
    return true;
}

bool HospitalSystem::cancelPatient(const std::string& ticket) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::Waiting) return false;
    it->second.status = PatientStatus::Cancelled;
    try { savePatient(it->second); }
    catch (...) { it->second.status = PatientStatus::Waiting; return false; }
    return true;
}

bool HospitalSystem::transferDepartment(const std::string& ticket, const std::string& department) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::InConsultation ||
        regularQueues_.find(department) == regularQueues_.end()) return false;
    Patient previous = it->second;
    it->second.department = department;
    it->second.status = PatientStatus::Waiting;
    it->second.arrivalOrder = nextArrivalOrder_++;
    enqueuePatient(it->second);
    try { savePatient(it->second); }
    catch (...) { it->second = std::move(previous); return false; }
    return true;
}

std::vector<Patient> HospitalSystem::waitingPatients() const {
    std::vector<Patient> result;
    for (const auto& [ticket, patient] : patients_) {
        (void)ticket;
        if (patient.status == PatientStatus::Waiting) result.push_back(patient);
    }
    std::sort(result.begin(), result.end(), [](const Patient& a, const Patient& b) {
        if (a.emergency != b.emergency) return a.emergency > b.emergency;
        if (a.emergency && a.triageLevel != b.triageLevel) return a.triageLevel < b.triageLevel;
        return a.arrivalOrder < b.arrivalOrder;
    });
    return result;
}

std::vector<Patient> HospitalSystem::allPatients() const {
    std::vector<Patient> result;
    result.reserve(patients_.size());
    for (const auto& [ticket, patient] : patients_) { (void)ticket; result.push_back(patient); }
    std::sort(result.begin(), result.end(), [](const Patient& a, const Patient& b) {
        return a.arrivalOrder < b.arrivalOrder;
    });
    return result;
}

std::vector<CallRecord> HospitalSystem::callHistory() const {
    std::vector<CallRecord> result;
    result.reserve(callHistory_.size());
    for (std::size_t i = 0; i < callHistory_.size(); ++i) result.push_back(callHistory_.at(i));
    return result;
}

std::optional<int> HospitalSystem::estimatedWaitMinutes(const std::string& ticket) const {
    const auto found = patients_.find(ticket);
    if (found == patients_.end() || found->second.status != PatientStatus::Waiting) return std::nullopt;
    const Patient& patient = found->second;
    int ahead = 0;
    if (patient.emergency) {
        for (const auto& [id, other] : patients_) {
            (void)id;
            if (other.status == PatientStatus::Waiting && other.emergency && other.department == patient.department &&
                (other.triageLevel < patient.triageLevel ||
                 (other.triageLevel == patient.triageLevel && other.arrivalOrder < patient.arrivalOrder))) ++ahead;
        }
    } else {
        for (const auto& [id, other] : patients_) {
            (void)id;
            if (other.status == PatientStatus::Waiting && other.emergency && other.department == patient.department) ++ahead;
            else if (other.status == PatientStatus::Waiting && !other.emergency &&
                     other.department == patient.department && other.arrivalOrder < patient.arrivalOrder) ++ahead;
        }
    }
    return ahead * serviceMinutes(patient.department);
}

const Patient* HospitalSystem::findPatient(const std::string& ticket) const {
    const auto it = patients_.find(ticket);
    return it == patients_.end() ? nullptr : &it->second;
}

std::size_t HospitalSystem::waitingCount() const {
    std::size_t count = 0;
    for (const auto& [id, patient] : patients_) { (void)id; if (patient.status == PatientStatus::Waiting) ++count; }
    return count;
}

std::string HospitalSystem::statusText(PatientStatus status) {
    switch (status) {
        case PatientStatus::Waiting: return "Dang cho";
        case PatientStatus::InConsultation: return "Dang kham";
        case PatientStatus::Completed: return "Da kham xong";
        case PatientStatus::Absent: return "Vang mat";
        case PatientStatus::Cancelled: return "Da huy";
    }
    return "Khong ro";
}

std::string HospitalSystem::currentTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%H:%M:%S %d/%m/%Y");
    return output.str();
}

std::string HospitalSystem::inferDepartment(const std::string& symptom) const {
    std::string text = symptom;
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (text.find("tim") != std::string::npos || text.find("nguc") != std::string::npos ||
        text.find("heart") != std::string::npos) return "Tim mach";
    if (text.find("tre em") != std::string::npos || text.find("nhi") != std::string::npos) return "Nhi";
    if (text.find("chan thuong") != std::string::npos || text.find("gay xuong") != std::string::npos) return "Chan thuong";
    if (text.find("duong huyet") != std::string::npos || text.find("noi tiet") != std::string::npos) return "Noi tiet";
    return "Kham tong quat";
}

int HospitalSystem::serviceMinutes(const std::string& department) const {
    if (department == "Chan thuong") return 20;
    if (department == "Tim mach") return 25;
    if (department == "Noi tiet") return 18;
    if (department == "Nhi") return 15;
    return 15;
}
