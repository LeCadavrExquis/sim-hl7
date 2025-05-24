#ifndef DICOMPARSER_H
#define DICOMPARSER_H

#include <dicomhero6/dicomhero.h> // Changed from dcmtk
#include "../models/Patient.h"
#include "../models/Study.h"
#include <string>
#include <optional> // Required for std::optional

class DicomParser {
public:
    DicomParser();
    bool loadFile(const std::string& filePath);
    Patient getPatientInfo() const;
    Study getStudyInfo() const;

private:
    std::optional<dicomhero::DataSet> dataSet; // Changed from DcmFileFormat to std::optional<dicomhero::DataSet>
    // Helper to extract string values, adapted for DicomHero6
    std::string getString(dicomhero::tagId_t tagValue) const;
    // Consider adding a helper for potentially multi-valued or specific types if needed
};

#endif // DICOMPARSER_H
