# minr

Minr is a multi-purpose command line tool used for data mining. Minr downloads, indexes and imports source code metadata into the knowledge base. Minr can be also used locally for detecting licences and cryptographic algorithms usage.

# Setup 
Component mining requires a Knowledge database installed. Scanoss use the SCANOSS LDB (Linked-list database) as a shared library. LDB Source code and installation guide can be found on https://github.com/scanoss/ldb 
# Usage (downloading)

Minr takes two arguments: 

* `-d`: Target metadata (CSV) 
* `-u`: Target URL (pointing to a downloadable archive or a local folder)

Usage example: 

```
$ minr -d madler,pigz,2.4 -u https://github.com/madler/pigz/archive/v2.4.zip 
```

The target metadata is a comma delimited list of the following fields: 

* Vendor 
* Component 
* Version 
* Release date
* Declared license
* Package URL (PURL)

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
* **-L** Scans for licences
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
$ minr -d scanoss,webhook,1.0 -u https://github.com/scanoss/webhook/archive/1.0.tar.gz
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
        "pkg:github/unoconv/unoconv"
      ],
      "vendor": "unoconv",
      "component": "unoconv",
      "version": "0.8.2",
      "latest": "0.8.2",
      "url": "https://github.com/unoconv/unoconv",
      "release_date": "",
      "file": "0.8.2.zip",
      "url_hash": "c36074c3996ba9d7d85f4a57787b5645",
      "file_hash": "c36074c3996ba9d7d85f4a57787b5645",
      "file_url": "https://github.com/unoconv/unoconv/archive/0.8.2.zip",
      "dependencies": [],
      "licenses": [
        {
          "name": "GPL-2.0-only",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/GPL-2.0-only.txt",
          "copyleft": "yes",
          "patent_hints": "yes",
          "incompatible_with": "Apache-1.0, Apache-1.1, Apache-2.0, BSD-4-Clause, BSD-4-Clause-UC, FTL, IJG, OpenSSL, Python-2.0, zlib-acknowledgement, XFree86-1.1",
          "source": "component_declared"
        }
      ],
      "copyrights": [],
      "vulnerabilities": [
        {
          "ID": "GHSA-27p5-7cw6-m45h",
          "CVE": "CVE-2019-17400",
          "severity": "MODERATE",
          "reported": "2019-11-01",
          "introduced": "",
          "patched": "<0.9.0",
          "summary": "Mishandled untrusted pathnames could lead to SSRF and local file inclusion",
          "source": "github_advisories"
        }
      ],
      "quality": [],
      "cryptography": [],
      "server": {
        "hostname": "p8",
        "version": "4.2.5",
        "flags": "0",
        "elapsed": "0.001449s"
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
        "pkg:github/unoconv/unoconv"
      ],
      "vendor": "unoconv",
      "component": "unoconv",
      "version": "0.8.2",
      "latest": "0.8.2",
      "url": "https://github.com/unoconv/unoconv",
      "release_date": "",
      "file": "unoconv-0.8.2/unoconv",
      "url_hash": "c36074c3996ba9d7d85f4a57787b5645",
      "file_hash": "0f55e083dcc72a11334eb1a77137e2c4",
      "file_url": "https://osskb.org/api/file_contents/0f55e083dcc72a11334eb1a77137e2c4",
      "dependencies": [],
      "licenses": [
        {
          "name": "GPL-2.0-only",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/GPL-2.0-only.txt",
          "copyleft": "yes",
          "patent_hints": "yes",
          "incompatible_with": "Apache-1.0, Apache-1.1, Apache-2.0, BSD-4-Clause, BSD-4-Clause-UC, FTL, IJG, OpenSSL, Python-2.0, zlib-acknowledgement, XFree86-1.1",
          "source": "component_declared"
        }
      ],
      "copyrights": [
        {
          "name": "Copyright 2007-2010 Dag Wieers <dag@wieers.com>",
          "source": "file_header"
        }
      ],
      "vulnerabilities": [
        {
          "ID": "GHSA-27p5-7cw6-m45h",
          "CVE": "CVE-2019-17400",
          "severity": "MODERATE",
          "reported": "2019-11-01",
          "introduced": "",
          "patched": "<0.9.0",
          "summary": "Mishandled untrusted pathnames could lead to SSRF and local file inclusion",
          "source": "github_advisories"
        }
      ],
      "quality": [
        {
          "score": "2/5",
          "source": "best_practices"
        }
      ],
      "cryptography": [],
      "server": {
        "hostname": "p8",
        "version": "4.2.5",
        "flags": "0",
        "elapsed": "0.001970s"
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
      "lines": "21-1391",
      "oss_lines": "30-1400",
      "matched": "98%",
      "purl": [
        "pkg:github/unoconv/unoconv"
      ],
      "vendor": "unoconv",
      "component": "unoconv",
      "version": "0.8.2",
      "latest": "0.8.2",
      "url": "https://github.com/unoconv/unoconv",
      "release_date": "",
      "file": "unoconv-0.8.2/unoconv",
      "url_hash": "c36074c3996ba9d7d85f4a57787b5645",
      "file_hash": "0f55e083dcc72a11334eb1a77137e2c4",
      "file_url": "https://osskb.org/api/file_contents/0f55e083dcc72a11334eb1a77137e2c4",
      "dependencies": [],
      "licenses": [
        {
          "name": "GPL-2.0-only",
          "obligations": "https://www.osadl.org/fileadmin/checklists/unreflicenses/GPL-2.0-only.txt",
          "copyleft": "yes",
          "patent_hints": "yes",
          "incompatible_with": "Apache-1.0, Apache-1.1, Apache-2.0, BSD-4-Clause, BSD-4-Clause-UC, FTL, IJG, OpenSSL, Python-2.0, zlib-acknowledgement, XFree86-1.1",
          "source": "component_declared"
        }
      ],
      "copyrights": [
        {
          "name": "Copyright 2007-2010 Dag Wieers <dag@wieers.com>",
          "source": "file_header"
        }
      ],
      "vulnerabilities": [
        {
          "ID": "GHSA-27p5-7cw6-m45h",
          "CVE": "CVE-2019-17400",
          "severity": "MODERATE",
          "reported": "2019-11-01",
          "introduced": "",
          "patched": "<0.9.0",
          "summary": "Mishandled untrusted pathnames could lead to SSRF and local file inclusion",
          "source": "github_advisories"
        }
      ],
      "quality": [
        {
          "score": "2/5",
          "source": "best_practices"
        }
      ],
      "cryptography": [],
      "server": {
        "hostname": "p8",
        "version": "4.2.5",
        "flags": "0",
        "elapsed": "0.004989s"
      }
    }
  ]
}
$
```
 
# Mined/ output structure 

The minr command produces a directory called mined/ containing the mined metadata and original OSS files.  

## Components 

OSS components are saved in a single CSV file called mined/components.csv which contains the original archive md5 hash,  vendor, name, version and url as in the following example: 

```
6445e4a453f9a913e687d93c0e52161b,scanoss,minr,2.2.1,2021-08-15,GPL-2.0-only,pkg:github/scanoss/minr,https://github.com/scanoss/minr/archive/refs/tags/2.2.1.zip
ae28c57b236c8d5a50edcc645d57bc51,scanoss,engine,4.2.5,2021-08-14,GPL-2.0-only,pkg:github/scanoss/engine,https://github.com/scanoss/engine/archive/refs/tags/4.2.5.zip
```

## Files 

OSS files are split in 256 files named `00.csv` to `ff.csv` containing the component archive md5, the file md5, the file length and the file name, as follows: 

```
6c067f97266c817b339f0e989499c8e4,00fc6e8b3ae062fbcfbe8d2e40d36e68,47214,bison-3.5/src/ielr.c
6c067f97266c817b339f0e989499c8e4,00ac96f26eed6fab33d7451e8f697939,17570,bison-3.5/lib/stat-w32.c 
```

## Snippets 

OSS snippets are 32-bit identifiers calculated with the winnowing algorithm. They are saved in binary format split in 256 files called `00.bin` - `ff.bin` containing sequential records composed of:

* 3 bytes for the snippet wfp identifier (the first byte is the file name) 
* 16 bytes for the file md5 where the snippet is found 
* 2 bytes for the line number where the wfp starts 

These `.bin` files can be joined by simple concatenation.

## Sources 

The `sources/` directory is where the original downloaded source files are stored. They are kept in 65536 `.mz` archive files. These files can be listed and extracted using the `unmz` command, which is part of minr. Unlike ZIP files, MZ files can be joined by simple concatenation.

## Dependencies
The `dependencies.csv` file contains mined metadata on declared dependencies.

## PURL
The `purls.csv` file contains component-level information as well as relations between purls.
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
