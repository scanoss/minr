# minr

Minr is a multi-purpose command line tool used for data mining. Minr downloads, indexes and imports source code metadata into the knowledge base.

# Usage (downloading)

Minr takes two arguments: 

* -d: Target metadata (CSV) 
* -u: Target URL (pointing to a downloadable archive of a given component/version) 

Usage example: 

```
$ minr -d madler,pigz,2.4 -u https://github.com/madler/pigz/archive/v2.4.zip 
```

The target metadata is a comma delimited list of the following fields: 

* Vendor 
* Component 
* Version 

Note: Commas are not admitted in any of these fields.  

## File Discrimination 

The following criteria will be use to discriminate files: 

* File is not a physical file (no symlinks, etc) 
* File extension is blacklisted 
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

A mined/ directory is created with the downloaded metadata. This includes component and file metadata and source code archives which are kept using the .mz archives, specifically designed for this purpose. 

## Snippet mining

Snippet mining is a second step, where snippet information is mined from the .mz archives. This is achieved with the -z parameter.

```
$ minr -z mined
mined/sources/146a.mz
...
$
```

A mined/snippets directory is created with the snippet wfp fingerprints extracted from the .mz files located in mined/sources

## Data importation into the LDB

```
$ minr -i mined/
$
```

The LDB is now loaded with the component information and a scan can be performed.

## Scanning against the LDB Knowledge Base

The following example shows an entire component match:

```
$ scanoss 1.0.tar.gz 
{
  "1.0.tar.gz": [
    {
      "id": "component",
      "elapsed": "0.001139s",
      "lines": "all",
      "oss_lines": "all",
      "matched": "100%",
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook/archive/1.0.tar.gz",
      "file": "all",
      "size": "N/A",
      "dependencies": [],
      "licenses": []
    }
  ]
}
$
```

The following example shows an entire file match:

```
$ scanoss webhook-1.0/scanoss/github.py 
{
  "webhook-1.0/scanoss/github.py": [
    {
      "id": "file",
      "elapsed": "0.000395s",
      "lines": "all",
      "oss_lines": "all",
      "matched": "100%",
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook/archive/1.0.tar.gz",
      "file": "webhook-1.0/scanoss/github.py",
      "size": "6624",
      "dependencies": [],
      "licenses": []
    }
  ]
}
$
```

The following example adds a LF to the end of a file. The modified file does not match entirely anymore, but instead the snippet detection comes into effect: 

```
$ echo -e "\n" >> webhook-1.0/scanoss/github.py
$ scanoss webhook-1.0/scanoss/github.py 
{
  "webhook-1.0/scanoss/github.py": [
    {
      "id": "snippet",
      "elapsed": "0.001812s",
      "lines": "1-192",
      "oss_lines": "3-194",
      "matched": "99%",
      "vendor": "scanoss",
      "component": "webhook",
      "version": "1.0",
      "latest": "1.0",
      "url": "https://github.com/scanoss/webhook/archive/1.0.tar.gz",
      "file": "webhook-1.0/scanoss/github.py",
      "size": "6624",
      "dependencies": [],
      "licenses": []
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
 6c067f97266c817b339f0e989499c8e4,gnu,bison,3.5,https://ftp.gnu.org/gnu/bison/bison-3.5.tar.gzd004ad1ee8d2895994663ab5e05be4d2,gloac,gloac,0.0.2,https://rubygems.org/downloads/gloac-0.0.2.gem 
```

## Files 

OSS files are split in 256 files named 00.csv to ff.csv containing the component archive md5, the file md5, the file length and the file name, as follows: 

```
6c067f97266c817b339f0e989499c8e4,00fc6e8b3ae062fbcfbe8d2e40d36e68,47214,bison-3.5/src/ielr.c6c067f97266c817b339f0e989499c8e4,00ac96f26eed6fab33d7451e8f697939,17570,bison-3.5/lib/stat-w32.c 
```

## Snippets 

OSS snippets are 32-bit identifiers calculated with the winnowing algorithm. They are saved in binary format split in 256 files called 00.bin-ff.bin containing sequential records composed of: 

* 3 bytes for the snippet wfp identifier (the first byte is the file name) 
* 16 bytes for the file md5 where the snippet is found 
* 2 bytes for the line number where the wfp starts 

## Sorting 

Components, files and snippet files should be sorted before being imported into the LDB in order to ensure an optimized database and the fastest importation times. CSV files are sorted with the sort command, while snippet binary files are sorted with: bsort –r21 –k21 file.bin 

## Sources 

The sources/ directory is there the original downloaded source files are stored. They are kept in 65536 .mz archive files. 

# Minr join 

Minr can also be used to join two mined/ structures as follows: 

```
$ minr -f mined01/files/01.csv -t mined02
```

The command will concatenate mined01/files/01.csv to mined02/files/01.csv 

Minr can join not only CSV files (component and file metadata), but also wfp .bin files (snippet data) and .mz files (source code archives), since .csv, .bin and .mz files can be joined by simple concatenation. 

Minr join takes a directory as destination with the purpose of automatically calculating the destination file and avoiding human error (ie concatenating a 01.csv to a 02.csv). 

Minr join also performs a pre-validation on .bin and .mz file integrity (otherwise an error is generated and concatenation is aborted). 


# License

Minr is released under the GPL 2.0 license. Please check the LICENSE file for further details.
