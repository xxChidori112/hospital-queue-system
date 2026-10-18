#include "hospital_system.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

HospitalSystem::HospitalSystem()
    : departments_{"Kham tong quat", "Tim mach", "Noi tiet", "Nhi", "Chan thuong"} {
    for (const auto& department : departments_) regularQueues_.try_emplace(department);
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
    it->second.triageLevel = triageLevel;
    it->second.emergency = true;
    enqueuePatient(it->second); // queueVersion làm vô hiệu bản ghi cũ trong hàng đợi khoa.
    return true;
}

bool HospitalSystem::isCurrentWaitingEntry(const WaitingEntry& entry) const {
    const auto it = patients_.find(entry.ticket);
    return it != patients_.end() && it->second.status == PatientStatus::Waiting &&
           it->second.queueVersion == entry.queueVersion && it->second.emergency == (entry.triageLevel <= 2);
}

std::optional<WaitingEntry> HospitalSystem::takeNextRegularEntry() {
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

std::optional<Patient> HospitalSystem::callNextPatient() {
    std::optional<WaitingEntry> entry;
    while (!emergencyQueue_.empty()) {
        auto candidate = emergencyQueue_.pop();
        if (isCurrentWaitingEntry(candidate)) { entry = std::move(candidate); break; }
    }
    if (!entry) entry = takeNextRegularEntry();
    if (!entry) return std::nullopt;

    auto& patient = patients_.at(entry->ticket);
    patient.status = PatientStatus::InConsultation;
    CallRecord record{patient.ticket, patient.name, patient.department, currentTime(), patient.emergency};
    callHistory_.push_front(std::move(record));
    while (callHistory_.size() > historyLimit_) callHistory_.pop_back();
    return patient;
}

bool HospitalSystem::completeConsultation(const std::string& ticket) {
    auto it = patients_.find(ticket);
    if (it == patients_.end() || it->second.status != PatientStatus::InConsultation) return false;
    it->second.status = PatientStatus::Completed;
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
            if (other.status == PatientStatus::Waiting && other.emergency &&
                (other.triageLevel < patient.triageLevel ||
                 (other.triageLevel == patient.triageLevel && other.arrivalOrder < patient.arrivalOrder))) ++ahead;
        }
    } else {
        for (const auto& [id, other] : patients_) {
            (void)id;
            if (other.status == PatientStatus::Waiting && other.emergency) ++ahead;
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
