#include "DicomParser.h"
#include <dicomhero6/dicomhero.h>
#include <iostream>
#include <iomanip> // Required for std::hex
#include <optional> // Required for std::optional

DicomParser::DicomParser() {
    // Constructor for DicomParser
}

bool DicomParser::loadFile(const std::string& filePath) {
    try {
        dataSet.emplace(dicomhero::CodecFactory::load(filePath));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading DICOM file with DicomHero6: " << e.what() << " (file: " << filePath << ")" << std::endl;
        dataSet.reset();
        return false;
    }
}

std::string DicomParser::getString(dicomhero::tagId_t tagValue) const {
    if (!dataSet.has_value()) { // Check if optional has a value
        std::cerr << "Dataset not loaded." << std::endl;
        return "";
    }
    try {
        std::wstring w_value = dataSet->getUnicodeString(dicomhero::TagId(tagValue), 0);
        if (!w_value.empty()) {
            // Simple conversion from wstring to string (may lose data for non-ASCII)
            return std::string(w_value.begin(), w_value.end());
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading tag " << std::hex << static_cast<uint32_t>(tagValue) << ": " << e.what() << std::endl;
    }
    return "";
}

Patient DicomParser::getPatientInfo() const {
    Patient p;
    if (!dataSet.has_value()) { // Check if optional has a value
        std::cerr << "Dataset not loaded when calling getPatientInfo." << std::endl;
        return p; // Return empty Patient object
    }

    p.patientID = getString(dicomhero::tagId_t::PatientID_0010_0020);
    try {
        dicomhero::UnicodePersonName personName = dataSet->getUnicodePersonName(dicomhero::TagId(dicomhero::tagId_t::PatientName_0010_0010), 0);
        std::wstring w_name = personName.getAlphabeticRepresentation();
        p.name = std::string(w_name.begin(), w_name.end());
    } catch (const std::exception& e) {
        std::cerr << "Error reading PatientName: " << e.what() << std::endl;
    }
    p.dateOfBirth = getString(dicomhero::tagId_t::PatientBirthDate_0010_0030);
    p.sex = getString(dicomhero::tagId_t::PatientSex_0010_0040);

    return p;
}

Study DicomParser::getStudyInfo() const {
    Study s;
    if (!dataSet.has_value()) {
        std::cerr << "Dataset not loaded when calling getStudyInfo." << std::endl;
        return s;
    }

    s.studyInstanceUID = getString(dicomhero::tagId_t::StudyInstanceUID_0020_000D);
    s.patientId = getString(dicomhero::tagId_t::PatientID_0010_0020);
    s.accessionNumber = getString(dicomhero::tagId_t::AccessionNumber_0008_0050);
    s.studyDate = getString(dicomhero::tagId_t::StudyDate_0008_0020);
    s.studyTime = getString(dicomhero::tagId_t::StudyTime_0008_0030);
    s.modality = getString(dicomhero::tagId_t::Modality_0008_0060);
    s.studyDescription = getString(dicomhero::tagId_t::StudyDescription_0008_1030);
    try {
        dicomhero::UnicodePersonName physicianName = dataSet->getUnicodePersonName(dicomhero::TagId(dicomhero::tagId_t::ReferringPhysicianName_0008_0090), 0);
        std::wstring w_physicianName = physicianName.getAlphabeticRepresentation();
        s.referringPhysicianName = std::string(w_physicianName.begin(), w_physicianName.end());
    } catch (const std::exception& e) {
        std::cerr << "Error reading ReferringPhysicianName: " << e.what() << std::endl;
    }

    return s;
}

