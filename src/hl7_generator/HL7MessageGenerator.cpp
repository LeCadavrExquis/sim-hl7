#include "HL7MessageGenerator.h"
#include <fstream>
#include <iostream>
#include <sstream> // For string stream
#include <iomanip> // For std::put_time
#include <chrono>  // For system_clock
#include <random>  // For UUID generation
#include <cstdio>  // For sprintf

// Define some default constants
const std::string DEFAULT_STRING = "Unknown";
const std::string DEFAULT_OID_ROOT = "2.25.0.0.0.0"; // Example placeholder OID
const std::string DEFAULT_CODE = "UNK";
const std::string DEFAULT_CODESYSTEM_NULLFLAVOR = "2.16.840.1.113883.5.1008"; // HL7 NullFlavor
const std::string DEFAULT_DISPLAYNAME = "Unknown";
const std::string FIXED_TYPEID_ROOT = "2.16.840.1.113883.1.3"; // CDA R2 fixed value for typeId/@root
const std::string DEFAULT_TYPEID_EXTENSION = "POCD_HD000040"; // Common extension for base CDA
const std::string DEFAULT_LANGUAGE_CODE = "pl-PL";
const std::string DEFAULT_CONFIDENTIALITY_CODE = "N"; // Normal
const std::string DEFAULT_CONFIDENTIALITY_CODESYSTEM = "2.16.840.1.113883.5.25"; // HL7 Confidentiality
const std::string DEFAULT_REALM_CODE = "PL"; // Default realm

// Implementation for missing resetErrors method
void XSDValidationErrorHandler::resetErrors() {
    fSawErrors = false;
}

// Xerces-C++ specific includes (already in .h but good for context)
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

XERCES_CPP_NAMESPACE_USE

void XSDValidationErrorHandler::warning(const SAXParseException& exc) {
    char* msg = XMLString::transcode(exc.getMessage());
    std::cerr << "XSD Validation Warning: " << msg
              << " at line " << exc.getLineNumber()
              << " column " << exc.getColumnNumber() << std::endl;
    XMLString::release(&msg);
}

void XSDValidationErrorHandler::error(const SAXParseException& exc) {
    fSawErrors = true;
    char* msg = XMLString::transcode(exc.getMessage());
    std::cerr << "XSD Validation Error: " << msg
              << " at line " << exc.getLineNumber()
              << " column " << exc.getColumnNumber() << std::endl;
    XMLString::release(&msg);
}

void XSDValidationErrorHandler::fatalError(const SAXParseException& exc) {
    fSawErrors = true;
    char* msg = XMLString::transcode(exc.getMessage());
    std::cerr << "XSD Validation Fatal Error: " << msg
              << " at line " << exc.getLineNumber()
              << " column " << exc.getColumnNumber() << std::endl;
    XMLString::release(&msg);
}

void HL7MessageGenerator::initializeXerces() {
    try {
        XMLPlatformUtils::Initialize();
        std::cout << "Xerces-C++ initialized successfully." << std::endl;
    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Error during Xerces-C++ initialization! Message: "
                  << message << std::endl;
        XMLString::release(&message);
    }
}

void HL7MessageGenerator::terminateXerces() {
    try {
        XMLPlatformUtils::Terminate();
        std::cout << "Xerces-C++ terminated successfully." << std::endl;
    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Error during Xerces-C++ termination! Message: "
                  << message << std::endl;
        XMLString::release(&message);
    }
}


// --- Constructor and Destructor ---
HL7MessageGenerator::HL7MessageGenerator(const AppConfig& configuration) : config(configuration) {
    std::cout << "HL7MessageGenerator initialized with AppConfig." << std::endl;
}

// Destructor
HL7MessageGenerator::~HL7MessageGenerator() {
    std::cout << "HL7MessageGenerator destroyed." << std::endl;
}

std::string HL7MessageGenerator::getCurrentTimestamp(const char* format) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&buf, format);
    return ss.str();
}

std::string HL7MessageGenerator::generateUUID() {
    // Basic UUID-like string generator (not a true UUID v4, but often sufficient for HL7 IDs)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // UUID version 4
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen); // Variant (8,9,a,b)
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

// Main message generation function using pugixml
std::string HL7MessageGenerator::generateORUMessage(const Patient& patient, const Study& study) {
    std::cout << "Generating ORU message for patient: " << patient.name
              << " and study: " << study.studyDescription << std::endl;

    pugi::xml_document doc;

    // Add XML declaration
    pugi::xml_node declarationNode = doc.append_child(pugi::node_declaration);
    declarationNode.append_attribute("version") = "1.0";
    declarationNode.append_attribute("encoding") = "UTF-8";

    // Root: ClinicalDocument
    pugi::xml_node clinicalDocument = doc.append_child("ClinicalDocument");
    clinicalDocument.append_attribute("xmlns") = "urn:hl7-org:v3";
    clinicalDocument.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    if (!config.cdaXsdPath.empty()) {
         std::string schemaLocationValue = "urn:hl7-org:v3 " + config.cdaXsdPath; 
         clinicalDocument.append_attribute("xsi:schemaLocation") = schemaLocationValue.c_str();
    }


    std::string effectiveTime = getCurrentTimestamp();
    std::string documentIdExtension = generateUUID(); // Unique ID for this document

    // Build various parts of the CDA using config
    addHeader(doc, patient, study, effectiveTime, documentIdExtension);
    addRecordTarget(clinicalDocument, patient);
    addAuthor(clinicalDocument, effectiveTime);
    addCustodian(clinicalDocument);
    addComponentOf(clinicalDocument, study);
    addStructuredBody(clinicalDocument, study); // Placeholder for actual study content

    // Convert the XML document to a string
    std::stringstream ss;
    doc.save(ss, "  ", pugi::format_default, pugi::encoding_utf8);

    std::cout << "HL7 CDA message generated successfully." << std::endl;
    return ss.str();
}

void HL7MessageGenerator::addHeader(pugi::xml_document& doc, const Patient& patient, const Study& study,
                                    const std::string& effectiveTime, const std::string& documentIdExt) {
    pugi::xml_node clinicalDocument = doc.child("ClinicalDocument");
    if (!clinicalDocument) return;

    clinicalDocument.append_child("realmCode").append_attribute("code") = config.realmCode.empty() ? DEFAULT_REALM_CODE.c_str() : config.realmCode.c_str();
    
    pugi::xml_node typeId = clinicalDocument.append_child("typeId");
    typeId.append_attribute("root") = FIXED_TYPEID_ROOT.c_str(); // Use #FIXED value
    typeId.append_attribute("extension") = config.typeIdExtension.empty() ? DEFAULT_TYPEID_EXTENSION.c_str() : config.typeIdExtension.c_str();

    for(const auto& tmplId : config.templateIds) {
        pugi::xml_node templateIdNode = clinicalDocument.append_child("templateId");
        templateIdNode.append_attribute("root") = tmplId.root.empty() ? DEFAULT_OID_ROOT.c_str() : tmplId.root.c_str();
        if (!tmplId.extension.empty()) { // Extension is optional for templateId
            templateIdNode.append_attribute("extension") = tmplId.extension.c_str();
        }
    }

    pugi::xml_node idNode = clinicalDocument.append_child("id");
    std::string docIdRoot = config.documentIdRootOid.empty() ? (config.organizationOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.organizationOid.c_str()) : config.documentIdRootOid.c_str();
    idNode.append_attribute("root") = docIdRoot.c_str();
    idNode.append_attribute("extension") = documentIdExt.c_str(); // Should be non-empty (UUID)

    pugi::xml_node codeNode = clinicalDocument.append_child("code");
    codeNode.append_attribute("code") = config.documentCode.code.empty() ? DEFAULT_CODE.c_str() : config.documentCode.code.c_str();
    codeNode.append_attribute("codeSystem") = config.documentCode.codeSystem.empty() ? DEFAULT_CODESYSTEM_NULLFLAVOR.c_str() : config.documentCode.codeSystem.c_str();
    codeNode.append_attribute("codeSystemName") = config.documentCode.codeSystemName.empty() ? DEFAULT_STRING.c_str() : config.documentCode.codeSystemName.c_str();
    codeNode.append_attribute("displayName") = config.documentCode.displayName.empty() ? DEFAULT_DISPLAYNAME.c_str() : config.documentCode.displayName.c_str();

    clinicalDocument.append_child("title").text().set(config.documentTitle.empty() ? (std::string("Report - ") + (study.studyDescription.empty() ? DEFAULT_STRING : study.studyDescription)).c_str() : config.documentTitle.c_str());
    clinicalDocument.append_child("effectiveTime").append_attribute("value") = effectiveTime.c_str(); // Should be non-empty
    
    pugi::xml_node confidentialityCode = clinicalDocument.append_child("confidentialityCode");
    std::string confCodeVal = config.confidentialityCode.code.empty() ? DEFAULT_CONFIDENTIALITY_CODE.c_str() : config.confidentialityCode.code.c_str();
    std::string confCodeSysVal = config.confidentialityCode.codeSystem.empty() ? DEFAULT_CONFIDENTIALITY_CODESYSTEM.c_str() : config.confidentialityCode.codeSystem.c_str();
    confidentialityCode.append_attribute("code") = confCodeVal.c_str();
    confidentialityCode.append_attribute("codeSystem") = confCodeSysVal.c_str();
    
    if (!config.confidentialityCode.displayName.empty()){
        confidentialityCode.append_attribute("displayName") = config.confidentialityCode.displayName.c_str();
    } else {
         if (confCodeSysVal == DEFAULT_CONFIDENTIALITY_CODESYSTEM) {
            if (confCodeVal == "N") confidentialityCode.append_attribute("displayName") = "Normal";
            else if (confCodeVal == "R") confidentialityCode.append_attribute("displayName") = "Restricted";
            else if (confCodeVal == "V") confidentialityCode.append_attribute("displayName") = "Very Restricted";
         }
    }

    clinicalDocument.append_child("languageCode").append_attribute("code") = config.languageCode.empty() ? DEFAULT_LANGUAGE_CODE.c_str() : config.languageCode.c_str();
}

void HL7MessageGenerator::addRecordTarget(pugi::xml_node& parentNode, const Patient& patient) {
    pugi::xml_node recordTarget = parentNode.append_child("recordTarget");
    pugi::xml_node patientRole = recordTarget.append_child("patientRole");
    pugi::xml_node idNode = patientRole.append_child("id");
    idNode.append_attribute("extension") = patient.patientID.empty() ? DEFAULT_STRING.c_str() : patient.patientID.c_str(); 
    idNode.append_attribute("root") = config.patientIdRootOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.patientIdRootOid.c_str();

    pugi::xml_node patientNode = patientRole.append_child("patient");
    pugi::xml_node nameNode = patientNode.append_child("name");
    std::string familyName, givenName;
    
    if (patient.name.empty() || patient.name == DEFAULT_STRING) {
        familyName = DEFAULT_STRING; 
        givenName = DEFAULT_STRING; 
    } else {
        size_t space_pos = patient.name.find(' ');
        if (space_pos != std::string::npos) {
            familyName = patient.name.substr(0, space_pos);
            givenName = patient.name.substr(space_pos + 1);
        } else {
            familyName = patient.name; 
            givenName = DEFAULT_STRING; 
        }
    }
    nameNode.append_child("given").text().set(givenName.empty() ? DEFAULT_STRING.c_str() : givenName.c_str());
    nameNode.append_child("family").text().set(familyName.empty() ? DEFAULT_STRING.c_str() : familyName.c_str());

    pugi::xml_node genderCode = patientNode.append_child("administrativeGenderCode");
    genderCode.append_attribute("code") = patient.sex.empty() ? DEFAULT_CODE.c_str() : patient.sex.c_str(); 
    genderCode.append_attribute("codeSystem") = config.genderCodeSystem.empty() ? DEFAULT_CODESYSTEM_NULLFLAVOR.c_str() : config.genderCodeSystem.c_str(); 

    patientNode.append_child("birthTime").append_attribute("value") = patient.dateOfBirth.empty() ? "19000101" : patient.dateOfBirth.c_str(); // Default DOB if empty
}

void HL7MessageGenerator::addAuthor(pugi::xml_node& parentNode, const std::string& effectiveTime) {
    pugi::xml_node author = parentNode.append_child("author");
    author.append_child("time").append_attribute("value") = effectiveTime.c_str(); // Should be non-empty
    pugi::xml_node assignedAuthor = author.append_child("assignedAuthor");
    pugi::xml_node idNode = assignedAuthor.append_child("id");
    
    std::string authorRoot = config.authorIdRootOid.empty() ? (config.organizationOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.organizationOid.c_str()) : config.authorIdRootOid.c_str();
    idNode.append_attribute("root") = authorRoot.c_str();
    
    std::string authorExt = config.authorIdExtension.empty() ? (config.defaultSendingApplication.empty() ? DEFAULT_STRING.c_str() : config.defaultSendingApplication.c_str()) : config.authorIdExtension.c_str();
    idNode.append_attribute("extension") = authorExt.c_str();

    pugi::xml_node assignedAuthoringDevice = assignedAuthor.append_child("assignedAuthoringDevice");
    assignedAuthoringDevice.append_child("manufacturerModelName").text().set(config.authorDeviceManufacturer.empty() ? DEFAULT_STRING.c_str() : config.authorDeviceManufacturer.c_str());
    assignedAuthoringDevice.append_child("softwareName").text().set(config.authorDeviceSoftwareName.empty() ? DEFAULT_STRING.c_str() : config.authorDeviceSoftwareName.c_str());
}

void HL7MessageGenerator::addCustodian(pugi::xml_node& parentNode) {
    pugi::xml_node custodian = parentNode.append_child("custodian");
    pugi::xml_node assignedCustodian = custodian.append_child("assignedCustodian");
    pugi::xml_node representedCustodianOrg = assignedCustodian.append_child("representedCustodianOrganization");
    pugi::xml_node idNode = representedCustodianOrg.append_child("id");

    std::string custodianRoot = config.custodianOrgIdRootOid.empty() ? (config.organizationOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.organizationOid.c_str()) : config.custodianOrgIdRootOid.c_str();
    idNode.append_attribute("root") = custodianRoot.c_str();
    if (!config.custodianOrgIdExtension.empty()) { // Extension is optional
         idNode.append_attribute("extension") = config.custodianOrgIdExtension.c_str();
    } else { // If root is default, provide default extension
        if (custodianRoot == DEFAULT_OID_ROOT) {
             idNode.append_attribute("extension") = DEFAULT_STRING.c_str();
        }
    }
    
    std::string orgName = config.custodianOrgName.empty() ? (config.sendingFacility.empty() ? DEFAULT_STRING.c_str() : config.sendingFacility.c_str()) : config.custodianOrgName.c_str();
    representedCustodianOrg.append_child("name").text().set(orgName.c_str());
}

void HL7MessageGenerator::addComponentOf(pugi::xml_node& parentNode, const Study& study) {
    pugi::xml_node componentOf = parentNode.append_child("componentOf");
    pugi::xml_node encompassingEncounter = componentOf.append_child("encompassingEncounter");
    
    pugi::xml_node idNode = encompassingEncounter.append_child("id");
    std::string encounterRoot = config.encounterIdRootOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.encounterIdRootOid.c_str();
    idNode.append_attribute("root") = encounterRoot.c_str(); 
    
    std::string encounterExt = study.accessionNumber.empty() ? (study.studyInstanceUID.empty() ? DEFAULT_STRING.c_str() : study.studyInstanceUID.c_str()) : study.accessionNumber.c_str();
    idNode.append_attribute("extension") = encounterExt.c_str();

    if (!config.encounterTypeCode.code.empty()) {
        pugi::xml_node codeNode = encompassingEncounter.append_child("code"); 
        codeNode.append_attribute("code") = config.encounterTypeCode.code.c_str();
        codeNode.append_attribute("codeSystem") = config.encounterTypeCode.codeSystem.empty() ? DEFAULT_CODESYSTEM_NULLFLAVOR.c_str() : config.encounterTypeCode.codeSystem.c_str();
        codeNode.append_attribute("displayName") = config.encounterTypeCode.displayName.empty() ? DEFAULT_DISPLAYNAME.c_str() : config.encounterTypeCode.displayName.c_str();
    } // If config.encounterTypeCode.code is empty, the entire <code> element is omitted. This is usually fine as it's often optional.

    pugi::xml_node effectiveTimeNode = encompassingEncounter.append_child("effectiveTime");
    std::string studyDateTimeLow = study.studyDate; 
    if (studyDateTimeLow.empty()) studyDateTimeLow = "19000101"; // Default date if empty
    
    if (!study.studyTime.empty() && study.studyTime.length() >= 4) { 
        studyDateTimeLow += study.studyTime.substr(0, 4); // HHMM
        if (study.studyTime.length() >=6) {
            studyDateTimeLow += study.studyTime.substr(4,2); // SS
        } else {
             studyDateTimeLow += "00"; // Default seconds
        }
    } else {
        studyDateTimeLow += "000000"; // Default time HHMMSSto
    }
    pugi::xml_node lowNode = effectiveTimeNode.append_child("low");
    lowNode.append_attribute("value") = studyDateTimeLow.c_str();

    pugi::xml_node locationNode = encompassingEncounter.append_child("location");
    pugi::xml_node healthCareFacilityNode = locationNode.append_child("healthCareFacility");
    pugi::xml_node facilityIdNode = healthCareFacilityNode.append_child("id");
    
    std::string facilityRoot = config.locationFacilityIdRootOid.empty() ? (config.organizationOid.empty() ? DEFAULT_OID_ROOT.c_str() : config.organizationOid.c_str()) : config.locationFacilityIdRootOid.c_str();
    facilityIdNode.append_attribute("root") = facilityRoot.c_str();
    if (!config.locationFacilityIdExtension.empty()) { 
        facilityIdNode.append_attribute("extension") = config.locationFacilityIdExtension.c_str();
    } else {
        if (facilityRoot == DEFAULT_OID_ROOT) { // If root is default, provide default extension
            facilityIdNode.append_attribute("extension") = DEFAULT_STRING.c_str();
        }
    }
    pugi::xml_node locationPlaceNode = healthCareFacilityNode.append_child("location"); 
    locationPlaceNode.append_attribute("classCode") = "PLC";
    locationPlaceNode.append_attribute("determinerCode") = "INSTANCE";
    pugi::xml_node locationNameNode = locationPlaceNode.append_child("name");
    std::string facilityName = config.locationFacilityName.empty() ? (config.sendingFacility.empty() ? DEFAULT_STRING.c_str() : config.sendingFacility.c_str()) : config.locationFacilityName.c_str();
    locationNameNode.text().set(facilityName.c_str());
}

void HL7MessageGenerator::addStructuredBody(pugi::xml_node& parentNode, const Study& study) {
    pugi::xml_node component = parentNode.append_child("component");
    pugi::xml_node structuredBody = component.append_child("structuredBody");

    pugi::xml_node sectionComponent = structuredBody.append_child("component");
    pugi::xml_node section = sectionComponent.append_child("section");

    pugi::xml_node sectionCode = section.append_child("code");
    std::string rsc_code = config.reportSectionCode.code.empty() ? "18748-4" : config.reportSectionCode.code; // Default LOINC code for Diag Imaging Report
    std::string rsc_cs = config.reportSectionCode.codeSystem.empty() ? "2.16.840.1.113883.6.1" : config.reportSectionCode.codeSystem; // Default LOINC OID
    std::string rsc_csn = config.reportSectionCode.codeSystemName.empty() ? "LOINC" : config.reportSectionCode.codeSystemName;
    std::string rsc_dn = config.reportSectionCode.displayName.empty() ? "Diagnostic Imaging Report Section" : config.reportSectionCode.displayName;

    sectionCode.append_attribute("code") = rsc_code.c_str(); 
    sectionCode.append_attribute("codeSystem") = rsc_cs.c_str(); 
    sectionCode.append_attribute("codeSystemName") = rsc_csn.c_str();
    sectionCode.append_attribute("displayName") = rsc_dn.c_str();
    
    section.append_child("title").text().set(study.studyDescription.empty() ? DEFAULT_STRING.c_str() : study.studyDescription.c_str());

    pugi::xml_node textNode = section.append_child("text");
    std::string narrative = std::string("Study Description: ") + (study.studyDescription.empty() ? DEFAULT_STRING : study.studyDescription) +
                            ". Modality: " + (study.modality.empty() ? DEFAULT_STRING : study.modality) + ".";
    if (!study.studyInstanceUID.empty()) {
        narrative += " Study UID: " + study.studyInstanceUID + ".";
    } else {
        narrative += " Study UID: " + DEFAULT_STRING + ".";
    }
    
    pugi::xml_node paragraph = textNode.append_child("paragraph");
    paragraph.text().set(narrative.c_str());
}


bool HL7MessageGenerator::saveMessageToFile(const std::string& message, const std::string& filePath) {
    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for saving HL7 message: " << filePath << std::endl;
        return false;
    }
    outFile << message;
    outFile.close();
    std::cout << "HL7 message saved to: " << filePath << std::endl;
    return true;
}

// --- XSD Validation Implementation ---
bool HL7MessageGenerator::validateMessageWithXSD(const std::string& xmlMessage) {
    if (config.cdaXsdPath.empty()) {
        std::cout << "XSD validation skipped: No XSD path configured." << std::endl;
        return true;
    }
    std::cout << "Validating message with XSD: " << config.cdaXsdPath << std::endl;

    XercesDOMParser* parser = new XercesDOMParser();
    XSDValidationErrorHandler errorHandler;

    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);
    parser->setDoSchema(true);
    parser->setValidationSchemaFullChecking(true);

    std::string actualSchemaPathForParser = config.cdaXsdPath;
    std::string schemaLocationArg = "urn:hl7-org:v3 " + actualSchemaPathForParser;
    
    XMLCh* schemaLocation = XMLString::transcode(schemaLocationArg.c_str());
    parser->setExternalSchemaLocation(schemaLocation);
    XMLString::release(&schemaLocation);
    
    parser->setErrorHandler(&errorHandler);

    bool validationSuccess = false;
    try {
        MemBufInputSource memBufIS(
            (const XMLByte*)xmlMessage.c_str(),
            xmlMessage.length(),
            "InMemoryDocument",
            false
        );
        parser->parse(memBufIS);
        if (errorHandler.getSawErrors()) {
            std::cerr << "XSD Validation Failed with errors." << std::endl;
            validationSuccess = false;
        } else {
            std::cout << "XSD Validation Successful: Document is valid." << std::endl;
            validationSuccess = true;
        }
    } catch (const OutOfMemoryException&) {
        std::cerr << "XSD Validation Error: OutOfMemoryException" << std::endl;
        validationSuccess = false;
    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "XSD Validation Error (XMLException): " << message << std::endl;
        XMLString::release(&message);
        validationSuccess = false;
    } catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "XSD Validation Error (DOMException): " << message << std::endl;
        XMLString::release(&message);
        validationSuccess = false;
    } catch (...) {
        std::cerr << "XSD Validation Error: An unknown exception occurred." << std::endl;
        validationSuccess = false;
    }
    delete parser;
    return validationSuccess;
}
