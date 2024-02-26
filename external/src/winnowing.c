// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/main.c
 *
 * Winnowing algorithm implementation
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


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crc32c.h"
#include "winnowing.h"

uint8_t GRAM  = 30;   // Winnowing gram size in bytes
uint8_t WINDOW = 64;  // Winnowing window size in bytes
uint32_t MAX_UINT32 = 4294967295;

/* Convert case to lowercase, and return zero if it isn't a letter or number
   Do it fast and independent from the locale configuration (avoid string.h) */
static uint8_t normalize (uint8_t byte)
{
	if (byte < '0')  return 0;
	if (byte > 'z')  return 0;
	if (byte <= '9')  return byte;
	if (byte >= 'a') return byte;
	if ((byte >= 'A') && (byte <= 'Z')) return byte + 32 ;
	return 0;
}

/* Select smaller hash for the given window */
static uint32_t smaller_hash(uint32_t *window)
{
	uint32_t hash = MAX_UINT32;
	for (uint32_t h = 0; h < WINDOW; h++)
	{
		if (window[h] < hash) hash = window[h];
	}
	return hash;
}

/* Add the given "hash" to the "hashes" array and the corresponding "line" to the "lines" array
   updating the hash counter and returning the last added hash */
static uint32_t add_hash(uint32_t hash, uint32_t line, uint32_t *hashes, uint32_t *lines, uint32_t last, uint32_t *counter)
{

	/* Consecutive repeating hashes are ignored */
	if (hash != last)
	{
		/* 	Hashing the hash will result in a better balanced resulting data set
			as it will counter the winnowing effect which selects the "minimum"
			hash in each window */

		hashes [*counter] = calc_crc32c((char *)&hash, 4);
		lines  [*counter] = line;

		last = hash;
		(*counter)++;
	}

	return last;
}

/* Performs winning on the given FILE, limited to "limit" hashes. The provided array
   "hashes" is filled with hashes and "lines" is filled with the respective line numbers.
   The function returns the number of hashes found */

uint32_t winnowing(char *src, uint32_t *hashes, uint32_t *lines, uint32_t limit)
{
	uint32_t hash = MAX_UINT32;
	uint32_t last = 0;

	uint8_t *grams =  calloc(limit, 1);
	uint32_t *windows = calloc (limit * 4,1);

	if (!grams || !windows)
	{
		free(grams);
		free(windows);
		return 0;
	}
	
	uint8_t *gram = grams;
	uint32_t *window = windows;
	
	uint32_t gram_ptr = 0;
	uint32_t window_ptr = 0;

	/* Process one byte at a time */
	uint32_t line = 1;
	uint32_t counter = 0;
	uint32_t line_char = 0;
	while (*src)
	{
		if (*src == '\n') 
		{
			line++;
			line_char = 0;
		}
		else
		{
			line_char++;
		}

		if (line > 65384 || line_char > 16384)
		{
			break;
		}

		uint8_t byte = normalize(*(src++));
		if (!byte) continue;

		/* Add byte to the gram */
		gram[gram_ptr++] = byte;

		/* Got a full gram? */
		if (gram_ptr >= GRAM)
		{
			/* Add fingerprint to the window */
			window[window_ptr++] = calc_crc32c((char *) gram, GRAM);

			/* Got a full window? */
			if (window_ptr >= WINDOW)
			{
				/* Add hash */
				hash = smaller_hash(window);
				last = add_hash(hash, line, hashes, lines, last, &counter);

				if (counter >= limit) 
					break;

				window++;
				if (window - windows >= limit * 4)
					break;
				
				window_ptr = WINDOW - 1;
			}
			gram++;
			if (gram - grams >= limit)
				break;
			gram_ptr = GRAM - 1;
		}
	}

	free (windows);
	free (grams);
	return counter;
}
