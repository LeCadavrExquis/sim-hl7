cmake_minimum_required(VERSION 3.10)
project(HL7Generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Global include directory for project sources
include_directories(src)

# --- Define Source Files ---
# Assuming main.cpp is in src/main.cpp
set(MAIN_CPP_FILE ${CMAKE_SOURCE_DIR}/src/main.cpp)

# Get all .cpp files from src and its subdirectories
file(GLOB_RECURSE ALL_PROJECT_SOURCES "src/*.cpp" "src/**/*.cpp")

# Exclude main.cpp from the library sources
list(REMOVE_ITEM ALL_PROJECT_SOURCES ${MAIN_CPP_FILE})
set(HL7CORE_SOURCES ${ALL_PROJECT_SOURCES})

# --- Create HL7Core Library ---
add_library(HL7Core STATIC ${HL7CORE_SOURCES})
# The library will pick up its own headers via the global `include_directories(src)`

# --- ODBC Configuration ---
# Option 1: Find ODBC package (preferred for system-wide installs)
find_package(ODBC REQUIRED)
if(ODBC_FOUND)
    message(STATUS "ODBC Found. Include Dirs: ${ODBC_INCLUDE_DIRS}, Libraries: ${ODBC_LIBRARIES}")
    include_directories(${ODBC_INCLUDE_DIRS}) # Apply includes globally or target specific
    target_link_libraries(HL7Core PRIVATE ${ODBC_LIBRARIES}) # Link library to ODBC
else()
    message(FATAL_ERROR "ODBC library not found. Please install it (e.g., unixodbc-dev on Linux) and ensure it can be found by CMake.")
endif()
# On Windows, ODBC is often part of the SDK and might link via odbc32.lib without find_package.
# if(WIN32)
#   target_link_libraries(HL7Generator PRIVATE odbc32)
# endif()

# --- XML Library: pugixml ---
# Option 1: Find pugixml package (if installed system-wide and has CMake config)
# find_package(pugixml REQUIRED)
# if(PUGIXML_FOUND)
#     message(STATUS "pugixml found through find_package.")
#     # Link against the imported target if available (modern CMake)
#     # Common imported targets are pugixml::pugixml or pugixml::static / pugixml::shared
#     # If an imported target like pugixml::static or pugixml::pugixml is available:
#     if(TARGET pugixml::pugixml)
#         target_link_libraries(HL7Generator PRIVATE pugixml::pugixml)
#     elseif(TARGET pugixml::static)
#         target_link_libraries(HL7Generator PRIVATE pugixml::static)
#     else()
#         # Fallback to linking by library name if no imported target is found by find_package
#         # This might require include_directories(${PUGIXML_INCLUDE_DIRS}) if not handled by find_package
#         if(PUGIXML_INCLUDE_DIRS)
#              include_directories(${PUGIXML_INCLUDE_DIRS})
#         endif()
#         target_link_libraries(HL7Generator PRIVATE ${PUGIXML_LIBRARIES}) # PUGIXML_LIBRARIES should be set by find_package
#     endif()
# else()
#     message(FATAL_ERROR "pugixml library not found. Please install it (e.g., libpugixml-dev on Linux) or ensure it can be found by CMake (e.g., via CMAKE_PREFIX_PATH).")
# endif()

# Option 2: If pugixml was added via add_subdirectory
# target_link_libraries(HL7Generator PRIVATE pugixml) # Uncomment if pugixml is a subdirectory

# Option 3: If you are compiling pugixml.cpp directly (already added to SOURCES)
# No extra target_link_libraries needed for pugixml itself, just ensure include directories are set.

# Option 4: If pugixml is installed system-wide but no CMake config (manual linking)
include_directories(/usr/include) # Or wherever pugixml.hpp is - applied globally
target_link_libraries(HL7Core PRIVATE pugixml) # Link library to pugixml

# --- XSD Validation Library: Xerces-C++ ---
find_package(XercesC REQUIRED)
if(XERCESC_FOUND)
    message(STATUS "Xerces-C++ found.")
    include_directories(${XERCESC_INCLUDE_DIR}) # Applied globally
    target_link_libraries(HL7Core PRIVATE ${XERCESC_LIBRARIES}) # Link library to XercesC
else()
    message(FATAL_ERROR "Xerces-C++ library not found. Please install it (e.g., libxerces-c-dev on Linux) and ensure it can be found by CMake.")
endif()

# --- Threads ---
find_package(Threads REQUIRED)

# --- Main Executable ---
add_executable(HL7Generator ${MAIN_CPP_FILE}) # Build main.cpp into the executable
target_link_libraries(HL7Generator PRIVATE HL7Core Threads::Threads) # Link executable against our library and Threads

# --- Installation (Optional) ---
# Example: Install the executable to the system bin directory
# install(TARGETS HL7Generator DESTINATION bin)

# Example: Install the library
# install(TARGETS HL7Core DESTINATION lib)

# Example: Install header files (if you have a public API)
# install(DIRECTORY src/your_public_api_headers/ DESTINATION include/your_project_name)

# --- Packaging with CPack (Optional) ---
# include(CPack)
# CPack can be configured here to create installers or packages (e.g., .deb, .rpm, .zip)
# For example:
# set(CPACK_GENERATOR "TGZ") # Create a .tar.gz package
# set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
# set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION}) # Define PROJECT_VERSION earlier
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HL7 Message Generator Application")
# set(CPACK_PACKAGE_VENDOR "Your Name/Organization")
# set(CPACK_PACKAGE_CONTACT "your.email@example.com")
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt") # If you have a license
# set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")   # If you have a README

# --- DICOM Configuration ---
find_package(dicomhero6 REQUIRED) # Changed DicomHero6 to dicomhero6
if(dicomhero6_FOUND) # Changed DICOMHERO6_FOUND to dicomhero6_FOUND
    include_directories(${dicomhero6_INCLUDE_DIRS}) # Changed DICOMHERO6_INCLUDE_DIRS
    # The dicomhero6Config.cmake should correctly define dicomhero6_LIBRARIES
    # to include libdicomhero6.so and potentially libdicomheroObjects6.so if it's a dependency.
    target_link_libraries(HL7Generator PRIVATE ${dicomhero6_LIBRARIES}) # Changed DICOMHERO6_LIBRARIES
    message(STATUS "dicomhero6 found and configured.")
else()
    message(FATAL_ERROR "dicomhero6 not found. Please ensure libdicomhero6 is installed and discoverable by CMake, or provide dicomhero6_DIR.")
endif()

# Link Threads::Threads to HL7Core as well if any of its dependencies require it.
# It's safer to link it to both or to the final executable if unsure.
target_link_libraries(HL7Core PRIVATE Threads::Threads)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Force manual linking for DicomHero and Xerces-C++ (workaround for CMake config issues) ---
target_link_libraries(HL7Generator PRIVATE dicomhero6 dicomheroObjects6 xerces-c)

