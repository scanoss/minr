// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/hex.c
 *
 * Hexadecimal and numeric conversions
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
  * @file hex.c
  * @date 7 Feb 2021 
  * @brief Helper functions to work with Hex and Dec conversions
  */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hex.h"
/**
 * @brief Prints a hexdump of 'len' bytes from 'data' organized 'width' columns
 * 
 * @param data pointer to data buffer
 * @param len data lenght
 * @param width columns width
 */
void hexprint(uint8_t *data, uint32_t len, uint8_t width)
{
	uint8_t b16[] = "0123456789abcdef";
	for (int i = 0; i <= width * (int)((len + width) / width); i++)
		if (i && !(i % width))
		{
			printf("%04d  ", i - width);
			for (int t = i - width; t < i; t++)
				printf("%c%c", t < len ? b16[(data[t] & 0xF0) >> 4] : 32, t < len ? b16[data[t] & 0x0F] : 32);
			printf("  ");
			for (int t = i - width; t < i; t++)
				printf("%c", t < len ? ((data[t] > 31 && data[t] < 127) ? data[t] : 46) : 32);
			printf("\n");
			if (i == len)
				break;
		}
}


/**
 * @brief Returns a hexadecimal representation of the first "len" bytes in "bin"
 * 
 * @param bin pointer to data buffer
 * @param len data buffer len
 * @return string with hexadecimal representation 
 */
char *bin_to_hex(uint8_t *bin, uint32_t len)
{
	char digits[] = "0123456789abcdef";
	char *out = malloc (2 * len + 1);
	uint32_t ptr = 0;

	for (uint32_t i = 0; i < len; i++)
	{
		out[ptr++] = digits[(bin[i] & 0xF0) >> 4];
		out[ptr++] = digits[bin[i] & 0x0F];
	}

	out[ptr]=0;
	return out;
}

/**
 * @brief Joins two chars into a single int. It does c1 | c2 << 8
 * 
 * IMPORTANT: data must be an array of at least 2 bytes.
 * 
 * @param data A pointer to the first char
 * @return return a uint16_t
 */
uint16_t uint16(uint8_t *data)
{
    return 256 * data[0] + data[1];
}


/**
 * @brief  Reverse an uint32 number
 * Example: AABBCCDD -> DDCCBBAA
 * 
 * @param data pointer to the number to be inverted
 */
void uint32_reverse(uint8_t *data)
{
	uint8_t tmp = data[0];
	data[0] = data[3];
	data[3] = tmp;
	tmp = data[1];
	data[1] = data[2];
	data[2] = tmp;
}
