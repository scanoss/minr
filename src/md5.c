// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/md5.c
 *
 * MD5 calculation
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

/* Returns the hexadecimal md5 sum for "path" */
#include <openssl/md5.h>

#include "minr.h"
#include "md5.h"

uint8_t *file_md5 (char *path)
{
	uint8_t *c = calloc(16,1);
	FILE *fp = fopen(path, "rb");
	MD5_CTX mdContext;
	uint32_t bytes;

	if (fp != NULL)
	{
		uint8_t *buffer = malloc(BUFFER_SIZE);
		MD5_Init (&mdContext);

		while ((bytes = fread(buffer, 1, BUFFER_SIZE, fp)) != 0)
			MD5_Update(&mdContext, buffer, bytes);

		MD5_Final(c, &mdContext);
		fclose(fp);
		free(buffer);
	}
	return c;
}

void calc_md5(char *data, int size, uint8_t *out)
{
	MD5_CTX mdContext;
	MD5_Init (&mdContext);
	MD5_Update(&mdContext, data, size);
	MD5_Final(out, &mdContext);
}
