// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/string.c
 *
 * String handling functions
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
  * @file string.c
  * @date 26 Oct 2021 
  * @brief Implements useful string functions
  */


#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "minr.h"

/**
 * @brief Replace character
 * 
 * @param string input string
 * @param replace char to be replaced
 * @param with replacement char
 */
void char_replace(char *string, char replace, char with)
{
	char *s = string;
	while (*s)
	{
		if (*s == replace) *s = with;
		s++;
	}
}

/**
 * @brief Case insensitive string comparison
 * 
 * @param a string a
 * @param b  string b
 * @param len string len
 * @return true if they are the equals
 */
bool strn_icmp(char *a, char *b, int len)
{
	for (int i = 0; i < len; i++) if (tolower(a[i]) != tolower(b[i])) return false;
	return true;
}

/**
 * @brief Normalize string removed unwanted characters
 * 
 * @param src input string
 * @param src_ln string size
 * @param out out string
 * @param max_in max input characters
 * @param max_out max output characters
 */
void normalize_src(char *src, uint64_t src_ln, char *out, int max_in, int max_out)
{
	int out_ptr = 0;

	for (int i = 0; i < max_in; i++)
	{
		if (!src[i]) break;
		if (isalnum(src[i])) out[out_ptr++] = tolower(src[i]);
		else if (src[i] == '+') out[out_ptr++] = src[i];
		if (out_ptr >= max_out) break;
	}
	out[out_ptr] = 0;
}

/**
 * @brief Count number of non alphanumeric (nor spaces) characters in string until EOL
 * 
 * @param str input string
 * @return int number of non alphanumeric characters
 */
int count_nonalnum(char *str)
{
	char *s = str;
	int count = 0;
	while (*s && *s != '\n')
	{
		if (!isalnum(*s) && *s != ' ') count++;
		s++;
	}
	return count;
}

/**
 * @brief Count number of alphanumeric characters in string until EOL
 * 
 * @param str input string
 * @return int numbre of alphanumeric
 */
int count_alnum(char *str)
{
	char *s = str;
	int count = 0;
	while (*s && *s != '\n')
	{
		if (isalnum(*s)) count++;
		s++;
	}
	return count;
}

/**
 * @brief Count number of chr occurrences in str
 * 
 * @param chr wanted char
 * @param str input string
 * @return int number of ocurrences
 */
int count_chr(char chr, char *str)
{
	char *s = str;
	int count = 0;
	while (*s)
	{
		if (*s == chr) count++;
		s++;
	}
	return count;
}

/**
 * @brief Count number of chars in str until EOL
 * 
 * @param str input string
 * @return int number of chars
 */
int linelen(char *str)
{
	char *s = str;
	int count = 0;
	while (*s && *s != '\n')
	{
		count++;
		s++;
	}
	return count;
}
