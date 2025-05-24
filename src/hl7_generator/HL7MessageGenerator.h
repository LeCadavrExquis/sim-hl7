#ifndef HL7MESSAGEGENERATOR_H
#define HL7MESSAGEGENERATOR_H

#include <string>
#include "../models/Patient.h"
#include "../models/Study.h"
#include "../config_manager/ConfigManager.h" // Include AppConfig
#include "pugixml.hpp"

// Xerces-C++ Includes for XSD validation
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/validators/common/Grammar.hpp>


XERCES_CPP_NAMESPACE_USE


class HL7MessageGenerator {
public:
    HL7MessageGenerator(const AppConfig& configuration);
    ~HL7MessageGenerator(); // Destructor for Xerces-C++ cleanup

    std::string generateORUMessage(const Patient& patient, const Study& study);
    bool saveMessageToFile(const std::string& message, const std::string& filePath);
    bool validateMessageWithXSD(const std::string& xmlMessage);

    // Static members for Xerces initialization (call once)
    static bool xercesInitialized;
    static void initializeXerces();
    static void terminateXerces();

    std::string getCurrentTimestamp(const char* format = "%Y%m%d%H%M%S%z");

private:
    const AppConfig& config;

    void addHeader(pugi::xml_document& doc, const Patient& patient, const Study& study, const std::string& effectiveTime, const std::string& documentIdExt);
    void addRecordTarget(pugi::xml_node& parentNode, const Patient& patient);
    void addAuthor(pugi::xml_node& parentNode, const std::string& effectiveTime);
    void addCustodian(pugi::xml_node& parentNode);
    void addComponentOf(pugi::xml_node& parentNode, const Study& study);
    void addStructuredBody(pugi::xml_node& parentNode, const Study& study);

    std::string generateUUID();
    void addPatientRole(pugi::xml_node& recordTargetNode, const Patient& patient);
    void addPlayingEntity(pugi::xml_node& patientRoleNode, const Patient& patient);
    void addAssignedAuthor(pugi::xml_node& authorNode, const std::string& effectiveTime);
    void addAssignedCustodian(pugi::xml_node& custodianNode);
    void addEncompassingEncounter(pugi::xml_node& componentOfNode, const Study& study);
    void addLocation(pugi::xml_node& encompassingEncounterNode);
    void addServiceProviderOrganization(pugi::xml_node& locationNode);
    void addObservation(pugi::xml_node& componentNode, const Study& study);
    void addParticipant(pugi::xml_node& parentNode, const std::string& typeCode, const std::string& roleClassOID, const std::string& roleCode, const std::string& roleCodeSystem, const std::string& roleDisplayName, const std::string& entityName, const std::string& entityIdRoot, const std::string& entityIdExt);


};

// Custom Error Handler for Xerces-C++ Validation
class XSDValidationErrorHandler : public HandlerBase {
public:
    XSDValidationErrorHandler() : fSawErrors(false) {}
    ~XSDValidationErrorHandler() override = default;

    void warning(const SAXParseException& exc) override;
    void error(const SAXParseException& exc) override;
    void fatalError(const SAXParseException& exc) override;
    void resetErrors() override;
    bool getSawErrors() const { return fSawErrors; }

private:
    bool fSawErrors;
};


#endif // HL7MESSAGEGENERATOR_H
