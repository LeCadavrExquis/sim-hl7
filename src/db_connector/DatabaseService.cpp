#include "DatabaseService.h"
#include <iostream>
#include <stdexcept>
#include "dicom_parser/DicomParser.h"

DatabaseService::DatabaseService() : henv(SQL_NULL_HENV), hdbc(SQL_NULL_HDBC), hstmt(SQL_NULL_HSTMT), connected(false) {
    SQLRETURN ret;
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        // Cannot use handleError here as it might depend on the handles themselves
        std::cerr << "Error allocating environment handle: " << ret << std::endl;
        throw std::runtime_error("Failed to allocate ODBC environment handle.");
    }

    ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        henv = SQL_NULL_HENV;
        std::cerr << "Error setting ODBC version: " << ret << std::endl;
        throw std::runtime_error("Failed to set ODBC version.");
    }
    std::cout << "DatabaseService initialized successfully." << std::endl;
}

DatabaseService::~DatabaseService() {
    disconnect();
    if (henv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        henv = SQL_NULL_HENV;
    }
}

bool DatabaseService::connect(const std::string& dsn, const std::string& user, const std::string& password) {
    if (connected) {
        std::cout << "Already connected." << std::endl;
        return true;
    }
    if (henv == SQL_NULL_HENV) {
        std::cerr << "Environment handle not initialized." << std::endl;
        return false;
    }

    SQLRETURN ret;
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_ENV, henv, "Error allocating connection handle");
        return false;
    }

    // Construct connection string
    std::string connStr = "DSN=" + dsn + ";";
    if (!user.empty()) {
        connStr += "UID=" + user + ";";
    }
    if (!password.empty()) {
        connStr += "PWD=" + password + ";";
    }

    ret = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connStr.c_str(),
                           SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error connecting to DSN: " + dsn);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        hdbc = SQL_NULL_HDBC;
        return false;
    }

    std::cout << "Successfully connected to DSN: " << dsn << std::endl;
    connected = true;
    return true;
}

void DatabaseService::disconnect() {
    if (hstmt != SQL_NULL_HSTMT) {
        SQLFreeStmt(hstmt, SQL_CLOSE); // Close cursor if open
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    if (hdbc != SQL_NULL_HDBC) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        hdbc = SQL_NULL_HDBC;
    }
    if (connected) {
        std::cout << "Disconnected from database." << std::endl;
    }
    connected = false;
}

void DatabaseService::handleError(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& message) {
    SQLCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLCHAR messageText[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT textLength;
    SQLRETURN ret;

    std::cerr << "ODBC Error: " << message << std::endl;

    SQLLEN numRecs = 0;
    SQLGetDiagField(handleType, handle, 0, SQL_DIAG_NUMBER, &numRecs, 0, nullptr);

    for (SQLSMALLINT i = 1; i <= numRecs; ++i) {
        ret = SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError, messageText, sizeof(messageText), &textLength);
        if (SQL_SUCCEEDED(ret)) {
            std::cerr << "  SQLState: " << sqlState
                      << ", NativeError: " << nativeError
                      << ", Message: " << messageText << std::endl;
        } else {
            std::cerr << "  Failed to retrieve diagnostic record " << i << std::endl;
            break; 
        }
    }
}

std::vector<Patient> DatabaseService::getAllPatients() {
    std::vector<Patient> patients;
    if (!connected) {
        std::cerr << "Not connected to database for getAllPatients." << std::endl;
        return patients;
    }

    SQLRETURN ret;
    if (hstmt != SQL_NULL_HSTMT) { // Free previous statement if any
        SQLFreeStmt(hstmt, SQL_CLOSE); // Close any open cursor first
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error allocating statement handle for getAllPatients");
        return patients;
    }

    // Standardized Patient fields: patientID, name, dateOfBirth, sex
    // Assuming DB columns: pat_id, pat_name, pat_birth_dt (YYYYMMDD), pat_gender_code
    std::string sqlQuery = "SELECT pat_id, pat_name, pat_birth_dt, pat_gender_code FROM Patients"; // Adjusted placeholder

    ret = SQLExecDirect(hstmt, (SQLCHAR*)sqlQuery.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error executing query for getAllPatients: " + sqlQuery);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return patients;
    }

    // Bind columns to variables
    SQLCHAR db_patientID[256];
    SQLCHAR db_name[256];
    SQLCHAR db_dateOfBirth[11]; // YYYYMMDD + null terminator, or YYYY-MM-DD
    SQLCHAR db_sex[16];
    SQLLEN len_patientID, len_name, len_dateOfBirth, len_sex;

    SQLBindCol(hstmt, 1, SQL_C_CHAR, db_patientID, sizeof(db_patientID), &len_patientID);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, db_name, sizeof(db_name), &len_name);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, db_dateOfBirth, sizeof(db_dateOfBirth), &len_dateOfBirth);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, db_sex, sizeof(db_sex), &len_sex);

    std::cout << "Fetching patients..." << std::endl;
    while (SQLFetch(hstmt) == SQL_SUCCESS) {
        Patient p;
        p.patientID = (len_patientID == SQL_NULL_DATA || len_patientID == 0) ? "" : std::string((char*)db_patientID, len_patientID);
        p.name = (len_name == SQL_NULL_DATA || len_name == 0) ? "" : std::string((char*)db_name, len_name);
        p.dateOfBirth = (len_dateOfBirth == SQL_NULL_DATA || len_dateOfBirth == 0) ? "" : std::string((char*)db_dateOfBirth, len_dateOfBirth);
        p.sex = (len_sex == SQL_NULL_DATA || len_sex == 0) ? "" : std::string((char*)db_sex, len_sex);

        // Validate mandatory fields
        if (p.patientID.empty()) {
            std::cerr << "Warning: Fetched patient record with missing patientID." << std::endl;
        }
        if (p.name.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing name." << std::endl;
        }
        if (p.dateOfBirth.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing date of birth." << std::endl;
        }
        if (p.sex.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing sex." << std::endl;
        }

        patients.push_back(p);
        std::cout << "  Fetched: " << p.patientID << " - " << p.name << std::endl;
    }

    if (patients.empty()) {
        std::cout << "No patients found or query failed to return rows." << std::endl;
    }

    return patients;
}

std::vector<Patient> DatabaseService::searchPatients(const std::string& searchTerm) {
    std::vector<Patient> patients;
    if (!connected) {
        std::cerr << "Not connected to database for searchPatients." << std::endl;
        return patients;
    }
    if (searchTerm.empty()) {
        return getAllPatients(); 
    }

    SQLRETURN ret;
    if (hstmt != SQL_NULL_HSTMT) { 
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error allocating statement handle for searchPatients");
        return patients;
    }

    // Standardized Patient fields: patientID, name
    // Assuming DB columns: pat_id, pat_name
    std::string baseQuery = "SELECT pat_id, pat_name, pat_birth_dt, pat_gender_code FROM Patients WHERE pat_name LIKE ? OR pat_id LIKE ?"; // Adjusted placeholder
    std::string searchQuery = "%" + searchTerm + "%";

    ret = SQLPrepare(hstmt, (SQLCHAR*)baseQuery.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error preparing query for searchPatients: " + baseQuery);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return patients;
    }

    ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, searchQuery.length(), 0, (SQLPOINTER)searchQuery.c_str(), 0, NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error binding parameter 1 for searchPatients");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return patients;
    }
    ret = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, searchQuery.length(), 0, (SQLPOINTER)searchQuery.c_str(), 0, NULL);
     if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error binding parameter 2 for searchPatients");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return patients;
    }

    ret = SQLExecute(hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error executing prepared query for searchPatients");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return patients;
    }

    SQLCHAR db_patientID[256];
    SQLCHAR db_name[256];
    SQLCHAR db_dateOfBirth[11];
    SQLCHAR db_sex[16];
    SQLLEN len_patientID, len_name, len_dateOfBirth, len_sex;

    SQLBindCol(hstmt, 1, SQL_C_CHAR, db_patientID, sizeof(db_patientID), &len_patientID);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, db_name, sizeof(db_name), &len_name);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, db_dateOfBirth, sizeof(db_dateOfBirth), &len_dateOfBirth);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, db_sex, sizeof(db_sex), &len_sex);

    std::cout << "Searching patients with term: '" << searchTerm << "'..." << std::endl;
    while (SQLFetch(hstmt) == SQL_SUCCESS) {
        Patient p;
        p.patientID = (len_patientID == SQL_NULL_DATA || len_patientID == 0) ? "" : std::string((char*)db_patientID, len_patientID);
        p.name = (len_name == SQL_NULL_DATA || len_name == 0) ? "" : std::string((char*)db_name, len_name);
        p.dateOfBirth = (len_dateOfBirth == SQL_NULL_DATA || len_dateOfBirth == 0) ? "" : std::string((char*)db_dateOfBirth, len_dateOfBirth);
        p.sex = (len_sex == SQL_NULL_DATA || len_sex == 0) ? "" : std::string((char*)db_sex, len_sex);
        
        // Validate mandatory fields
        if (p.patientID.empty()) {
            std::cerr << "Warning: Searched patient record with missing patientID." << std::endl;
        }
        if (p.name.empty()) {
            std::cerr << "Warning: Searched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing name." << std::endl;
        }
        if (p.dateOfBirth.empty()) {
            std::cerr << "Warning: Searched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing date of birth." << std::endl;
        }
        if (p.sex.empty()) {
            std::cerr << "Warning: Searched patient (ID: " << (p.patientID.empty() ? "UNKNOWN" : p.patientID) << ") with missing sex." << std::endl;
        }

        patients.push_back(p);
        std::cout << "  Found: " << p.patientID << " - " << p.name << std::endl;
    }

    // SQLFreeStmt(hstmt, SQL_CLOSE); 

    if (patients.empty()) {
        std::cout << "No patients found matching term: '" << searchTerm << "'" << std::endl;
    }
    return patients;
}

Patient DatabaseService::getPatientById(const std::string& patientIdToFind) {
    Patient p; // Return empty patient if not found or error
    if (!connected) {
        std::cerr << "Not connected to database for getPatientById." << std::endl;
        return p;
    }
    if (patientIdToFind.empty()) {
        std::cerr << "Patient ID to find is empty." << std::endl;
        return p;
    }

    SQLRETURN ret;
    if (hstmt != SQL_NULL_HSTMT) { 
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error allocating statement handle for getPatientById");
        return p;
    }

    std::string baseQuery = "SELECT pat_id, pat_name, pat_birth_dt, pat_gender_code FROM Patients WHERE pat_id = ?"; // Adjusted placeholder

    ret = SQLPrepare(hstmt, (SQLCHAR*)baseQuery.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error preparing query for getPatientById: " + baseQuery);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return p;
    }

    ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, patientIdToFind.length(), 0, (SQLPOINTER)patientIdToFind.c_str(), 0, NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error binding parameter for getPatientById");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return p;
    }

    ret = SQLExecute(hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error executing prepared query for getPatientById");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return p;
    }

    SQLCHAR db_patientID[256];
    SQLCHAR db_name[256];
    SQLCHAR db_dateOfBirth[11];
    SQLCHAR db_sex[16];
    SQLLEN len_patientID, len_name, len_dateOfBirth, len_sex;

    SQLBindCol(hstmt, 1, SQL_C_CHAR, db_patientID, sizeof(db_patientID), &len_patientID);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, db_name, sizeof(db_name), &len_name);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, db_dateOfBirth, sizeof(db_dateOfBirth), &len_dateOfBirth);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, db_sex, sizeof(db_sex), &len_sex);

    if (SQLFetch(hstmt) == SQL_SUCCESS) {
        p.patientID = (len_patientID == SQL_NULL_DATA || len_patientID == 0) ? "" : std::string((char*)db_patientID, len_patientID);
        p.name = (len_name == SQL_NULL_DATA || len_name == 0) ? "" : std::string((char*)db_name, len_name);
        p.dateOfBirth = (len_dateOfBirth == SQL_NULL_DATA || len_dateOfBirth == 0) ? "" : std::string((char*)db_dateOfBirth, len_dateOfBirth);
        p.sex = (len_sex == SQL_NULL_DATA || len_sex == 0) ? "" : std::string((char*)db_sex, len_sex);
        
        // Validate mandatory fields
        if (p.patientID.empty()) { // This case should ideally not happen if patientIdToFind was valid
            std::cerr << "Warning: Fetched patient by ID (" << patientIdToFind << ") but result has missing patientID." << std::endl;
        }
        if (p.name.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << p.patientID << ") with missing name." << std::endl;
        }
        if (p.dateOfBirth.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << p.patientID << ") with missing date of birth." << std::endl;
        }
        if (p.sex.empty()) {
            std::cerr << "Warning: Fetched patient (ID: " << p.patientID << ") with missing sex." << std::endl;
        }

        std::cout << "Fetched patient by ID: " << p.patientID << " - " << p.name << std::endl;
    } else {
        std::cout << "Patient with ID '" << patientIdToFind << "' not found." << std::endl;
    }

    // SQLFreeStmt(hstmt, SQL_CLOSE);
    return p;
}

std::vector<Study> DatabaseService::getStudiesForPatient(const std::string& patientID) {
    std::vector<Study> studies;
    if (!connected) {
        std::cerr << "Not connected to database for getStudiesForPatient." << std::endl;
        return studies;
    }
     if (patientID.empty()) {
        std::cerr << "Patient ID is empty for getStudiesForPatient." << std::endl;
        return studies;
    }

    SQLRETURN ret;
    if (hstmt != SQL_NULL_HSTMT) { 
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error allocating statement handle for getStudiesForPatient");
        return studies;
    }

    // Study fields: studyInstanceUID, patientId, accessionNumber, studyDate, studyTime, modality, studyDescription, referringPhysicianName
    // Assuming DB columns: study_uid, pat_id, acc_num, study_dt (YYYYMMDD), study_tm (HHMMSS), mod, study_desc, ref_phys_name
    std::string baseQuery = "SELECT study_uid, pat_id, acc_num, study_dt, study_tm, mod, study_desc, ref_phys_name FROM Studies WHERE pat_id = ?"; // Adjusted placeholder

    ret = SQLPrepare(hstmt, (SQLCHAR*)baseQuery.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error preparing query for getStudiesForPatient: " + baseQuery);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return studies;
    }

    ret = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, patientID.length(), 0, (SQLPOINTER)patientID.c_str(), 0, NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error binding parameter for getStudiesForPatient");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return studies;
    }

    ret = SQLExecute(hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error executing prepared query for getStudiesForPatient");
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return studies;
    }

    SQLCHAR db_studyInstanceUID[256];
    SQLCHAR db_patId_fk[256]; // patientId foreign key from studies table
    SQLCHAR db_accessionNumber[256];
    SQLCHAR db_studyDate[11];
    SQLCHAR db_studyTime[9]; // HHMMSS + null
    SQLCHAR db_modality[16];
    SQLCHAR db_studyDescription[512];
    SQLCHAR db_referringPhysicianName[256];
    SQLLEN len_studyInstanceUID, len_patId_fk, len_accessionNumber, len_studyDate, len_studyTime, len_modality, len_studyDescription, len_referringPhysicianName;

    SQLBindCol(hstmt, 1, SQL_C_CHAR, db_studyInstanceUID, sizeof(db_studyInstanceUID), &len_studyInstanceUID);
    SQLBindCol(hstmt, 2, SQL_C_CHAR, db_patId_fk, sizeof(db_patId_fk), &len_patId_fk);
    SQLBindCol(hstmt, 3, SQL_C_CHAR, db_accessionNumber, sizeof(db_accessionNumber), &len_accessionNumber);
    SQLBindCol(hstmt, 4, SQL_C_CHAR, db_studyDate, sizeof(db_studyDate), &len_studyDate);
    SQLBindCol(hstmt, 5, SQL_C_CHAR, db_studyTime, sizeof(db_studyTime), &len_studyTime);
    SQLBindCol(hstmt, 6, SQL_C_CHAR, db_modality, sizeof(db_modality), &len_modality);
    SQLBindCol(hstmt, 7, SQL_C_CHAR, db_studyDescription, sizeof(db_studyDescription), &len_studyDescription);
    SQLBindCol(hstmt, 8, SQL_C_CHAR, db_referringPhysicianName, sizeof(db_referringPhysicianName), &len_referringPhysicianName);

    std::cout << "Fetching studies for patient ID: " << patientID << "..." << std::endl;
    while (SQLFetch(hstmt) == SQL_SUCCESS) {
        Study s;
        s.studyInstanceUID = (len_studyInstanceUID == SQL_NULL_DATA || len_studyInstanceUID == 0) ? "" : std::string((char*)db_studyInstanceUID, len_studyInstanceUID);
        s.patientId = (len_patId_fk == SQL_NULL_DATA || len_patId_fk == 0) ? "" : std::string((char*)db_patId_fk, len_patId_fk);
        s.accessionNumber = (len_accessionNumber == SQL_NULL_DATA || len_accessionNumber == 0) ? "" : std::string((char*)db_accessionNumber, len_accessionNumber);
        s.studyDate = (len_studyDate == SQL_NULL_DATA || len_studyDate == 0) ? "" : std::string((char*)db_studyDate, len_studyDate);
        s.studyTime = (len_studyTime == SQL_NULL_DATA || len_studyTime == 0) ? "" : std::string((char*)db_studyTime, len_studyTime);
        s.modality = (len_modality == SQL_NULL_DATA || len_modality == 0) ? "" : std::string((char*)db_modality, len_modality);
        s.studyDescription = (len_studyDescription == SQL_NULL_DATA || len_studyDescription == 0) ? "" : std::string((char*)db_studyDescription, len_studyDescription);
        s.referringPhysicianName = (len_referringPhysicianName == SQL_NULL_DATA || len_referringPhysicianName == 0) ? "" : std::string((char*)db_referringPhysicianName, len_referringPhysicianName);

        // Validate mandatory fields for Study
        if (s.studyInstanceUID.empty()) {
            std::cerr << "Warning: Fetched study for patient (ID: " << patientID << ") with missing studyInstanceUID." << std::endl;
        }
        if (s.patientId.empty()) { // Should match the input patientID
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with missing patientId linking field." << std::endl;
        } else if (s.patientId != patientID) {
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with mismatched patientId (expected " << patientID << ", got " << s.patientId << ")." << std::endl;
        }
        if (s.accessionNumber.empty()) {
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with missing accessionNumber." << std::endl;
        }
        if (s.studyDate.empty()) {
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with missing studyDate." << std::endl;
        }
        if (s.modality.empty()) {
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with missing modality." << std::endl;
        }
        if (s.studyDescription.empty()) {
            std::cerr << "Warning: Fetched study (UID: " << (s.studyInstanceUID.empty() ? "UNKNOWN" : s.studyInstanceUID) << ") with missing studyDescription." << std::endl;
        }

        studies.push_back(s);
        std::cout << "  Fetched study: " << s.studyInstanceUID << " - " << s.studyDescription << std::endl;
    }

    // SQLFreeStmt(hstmt, SQL_CLOSE);

    if (studies.empty()) {
        std::cout << "No studies found for patient ID: " << patientID << std::endl;
    }
    return studies;
}

Patient DatabaseService::getPatientFromDicom(const std::string& dicomFilePath) {
    DicomParser parser;
    if (parser.loadFile(dicomFilePath)) {
        return parser.getPatientInfo();
    }
    return Patient(); // Return empty patient on failure
}

Study DatabaseService::getStudyFromDicom(const std::string& dicomFilePath) {
    DicomParser parser;
    if (parser.loadFile(dicomFilePath)) {
        return parser.getStudyInfo();
    }
    return Study(); // Return empty study on failure
}

// Optional: Implementation for reading multiple studies from a DICOM directory
// This would require more sophisticated DICOMDIR parsing or iterating through files.
std::vector<Study> DatabaseService::getStudiesFromDicomDirectory(const std::string& directoryPath) {
    std::vector<Study> studies;
    // This is a placeholder. Actual implementation would involve:
    // 1. Listing all .dcm files in the directoryPath.
    // 2. For each file, creating a DicomParser, loading the file, and getting study info.
    // 3. Potentially using DCMTK's DicomDirInterface if a DICOMDIR file is present.
    std::cerr << "getStudiesFromDicomDirectory is not fully implemented yet." << std::endl;
    // Example (very basic, assumes all files are individual studies and readable):
    // For a real implementation, you'd use filesystem libraries to list files.
    // For now, let's assume a hypothetical function `listDicomFilesInDir` exists.
    /*
    std::vector<std::string> dicomFiles = listDicomFilesInDir(directoryPath);
    for (const auto& filePath : dicomFiles) {
        DicomParser parser;
        if (parser.loadFile(filePath)) {
            Study s = parser.getStudyInfo();
            if (!s.studyInstanceUID.empty()) { // Basic validation
                studies.push_back(s);
            }
        }
    }
    */
    return studies;
}

// Placeholder for a generic query execution method if needed later
/*
bool DatabaseService::executeQuery(const std::string& sqlQuery) {
    if (!connected) {
        std::cerr << "Not connected to database." << std::endl;
        return false;
    }
    SQLRETURN ret;
    if (hstmt != SQL_NULL_HSTMT) { 
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (SQL_FAILED(ret)) {
        handleError(SQL_HANDLE_DBC, hdbc, "Error allocating statement handle for executeQuery");
        return false;
    }

    ret = SQLExecDirect(hstmt, (SQLCHAR*)sqlQuery.c_str(), SQL_NTS);
    if (SQL_FAILED(ret)) {
        handleError(SQL_HANDLE_STMT, hstmt, "Error executing query: " + sqlQuery);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
        return false;
    }
    std::cout << "Query executed successfully: " << sqlQuery << std::endl;
    // For SELECT queries, you would then fetch results here.
    // For INSERT/UPDATE/DELETE, you might check affected rows using SQLRowCount.
    // SQLFreeHandle(SQL_HANDLE_STMT, hstmt); // Or keep for fetching
    // hstmt = SQL_NULL_HSTMT;
    return true;
}
*/

// Remember to close and free the statement handle (hstmt) appropriately.
// It's often kept alive if multiple operations are expected on it (like fetching rows after an execute),
// but should be freed before allocating a new hstmt for a different operation or when disconnecting.
// The current pattern of freeing/reallocating hstmt at the start of each method is safe but might be slightly less performant
// than reusing a prepared statement handle if the same query (with different params) were run multiple times.
// For simplicity and clarity in this stage, the current approach is acceptable.
// Ensure SQLFreeStmt(hstmt, SQL_CLOSE) is called to close cursors before freeing the handle or re-executing.
// The destructor and disconnect method handle the general cleanup.

