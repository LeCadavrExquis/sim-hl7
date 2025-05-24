# HL7 Scintigraphy Report Generator

This C++ application generates HL7 v3 Clinical Document Architecture (CDA) compliant XML messages for patient scintigraphy studies. It retrieves patient and study data from a relational database via ODBC.

---

## 1. Setup

### 1.1. Prerequisites

*   **Common:**
    *   Git (for cloning the repository).
    *   A C++17 compatible compiler (e.g., GCC, Clang, MSVC).
    *   CMake (version 3.10 or higher).

*   **For Running with Docker (Recommended for simplified environment):**
    *   Docker Engine.
    *   Docker Compose.

*   **For Running Locally (Without Docker):**
    *   ODBC driver manager (e.g., unixODBC on Linux, Windows ODBC Data Source Administrator).
    *   PostgreSQL ODBC driver.
    *   pugixml library (development headers).
    *   Xerces-C++ library (development headers).
    *   A PostgreSQL server instance (can be local or remote).

### 1.2. Database Setup

The application requires a PostgreSQL database named `simdb` with a user `simuser` and password `simpassword`.

#### 1.2.1 Using Docker (Recommended)

The `docker-compose.yml` file defines a PostgreSQL service (`postgres`).

1.  **Start the database service:**
    From the project root directory:
    ```bash
    docker-compose up -d
    ```     

#### 1.2.2. Using a Local PostgreSQL Instance

1.  Ensure you have a PostgreSQL server installed and running.
2.  Create the database and user. Using `psql` or a GUI tool:
    ```sql
    CREATE DATABASE simdb;
    CREATE USER simuser WITH PASSWORD 'simpassword';
    ALTER DATABASE simdb OWNER TO simuser;
    GRANT ALL PRIVILEGES ON DATABASE simdb TO simuser;
    ```
3.  Connect to your new database as `simuser` (or a superuser initially) and run the initialization script:
    ```bash
    psql -U simuser -d simdb -h localhost -f db/init_db.sql
    ```

### 1.2.3. ODBC Configuration

The application uses ODBC to connect to the database. Configure an ODBC Data Source Name (DSN).

*   **Database Connection Details:**
    *   **Server/Host:** `localhost` (if using the Dockerized DB or a local DB instance).
    *   **Port:** `5432` (default PostgreSQL port).
    *   **Database Name:** `simdb`
    *   **User:** `simuser`
    *   **Password:** `simpassword`

*   **Linux (using unixODBC):**
    1.  Install the PostgreSQL ODBC driver (if not already present):
        ```bash
        sudo apt-get update
        sudo apt-get install odbc-postgresql unixodbc unixodbc-dev
        ```
    2.  Edit `/etc/odbc.ini` (for a system DSN) or `~/.odbc.ini` (for a user DSN). Add:
        ```ini
        [simdb_dsn]
        Description         = PostgreSQL simdb
        Driver              = PostgreSQLUnicode
        Database            = simdb
        Servername          = localhost
        UserName            = simuser
        Password            = simpassword
        Port                = 5432
        ReadOnly            = No
        RowVersioning       = No
        ShowSystemTables    = No
        ShowOidColumn       = No
        FakeOidIndex        = No
        ```
    3.  Ensure the `PostgreSQLUnicode` driver is defined in `/etc/odbcinst.ini`. It should look something like this (path to driver may vary):
        ```ini
        [PostgreSQLUnicode]
        Description         = ODBC for PostgreSQL
        Driver              = /usr/lib/x86_64-linux-gnu/odbc/psqlodbcw.so
        Setup               = /usr/lib/x86_64-linux-gnu/odbc/libodbcpsqlS.so
        FileUsage           = 1
        ```
    4.  **Test the DSN:**
        ```bash
        isql -v simdb_dsn simuser simpassword
        ```
        You should see "Connected!" and a SQL prompt. Type `quit` to exit.

*   **Windows:**
    1.  Download and install the PostgreSQL ODBC driver (psqlODBC) from the official PostgreSQL website if you haven't already.
    2.  Open "ODBC Data Source Administrator" (search for "ODBC" in Windows search, choose the 64-bit version if applicable).
    3.  Go to the "System DSN" or "User DSN" tab and click "Add...".
    4.  Select "PostgreSQL Unicode(x64)" (or similar) driver and click "Finish".
    5.  Fill in the configuration:
        *   Data Source Name (DSN): `simdb_dsn`
        *   Database: `simdb`
        *   Server: `localhost`
        *   Port: `5432`
        *   User Name: `simuser`
        *   Password: `simpassword`
        *   SSL Mode: `disable` (or as appropriate for your setup).
    6.  Click "Test". If successful, click "Save".

## 2. Running the Application

### 2.1. Running with Docker

This is the recommended way to run the application as it encapsulates dependencies and configuration.

*   **Execute the HL7 Generator Application:**
    ```bash
    docker-compose run --rm app
    ```

### 2.2. Running Locally (Without Docker)

*   **Build the Project:**
    1.  Create a build directory (if it doesn't already exist) and navigate into it:
        ```bash
        mkdir -p build
        cd build
        ```
    2.  Configure the project with CMake:
        ```bash
        cmake ..
        ```
    3.  Compile the project:
        ```bash
        make
        ```

*   **Execute the Application:**
    1.  If you are not already in the `build` directory, navigate to it:
        ```bash
        cd build
        ```
    2.  Run the application:
        ```bash
        ./HL7Generator
        ```
---

## 3. Using the Application (Console UI)

- **Main Menu:** Allows you to search for patients, generate HL7 messages for a selected study, or exit the application.
- **Patient Search:** You can enter a search term (e.g., patient name or ID) or list all available patients from the database.
- **Study Selection:** After selecting a patient, you can choose from their available scintigraphy studies.
- **HL7 Generation:** Once a study is selected, the application generates and saves an HL7 CDA compliant XML file. The filename and location are typically logged to the console and depend on the `OutputPath` in `hl7_config.xml`.

---