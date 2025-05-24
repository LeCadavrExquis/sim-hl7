FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \ 
    g++ \ 
    cmake \ 
    make \ 
    unixodbc-dev \ 
    libpugixml-dev \ 
    libxerces-c-dev \ 
    wget \ 
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .


# Copy and install Dicom HERO .deb package from lib directory
COPY lib/dicomhero6-6.2.1.0-Linux_Ubuntu2204.deb /app/
RUN dpkg -i /app/dicomhero6-6.2.1.0-Linux_Ubuntu2204.deb || apt-get install -f -y --no-install-recommends

# Build the application
RUN rm -rf build && mkdir build && cd build && cmake .. && make HL7Generator
RUN echo "--- HL7Generator dependencies ---" && ldd /app/build/HL7Generator && echo "---------------------------------"

# --- Runtime Stage ---
FROM ubuntu:22.04

ENV LD_LIBRARY_PATH=/usr/lib

# Install runtime dependencies and PostgreSQL ODBC driver
RUN apt-get update && apt-get install -y --no-install-recommends \
    unixodbc \
    libodbc1 \
    libpugixml1v5 \
    libxerces-c3.2 \
    libicu70 \
    libstdc++6 \
    odbc-postgresql \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled application from the builder stage
COPY --from=builder /app/build/HL7Generator /app/HL7Generator

# Copy all Dicom HERO library files
COPY --from=builder /usr/lib/libdicomhero6.so* /usr/lib/
# Ensure symlinks exist for the dynamic linker
RUN if [ ! -e /usr/lib/libdicomhero6.so.6 ] && [ -e /usr/lib/libdicomhero6.so.6.2.1 ]; then ln -s /usr/lib/libdicomhero6.so.6.2.1 /usr/lib/libdicomhero6.so.6; fi && \
    if [ ! -e /usr/lib/libdicomhero6.so ] && [ -e /usr/lib/libdicomhero6.so.6.2.1 ]; then ln -s /usr/lib/libdicomhero6.so.6.2.1 /usr/lib/libdicomhero6.so; fi
RUN ldconfig
# Copy Dicom HERO headers (less critical for runtime, but can be included)
COPY --from=builder /usr/include/dicomhero6 /usr/include/dicomhero6

# Add execute permissions
RUN chmod +x /app/HL7Generator

# Copy configuration files
COPY config /app/config
COPY db/odbc.ini /etc/odbc.ini
COPY db/odbcinst.ini /etc/odbcinst.ini

# Copy CDA schemas and related files for validation
COPY cda_r2_normativewebedition2010 /app/cda_r2_normativewebedition2010

# Ensure the output directory exists and is writable if the app writes there
RUN mkdir -p /app/output && chmod 777 /app/output

# Set the entrypoint
ENTRYPOINT ["/app/HL7Generator"]
