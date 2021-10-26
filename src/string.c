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


/* Perform a fast case-insensitive string comparison */
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "minr.h"

/* Replace character */
void char_replace(char *string, char replace, char with)
{
	char *s = string;
	while (*s)
	{
		if (*s == replace) *s = with;
		s++;
	}
}

/* Case insensitive string comparison */
bool strn_icmp(char *a, char *b, int len)
{
	for (int i = 0; i < len; i++) if (tolower(a[i]) != tolower(b[i])) return false;
	return true;
}

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

/* Count number of non alphanumeric (nor spaces) characters in string until EOL */
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

/* Count number of alphanumeric characters in string until EOL */
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

/* Count number of chr occurrences in str */
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

/* Count number of chars in str until EOL */
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
