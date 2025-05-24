#ifndef CONSOLEUI_H
#define CONSOLEUI_H

#include "../db_connector/DatabaseService.h"
#include "../models/Patient.h"
#include "../models/Study.h"
#include <string>
#include <vector>

class ConsoleUI {
public:
    ConsoleUI(DatabaseService& dbService);

    void displayMainMenu();
    void displayPatientSearch();
    
    // New method to encapsulate the patient and study selection process
    // It will modify the passed-in Patient and Study objects by reference.
    void handlePatientAndStudySelection(Patient& outPatient, Study& outStudy);

    Patient getSelectedPatient(); 
    Study getSelectedStudy(const std::string& patientId); 

private:
    DatabaseService& dbService;
    Patient currentSelectedPatient;
    Study currentSelectedStudy;

    void listPatients(const std::vector<Patient>& patients);
    void listStudies(const std::vector<Study>& studies);
};

#endif // CONSOLEUI_H
