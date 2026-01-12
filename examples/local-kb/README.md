# Local Knowledge Base Example

Build a local SCANOSS Knowledge Base (KB) with support for **file matching** and **snippet matching**.

## Tools Included

- **ldb**: Linked-list database for storing KB data
- **minr**: Mining tool for downloading and indexing OSS components
- **scanoss**: Scanning engine for querying the KB

## Quick Start

### 1. Build the Docker Image

```bash
docker build -t scanoss-stack .
```

### 2. Start the Container

```bash
docker run -d --name scanoss scanoss-stack sleep infinity
```

### 3. Create a Knowledge Base

```bash
# Mine a component
docker exec scanoss minr -d scanoss,webhook,1.0,20200320,BSD-3-Clause,pkg:github/scanoss/webhook \
     -u https://github.com/scanoss/webhook/archive/1.0.tar.gz

# Extract snippet fingerprints
docker exec scanoss minr -z mined

# Create version file (required for import)
docker exec scanoss bash -c "echo '{\"monthly\":\"25.01\", \"daily\":\"25.01.12\"}' > mined/version.json"

# Import into KB
docker exec scanoss minr -i mined/
```

### 4. Download Test Files

Download and extract a file from the same archive to test scanning:

```bash
docker exec scanoss curl -sL https://github.com/scanoss/webhook/archive/1.0.tar.gz -o /tmp/test.tar.gz
docker exec scanoss tar -xzf /tmp/test.tar.gz -C /tmp
```

### 5. Scan Original File (100% Match)

```bash
docker exec scanoss scanoss /tmp/webhook-1.0/scanoss/github.py | jq
```

Expected output:
```json
{
  "github.py": [{
    "id": "file",
    "matched": "100%",
    "purl": ["pkg:github/scanoss/webhook"],
    ...
  }]
}
```

### 6. Modify the File

```bash
docker exec scanoss bash -c "echo '# modified by user' >> /tmp/webhook-1.0/scanoss/github.py"
```

### 7. Scan Modified File (Snippet Match)

```bash
docker exec scanoss scanoss /tmp/webhook-1.0/scanoss/github.py | jq
```

Expected output:
```json
{
  "github.py": [{
    "id": "snippet",
    "matched": "97%",
    "lines": "1-192",
    "oss_lines": "3-194",
    "purl": ["pkg:github/scanoss/webhook"],
    ...
  }]
}
```

### 8. Cleanup

```bash
docker stop scanoss && docker rm scanoss
```

## Complete Example (Copy & Paste)

```bash
# Build and start
docker build -t scanoss-stack .
docker run -d --name scanoss scanoss-stack sleep infinity

# Create Knowledge Base
docker exec scanoss minr -d scanoss,webhook,1.0,20200320,BSD-3-Clause,pkg:github/scanoss/webhook \
     -u https://github.com/scanoss/webhook/archive/1.0.tar.gz
docker exec scanoss minr -z mined
docker exec scanoss bash -c "echo '{\"monthly\":\"25.01\", \"daily\":\"25.01.12\"}' > mined/version.json"
docker exec scanoss minr -i mined/

# Download test files
docker exec scanoss curl -sL https://github.com/scanoss/webhook/archive/1.0.tar.gz -o /tmp/test.tar.gz
docker exec scanoss tar -xzf /tmp/test.tar.gz -C /tmp

# Test file match (100%)
docker exec scanoss scanoss /tmp/webhook-1.0/scanoss/github.py | jq

# Modify file
docker exec scanoss bash -c "echo '# modified' >> /tmp/webhook-1.0/scanoss/github.py"

# Test snippet match
docker exec scanoss scanoss /tmp/webhook-1.0/scanoss/github.py | jq

# Cleanup
docker stop scanoss && docker rm scanoss
```

## Scanning Your Own Files

To scan files from your host machine, copy them into the container:

```bash
# Copy a file into the container
docker cp /path/to/your/file.py scanoss:/tmp/file.py

# Scan it
docker exec scanoss scanoss /tmp/file.py
```

Or mount a volume when starting the container:

```bash
docker run -d --name scanoss -v $(pwd)/mycode:/code scanoss-stack sleep infinity
docker exec scanoss scanoss /code/myfile.py
```
