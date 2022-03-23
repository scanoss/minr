// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/help.c
 *
 * Minr help
 *
 * Copyright (C) 2018-2021 SCANOSS.COM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
  * @file help.c
  * @date 19 October 2021 
  * @brief Contains the help function
  */

#include "minr.h"
#include "help.h"
void show_help ()
{
	printf("\n");
	printf("SCANOSS minr v%s\n", MINR_VERSION);
	printf("\n");

	printf("Description:\n");
	printf("\n");
	printf("Minr is used to data mine a URL or file. Information is saved in a mined/ directory for\n");
	printf("importation into the SCANOSS Knowledgebase. Minr collects metadata and code snippet\n");
	printf("fingerprints for the provided URL or file. Output is saved in different data structures\n");
	printf("in ./mined/ or in the directory specified.\n");
	printf("\n");

	printf("Mining URLs: Minr performs download, extraction, indexing and archive of files for the provided URL.\n");
	printf("minr -d METADATA -u URL\n");
	printf("\n");
	printf("-d METADATA  Comma separated list of vendor,component,version,release_date,license,purl\n");
	printf("-u URL       Url to be mined (it also accepts a local folder)\n");
	printf("-U URL       Optional URL to be recorded instead of the mined one (-u)\n");
	printf("-S For SCANCODE license mining\n");
	printf("-A For extra-tables mode (mine everything). The ignored files will be mined inside the mined/extra directory\n");
	printf("\n");

	printf("Mining configuration:\n");
	printf("\n");
	printf("-a     Process all files, regardless of their extensions (default: off)\n");
	printf("-x     Exclude .mz generation (do not keep a copy of the original source code)\n");
	printf("-X     Exclude metadata detection (license, copyright, quality, etc)\n");
	printf("\n");

	printf("Mining snippet WFP: Minr extracts snippet fingerprint from all code in a .mz arhive (mined/sources/)\n");
	printf("into mined/snippets/\n");
	printf("-z DIRECTORY   Indicates the location of the mined/ directory\n");
	printf("\n");

	printf("Merging mined/ data: Mined data is organized in directories and contained in files with:\n");
	printf(".csv (components, files, licenses, vulnerabilities, quality, etc),\n");
	printf(".bin (wfp snippets)\n");
	printf(".mz  (original source code).\n");
	printf("\n");

	printf("All these formats can be aggregated by concatenation. Minr provides a mined/ joining function:\n");
	printf("\n");
	printf("-f DIR Merge source DIR\n");
	printf("-t DIR into destination DIR (and erase source DIR)\n");
	printf("\n");
	printf("Example minr -f dir1/mined -t dir2/mined\n");
	printf("\n");

	printf("Importing mined/ data into the LDB:\n");
	printf("\n");
	
	printf("-i DIR    Import the mined DIRectory into the LDB database.\n\
	  A JSON file called \"version.json\" must be present into the mined directory with the following format:\n\
	  {\"monthly\":\"YY.MM\", \"daily\":\"YY.MM.DD\"}. NOTE 1: It is mandatory  at lest one of these keys in version.json\n");
	printf("          Note 2: data in DIR is wiped after imported)\n");
	printf("-b        Import in binary mode: take a base64 encoded csv and insert in the KB as binary. libscanoss-encoder must be present in the system\n");
	printf("-D        Set the OSS DB name (default: oss)\n");
	printf("-I TABLE  Restrict importation to a specific table\n");
	printf("-O        Overwrite destination data rather than appending (MAY LEAD TO DATA LOSS)\n");
	printf("\n\n");
	printf("Local mining:\n\n");
	printf("-L TARGET  Analyse file/directory (and sub directories) to detect license license declarations \n");
	printf("-C TARGET  Analyse file/directory (and sub directories) to detect copyrigths \n");
	printf("-Q TARGET  Analyse file/directory (and sub directories) for evaluating code quality\n");
	printf("-Y TARGET  Analyse file/directory (and sub directories) to detect cryptographic algorithms usage. \n");
	printf("\n");
	printf("Misc options:\n");
	printf("\n");

	printf("-l DIR  Auto-generates license_ids.c (used by unmz for license detection) from DIR\n");
	printf("-o      Set location for the mined/ output directory (defaults to current directory)\n");
	printf("-s      Skip sorting of .csv and .bin files before importing (-i)\n");
	printf("-T      Set absolute temporary directory (defaults to /dev/shm)\n");
	printf("-n      Skip number of csv fields checking in url and file importation (-i)\n");
	printf("-k      Keep files after merging or importing\n");
	printf("-v      Display version and exit\n");
	printf("-h      Display this help and exit\n");
	printf("\n");
	printf("Copyright (C) 2018-2021 SCANOSS.COM\n");
	printf("\n");
}

