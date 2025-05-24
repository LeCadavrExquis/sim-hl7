-- This script will be run automatically by the official postgres Docker image on first container startup.
\i /docker-entrypoint-initdb.d/init_db.sql
