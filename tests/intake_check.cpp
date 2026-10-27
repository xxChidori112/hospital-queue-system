#include "hospital_system.hpp"
#include <sqlite3.h>
#include <cassert>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 2);
    sqlite3* db = nullptr;
    assert(sqlite3_open(argv[1], &db) == SQLITE_OK);
    assert(sqlite3_exec(db, "CREATE TABLE patients(ticket TEXT PRIMARY KEY,name TEXT NOT NULL,age INTEGER NOT NULL,symptom TEXT NOT NULL,department TEXT NOT NULL,triage_level INTEGER NOT NULL,arrival_order INTEGER NOT NULL,queue_version INTEGER NOT NULL,arrival_time TEXT NOT NULL,status INTEGER NOT NULL,emergency INTEGER NOT NULL);"
                            "INSERT INTO patients VALUES('BN0001','Cu',30,'','Nhi',5,1,1,'10:00',0,0);", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(db);
    const auto timestamp = HospitalSystem::currentTime();
    const int year = std::stoi(timestamp.substr(timestamp.size() - 4));
    std::string ticket;
    {
        HospitalSystem hospital(argv[1]);
        const Patient* old = hospital.findPatient("BN0001");
        assert(old && old->age == 30 && old->birthYear == 0 && old->gender.empty());
        auto patient = hospital.registerPatient("Moi", 23, "Ho", 5, "Nhi", year - 23, "female", "student_bhyt");
        ticket = patient.ticket;
        bool invalid = false;
        try { hospital.registerPatient("Sai", 23, "Ho", 5, "Nhi", year + 1, "female", "student_bhyt"); }
        catch (const std::invalid_argument&) { invalid = true; }
        assert(invalid);
    }
    {
        HospitalSystem hospital(argv[1]);
        const Patient* patient = hospital.findPatient(ticket);
        assert(patient && patient->birthYear == year - 23 && patient->gender == "female" && patient->category == "student_bhyt");
        auto first = hospital.callNextPatient("Nhi");
        assert(first && first->ticket == "BN0001");
    }
}
