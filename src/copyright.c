// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/copyright.c
 *
 * SCANOSS Copyright detection subroutines
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
  * @file copyright.c
  * @date 9 June 2021 
  * @brief ???
  */

#include <libgen.h>

#include "minr.h"
#include "copyright.h"

bool is_file(char *path);
bool is_dir(char *path);
bool not_a_dot(char *path);
bool strn_icmp(char *a, char *b, int len);
int count_nonalnum(char *str);
int count_alnum(char *str);
int linelen(char *str);

/**
 * @brief Determines whether a string begins with the characters of a specified string
 * 
 * @param str Long string
 * @param start Short string 
 * @return true if the given characters are found at the beginning of the string; otherwise, false.
 */
bool string_starts_with(char *str, char *start)
{
	int len = strlen(start);
	if (strn_icmp(str, start, len)) return true;
	return false;
}

/**
 * @brief Returns true if word copyright is followed by non wanted words
 * 
 * @param str 
 * @return true 
 * @return false 
 */
bool ignore_copyright(char *str)
{
	if (string_starts_with(str, "copyright notice")) return true;
	if (string_starts_with(str, "copyright ownership")) return true;
	if (string_starts_with(str, "copyright laws")) return true;
	if (string_starts_with(str, "copyright law.")) return true;
	if (string_starts_with(str, "copyright_")) return true;
	if (string_starts_with(str, "copyright and ")) return true;
	return false;
}

/**
 * @brief Search in src an '@author' doxygen tag
 * 
 * @param src String to search the tag
 * @return true exists the tag, false otherwise.
 */
bool is_author(char *src)
{
	int tag_len = 0;

	if (strn_icmp(src,"@author", 7))
	{
		tag_len = 7; // length of "@author"
	}

	if (!tag_len) return false;

	if (linelen(src) > MAX_COPYRIGHT_LEN) return false;

	if (count_alnum(src + tag_len) < MIN_ALNUM_IN_COPYRIGHT) return false;
	if (count_nonalnum(src + tag_len) > MAX_NONALNUM_IN_COPYRIGHT) return false;

	return true;
}


/**
 * @brief Verify if the string src contains a copyright declaration
 * 
 * @param src String to search the copyright
 * @return true If the string contains a copyright declaration, false otherwise.
 */
bool is_copyright(char *src)
{
	int tag_len = 0;

	if (strn_icmp(src,"(C)", 3))
	{
		tag_len = 3; // length of "(C)"
	}

	if (!tag_len) if (strn_icmp(src,"Copyright", 9))
	{
		tag_len = 9; // length of "Copyright"
		if (ignore_copyright(src)) return false;
	}

	if (!tag_len) return false;

	if (linelen(src) > MAX_COPYRIGHT_LEN) return false;

	if (count_alnum(src + tag_len) < MIN_ALNUM_IN_COPYRIGHT) return false;
	if (count_nonalnum(src + tag_len) > MAX_NONALNUM_IN_COPYRIGHT) return false;

	return true;
}

/**
* @brief Calculate the length of the copyright until end of line or end of txt or MAX_COPYRIGHT_LEN
* 
* @param txt 
* @return int 
*/
int copyright_len(char *txt)
{
	for (int i = 0; i < MAX_COPYRIGHT_LEN; i++)
	{
		if (txt[i] == 0 || txt[i] == '\n') return i + 1;
	}
	return MAX_COPYRIGHT_LEN;
}

/**
 * @brief Allocates memory, copies copyright declaration and returns pointer (or NULL)
 * 
 * @param txt 
 * @return char*  
 */
char *extract_copyright(char *txt)
{
	int len = copyright_len(txt);
	if (!len) return NULL;

	char *copyright = malloc(len + 1);
	memcpy(copyright, txt, len);
	copyright[len] = 0;
	if (copyright[len - 1] == '\n') copyright[len - 1] = 0;

	char_replace(copyright, ',', ';');

	return copyright;
}

/**
 * @brief Extract a copyright statement from src
 * Copyright sources
 * 0 = Declared in component
 * 1 = Detected in file header
 * 2 = Declared in LICENSE file
 * 
 * @param mined_path 
 * @param md5 
 * @param src 
 * @param src_ln 
 * @param license_file 
 */
void mine_copyright(char *mined_path, char *md5, char *src, uint64_t src_ln, bool license_file)
{
	/* Max bytes/lines to analyze */
	int max_bytes = MAX_FILE_HEADER;
	if (src_ln < max_bytes) max_bytes = src_ln;
	int max_lines = 20;
	int line = 0;

	/* Assemble csv path */
	char csv_path[MAX_PATH_LEN] = "\0";
	if (mined_path)
	{
		strcpy(csv_path, mined_path);
		strcat(csv_path, "/copyrights.csv");
	}

	char *s = src;
	while (*s)
	{
		bool extract = false;

		/* If we find an opening bracket, we assume the header already ended */
		if (*s == '{') break;
		if (*s == '\n') line++;

		else
		{
			if (is_copyright(s)) extract = true;
			if (!extract) if (is_author(s))
			{
				extract = true;
				s += 8; // Skip "@author "
			}
		}

		if (extract)
		{
			char *copyright = extract_copyright(s);
			if (copyright)
			{
				if (*csv_path)
				{
					FILE *fp = fopen(csv_path, "a");
					if (license_file)
						fprintf(fp, "%s,2,%s\n", md5, copyright);
					else
						fprintf(fp, "%s,1,%s\n", md5, copyright);
					fclose(fp);
				}
				else printf("%s,%s\n", md5, copyright);

				free(copyright);
				return;
			}
		}

		if (((s++)-src) > max_bytes || line > max_lines) return;
	}
	return;
}
