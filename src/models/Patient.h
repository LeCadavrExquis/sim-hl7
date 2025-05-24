#ifndef PATIENT_H
#define PATIENT_H

#include <string>
#include <vector>

struct Patient {
    std::string patientID;    // Unique identifier for the patient (e.g., MRN)
    std::string name;         // Full name of the patient (e.g., "Doe, John C.")
    // std::string lastName;  // If separate components are preferred over a single name field
    // std::string firstName;
    // std::string middleName; 
    std::string dateOfBirth;  // Format: YYYYMMDD or as per CDA/DB requirements
    std::string sex;          // HL7 Values: M, F, O, U (Male, Female, Other, Unknown)
    
    // Optional demographic details that might be useful for CDA body or other sections
    std::string addressStreet; 
    std::string addressCity;
    std::string addressState;
    std::string addressZip;
    std::string addressCountry;
    std::string phoneNumber;
};

#endif // PATIENT_H
