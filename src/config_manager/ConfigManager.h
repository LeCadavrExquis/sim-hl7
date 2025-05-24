#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <vector>
#include "pugixml.hpp"

// Structure to hold OID configurations
struct OidConfig {
    std::string assigningAuthority; // e.g., "IHEXDSREGISTRY.PATIENTID"
    std::string oid;                // e.g., "1.3.6.1.4.1.21367.2017.2.5.88"
    std::string description;        // Optional description
};

// Helper struct for code fields (e.g., documentCode, confidentialityCode, etc.)
struct CodeConfig {
    std::string code;
    std::string codeSystem;
    std::string codeSystemName;
    std::string displayName;
};

// Helper struct for templateId fields
struct TemplateIdConfig {
    std::string root;
    std::string extension;
};

// Structure to hold all application configurations
struct AppConfig {
    std::string odbcDsn;
    std::string dbUser;
    std::string dbPassword;

    std::string outputPath;

    // HL7 Message Defaults
    std::string defaultSendingApplication;
    std::string defaultSendingFacility;
    std::string defaultReceivingApplication;
    std::string defaultReceivingFacility;
    std::string defaultSecurityText;
    std::string defaultLanguageCode; // e.g., "en-US"
    std::string defaultRealmCode;    // e.g., "US"
    std::string defaultTitle;
    std::string defaultConfidentialityCode;
    std::string defaultConfidentialityCodeSystem;
    std::string defaultConfidentialityDisplayName;


    // Organization details (for Author, Custodian, etc.)
    std::string organizationName;
    std::string organizationIdRoot; // OID for the organization ID
    std::string organizationTelecom;
    std::string organizationAddrStreet;
    std::string organizationAddrCity;
    std::string organizationAddrState;
    std::string organizationAddrZip;
    std::string organizationAddrCountry;


    // Key OIDs needed for CDA generation
    std::string patientIdRootOid; // Root OID for patient identifiers
    std::string documentIdRootOid; // Root OID for the CDA document instance identifier
    std::string setIdRootOid;      // Root OID for the CDA set id (versioning)
    std::string templateIdRootOid; // Root OID for the base CDA template
    std::string LoincSystemOid;    // OID for LOINC code system

    // CDA/XSD/HL7 specific fields
    std::string cdaXsdPath;
    std::string realmCode;
    std::string typeIdRoot;
    std::string typeIdExtension;
    std::vector<TemplateIdConfig> templateIds;
    CodeConfig documentCode;
    std::string documentTitle;
    CodeConfig confidentialityCode;
    std::string languageCode;
    std::string genderCodeSystem;
    std::string authorIdRootOid;
    std::string authorIdExtension;
    std::string authorDeviceManufacturer;
    std::string authorDeviceSoftwareName;
    std::string custodianOrgIdRootOid;
    std::string custodianOrgIdExtension;
    std::string custodianOrgName;
    std::string sendingFacility;
    std::string encounterIdRootOid;
    CodeConfig encounterTypeCode;
    std::string locationFacilityIdRootOid;
    std::string locationFacilityIdExtension;
    std::string locationFacilityName;
    CodeConfig reportSectionCode;
    std::string organizationOid;

    // Potentially a list of other relevant OIDs
    std::vector<OidConfig> customOids;
};


class ConfigManager {
public:
    ConfigManager();
    ConfigManager(const std::string& configFilepath);
    bool loadConfig(const std::string& configFilepath);
    bool loadConfig();
    const AppConfig& getConfig() const;

    // Getter methods for test compatibility
    std::string getDsn() const;
    std::string getDbUsername() const;
    std::string getDbPassword() const;
    std::string getCdaXsdPath() const;
    std::string getOutputPath() const;
    std::string getRootOid() const;

private:
    AppConfig appConfig;
    bool loaded;
    std::string configFilePath_;

    std::string getNodeText(const pugi::xml_node& node, const std::string& defaultValue = "");
};

#endif // CONFIGMANAGER_H
