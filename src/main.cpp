#include <iostream>
#include <string>
#include <vector>
#include <limits> // Required for std::numeric_limits
#include <fstream> // Required for std::ifstream

#include "db_connector/DatabaseService.h"
#include "hl7_generator/HL7MessageGenerator.h"
#include "models/Patient.h"
#include "models/Study.h"
#include "ui/ConsoleUI.h"
#include "config_manager/ConfigManager.h" // Include the new ConfigManager

#include <sys/stat.h> // For mkdir (Linux/macOS)
#include <cerrno>     // For errno
#ifdef _WIN32
#include <direct.h>   // For _mkdir (Windows)
#endif

// Function to create a directory if it doesn't exist
bool createDirectoryIfNotExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) { // Path does not exist
        int ret = 0;
#ifdef _WIN32
        ret = _mkdir(path.c_str());
#else
        ret = mkdir(path.c_str(), 0755); // Permissions 0755
#endif
        if (ret == 0) {
            std::cout << "Directory created: " << path << std::endl;
            return true;
        } else {
            std::cerr << "Error creating directory " << path << " : " << strerror(errno) << std::endl;
            return false;
        }
    } else if (info.st_mode & S_IFDIR) { // Path exists and is a directory
        return true;
    } else { // Path exists but is not a directory
        std::cerr << "Error: " << path << " exists but is not a directory." << std::endl;
        return false;
    }
}

int main(int argc, char *argv[]) {
    std::cout << "HL7 Generation Application Starting..." << std::endl;

    // Initialize Xerces-C++
    HL7MessageGenerator::initializeXerces();

    // 0. Initialize Configuration Manager
    ConfigManager configManager;
    
    // Determine the path to the executable
    std::string exePath = argv[0];
    std::string exeDir = exePath.substr(0, exePath.find_last_of("/\\\\"));

    // Construct the path to the config file relative to the executable's directory
    std::string configFilePath = "config/hl7_config.xml"; // Default config file path relative to build directory

    // Override with command line argument if provided
    if (argc > 1) { // Allow overriding config file path via command line argument
        configFilePath = argv[1];
    }
    std::cout << "Using default configuration file: " << configFilePath << std::endl;
    std::ifstream configFile(configFilePath);
    if (!configFile.good()) {
        std::cerr << "FATAL: Default configuration file \\'" << configFilePath << "\\' not found. Exiting." << std::endl;
        return 1;
    }
    configFile.close(); // Close the file stream

    if (!configManager.loadConfig(configFilePath)) {
        std::cerr << "FATAL: Failed to load configuration from '" << configFilePath << "'. Exiting." << std::endl;
        return 1;
    }
    const AppConfig& config = configManager.getConfig();

    std::cout << "Configuration loaded. Output path: " << config.outputPath << std::endl;

    // Create output directory if it doesn't exist
    if (!config.outputPath.empty()) {
        if (!createDirectoryIfNotExists(config.outputPath)) {
            std::cerr << "Could not create or access output directory: " << config.outputPath << ". Please check permissions or path." << std::endl;
        }
    } else {
        std::cout << "Warning: Output path is not configured. Messages will not be saved to file unless a path is provided interactively or set in config." << std::endl;
    }

    // 1. Initialize DatabaseService
    DatabaseService dbService;
    std::cout << "Attempting to connect to DSN: " << config.odbcDsn << std::endl;
    if (!dbService.connect(config.odbcDsn, config.dbUser, config.dbPassword)) { 
        std::cerr << "FATAL: Failed to connect to database. Please check DSN configuration and credentials in '" << configFilePath << "'." << std::endl;
        std::cerr << "Ensure DSN '" << config.odbcDsn << "' is correctly set up in your ODBC administrator." << std::endl;
        return 1;
    }
    std::cout << "Successfully connected to the database." << std::endl;

    // 2. Initialize UI
    ConsoleUI ui(dbService);
    Patient selectedPatient;
    Study selectedStudy;

    int choice = 0;
    bool patientSelected = false;
    bool studySelected = false;

    while (choice != 3) {
        ui.displayMainMenu(); // This will now be a simple prompt from main
        std::cout << "\nMain Menu (from main.cpp):\n";
        std::cout << "1. Select Patient & Study\n";
        std::cout << "2. Generate HL7 Message\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter your choice: ";
        
        std::string inputLine;
        std::getline(std::cin, inputLine);

        try {
            if (inputLine.empty()) {
                std::cout << "Invalid input. Please enter a number.\n";
                choice = 0;
                continue;
            }
            choice = std::stoi(inputLine);
        } catch (const std::invalid_argument& ia) {
            std::cout << "Invalid input. Please enter a number.\n";
            choice = 0; // Reset choice to ensure loop continues correctly
            continue;
        } catch (const std::out_of_range& oor) {
            std::cout << "Input out of range. Please enter a valid number.\n";
            choice = 0; // Reset choice
            continue;
        }

        // No need for std::cin.fail() check or std::cin.ignore() anymore with getline + stoi

        switch (choice) {
            case 1: // Select Patient & Study
                ui.handlePatientAndStudySelection(selectedPatient, selectedStudy);

                if (!selectedPatient.patientID.empty()) { // Check standardized patientID
                    patientSelected = true;
                    std::cout << "Patient " << selectedPatient.name << " selected." << std::endl; // Using standardized name
                    if (!selectedStudy.studyInstanceUID.empty()) {
                        studySelected = true;
                        std::cout << "Study " << selectedStudy.studyDescription << " selected." << std::endl;
                    } else {
                        studySelected = false;
                        std::cout << "No study selected for the patient." << std::endl;
                    }
                } else {
                    patientSelected = false;
                    studySelected = false;
                    std::cout << "No patient selected." << std::endl;
                }
                break;
            case 2: // Generate HL7 Message
                if (patientSelected && studySelected) {
                    // 3. Initialize HL7MessageGenerator with loaded config
                    HL7MessageGenerator hl7Generator(config); // Pass the AppConfig object

                    std::cout << "Generating HL7 message for " << selectedPatient.name << ", Study: " << selectedStudy.studyDescription << std::endl;
                    std::string hl7Message = hl7Generator.generateORUMessage(selectedPatient, selectedStudy);

                    if (!hl7Message.empty()) {
                        std::cout << "\n--- Generated HL7 Message ---" << std::endl;
                        std::cout << hl7Message << std::endl;
                        std::cout << "--- End of HL7 Message ---\n" << std::endl;

                        // 4. Validate the generated message
                        std::cout << "Validating generated HL7 message..." << std::endl;
                        if (hl7Generator.validateMessageWithXSD(hl7Message)) {
                            std::cout << "HL7 message validated successfully against XSD." << std::endl;

                            // 5. Save to file
                            if (!config.outputPath.empty()) {
                                std::string filename = config.outputPath + "/ORU_" + selectedPatient.patientID + "_" + selectedStudy.accessionNumber + "_" + hl7Generator.getCurrentTimestamp("%Y%m%d%H%M%S") + ".xml";
                                if (hl7Generator.saveMessageToFile(hl7Message, filename)) {
                                    std::cout << "Message saved to " << filename << std::endl;
                                } else {
                                    std::cerr << "Failed to save message to file." << std::endl;
                                }
                            } else {
                                std::cout << "Output path not configured. Message not saved to file." << std::endl;
                            }
                        } else {
                            std::cerr << "HL7 message validation FAILED. Message not saved." << std::endl;
                        }
                    } else {
                        std::cerr << "Failed to generate HL7 message." << std::endl;
                    }
                } else {
                    std::cout << "Please select a patient and a study first (Option 1).\n";
                }
                break;
            case 3:
                std::cout << "Exiting application.\n";
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }

    // Cleanup
    dbService.disconnect();
    std::cout << "Disconnected from database." << std::endl;

    // Terminate Xerces-C++
    HL7MessageGenerator::terminateXerces();

    std::cout << "HL7 Generation Application Ended." << std::endl;
    return 0;
}
