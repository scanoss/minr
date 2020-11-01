// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/mz.c
 *
 * SCANOSS .mz archive handling functions
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

/* Extract a file and print contents to stdout */
void mz_cat(char *mined, char *key, uint8_t *zsrc, char *src)
{
	int mz = -1;
	char hex_md5[32 + 1] = "\0";
	hex_md5[32] = 0;

    /* Calculate mz file path */
    char mz_file[5] = "\0\0\0\0\0";
    memcpy(mz_file, key, 4);
    sprintf(mined + strlen(mined), "/sources/%s.mz", mz_file);

    /* Open mz file */
    mz = open(mined, O_RDONLY);

    /* Extract first two MD5 bytes from the key */
    memcpy(hex_md5, key, 4);

    if (mz < 0)
    {
        printf("Cannot open mz file %s\n", mined);
        exit(EXIT_FAILURE);
    }

    /* Obtain file size */
    uint64_t size = lseek64(mz, 0, SEEK_END);
    if (!size)
    {
        printf("File %s is empty\n", mined);
        exit(EXIT_FAILURE);
    }

    /* Recurse mz contents */
    char header[MZ_HEAD];
    uint64_t ptr = 0;
    while (ptr < size)
    {
        /* Read header from ptr */
        lseek64 (mz, ptr, SEEK_SET);
        if (!read(mz, header, MZ_HEAD))
        {
            printf("[READ_ERROR]\n");
            exit(EXIT_FAILURE);
        }

        /* Extract remaining bytes of the MD5 from header */
        char *tmp = bin_to_hex((uint8_t *)header, MZ_MD5);
        memcpy(hex_md5 + 4, tmp, 28);
        free(tmp);
        ptr += MZ_HEAD;

        /* Extract compressed data size from header */
        uint64_t zsrc_ln, src_ln;
        uint32_t tmpln;
        memcpy((uint8_t*)&tmpln, header + MZ_MD5, MZ_SIZE);
        zsrc_ln = tmpln;
        if (!read(mz, zsrc, zsrc_ln)) break; // ERRROR
        src_ln = MAX_FILE_SIZE;

        /* Uncompress */
        bool skip = false;
        if (strcmp(hex_md5, key)) skip = true;

		if (!skip)
		{
			if (Z_OK != uncompress((uint8_t *)src, &src_ln, zsrc, zsrc_ln))
			{
				printf("ERROR: Corrupted archive\n");
				exit(EXIT_FAILURE);
			}
			else
			{
				printf(src);
				break;
			}
		}

		/* Increment ptr */
		ptr += zsrc_ln;

		if (ptr > size)
		{
			printf("%s integrity failed\n", mined);
			exit(EXIT_FAILURE);
		}
	}
	close(mz);
}

/* Extracts, lists, checks all files from the given mz file path */
void mz_extract(char *path, bool extract, int total_licenses, bool get_copyright, uint8_t *zsrc, char *src)
{
	/* Open mz file */
	int mz = open(path, O_RDONLY);
	if (mz < 0)
	{
		printf("Cannot open mz file %s\n", path);
		exit(EXIT_FAILURE);
	}

	/* Extract first two MD5 bytes from the file name */
	char hex_md5[33];
	memcpy(hex_md5, basename(path), 4);
	hex_md5[32] = 0;

	/* Obtain file size */
	uint64_t size = lseek64(mz, 0, SEEK_END);
	if (!size)
	{
		printf("File %s is empty\n", path);
		exit(EXIT_FAILURE);
	}

	/* Recurse mz contents */
	char header[MZ_HEAD];
	uint64_t ptr = 0;
	while (ptr < size)
	{
		/* Read header from ptr */
		lseek64 (mz, ptr, SEEK_SET);
		if (!read(mz, header, MZ_HEAD))
		{
			printf("[READ_ERROR]\n");
			exit(EXIT_FAILURE);
		}

		/* Extract remaining bytes of the MD5 from header */
		char *tmp = bin_to_hex((uint8_t *)header, MZ_MD5);
		memcpy(hex_md5 + 4, tmp, 28);
		free(tmp);
		ptr += MZ_HEAD;

		/* Extract compressed data size from header */
		uint64_t zsrc_ln, src_ln;
		uint32_t tmpln;
		memcpy((uint8_t*)&tmpln, header + MZ_MD5, MZ_SIZE);
		zsrc_ln = tmpln;
		if (!read(mz, zsrc, zsrc_ln)) break; // ERRROR
		src_ln = MAX_FILE_SIZE;

		/* Uncompress */
		if (!total_licenses && !get_copyright) printf("%s ", hex_md5);
		if (Z_OK != uncompress((uint8_t *)src, &src_ln, zsrc, zsrc_ln))
		{
			printf("[CORRUPTED]\n");
			exit(EXIT_FAILURE);
		}

		/* Write decompressed data to output file */
		uint8_t *actual_md5;
		char *actual;
		if (extract)
		{
			FILE *out = fopen(hex_md5, "w");
			if (!out)
			{
				printf("\nCannot open %s for writing\n", hex_md5);
				exit(EXIT_FAILURE);
			}
			fwrite(src, 1, src_ln-1, out);
			fclose(out);

			/* Calculate resulting file MD5 */
			actual_md5 = file_md5(hex_md5);
		}
		else
		{
			/* Calculate resulting data MD5 */
			actual_md5 = calloc(16, 1);
			calc_md5(src, src_ln - 1, actual_md5);
		}

		/* Compare data checksum to validate */
		actual = bin_to_hex(actual_md5, 16);
		if (strcmp(hex_md5, (char *)actual))
		{
			printf("[FAILED] %lu bytes\n", src_ln - 1);
			exit(EXIT_FAILURE);
		}
		else
		{
			if (total_licenses) mine_license(hex_md5, src, src_ln, total_licenses);
			else if (get_copyright) mine_copyright(hex_md5, src, src_ln);
			else printf("[OK] %lu bytes\n", src_ln - 1);
		}

		free(actual_md5);
		free(actual);

		/* Increment ptr */
		ptr += zsrc_ln;

		if (ptr > size)
		{
			printf("%s integrity failed\n", path);
			exit(EXIT_FAILURE);
		}
	}
	close(mz);
}

bool mz_exists_in_cache(uint8_t *md5, struct mz_cache_item *mz_cache)
{
	int mzid = uint16(md5);

	/* False if cache is empty */
	if (!mz_cache[mzid].length) return false;

	/* Search md5 in cache */
	uint8_t *cache = mz_cache[mzid].data;
	int cacheln = mz_cache[mzid].length;

	while (cache < (mz_cache[mzid].data + cacheln))
	{
		/* MD5 comparison starts on the third byte of the MD5 */
		if (!memcmp(md5 + 2, cache, MZ_MD5)) return true;

		/* Extract zsrc and displace cache pointer */
		uint32_t zsrc_ln;
		memcpy((uint8_t*)&zsrc_ln, cache + MZ_MD5, MZ_SIZE);
		cache += (MZ_HEAD + zsrc_ln);
	}

	return false;
}

bool mz_exists_in_disk(uint8_t *md5)
{
	char path[MAX_PATH_LEN];

	/* Assemble MZ path */
	uint16_t mzid = uint16(md5);
	sprintf(path, "%s/sources/%04x.mz", mined_path, mzid);

	/* Open mz file */
	int mz = open(path, O_RDONLY);
	if (mz < 0) return false;

	uint64_t ptr = 0;
	uint8_t header[MZ_HEAD];

	/* Get file size */
	uint64_t size = lseek64(mz, 0, SEEK_END);
	if (!size) return false;

	/* Recurse file */
	while (ptr < size)
	{
		/* Read MZ header */
		lseek64(mz, ptr, SEEK_SET);
		if (!read(mz, header, MZ_HEAD))
		{
			printf("[READ_ERROR]\n");
			exit(EXIT_FAILURE);
		}
		ptr += MZ_HEAD;

		/* If md5 matches, exit with true */
		if (!memcmp(md5 + 2, header, MZ_MD5))
		{
			close(mz);
			return true;
		}

		/* Increment pointer */
		uint32_t zsrc_ln;
		memcpy((uint8_t*)&zsrc_ln, header + MZ_MD5, MZ_SIZE);
		ptr += zsrc_ln;
	}
	close(mz);

	return false;
}


bool mz_exists(uint8_t *md5, struct mz_cache_item *mz_cache)
{
	if (mz_exists_in_cache(md5, mz_cache)) return true;

	return mz_exists_in_disk(md5);
}

void mz_write(int mzid, uint8_t *data, int datalen)
{
	char path[MAX_PATH_LEN];

	/* Create sources directory */
	sprintf(path, "%s/sources", mined_path);
	if (!create_dir(path))
	{
		printf("Cannot create file %s\n", path);
	}

	else
	{
		sprintf(path, "%s/sources/%04x.mz", mined_path, mzid);
		FILE *f = fopen(path, "a");
		if (f)
		{
			size_t written = fwrite(data, datalen, 1, f);
			if (!written)
			{
				printf("Error writing %s\n", path);
				exit(EXIT_FAILURE);
			}
			fclose(f);
		}
		else 
		{
			printf("Error opening %s\n", path);
			exit(EXIT_FAILURE);
		}
	}
}

/* Adds a file to the mz archive. File structure is a series of:
   MD5(14) + COMPRESSED_SRC_SIZE(4) + COMPRESSED_SRC(N)
   The first two bytes of the md5 are in the actual XXXX.mz filename
   */
void mz_add(uint8_t *md5, char *src, int src_ln, bool check, uint8_t *zsrc, struct mz_cache_item *mz_cache)
{
	if (check) if (mz_exists(md5, mz_cache)) return;

	uint64_t zsrc_ln = compressBound(src_ln + 1);

	/* We save the first bytes of zsrc to accomodate the MZ header */
	compress(zsrc + MZ_HEAD, &zsrc_ln, (uint8_t *) src, src_ln + 1);
	uint32_t zln = zsrc_ln;

	/* Only the last 14 bytes of the MD5 go to the mz record (first two bytes are the file name) */
	memcpy(zsrc, md5 + 2, MZ_MD5); 

	/* Add the 32-bit compressed file length */
	memcpy(zsrc + MZ_MD5, (char *) &zln, MZ_SIZE);

	int mzid = uint16(md5);
	int mzlen = zsrc_ln + MZ_HEAD;

	/* If it won't fit in the cache, write it directly */
	if (mzlen > MZ_CACHE_SIZE)
	{
		mz_write(mzid, zsrc, mzlen);
	}
	else
	{
		/* Flush cache and add to cache */
		if (mz_cache[mzid].length + mzlen > MZ_CACHE_SIZE)
		{
			mz_write(mzid, zsrc, mzlen);
			mz_cache[mzid].length = mzlen;
			memcpy(mz_cache[mzid].data, zsrc, mzlen);
		}

		/* Just add to cache */
		else
		{
			memcpy(mz_cache[mzid].data + mz_cache[mzid].length, zsrc, mzlen);
			mz_cache[mzid].length += mzlen;
		}
	}
}

/* Write all cached mz records */
void mz_flush(struct mz_cache_item *mz_cache)
{
	for (int i = 0; i < MZ_FILES; i++)
	{
		if (mz_cache[i].length)
		{
			mz_write(i, mz_cache[i].data, mz_cache[i].length);
			mz_cache[i].length = 0;
		}
	}
}

/* Checks mz container for integrity */
bool mz_check(char *path)
{

	/* Open mz file */
	int mzfile = open(path, O_RDONLY);
	if (mzfile < 0) return false;

	/* Obtain file size */
	uint64_t size = lseek64(mzfile, 0, SEEK_END);
	if (!size) return false;

	/* Read entire .mz file to memory */
	uint8_t *mz = malloc(size);	
	lseek64 (mzfile, 0, SEEK_SET);
	if (!read(mzfile, mz, size)) 
	{
		free(mz);
		return false;
	}
	close(mzfile);

	/* Recurse mz contents */
	uint64_t ptr = 0;
	uint32_t ln = 0;

	while (ptr < size)
	{
		/* Read compressed data length */
		memcpy((uint8_t*)&ln, mz + ptr + MZ_MD5, MZ_SIZE);

		/* Increment ptr */
		ptr += (MZ_HEAD + ln);
	}
	close(mzfile);
	free(mz);

	if (ptr > size) return false;

	return true;
}

