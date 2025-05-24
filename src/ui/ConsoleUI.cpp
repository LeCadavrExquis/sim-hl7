#include "ConsoleUI.h"
#include <iostream>
#include <limits> // Required for std::numeric_limits
#include <string> // Required for std::string, std::getline

ConsoleUI::ConsoleUI(DatabaseService& service) : dbService(service) {}

void ConsoleUI::displayMainMenu() {
    std::cout << "\nConsoleUI::displayMainMenu() called (Note: Main loop is in main.cpp)\n";
}

// New method to handle the combined patient and study selection flow
void ConsoleUI::handlePatientAndStudySelection(Patient& outPatient, Study& outStudy) {
    // Reset internal selections at the start of a new selection process
    currentSelectedPatient = {}; 
    currentSelectedStudy = {};

    // 1. Patient Search and Selection
    displayPatientSearch(); // This will set currentSelectedPatient if successful

    if (currentSelectedPatient.patientID.empty()) {
        std::cout << "No patient was selected. Cannot proceed to study selection." << std::endl;
        outPatient = {};
        outStudy = {};
        return;
    }

    // 2. Study Selection for the chosen patient
    // getSelectedStudy will use currentSelectedPatient.patientID and set currentSelectedStudy
    getSelectedStudy(currentSelectedPatient.patientID); 

    if (currentSelectedStudy.studyInstanceUID.empty()) {
        std::cout << "No study was selected for patient: " << currentSelectedPatient.name << std::endl;
        // Keep patient selected, but no study
        outPatient = currentSelectedPatient;
        outStudy = {};
        return;
    }

    // Both patient and study are selected
    outPatient = currentSelectedPatient;
    outStudy = currentSelectedStudy;

    std::cout << "Selection complete: Patient " << outPatient.name 
              << ", Study " << outStudy.studyDescription << std::endl;
}


// displayPatientSearch now primarily sets currentSelectedPatient
void ConsoleUI::displayPatientSearch() {
    std::string searchTerm;

    std::cin.clear(); 

    std::cout << "\nEnter patient search term (name or ID or type 'all' to list all): " << std::flush;
    std::getline(std::cin >> std::ws, searchTerm); // USE std::ws

    // Trim leading and trailing whitespace from searchTerm.
    size_t first = searchTerm.find_first_not_of(" \\t\\n\\r\\f\\v");
    if (std::string::npos == first) { // String is empty or all whitespace
        searchTerm.clear();
    } else {
        size_t last = searchTerm.find_last_not_of(" \\t\\n\\r\\f\\v");
        searchTerm = searchTerm.substr(first, (last - first + 1));
    }

    std::vector<Patient> patients;
    if (searchTerm.empty() || searchTerm == "all") {
        patients = dbService.getAllPatients();
    } else {
        patients = dbService.searchPatients(searchTerm);
    }

    if (patients.empty()) {
        std::cout << "No patients found.\n";
        currentSelectedPatient = {}; // Clear selection
        return;
    }

    listPatients(patients);

    int patientIndex = -1;
    std::string inputLine; // For reading user input

    while (true) {
        std::cout << "Select patient by number (or 0 to cancel): " << std::flush; // Ensure flush
        std::getline(std::cin >> std::ws, inputLine); // USE std::ws

        try {
            if (inputLine.empty()) { // Handle empty input if necessary
                 std::cout << "Invalid input. Please enter a number.\n";
                 patientIndex = -1; // Or some other indicator of invalid/no choice
                 continue;
            }
            patientIndex = std::stoi(inputLine);
        } catch (const std::invalid_argument& ia) {
            std::cout << "Invalid input. Please enter a number.\n";
            patientIndex = -1; // Reset or handle as appropriate
            continue;
        } catch (const std::out_of_range& oor) {
            std::cout << "Input out of range. Please enter a valid number.\n";
            patientIndex = -1; // Reset or handle as appropriate
            continue;
        }

        if (patientIndex >= 0 && patientIndex <= static_cast<int>(patients.size())) {
            break;
        }
        std::cout << "Invalid selection. Try again.\n";
    }

    if (patientIndex > 0 && patientIndex <= static_cast<int>(patients.size())) {
        currentSelectedPatient = patients[patientIndex - 1];
        std::cout << "Selected patient: " << currentSelectedPatient.name 
                  << " (ID: " << currentSelectedPatient.patientID << ")" << std::endl;
    } else {
        std::cout << "Patient selection cancelled.\n";
        currentSelectedPatient = {}; // Clear selection
    }
}

// getSelectedPatient returns the internally stored patient
Patient ConsoleUI::getSelectedPatient() {
    return currentSelectedPatient;
}

// getSelectedStudy now primarily sets currentSelectedStudy for the given patientId
// It is called by handlePatientAndStudySelection.
Study ConsoleUI::getSelectedStudy(const std::string& patientId) {
    if (patientId.empty()) {
        std::cout << "Cannot select study without a patient ID.\n";
        currentSelectedStudy = {};
        return currentSelectedStudy;
    }

    std::vector<Study> studies = dbService.getStudiesForPatient(patientId);

    if (studies.empty()) {
        std::cout << "No studies found for patient ID: " << patientId << ".\n";
        currentSelectedStudy = {};
        return currentSelectedStudy;
    }

    listStudies(studies); // listStudies uses currentSelectedPatient for context, ensure it's set

    int studyIndex = -1;
    std::string inputLine; // For reading user input

    while (true) {
        std::cout << "Select study by number (or 0 to cancel): " << std::flush; // Ensure flush
        std::getline(std::cin >> std::ws, inputLine); // USE std::ws

        try {
            if (inputLine.empty()) { // Handle empty input
                std::cout << "Invalid input. Please enter a number.\n";
                studyIndex = -1; 
                continue;
            }
            studyIndex = std::stoi(inputLine);
        } catch (const std::invalid_argument& ia) {
            std::cout << "Invalid input. Please enter a number.\n";
            studyIndex = -1; // Reset
            continue;
        } catch (const std::out_of_range& oor) {
            std::cout << "Input out of range. Please enter a valid number.\n";
            studyIndex = -1; // Reset
            continue;
        }

        if (studyIndex >= 0 && studyIndex <= static_cast<int>(studies.size())) {
            break;
        }
        std::cout << "Invalid selection. Try again.\n";
    }

    if (studyIndex > 0 && studyIndex <= static_cast<int>(studies.size())) {
        currentSelectedStudy = studies[studyIndex - 1];
        std::cout << "Selected study: " << currentSelectedStudy.studyDescription << std::endl;
    } else {
        std::cout << "Study selection cancelled.\n";
        currentSelectedStudy = {}; // Clear selection
    }
    return currentSelectedStudy;
}

void ConsoleUI::listPatients(const std::vector<Patient>& patients) {
    std::cout << "\n--- Patients --- \n";
    if (patients.empty()) {
        std::cout << "No patients to display.\n";
        return;
    }
    for (size_t i = 0; i < patients.size(); ++i) {
        std::cout << i + 1 << ". " << patients[i].name 
                  << " (ID: " << patients[i].patientID 
                  << ", DOB: " << patients[i].dateOfBirth 
                  << ", Sex: " << patients[i].sex << ")\n";
    }
    std::cout << "----------------\n";
}

void ConsoleUI::listStudies(const std::vector<Study>& studies) {
    if (!currentSelectedPatient.patientID.empty()) {
        std::cout << "\n--- Studies for Patient ID: " << currentSelectedPatient.patientID << " (Name: " << currentSelectedPatient.name << ") --- \n";
    } else {
        std::cout << "\n--- Studies --- \n"; // Generic header if patient context is missing
    }

    if (studies.empty()) {
        std::cout << "No studies to display.\n";
        return;
    }
    for (size_t i = 0; i < studies.size(); ++i) {
        std::cout << i + 1 << ". " << studies[i].studyDescription
                  << " (UID: " << studies[i].studyInstanceUID
                  << ", Accession: " << studies[i].accessionNumber
                  << ", Modality: " << studies[i].modality 
                  << ", Date: " << studies[i].studyDate << ")\n";
    }
    std::cout << "-------------------------------------\n";
}
