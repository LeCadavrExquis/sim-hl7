#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <string>
#include <vector>
#include "../models/Patient.h"
#include "../models/Study.h"

// Include ODBC headers
#include <sql.h>
#include <sqlext.h>

// Include DicomParser header
#include "dicom_parser/DicomParser.h"

class DatabaseService {
public:
    DatabaseService();
    ~DatabaseService();

    bool connect(const std::string& dsn, const std::string& user, const std::string& password);
    void disconnect();

    std::vector<Patient> getAllPatients();
    std::vector<Patient> searchPatients(const std::string& searchTerm);
    Patient getPatientById(const std::string& patientId);
    std::vector<Study> getStudiesForPatient(const std::string& patientId);

    // New methods for DICOM data
    Patient getPatientFromDicom(const std::string& dicomFilePath);
    Study getStudyFromDicom(const std::string& dicomFilePath);
    std::vector<Study> getStudiesFromDicomDirectory(const std::string& directoryPath); // Optional

private:
    // ODBC handles
    SQLHENV henv; // Environment handle
    SQLHDBC hdbc; // Connection handle
    SQLHSTMT hstmt; // Statement handle (can be allocated per query or reused)
    bool connected;

    void handleError(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& message);
};

#endif // DATABASESERVICE_H
