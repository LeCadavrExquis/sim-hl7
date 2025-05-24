#include "ConfigManager.h"
#include <iostream>


ConfigManager::ConfigManager() : loaded(false) {
    appConfig.outputPath = "";
    appConfig.odbcDsn = "";
    appConfig.dbUser = "";
    appConfig.dbPassword = "";
    appConfig.cdaXsdPath = "";
    appConfig.patientIdRootOid = "";
}

ConfigManager::ConfigManager(const std::string& configFilepath) : ConfigManager() {
    configFilePath_ = configFilepath;
}

std::string ConfigManager::getNodeText(const pugi::xml_node& node, const std::string& defaultValue) {
    if (node && node.first_child() && node.first_child().type() == pugi::node_pcdata) {
        return node.child_value();
    }
    return defaultValue;
}

bool ConfigManager::loadConfig(const std::string& configFilepath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(configFilepath.c_str());

    if (!result) {
        std::cerr << "Error parsing configuration file '" << configFilepath << "': " << result.description() << std::endl;
        loaded = false;
        return false;
    }

    // Accept both <HL7Config> and <config> as root for test compatibility
    pugi::xml_node rootNode = doc.child("HL7Config");
    if (!rootNode) rootNode = doc.child("config");
    if (!rootNode) {
        std::cerr << "Error: <HL7Config> or <config> root node not found in '" << configFilepath << "'." << std::endl;
        loaded = false;
        return false;
    }

    // Database Configuration
    pugi::xml_node dbNode = rootNode.child("Database");
    if (!dbNode) dbNode = rootNode.child("database");
    if (dbNode) {
        appConfig.odbcDsn = getNodeText(dbNode.child("ODBC_DSN"), getNodeText(dbNode.child("dsn"), ""));
        appConfig.dbUser = getNodeText(dbNode.child("User"), getNodeText(dbNode.child("username"), ""));
        appConfig.dbPassword = getNodeText(dbNode.child("Password"), getNodeText(dbNode.child("password"), ""));
    }

    // General Settings
    pugi::xml_node generalNode = rootNode.child("GeneralSettings");
    if (!generalNode) generalNode = rootNode.child("hl7");
    if (generalNode) {
        appConfig.outputPath = getNodeText(generalNode.child("OutputPath"), getNodeText(generalNode.child("outputPath"), ""));
        appConfig.cdaXsdPath = getNodeText(generalNode.child("CdaXsdPath"), getNodeText(generalNode.child("cdaXsdPath"), ""));
        // appConfig.patientIdRootOid = getNodeText(generalNode.child("RootOid"), getNodeText(generalNode.child("rootOid"), ""));
        appConfig.realmCode = getNodeText(generalNode.child("RealmCode"));
        appConfig.typeIdExtension = getNodeText(generalNode.child("TypeIdExtension"));
        appConfig.documentIdRootOid = getNodeText(generalNode.child("DocumentIdRootOid"));
        
        pugi::xml_node docCodeNode = generalNode.child("DocumentCode");
        if (docCodeNode) {
            appConfig.documentCode.code = getNodeText(docCodeNode.child("code"));
            appConfig.documentCode.codeSystem = getNodeText(docCodeNode.child("codeSystem"));
            appConfig.documentCode.codeSystemName = getNodeText(docCodeNode.child("codeSystemName"));
            appConfig.documentCode.displayName = getNodeText(docCodeNode.child("displayName"));
        }
        appConfig.documentTitle = getNodeText(generalNode.child("DocumentTitle"));

        pugi::xml_node confCodeNode = generalNode.child("ConfidentialityCode");
        if (confCodeNode) {
            appConfig.confidentialityCode.code = getNodeText(confCodeNode.child("code"));
            appConfig.confidentialityCode.codeSystem = getNodeText(confCodeNode.child("codeSystem"));
            appConfig.confidentialityCode.displayName = getNodeText(confCodeNode.child("displayName"));
        }
        appConfig.languageCode = getNodeText(generalNode.child("LanguageCode"));
        appConfig.organizationOid = getNodeText(generalNode.child("OrganizationOid"));
        appConfig.sendingFacility = getNodeText(generalNode.child("SendingFacility")); // This might be the same as defaultSendingFacility
        appConfig.defaultSendingApplication = getNodeText(generalNode.child("DefaultSendingApplication"));


    }

    pugi::xml_node hl7DefaultsNode = rootNode.child("HL7Defaults");
    if (hl7DefaultsNode) {
        appConfig.defaultSendingApplication = getNodeText(hl7DefaultsNode.child("SendingApplication"), "DefaultSender");
        appConfig.defaultSendingFacility = getNodeText(hl7DefaultsNode.child("SendingFacility"), "DefaultFacility");
        appConfig.defaultReceivingApplication = getNodeText(hl7DefaultsNode.child("ReceivingApplication"), "DefaultReceiver");
        appConfig.defaultReceivingFacility = getNodeText(hl7DefaultsNode.child("ReceivingFacility"), "DefaultFacility");
        appConfig.defaultSecurityText = getNodeText(hl7DefaultsNode.child("SecurityText"), "N");
        appConfig.defaultLanguageCode = getNodeText(hl7DefaultsNode.child("LanguageCode"), "en-US");
        appConfig.defaultRealmCode = getNodeText(hl7DefaultsNode.child("RealmCode"), "US");
        appConfig.defaultTitle = getNodeText(hl7DefaultsNode.child("Title"), "Scintigraphy Report");
        appConfig.defaultConfidentialityCode = getNodeText(hl7DefaultsNode.child("ConfidentialityCode"), "N");
        appConfig.defaultConfidentialityCodeSystem = getNodeText(hl7DefaultsNode.child("ConfidentialityCodeSystem"), "2.16.840.1.113883.5.25");
        appConfig.defaultConfidentialityDisplayName = getNodeText(hl7DefaultsNode.child("ConfidentialityDisplayName"), "Normal");
    }

    // Organization Details
    pugi::xml_node orgNode = rootNode.child("OrganizationDetails");
    if (orgNode) {
        appConfig.organizationName = getNodeText(orgNode.child("Name"), "Default Organization");
        appConfig.organizationIdRoot = getNodeText(orgNode.child("IdRootOid"), "2.16.840.1.113883.4.6"); // Example OID
        appConfig.organizationTelecom = getNodeText(orgNode.child("Telecom"), "tel:+1-555-555-1212");
        appConfig.organizationAddrStreet = getNodeText(orgNode.child("Address").child("Street"), "123 Main St");
        appConfig.organizationAddrCity = getNodeText(orgNode.child("Address").child("City"), "Anytown");
        appConfig.organizationAddrState = getNodeText(orgNode.child("Address").child("State"), "CA");
        appConfig.organizationAddrZip = getNodeText(orgNode.child("Address").child("Zip"), "90210");
        appConfig.organizationAddrCountry = getNodeText(orgNode.child("Address").child("Country"), "US");
    }

    // Key OIDs
    pugi::xml_node oidsNode = rootNode.child("KeyOIDs");
    if (oidsNode) {
        appConfig.patientIdRootOid = getNodeText(oidsNode.child("PatientIdRoot"), appConfig.patientIdRootOid); 
        appConfig.documentIdRootOid = getNodeText(oidsNode.child("DocumentIdRoot"), appConfig.documentIdRootOid); 
        appConfig.setIdRootOid = getNodeText(oidsNode.child("SetIdRoot")); 
        appConfig.templateIdRootOid = getNodeText(oidsNode.child("TemplateIdRoot")); 
        appConfig.LoincSystemOid = getNodeText(oidsNode.child("LoincSystemOid"), "2.16.840.1.113883.6.1");
        appConfig.customOids.clear();
        for (pugi::xml_node oid_node : oidsNode.children("CustomOid")) {
            OidConfig oid_conf;
            oid_conf.assigningAuthority = oid_node.attribute("assigningAuthority").value();
            oid_conf.oid = getNodeText(oid_node.child("OID"));
            oid_conf.description = getNodeText(oid_node.child("Description"));
            if (!oid_conf.oid.empty() && !oid_conf.assigningAuthority.empty()){
                 appConfig.customOids.push_back(oid_conf);
            }
        }
    }

    if (appConfig.patientIdRootOid.empty()) {
        appConfig.patientIdRootOid = getNodeText(rootNode.child("patientIdRootOid"));
    }
    // studyIdRootOid is not directly in AppConfig, but if it were, it would be read here.
    appConfig.genderCodeSystem = getNodeText(rootNode.child("genderCodeSystem"));


    // TemplateIds
    pugi::xml_node templateIdsNode = rootNode.child("TemplateIds");
    if (templateIdsNode) {
        appConfig.templateIds.clear();
        for (pugi::xml_node tmplNode : templateIdsNode.children("TemplateId")) {
            TemplateIdConfig tic;
            tic.root = getNodeText(tmplNode.child("root"));
            tic.extension = getNodeText(tmplNode.child("extension"));
            if (!tic.root.empty()) { // Root is mandatory for a templateId
                appConfig.templateIds.push_back(tic);
            }
        }
    }

    // Author
    pugi::xml_node authorNode = rootNode.child("Author");
    if (authorNode) {
        appConfig.authorIdRootOid = getNodeText(authorNode.child("AuthorIdRootOid"));
        appConfig.authorIdExtension = getNodeText(authorNode.child("AuthorIdExtension"));
        appConfig.authorDeviceManufacturer = getNodeText(authorNode.child("AuthorDeviceManufacturer"));
        appConfig.authorDeviceSoftwareName = getNodeText(authorNode.child("AuthorDeviceSoftwareName"));
    }

    // Custodian
    pugi::xml_node custodianNode = rootNode.child("Custodian");
    if (custodianNode) {
        appConfig.custodianOrgIdRootOid = getNodeText(custodianNode.child("CustodianOrgIdRootOid"));
        appConfig.custodianOrgIdExtension = getNodeText(custodianNode.child("CustodianOrgIdExtension"));
        appConfig.custodianOrgName = getNodeText(custodianNode.child("CustodianOrgName"));
    }

    // Encounter
    pugi::xml_node encounterNode = rootNode.child("Encounter");
    if (encounterNode) {
        appConfig.encounterIdRootOid = getNodeText(encounterNode.child("EncounterIdRootOid"));
        pugi::xml_node encounterTypeCodeNode = encounterNode.child("EncounterTypeCode");
        if (encounterTypeCodeNode) {
            appConfig.encounterTypeCode.code = getNodeText(encounterTypeCodeNode.child("code"));
            appConfig.encounterTypeCode.codeSystem = getNodeText(encounterTypeCodeNode.child("codeSystem"));
            appConfig.encounterTypeCode.displayName = getNodeText(encounterTypeCodeNode.child("displayName"));
        }
    }

    // Location
    pugi::xml_node locationNode = rootNode.child("Location");
    if (locationNode) {
        appConfig.locationFacilityIdRootOid = getNodeText(locationNode.child("LocationFacilityIdRootOid"));
        appConfig.locationFacilityIdExtension = getNodeText(locationNode.child("LocationFacilityIdExtension"));
        appConfig.locationFacilityName = getNodeText(locationNode.child("LocationFacilityName"));
    }

    // ReportSection
    pugi::xml_node reportSectionNode = rootNode.child("ReportSection");
    if (reportSectionNode) {
        pugi::xml_node reportSectionCodeNode = reportSectionNode.child("ReportSectionCode");
        if (reportSectionCodeNode) {
            appConfig.reportSectionCode.code = getNodeText(reportSectionCodeNode.child("code"));
            appConfig.reportSectionCode.codeSystem = getNodeText(reportSectionCodeNode.child("codeSystem"));
            appConfig.reportSectionCode.codeSystemName = getNodeText(reportSectionCodeNode.child("codeSystemName"));
            appConfig.reportSectionCode.displayName = getNodeText(reportSectionCodeNode.child("displayName"));
        }
    }

    std::cout << "Configuration loaded successfully from '" << configFilepath << "'." << std::endl;
    std::cout << " Loaded RealmCode: " << appConfig.realmCode << std::endl;
    std::cout << " Loaded TypeIdExtension: " << appConfig.typeIdExtension << std::endl;
    std::cout << " Loaded DocumentIdRootOid: " << appConfig.documentIdRootOid << std::endl;
    std::cout << " Loaded PatientIdRootOid: " << appConfig.patientIdRootOid << std::endl;


    loaded = true;
    return true;
}

bool ConfigManager::loadConfig() {
    if (configFilePath_.empty()) return false;
    return loadConfig(configFilePath_);
}

std::string ConfigManager::getDsn() const {
    return appConfig.odbcDsn;
}

std::string ConfigManager::getDbUsername() const {
    return appConfig.dbUser;
}

std::string ConfigManager::getDbPassword() const {
    return appConfig.dbPassword;
}

std::string ConfigManager::getCdaXsdPath() const {
    return appConfig.cdaXsdPath;
}

std::string ConfigManager::getOutputPath() const {
    return appConfig.outputPath;
}

std::string ConfigManager::getRootOid() const {
    return appConfig.patientIdRootOid;
}

const AppConfig& ConfigManager::getConfig() const {
    if (!loaded) {
        std::cerr << "Warning: Accessing configuration, but it was not loaded successfully or at all." << std::endl;
    }
    return appConfig;
}
