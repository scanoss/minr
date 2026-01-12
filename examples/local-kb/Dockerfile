# SCANOSS Local Knowledge Base Docker Image
# This image contains all tools needed to create a local SCANOSS Knowledge Base:
# - ldb: Linked-list database for storing KB data
# - minr: Mining tool for downloading and indexing OSS components
# - scanoss: Scanning engine for querying the KB

FROM debian:bookworm-slim

LABEL maintainer="SCANOSS <support@scanoss.com>"
LABEL description="SCANOSS Knowledge Base Builder with ldb, minr, and engine"

# Version pinning for reproducibility
ENV LDB_VERSION=4.1.9
ENV MINR_VERSION=2.6.0
ENV ENGINE_VERSION=5.4.19

# Install build and runtime dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    zlib1g-dev \
    libgcrypt-dev \
    curl \
    jq \
    p7zip-full \
    unrar-free \
    unzip \
    xz-utils \
    ruby \
    && rm -rf /var/lib/apt/lists/*

# Create required directories for LDB
RUN mkdir -p /var/lib/ldb/oss \
    /var/lib/scanoss \
    /var/log/scanoss/ldb \
    /usr/local/etc/scanoss/ldb

# Download and build ldb
WORKDIR /opt
RUN curl -sL https://github.com/scanoss/ldb/archive/refs/tags/v${LDB_VERSION}.tar.gz -o ldb.tar.gz && \
    tar -xzf ldb.tar.gz && \
    cd ldb-${LDB_VERSION} && \
    make all && \
    SUDO_USER=root make install && \
    ldconfig

# Download and build minr
RUN curl -sL https://github.com/scanoss/minr/archive/refs/tags/v${MINR_VERSION}.tar.gz -o minr.tar.gz && \
    tar -xzf minr.tar.gz && \
    cd minr-${MINR_VERSION} && \
    make all && \
    make install

# Download and build engine
RUN curl -sL https://github.com/scanoss/engine/archive/refs/tags/v${ENGINE_VERSION}.tar.gz -o engine.tar.gz && \
    tar -xzf engine.tar.gz && \
    cd engine-${ENGINE_VERSION} && \
    make && \
    make install

# Working directory for mining operations
WORKDIR /data

# Default command
CMD ["bash"]
