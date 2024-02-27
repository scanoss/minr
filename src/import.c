// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/import.c
 *
 * Data importation into LDB Knowledge Base
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
 * @file import.c
 * @date 25 October 2021
 * @brief Implement the functions used for import
 */

#include <sys/time.h>
#include <libgen.h>
#include <dirent.h>

#include "minr.h"
#include "import.h"
#include <ldb.h>
#include "ignored_wfp.h"
#include "bsort.h"
#include "file.h"
#include "hex.h"
#include "ignorelist.h"
#include "minr_log.h"


int (*decode) (int op, unsigned char *key, unsigned char *nonce,
		        const char *buffer_in, int buffer_in_len, unsigned char *buffer_out);

double progress_timer = 0;

/**
 * @brief Checks if two blocks of memory contain the same data, from last to first byte
 *
 * @param a
 * @param b
 * @param bytes
 * @return true
 */
bool reverse_memcmp(uint8_t *a, uint8_t *b, int bytes)
{
	for (int i = (bytes - 1); i >= 0; i--)
		if (*(a + i) != *(b + i))
			return false;
	return true;
}

bool check_file_extension(char * path, bool bin_mode)
{
	char test[MAX_PATH_LEN] = "\0";
	sprintf(test, "%s.enc", path);
	if (bin_mode || (decode && access(test, F_OK) == 0))
	{
		strcat(path, ".enc");
		return true;
	}
	return false;
}

/**
 * @brief Extract the numeric value of the two nibbles making the file name
 *
 * @param filename
 * @return uint8_t
 */
uint8_t first_byte(char *filename)
{
	long byte = strtol(basename(filename), NULL, 16);

	/* Check that 0 means 00 and not an invalid name */
	if (!byte)
		if (memcmp(basename(filename), "00", 2))
			byte = -1;

	/* Failure to retrieve the first wfp byte is fatal */
	if (byte < 0 || byte > 255)
	{
		printf("Invalid file name %s\n", filename);
		exit(EXIT_FAILURE);
	}

	return (uint8_t)byte;
}

/**
 * @brief Show progress in stdout
 *
 * @param prompt string message to show
 * @param count actual count
 * @param max maximum value
 * @param percent true for percent mode
 */
void progress(char *prompt, size_t count, size_t max, bool percent)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	double tmp = (double)(t.tv_usec) / 1000000 + (double)(t.tv_sec);
	if ((tmp - progress_timer) < 1)
		return;
	progress_timer = tmp;

	if (percent)
		printf("%s%.2f%%\r", prompt, ((double)count / (double)max) * 100);
	else
		printf("%s%lu\r", prompt, count);
	fflush(stdout);
}

/**
 * @brief Import a raw wfp file which simply contains a series of 21-byte records containing wfp(3)+md5(16)+line(2). While the wfp is 4 bytes,
 * the first byte is the file name
 *
 * @param db_name DB name
 * @param filename filename string
 * @param skip_delete true to avoid delete
 * @return true is succed
 */
bool ldb_import_snippets(char *db_name, char *filename, bool skip_delete)
{
	/* Table definition */
	struct ldb_table oss_wfp;
	strcpy(oss_wfp.db, db_name);
	strcpy(oss_wfp.table, "wfp");
	oss_wfp.key_ln = 4;
	oss_wfp.rec_ln = 18;
	oss_wfp.ts_ln = 2;
	oss_wfp.tmp = false;

	/* Progress counters */
	uint64_t totalbytes = file_size(filename);
	int reccounter = 0;
	size_t bytecounter = 0;
	int tick = 10000; // activate progress every "tick" records

	/* raw record length = wfp crc32(3) + file md5(16) + line(2) = 21 bytes */
	int raw_ln = 21;

	/* First three bytes are bytes 2nd-4th of the wfp) */
	int rec_ln = raw_ln - 3;

	/* First byte of the wfp is the file name */
	uint8_t key1 = first_byte(filename);

	/* File should contain 21 * N bytes */
	if (file_size(filename) % 21)
	{
		printf("File %s does not contain 21-byte records\n", filename);
		exit(EXIT_FAILURE);
	}

	/* Load ignored wfps into boolean array */
	bool *bl = calloc(256 * 256 * 256, 1);
	for (int i = 0; i < IGNORED_WFP_LN; i += 4)
		if (IGNORED_WFP[i] == key1)
			bl[IGNORED_WFP[i + 1] + IGNORED_WFP[i + 2] * 256 + IGNORED_WFP[i + 3] * 256 * 256] = true;

	FILE *in, *out;
	out = NULL;

	in = fopen(filename, "rb");
	if (in == NULL)
		return false;

	/* Lock DB */
	char lock_file[MAX_PATH_LEN];
	sprintf(lock_file, "%s.%s",oss_wfp.db,oss_wfp.table);
	ldb_lock(lock_file);

	uint64_t wfp_counter = 0;
	uint64_t ignore_counter = 0;

	/* We keep the last read key to group wfp records */
	uint8_t last_wfp[4] = "\0\0\0\0";
	uint8_t tmp_wfp[4] = "\0\0\0\0";
	*last_wfp = key1;
	*tmp_wfp = key1;

	/* This will store the LDB record, which cannot be larger than 65535 md5s(16)+line(2) (>1Mb) */
	uint8_t *record = malloc(256 * 256 * rec_ln);
	uint32_t record_ln = 0;

	/* The first byte of the wfp crc32(4) is the actual file name containing the records
		 We'll read 50 million wfp at a time (making a buffer of about 1Gb) */
	uint32_t buffer_ln = 50000000 * raw_ln;
	uint8_t *buffer = malloc(buffer_ln);

	/* Create table if it doesn't exist */
	if (!ldb_table_exists(db_name, "wfp"))
		ldb_create_table(db_name, "wfp", 4, rec_ln);

	/* Open ldb */
	out = ldb_open(oss_wfp, last_wfp, "r+");

	bool first_read = true;
	uint32_t bytes_read = 0;

	printf("%s\n", filename);

	while ((bytes_read = fread(buffer, 1, buffer_ln, in)) > 0)
	{
		for (int i = 0; (i + raw_ln) <= bytes_read; i += raw_ln)
		{
			uint8_t *wfp = buffer + i;
			uint8_t *rec = buffer + i + 3;

			if (bl[wfp[0] + wfp[1] * 256 + wfp[2] * 256 * 256])
			{
				ignore_counter++;
				continue;
			}

			bool new_key = (!reverse_memcmp(last_wfp + 1, wfp, 3));
			bool full_node = ((record_ln / rec_ln) >= 65535);

			/* Do we have a new key, or is the buffer full? */
			if (new_key || full_node || first_read)
			{
				first_read = false;

				/* If there is a buffer, write it */
				if (record_ln)
					ldb_node_write(oss_wfp, out, last_wfp, record, record_ln, (uint16_t)(record_ln / rec_ln));
				wfp_counter++;

				/* Initialize record */
				memcpy(record, rec, rec_ln);
				record_ln = rec_ln;

				/* Save key */
				memcpy(last_wfp + 1, wfp, 3);
			}

			/* Add file id to existing record */
			else
			{
				/* Skip duplicated records. Since md5 records to be imported are sorted, it will be faster
					 to compare them from last to first byte. Also, we only compare the 16 byte md5 */
				if (record_ln > 0)
					if (!reverse_memcmp(record + record_ln - rec_ln, rec, 16))
					{
						memcpy(record + record_ln, rec, rec_ln);
						record_ln += rec_ln;
						wfp_counter++;
					}
			}

			/* Update progress every "tick" records */
			if (++reccounter > tick)
			{
				bytecounter += (rec_ln * reccounter);
				progress("Importing: ", bytecounter, totalbytes, true);
				reccounter = 0;
			}
		}
	}
	progress("Importing: ", 100, 100, true);
	printf("%'lu wfp imported, %'lu ignored\n", wfp_counter, ignore_counter);

	if (record_ln)
		ldb_node_write(oss_wfp, out, last_wfp, record, record_ln, (uint16_t)(record_ln / rec_ln));
	if (out)
		fclose(out);

	fclose(in);
	if (!skip_delete)
		unlink(filename);

	free(record);
	free(buffer);
	free(bl);

	/* Lock DB */
	ldb_unlock(lock_file);

	return true;
}

/**
 * @brief Count number of comma delimited fields in data
 *
 * @param data pointer to data
 * @return int count
 */
int csv_fields(char *data)
{
	int commas = 0;
	while (*data)
		if (*data++ == ',')
			commas++;
	return commas + 1;
}

/**
 * @brief Returns a pointer to field n in data
 *
 * @param n filed number
 * @param data data buffer
 * @return char* to field
 */
char *field_n(int n, char *data)
{
	int commas = 0;
	while (*data)
		if (*data++ == ',')
			if (++commas == n - 1)
				return data;
	return NULL;
}

/**
 * @brief Extract binary item ID (and optional first field binary ID) from CSV line
 * where the first field is the hex itemid and the second could also be hex (if is_file_table)
 *
 * @param line pointer to line
 * @param first_byte fisrt byte to be added
 * @param got_1st_byte true if the line got the 1st byte.
 * @param itemid pointer to item id
 * @param field2 pointer to second field
 * @param is_file_table true for processing a file table
 * @return true if succed
 */
bool file_id_to_bin(char *line, uint8_t first_byte, bool got_1st_byte, uint8_t *itemid, uint8_t *field2, bool is_file_table)
{

	/* File ID is only the last 15 bytes (first byte should be in the file name) */
	if (line[30] == ',')
	{
		if (!got_1st_byte)
		{
			printf("Key is incomplete. File name does not contain the first byte\n");
			return false;
		}

		/* Concatenate first_byte and 15 remaining bytres into itemid */
		else
		{
			/* Add 1st byte */
			*itemid = first_byte;

			/* Convert remaining 15 bytes */
			ldb_hex_to_bin(line, MD5_LEN_HEX - 2, itemid + 1);

			/* Convert urlid if needed (file table) */
			if (is_file_table)
				ldb_hex_to_bin(line + (MD5_LEN_HEX - 2 + 1), MD5_LEN_HEX, field2);
		}
	}

	/* Entire file ID included */
	else
	{
		/* Convert item id */
		ldb_hex_to_bin(line, MD5_LEN_HEX, itemid);

		/* Convert url id if needed (file table) */
		if (is_file_table)
			ldb_hex_to_bin(field_n(2, line), MD5_LEN_HEX, field2);
	}

	uint8_t zero_md5[MD5_LEN] = {0xd4,0x1d,0x8c,0xd9,0x8f,0x00,0xb2,0x04,0xe9,0x80,0x09,0x98,0xec,0xf8,0x42,0x7e}; //empty string md5

	if (!memcmp(itemid,zero_md5, MD5_LEN)) //the md5 key of an empty string must be skipped.
		return false;
	
	memset(zero_md5, 0, MD5_LEN); // al zeros md5 must be skippets

	if (!memcmp(itemid,zero_md5, MD5_LEN))
		return false;


	return true;
}

/**
 * @brief Vaerify if a string is a valid hexadecimal number
 *
 * @param str input string
 * @param bytes bytes to be processed
 * @return true if it is valid
 */
bool valid_hex(char *str, int bytes)
{
	for (int i = 0; i < bytes; i++)
	{
		char h = str[i];
		if (h < '0' || (h > '9' && h < 'a') || h > 'f')
			return false;
	}
	return true;
}

/**
 * @brief Import a CSV file into the LDB database
 *
 * @param job pointer to minr job
 * @param filename file name string
 * @param table table name string
 * @param nfields number of fileds
 * @return true if succed
 */
bool ldb_import_csv(struct minr_job *job, char *filename, char *table, bool secondary_key, int nfields)
{
	bool bin_mode = false;
	bool skip_csv_check = job->skip_csv_check;
	if (job->bin_import || strstr(filename, ".enc"))
	{
		bin_mode = true;
		skip_csv_check = true;
	}

	bool skip_delete = job->skip_delete;
	int expected_fields = (skip_csv_check ? 0 : nfields);

	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t lineln;

	/* A CSV line should contain at least an MD5, a comma separator per field and a LF */
	int min_line_size = 2 * MD5_LEN + expected_fields + 1;

	/* Node size is a 16-bit int */
	int node_limit = 65536;

	uint8_t *itemid = calloc(MD5_LEN, 1);
	uint8_t *field2 = calloc(MD5_LEN, 1);
	uint8_t *item_buf = malloc(LDB_MAX_NODE_LN);
	uint8_t *item_lastid = calloc(MD5_LEN * 2 + 1, 1);
	uint16_t item_ptr = 0;
	FILE *item_sector = NULL;
	uint16_t item_rg_start = 0; // record group size
	uint16_t item_rg_size = 0;	// record group size
	char last_id[MD5_LEN * 2 + 1 -2]; //save last 30th chars from the last md5.
	memset(last_id, 0, MD5_LEN * 2 + 1);

	/* Counters */
	uint32_t imported = 0;
	uint32_t skipped = 0;

	uint64_t totalbytes = file_size(filename);
	size_t bytecounter = 0;
	int field2_ln = secondary_key ? 16 : 0;

	/* Get 1st byte of the item ID from csv filename (if available) */
	uint8_t first_byte = 0;
	bool got_1st_byte = false;
	if (valid_hex(basename(filename), 2))
		got_1st_byte = true;
	ldb_hex_to_bin(basename(filename), 2, &first_byte);

	/* Create table if it doesn't exist */
	if (!ldb_database_exists(job->dbname))
		ldb_create_database(job->dbname);
	if (!ldb_table_exists(job->dbname, table))
		ldb_create_table(job->dbname, table, 16, 0);

	/* Create table structure for bulk import (32-bit key) */
	struct ldb_table oss_bulk;
	strcpy(oss_bulk.db, job->dbname);
	strcpy(oss_bulk.table, table);
	oss_bulk.key_ln = 4;
	oss_bulk.rec_ln = 0;
	oss_bulk.ts_ln = 2;
	oss_bulk.tmp = false;

	char last_url_id[MAX_ARG_LEN] = "\0";

	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		minr_log( "File does not exist %s\n", filename);
	
		if (!skip_delete)
			unlink(filename);
		return false;
	}
	printf("%s\n", filename);

	/* Lock DB */
	char lock_file[MAX_PATH_LEN];
	sprintf(lock_file, "%s.%s",oss_bulk.db,oss_bulk.table);
	ldb_lock(lock_file);

	while ((lineln = getline(&line, &len, fp)) != -1)
	{
		
		bytecounter += lineln;
		
		/* Skip records with sizes out of range */
		if (lineln > MAX_CSV_LINE_LEN || lineln < min_line_size)
		{
			minr_log( "Line %s -- Skipped, %ld exceed MAX line size %d.\n", line, lineln, MAX_CSV_LINE_LEN);
			skipped++;
			continue;
		}

		/* Trim trailing chr(10) */
		if (line[lineln - 1] == 10)
			lineln--;
		line[lineln] = 0;

		/* Check if this ID is the same as last */
		bool dup_id = false;
		if (!memcmp(last_id, line, MD5_LEN * 2 - 2)) //compare 30 chars of the md5
			dup_id = true;
		else
			memcpy(last_id, line, MD5_LEN * 2 - 2); //copy 30 chars of the md5

		/* First CSV field is the data key. Data starts with the second CSV field */
		char *data = field_n(2, line);
		if (!data)
		{
			minr_log( "Line %s -- Skipped, Invalid line.\n", line);
			continue;
		}

		bool skip = false;

		/* File table will have the url id as the second field, which will be
			 converted to binary. Data then starts on the third field. Also file extensions
			 are checked and ruled out if ignored */
		if (secondary_key)
		{
			/* Skip line if the URL is the same as last, importing unique files per url */
			if (dup_id && *last_url_id && !memcmp(data, last_url_id, MD5_LEN * 2))
			{
				minr_log( "Line %s -- Skipped, repeated URL ID.\n", line);
				skip = true;
			}
			else
				memcpy(last_url_id, data, MD5_LEN * 2);
			
			if (nfields > 2)
			{
				data = field_n(3, line);
				if (!data)
				{
					minr_log( "Error in line %s -- Skipped\n", line);
					skipped++;
				}
			}
			else
				data = NULL;
		}

		/* Calculate record size */
		uint16_t r_size = 0;
		unsigned char data_bin[MAX_CSV_LINE_LEN];
		if (data)
		{
			if (bin_mode)
			{	
				if (decode)
					r_size = decode(DECODE_BASE64,NULL, NULL, data, strlen(data), data_bin);
				else
					ldb_error("libscanoss_encoder.so it is not available, \".enc\" files cannot be processed");
				
				if (r_size == 0)
					skip = true;
			}
			else
			{
				/* Calculate record size */
				r_size = strlen(data);
			}
			/* Check if number of fields matches the expectation */
			if (expected_fields)
				if (csv_fields(line) != expected_fields)
				{
					minr_log( "Line %s -- Skipped, Missing CSV fields. Expected: %d.\n", line, expected_fields);
					skip = true;
				}
			
			if (secondary_key && ignored_extension(data) && !bin_mode) //we dont know the file extension in bin_mode
				skip = true;

			if (skip)
			{
				skipped++;
				continue;
			}
		}

		if (data || (secondary_key && nfields < 3))
		{
			/* Convert id to binary (and 2nd field too if needed (files table)) */
			if (!file_id_to_bin(line, first_byte, got_1st_byte, itemid, field2, secondary_key))
			{
				fprintf(stderr, "failed to parse key: %s\n", line);
				continue;
			}

			/* Check if we have a whole new key (first 4 bytes), or just a new subkey (last 12 bytes) */
			bool new_key = (memcmp(itemid, item_lastid, 4) != 0);
			bool new_subkey = new_key ? true : (memcmp(itemid, item_lastid, MD5_LEN) != 0);

			/* If we have a new main key, or we exceed node size, we must flush and and initialize buffer */
			if (new_key || (item_ptr + 5 * NODE_PTR_LEN + MD5_LEN + 2 * REC_SIZE_LEN + r_size) >= node_limit)
			{
				/* Write buffer to disk and initialize buffer */
				if (item_rg_size > 0)
					uint16_write(item_buf + item_rg_start + 12, item_rg_size);
				
				if (item_ptr)
				{
					if (!item_sector)
						item_sector = ldb_open(oss_bulk, item_lastid, "r+");
					else
						ldb_node_write(oss_bulk, item_sector, item_lastid, item_buf, item_ptr, 0);
				}
				/* Open new sector if needed */
				if (*itemid != *item_lastid || (*itemid == 0 && !item_ptr))
				{
					if (item_sector)
						fclose(item_sector);
					item_sector = ldb_open(oss_bulk, itemid, "r+");
				}
				
				item_ptr = 0;
				item_rg_start = 0;
				item_rg_size = 0;
				new_subkey = true;
			}

						/* New file id, start a new record group */
			if (new_subkey)
			{
				/* Write size of previous record group */
				if (item_rg_size > 0)
					uint16_write(item_buf + item_rg_start + 12, item_rg_size);
				item_rg_start = item_ptr;

				/* K: Add remaining part of key to buffer */
				memcpy(item_buf + item_ptr, itemid + 4, MD5_LEN - LDB_KEY_LN);
				item_ptr += MD5_LEN - LDB_KEY_LN;

				/* GS: Add record group size (zeroed) */
				uint16_t zero = 0;
				uint16_write(item_buf + item_ptr, zero);
				item_ptr += REC_SIZE_LEN;

				/* Update item_lastid */
				memcpy(item_lastid, itemid, MD5_LEN);

				/* Update variables */
				item_rg_size = 0;
			}


			/* Add record length to record */
			uint16_write(item_buf + item_ptr, r_size + field2_ln);
			item_ptr += REC_SIZE_LEN;

			/* Add url id to record */
			memcpy(item_buf + item_ptr, field2, field2_ln);
			item_ptr += field2_ln;
			item_rg_size += (field2_ln + REC_SIZE_LEN);
			if (data)
			{
				/* Add record to buffer */
				if (!bin_mode)
					memcpy(item_buf + item_ptr, data, r_size);
				else
					memcpy(item_buf + item_ptr, data_bin, r_size);
				item_ptr += r_size;
				item_rg_size += r_size;
			}
			imported++;
		}
		progress("Importing: ", bytecounter, totalbytes, true);
	}
	progress("Importing: ", 100, 100, true);

	/* Flush buffer */
	if (item_rg_size > 0)
		uint16_write(item_buf + item_rg_start + MD5_LEN - LDB_KEY_LN, item_rg_size);
	
	if (item_ptr)
	{
		if (!item_sector)
			item_sector = ldb_open(oss_bulk, itemid, "r+");
		
		ldb_node_write(oss_bulk, item_sector, item_lastid, item_buf, item_ptr, 0);
	}
	
	if (item_sector)
		fclose(item_sector);

	printf("%u records imported, %u skipped\n", imported, skipped);

	fclose(fp);
	
	if (!skip_delete)
		unlink(filename);

	if (line)
		free(line);
	free(itemid);
	free(item_buf);
	free(item_lastid);
	free(field2);

	/* Lock DB */
	ldb_unlock(lock_file);

	return true;
}

/**
 * @brief Sort a csv file invokinkg a new process and executing a sort command
 *
 * @param file_path file path to be processed
 * @param skip_sort true to skip sort
 * @return true if succed
 */
bool csv_sort(char *file_path, bool skip_sort)
{
	if (skip_sort)
		return true;
	if (!file_size(file_path))
		return true;

	/* Assemble command */
	char *command = malloc(MAX_ARG_LEN + 3 * MAX_PATH_LEN);
	sprintf(command, "sort -T %s -u -o %s %s", tmp_path, file_path, file_path);

	FILE *p = popen(command, "r");
	if (p)
		pclose(p);
	else
	{
		printf("Cannot execute %s\n", command);
		free(command);
		return false;
	}
	free(command);
	return true;
}

/**
 * @brief Execute bsort over a file
 *
 * @param file_path pointer to file path
 * @param skip_sort
 * @return true
 */
bool bin_sort(char *file_path, bool skip_sort)
{
	if (!file_size(file_path))
		return false;
	if (skip_sort)
		return true;

	return bsort(file_path);
}

/**
 * @brief
 *
 * @param table table path
 * @param job pointer to minr job
 * @return true if succed
 */
bool this_table(char *table, struct minr_job *job)
{
	if (!*job->import_table)
		return true;
	if (!strcmp(job->import_table, table))
		return true;
	return false;
}

/**
 * @brief Wipes table before importing (-O)
 *
 * @param table path to table
 * @param job pointer to mner job
 */
void wipe_table(char *table, struct minr_job *job)
{
	if (!job->import_overwrite)
		return;

	bool is_mz = false;
	if (!strcmp(table, "sources") || !strcmp(table, "notices"))
		is_mz = true;

	char path[2 * MAX_PATH_LEN] = "\0";
	sprintf(path, "%s/%s/%s", LDB_ROOT, job->dbname, table);

	printf("Wiping  %s\n", path);
	if (is_dir(path))
	{
		for (int i = 0; i < (is_mz ? 65535 : 256); i++)
		{
			if (is_mz)
			{
				sprintf(path, "%s/%s/%s/%04x.mz", LDB_ROOT, job->dbname, table, i);
				check_file_extension(path, job->bin_import);
			}
			else
				sprintf(path, "%s/%s/%s/%02x.ldb", LDB_ROOT, job->dbname, table, i);
			unlink(path);
		}
	}
}

/**
 * @brief Import files
 *
 * @param job pointer to minr job
 */
void import_multiple_files(struct minr_job *job, char * table, bool secondary_key, int fields)
{
	if (!this_table(table, job))
		return;
	/* Wipe existing data if overwrite is requested */
	wipe_table(table, job);

	char path[2 * MAX_PATH_LEN] = "\0";
	sprintf(path, "%s/%s", job->import_path, table);

	if (is_dir(path))
	{
		for (int i = 0; i < 256; i++)
		{
			sprintf(path, "%s/%s/%02x.csv", job->import_path, table,i);
			check_file_extension(path, job->bin_import);

			if (csv_sort(path, job->skip_sort))
			{
				/* 3 fields expected (file id, url id, URL) */
				ldb_import_csv(job, path, table, true, fields);
			}
		}
		sprintf(path, "%s/%s", job->import_path, table);
		if (!job->skip_delete)
			rmdir(path);
	}
}

/* Import snippets */
void import_snippets(struct minr_job *job)
{
	/* Wipe existing data if overwrite is requested */
	wipe_table("wfp", job);

	char path[2 * MAX_PATH_LEN] = "\0";
	sprintf(path, "%s/%s", job->import_path, TABLE_NAME_WFP);
	if (is_dir(path))
	{
		printf("WFP IDs in ignorelist: %lu\n", IGNORED_WFP_LN / 4);
		for (int i = 0; i < 256; i++)
		{
			sprintf(path, "%s/%s/%02x.bin", job->import_path, TABLE_NAME_WFP, i);
			if (bin_sort(path, job->skip_sort))
			{
				ldb_import_snippets(job->dbname, path, job->skip_delete);
			}
		}
	}
	sprintf(path, "%s/%s", job->import_path, TABLE_NAME_WFP);
	if (!job->skip_delete)
		rmdir(path);
}


/**
 * @brief Import a single file
 *
 * @param job pointer to minr job
 * @param filename file name string
 * @param tablename table name sting
 * @param nfields number of fields
 */
void single_file_import(struct minr_job *job, char *filename, char *tablename, int nfields)
{
	if (!this_table(tablename, job))
		return;

	/* Wipe existing data if overwrite is requested */
	wipe_table(tablename, job);

	printf("Importing %s\n", filename);

	char path[2 * MAX_PATH_LEN] = "\0";
	sprintf(path, "%s/%s", job->import_path, filename);
	check_file_extension(path, job->bin_import);

	if (is_file(path))
	{
		if (csv_sort(path, job->skip_sort))
		{
			ldb_import_csv(job, path, tablename,false, nfields);
		}
	}
}

static char * version_get_daily(char * json)
{
	if (!json)
		return NULL;

	char * daily = strstr(json, "\"daily\":");

	if (daily)
	{
		daily = strchr(daily,':');
		daily = (char*) memrchr(daily,'"', 3);
		
		if(!daily)
			return NULL;

		char * daily_date = strndup(daily+1, 8);
		if (strstr(daily_date, "N/A"))
		{
			free(daily_date);
			return NULL;
		}
		return daily_date;
	}

	return NULL;

}

static char * version_get_monthly(char * json)
{
	if (!json)
		return NULL;

	char * monthly = strstr(json, "\"monthly\":");

	if (monthly)
	{
		monthly = strchr(monthly,':');
		monthly = memrchr(monthly,'"', 4);
		if (!monthly)
			return NULL;
		char * monthly_date = strndup(monthly+1, 5);
		if (strstr(monthly_date, "N/A"))
		{
			free(monthly_date);
			return NULL;
		}
		return monthly_date;
	}

	return NULL;
}

static char * version_file_open(char * path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char * file_content = malloc(len + 1);
	fread(file_content, len, 1, f);
	fclose(f);

	return file_content;
}


/**
 * @brief Import version.json
 * @param job pointer to minr job
 * @return true if succed
 */
bool version_import(struct minr_job *job)
{
	#define JSON_CONTENT  "{\"monthly\":\"%s\", \"daily\":\"%s\"}"
	char *path = NULL;
	asprintf(&path, "%s/version.json", job->import_path);

	if (!is_file(path))
	{
		fprintf(stderr, "Cannot find version file in path %s\n",job->import_path);
		return false;
	}
	char * vf_import = version_file_open(path);
	free(path);

	char * daily_date_i = version_get_daily(vf_import);
	char * monthly_date_i = version_get_monthly(vf_import);

	int test_len = strlen(vf_import);
	
	if (daily_date_i)
		test_len -= (strlen(daily_date_i) + strlen("\"daily\":"));
	if (monthly_date_i)
		test_len -= (strlen(monthly_date_i) + strlen("\"monthly\":"));
	//exit if cannot find daily or monthly or there are am excess of characteres in the json
	if ((!daily_date_i && !monthly_date_i) || test_len > 10)
	{
		fprintf(stderr, "Failed to process version file: %s\n", vf_import);
		free(vf_import);
		free(daily_date_i);
		free(monthly_date_i);
		return false;
	}

	asprintf(&path, "/var/lib/ldb/%s/version.json", job->dbname);
	char * vf_actual = version_file_open(path);		
	char * daily_date_o = version_get_daily(vf_actual);
	char * monthly_date_o = version_get_monthly(vf_actual);


	if (!daily_date_i && daily_date_o)
	{
		daily_date_i = daily_date_o;
		daily_date_o = NULL;
	}
	
	if (!monthly_date_i && monthly_date_o)
	{
		monthly_date_i = monthly_date_o;
		monthly_date_o = NULL;
	}


	FILE *f = fopen(path, "w+");
	
	if (!f)
		return false;

	fprintf(f, JSON_CONTENT, monthly_date_i == NULL ? "N/A" : monthly_date_i ,daily_date_i == NULL ? "N/A" :  daily_date_i);
	fclose(f);
	free(path);
	free(daily_date_i);
	free(daily_date_o);
	free(monthly_date_i);
	free(monthly_date_o);
	free(vf_actual);
	free(vf_import);

	return true;
}

/**
 * @brief Import CSV files and load into database
 *
 * @param job pointer to mnir job
 */
void mined_import(struct minr_job *job)
{
	/* Create database */
	if (!ldb_database_exists(job->dbname))
		ldb_create_database(job->dbname);

	/* Import version.json file */
	if (!version_import(job))
	{
		printf("Failed to import version.json file. This file must be present and must has a valid format to continue. Check at README.md for more details.\n");
		exit(EXIT_FAILURE);
	}

	char db_path[MAX_PATH_LEN * 2];
	sprintf(db_path, "%s/%s", LDB_ROOT, job->dbname);

	/* Import MZ archives */
	if (this_table("sources", job))
	{
		/* Wipe existing data if overwrite is requested */
		wipe_table("sources", job);
		minr_join_mz("sources", job->import_path, db_path, job->skip_delete, job->bin_import);
	}

	if (this_table("notices", job))
	{
		/* Wipe existing data if overwrite is requested */
		wipe_table("notices", job);
		minr_join_mz("notices", job->import_path, db_path, job->skip_delete, job->bin_import);
	}

	/* Attribution ts 2 fields: id, notice ID */
	single_file_import(job, TABLE_NAME_ATTRIBUTION".csv", "attribution", 2);

	/* PURLs expects either:
	 * 7 fields: id, created, latest, updated, star, watch, fork
	 * or a single field: related PURL. Therefore, 0 is passed as required fields */
	single_file_import(job, TABLE_NAME_PURL".csv", "purl", 0);

	/* Dependencies expect 5 fields: id, source, vendor, component, version */
	single_file_import(job, TABLE_NAME_DEPENDENCY".csv", "dependency", 5);

	/* Licenses expects 3 fields: id, source, license */
	single_file_import(job, TABLE_NAME_LICENSE".csv", "license", 3);

	/* Copyrights expects 3 fields: id, source, copyright statement */
	single_file_import(job, TABLE_NAME_COPYRIGHT".csv", "copyright", 3);

	/* Vulnerability expects 10 fields: id, source, purl, version from,
	   version patched, CVE, advisory ID (Github/CPE), Severity, Date, Summary */
	single_file_import(job, TABLE_NAME_VULNERABILITY".csv", "vulnerability", 10);

	/* Quality expects 3 CSV fields: id, source, value */
	single_file_import(job, TABLE_NAME_QUALITY".csv", "quality", 3);

	/* Cryptography expects 3 fields: id, algorithm, strength */
	single_file_import(job, TABLE_NAME_CRYPTOGRAPHY".csv", "cryptography", 3);

	/* URLs expects 8 fields: url id, vendor, component, version, release_date, license, purl, download_url */
	single_file_import(job, TABLE_NAME_URL".csv", "url", 8);

	/* Import files */
	import_multiple_files(job, TABLE_NAME_FILE, true, 3);

	/* Import pivot url/files */
	import_multiple_files(job, TABLE_NAME_PIVOT, true, 2);

	/* Import .bin files */
	if (this_table("wfp", job))
		import_snippets(job);

	/* Remove mined directory */
	if (!job->skip_delete)
		rmdir(job->import_path);
}
