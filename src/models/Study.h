#ifndef STUDY_H
#define STUDY_H

#include <string>
#include <vector>

struct Study {
    std::string studyInstanceUID; // DICOM Unique Identifier for the Study
    std::string patientId; // Foreign key to Patient
    std::string accessionNumber; // Accession Number
    std::string studyDate;       // YYYYMMDD
    std::string studyTime;       // HHMMSS
    std::string modality;        // e.g., NM (Nuclear Medicine), CR, CT, MR
    std::string studyDescription;
    std::string referringPhysicianName;
    std::string performingPhysicianName; // Optional
};

#endif // STUDY_H
