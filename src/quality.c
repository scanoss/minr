// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/quality.c
 *
 * SCANOSS Quality assessment subroutines
 *
 * Copyright (C) 2018-2020 SCANOSS.COM
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

/* Make quality assessment on an individual file */
void mine_quality(char *md5, char *src, long size)
{
	/* Skip files below MIN_FILE_SIZE */
	if (size < MIN_FILE_SIZE) return;

	char *s = src;
	int bytes = 0;
	int lines = 0;
	int comments = 0;
	int longest_line = 0;
	int line_length = 0;
	int relevant_bytes = 0;
	bool line_start = true;
	bool tab_start = false;
	bool space_start = false;
	bool spdx_tag = false;

	/* Ready byte by byte */
	while (*s)
	{
		if (line_start)
		{
			/* Deal with leading spaces and tabs */
			if (*s == ' ') space_start = true;
			else if (*s == '\t') tab_start = true;
			
			/* Deal with slash star or double slash */
			else if (*s == '/')
			{
				char *next = s + 1;
				if (*next == '/' || *next == '*') comments++;
				line_start = false;
			}

			/* Deal with "#" */
			else if (*s == '#')
			{
				comments++;
				line_start = false;
			}

			else line_start = false;
		}

		/* Deal with new line */
		if (*s == '\n')
		{
			if (line_length > longest_line) longest_line = line_length;
			if (line_length > 2) relevant_bytes += line_length;
			lines++;
			line_start = true;
			line_length = 0;
		}
		else line_length++;

		/* Search for SPDX-License-Identifier */
		if (!spdx_tag) if (bytes < MAX_FILE_HEADER) spdx_tag = is_spdx_license_identifier(s);

		bytes++;
		s++;
	}

	/* Skip binaries */
	if (size == bytes && lines)
	{
		int average_line = relevant_bytes / lines;
		printf("%s,%d,%d,%d,%d,%s,%s\n", md5, lines, average_line, longest_line, comments, (tab_start && space_start ? "MIX" : "OK"), (spdx_tag ? "OK" : "NL"));
	}
	else printf("!%s,%ld\n", md5, size);
}

