-- Copy of init_db.sql for Docker automatic initialization
-- Create Patients table
CREATE TABLE IF NOT EXISTS Patients (
    pat_id VARCHAR(255) PRIMARY KEY,
    pat_name VARCHAR(255) NOT NULL,
    pat_birth_dt VARCHAR(8) NOT NULL, -- Format: YYYYMMDD
    pat_gender_code VARCHAR(10)
);

-- Create Studies table
CREATE TABLE IF NOT EXISTS Studies (
    study_uid VARCHAR(255) PRIMARY KEY,
    pat_id VARCHAR(255) REFERENCES Patients(pat_id),
    acc_num VARCHAR(255),
    study_dt VARCHAR(8),   -- Format: YYYYMMDD
    study_tm VARCHAR(6),   -- Format: HHMMSS
    mod VARCHAR(10),
    study_desc VARCHAR(1000),
    ref_phys_name VARCHAR(255)
);

-- Insert dummy patients
INSERT INTO Patients (pat_id, pat_name, pat_birth_dt, pat_gender_code) VALUES
('P001', 'John Doe', '19800101', 'M'),
('P002', 'Jane Smith', '19900202', 'F');

-- Insert dummy studies
INSERT INTO Studies (study_uid, pat_id, acc_num, study_dt, study_tm, mod, study_desc, ref_phys_name) VALUES
('S001', 'P001', 'A123', '20240501', '101500', 'NM', 'Bone scan', 'Dr. House'),
('S002', 'P002', 'B456', '20240502', '113000', 'CT', 'Head CT', 'Dr. Wilson');
