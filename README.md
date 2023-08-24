# minr

Minr is a multi-purpose command line tool used for data mining. Minr downloads, indexes and imports source code metadata into the knowledge base. Minr can be also used locally for detecting licenses and cryptographic algorithms usage.

# Setup 
Component mining requires a Knowledge database installed. Scanoss use the SCANOSS LDB (Linked-list database) as a shared library. LDB Source code and installation guide can be found on https://github.com/scanoss/ldb 

# System dependencies
- coreutils
- gzip
- tar
- 7z
- unrar
- unzip
- xz-utils
- ruby
- curl

Only for scancode mode:
- jq
- scancode

# Build and Install

```
wget -O minr.zip https://github.com/scanoss/minr/archive/refs/heads/master.zip
unzip minr.zip
cd minr-master
make all
sudo make install
cd ..
```
You also need to create the required ldb tables/dirs:

```
sudo mkdir -p /var/lib/ldb/oss/{purl,url,file,wfp}
sudo chown -R user.user /var/lib/ldb/  # replace 'user' with your current user
sudo chmod -R 755 /var/lib/ldb/
```

# Usage (downloading)

Minr takes two arguments: 

* `-d`: Target metadata (CSV) 
* `-u`: Target URL (pointing to a downloadable archive or a local folder)

Usage example: 

```
$ minr -d madler,pigz,2.4,20171227,zlib,pkg:github/madler/pigz -u https://github.com/madler/pigz/archive/v2.4.zip 
```

The target metadata is a comma delimited list of the following fields: 

* Vendor 
* Component 
* Version 
* Release date

* License
* Purl (see https://github.com/package-url/purl-spec)


Note: Commas are not admitted in any of these fields.  

# Local mining
Minr can also be used to search for licenses, quality code, copyrights and cryptographic algorithms from a local directory.
License mining is available using -L option

Usage example: 

```
$ minr -L /home/johndoe/os-projects
```

Mines the **os-projects** directory (and recursively, its subfolders) searching for licenses declaration. The output is presented at the standard output.
Available options are
* **-L** Scans for licenses
* **-C** Scans for copyrights
* **-Q** Scans for code quality
* **-Y** Scans for cryptographic algorithms

For the given example using the **-Y** option, the output is like:
```
/home/johndoe/os-projects/myPersonalWallet/main.c,SHA,128
/home/johndoe/os-projects/fileMgmt/fileordering/unique.c,MD5,128
/home/johndoe/os-projects/fileMgmt/fileochecking/mycrc32.c,CRC32,128
```

Where each algorithm is reported once for each file. The first data of each row is the file path and its name, the second is the algorithm name and the last is the coding strenght for that algorithm.


## File Discrimination 

The following criteria will be use to discriminate files: 

* File is not a physical file (no symlinks, etc) 
* File extension is ignored
* File size is below a predefined limit
* File header has unwanted contents 

# Creating an Open Source Knowledge Base

The following examples show the entire process for downloading an OSS component, importing it into the Knowledge Base and performing a scan against it using the SCANOSS Open Source Inventory Engine:


## URL mining

URL mining is the process of downloading a component, expanding the files and saving component, metadata and original sources for snippet mining.

```
$ minr -d scanoss,webhook,1.0,20200320,BSD-3-Clause,pkg:github/scanoss/webhook -u https://github.com/scanoss/webhook/archive/1.0.tar.gz
Downloading https://github.com/scanoss/webhook/archive/1.0.tar.gz
$
```

A `mined/` directory is created with the downloaded metadata. This includes component and file metadata and source code archives which are kept using the `.mz` archives, specifically designed for this purpose. 

## Snippet mining

Snippet mining is a second step, where snippet information is mined from the `.mz` archives. This is achieved with the `-z` parameter.

```
$ minr -z mined
mined/sources/146a.mz
...
$
```

A `mined/snippets` directory is created with the snippet wfp fingerprints extracted from the `.mz` files located in `mined/sources`

## Data importation into the LDB
To be able to import data into the LDB the version.json file must be present inside the mined directory. This file provide the last update date and will be imported join to the mined tables.
The "version.json file must have the following format:

```
{"monthly":"YY.MM", "daily":"YY.MM.DD"}
```

This file can be generated with this command in bash:
```
echo '{"monthly":"22.01", "daily":"22.01.24"}' > mined/version.json
```
Then the importation can be performed:
```
$ minr -i mined/
$
```

The LDB is now loaded with the component information and a scan can be performed.

## Scanning against the LDB Knowledge Base

The following example shows an entire component match:

```
$ scanoss 0.8.2.zip
{
  "0.8.2.zip": [
    {
      "id": "file",
      "status": "pending",
      "lines": "all",
      "oss_lines": "all",
      "matched": "100%",
      "purl": [
        "pkg:github/scanoss/webhook"
      ],
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook",
      "release_date": "20200320",
      "file": "1.0.tar.gz",
      "url_hash": "611e5c3a58a3c2b78385556368c5230e",
      "file_hash": "611e5c3a58a3c2b78385556368c5230e",
      "file_url": "https://github.com/scanoss/webhook/archive/1.0.tar.gz",
      "dependencies": [],
      "licenses": [
        {
          "name": "BSD-3-Clause",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/BSD-3-Clause.txt",
          "copyleft": "no",
          "patent_hints": "no",
          "source": "component_declared"
        }
      ],
      "copyrights": [
        {
          "name": "Copyright (C) 2017-2020; SCANOSS Ltd. All rights reserved.",
          "source": "license_file"
        }
      ],
      "vulnerabilities": [],
      "quality": [],
      "cryptography": [],
      "server": {
        "hostname": "localhost",
        "version": "4.2.4",
        "flags": "0",
        "elapsed": "0.041169s"
      }
    }
  ]
}

$
```

The following example shows an entire file match:

```
$ scanoss test.c
{
  "test.c": [
    {
      "id": "file",
      "status": "pending",
      "lines": "all",
      "oss_lines": "all",
      "matched": "100%",
      "purl": [

        "pkg:github/scanoss/webhook"
      ],
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook",
      "release_date": "20200320",
      "file": "webhook-1.0/scanoss/github.py",
      "url_hash": "611e5c3a58a3c2b78385556368c5230e",
      "file_hash": "8c2fa3f24a09137f9bb3860fa21c677e",
      "file_url": "https://osskb.org/api/file_contents/8c2fa3f24a09137f9bb3860fa21c677e",
      "dependencies": [],
      "licenses": [
        {
          "name": "BSD-3-Clause",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/BSD-3-Clause.txt",
          "copyleft": "no",
          "patent_hints": "no",
          "source": "component_declared"
        }
      ],
      "copyrights": [
        {
          "name": "Copyright (C) 2017-2020; SCANOSS Ltd. All rights reserved.",
          "source": "license_file"
        }
      ],
      "vulnerabilities": [],
      "quality": [],
      "cryptography": [],
      "server": {
        "hostname": "localhost",
        "version": "4.2.4",
        "flags": "0",
        "elapsed": "0.129558s"
      }
    }
  ]
}
$
```

The following example adds a LF to the end of a file. The modified file does not match entirely anymore, but instead the snippet detection comes into effect: 

```
$ echo -e "\n" >> test.c
$ scanoss test.c
{
  "test.c": [
    {
      "id": "snippet",
      "status": "pending",
      "lines": "1-192",
      "oss_lines": "3-194",
      "matched": "99%",
      "purl": [
        "pkg:github/scanoss/webhook"
      ],
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook",
      "release_date": "20200320",
      "file": "webhook-1.0/scanoss/github.py",
      "url_hash": "611e5c3a58a3c2b78385556368c5230e",
      "file_hash": "8c2fa3f24a09137f9bb3860fa21c677e",
      "file_url": "https://osskb.org/api/file_contents/8c2fa3f24a09137f9bb3860fa21c677e",
      "dependencies": [],
      "licenses": [
        {
          "name": "BSD-3-Clause",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/BSD-3-Clause.txt",
          "copyleft": "no",
          "patent_hints": "no",
          "source": "component_declared"
        }
      ],
      "copyrights": [
        {
          "name": "Copyright (C) 2017-2020; SCANOSS Ltd. All rights reserved.",
          "source": "license_file"
        }
      ],
      "vulnerabilities": [],
      "quality": [],
      "cryptography": [],
      "server": {
        "hostname": "localhost",
        "version": "4.2.4",
        "flags": "0",
        "elapsed": "0.067090s"

      }
    }
  ]
}
$
```
 
# Mined/ output structure 

The minr command produces a directory called mined/ containing the mined metadata and original OSS files.  

## Url table csv (mined/url.csv) 

OSS components are saved in a single CSV file called `mined/url.csv` which contains the original archive md5 hash, vendor, name, version, release date, license, purl and url as in the following example: 

```
611e5c3a58a3c2b78385556368c5230e,scanoss,webhook,1.0,20200320,BSD-3-Clause,pkg:github/scanoss/webhook,https://github.com/scanoss/webhook/archive/1.0.tar.gz 
```

Other metadata are saved in `mined/attribution.csv`, `mined/copyrights.csv`, `mined/cryptography.csv`, `mined/licenses.csv` and `mined/quality.csv` files. <!--FIXME: add more details here -->

## File table csv (mined/file)

OSS files are split in 256 files named `00.csv` to `ff.csv` containing the component archive md5, the file md5 and the file name, as follows: 

```
6c067f97266c817b339f0e989499c8e4,00fc6e8b3ae062fbcfbe8d2e40d36e68,bison-3.5/src/ielr.c
6c067f97266c817b339f0e989499c8e4,00ac96f26eed6fab33d7451e8f697939,bison-3.5/lib/stat-w32.c 
```
## Pivot table csv (mined/pivot.csv)

This table is used to mantain the relation between a file and an url, the md5 of the files are saved in a single CSV file called `mined/pivot.csv` which contains the url hash and the original archive md5 hash as in the following example: 

```
00fc6e8b3ae062fbcfbe8d2e40d36e68,6c067f97266c817b339f0e989499c8e4
```
## Wfp table csv (mined/wfp) 

OSS snippets are 32-bit identifiers calculated with the winnowing algorithm. They are saved in binary format split in 256 files called `00.bin` - `ff.bin` containing sequential records composed of:

* 3 bytes for the snippet wfp identifier (the first byte is the file name) 
* 16 bytes for the file md5 where the snippet is found 
* 2 bytes for the line number where the wfp starts 

These `.bin` files can be joined by simple concatenation.

## Sources table mz file (mined/sources)

The `sources/` directory is where the original downloaded source files are stored. They are kept in 65536 `.mz` archive files. These files can be listed and extracted using the `unmz` command, which is part of minr. Unlike ZIP files, MZ files can be joined by simple concatenation.

## Dependency table csv file (mined/dependency.csv)
The `dependency.csv` file contains mined metadata on declared dependencies.

## PURLs table csv file (mined/purl.csv) 
The `purl.csv` file contains component-level information as well as relations between purls.
Component-level information is presented with seven CSV fields:

* Creation date,
* Latest date,
* Updated date,
* Stars,
* Watchers,
* Is_a_fork

Alternatively, purl relations are declared here with a single field:
* related PURL

# Minr join 

Minr can also be used to join two `mined/` structures as follows: 

```
$ minr -f dir1/mined -t dir2/mined

```

All files within a `mined/` structure can be joined by simple concatenation. This applies to `.csv`, `.bin` (snippet records) and `.mz` (compressed source code archives). The command above will concatenate all `.csv`, `.bin` and `.mz` files from `dir1/mined` into `dir2/mined`. Files in `dir1/mined` will be erased after concatenation is done.

Minr join also performs a pre-validation on .bin and .mz file integrity (otherwise an error is generated and concatenation is aborted). 

# License

Minr is released under the GPL 2.0 license. Please check the LICENSE file for further details.
