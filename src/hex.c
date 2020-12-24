// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/hex.c
 *
 * Hexadecimal and numeric conversions
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

/* Prints a hexdump of 'len' bytes from 'data' organized 'width' columns */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hex.h"

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

/* Returns a hexadecimal representation of the first "len" bytes in "bin" */
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

uint16_t uint16(uint8_t *data)
{
    return 256 * data[0] + data[1];
}

/* Reverse an uint32 number  */
void uint32_reverse(uint8_t *data)
{
	uint8_t tmp = data[0];
	data[0] = data[3];
	data[3] = tmp;
	tmp = data[1];
	data[1] = data[2];
	data[2] = tmp;
}

uint32_t h2d(uint32_t h)
{
    if (h >= '0' && h <= '9')
        return h - 48;
    else if (h >= 'a' && h <= 'f')
        return h - 97 + 10;
    else if (h >= 'A' && h <= 'F')
        return h - 65 + 10;
    return 0;
}

void hex_to_bin(char *hex, uint32_t len, uint8_t *out)
{
    uint32_t ptr = 0;
    for (uint32_t i = 0; i < len; i += 2)
        out[ptr++] = 16 * h2d(hex[i]) + h2d(hex[i + 1]);
}

