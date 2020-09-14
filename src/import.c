// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/import.c
 *
 * Data importation into LDB Knowledge Base
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

#include "external/ldb/ldb.c"
double progress_timer = 0;

/* Checks if two blocks of memory contain the same data, from last to first byte */
bool reverse_memcmp(uint8_t *a, uint8_t *b, int bytes) 
{
	for (int i = (bytes - 1); i >= 0; i--)
		if (*(a + i) != *(b + i)) return false;
	return true;
}

/* Extract the numeric value of the two nibbles making the file name */
uint8_t first_byte(char *filename)
{
	long byte = strtol(basename(filename), NULL, 16);

	/* Check that 0 means 00 and not an invalid name */
	if (!byte) if (memcmp(basename(filename), "00", 2)) byte = -1;

	/* Failure to retrieve the first wfp byte is fatal */	
	if (byte < 0 || byte > 255)
	{
		printf("Invalid file name %s\n", filename);
		exit(EXIT_FAILURE);
	}

	return (uint8_t) byte;
}

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

/* Import a raw wfp file which simply contains a series of 21-byte records
   containing wfp(3)+md5(16)+line(2). While the wfp is 4 bytes, the first 
   byte is the file name 
*/
bool ldb_import_snippets(char *filename)
{

	/* Table definition */
	struct ldb_table oss_wfp;
	strcpy(oss_wfp.db, "oss");
	strcpy(oss_wfp.table, "wfp");
	oss_wfp.key_ln = 4;
	oss_wfp.rec_ln = 18;
	oss_wfp.ts_ln = 2;
	oss_wfp.tmp = false;

	/* raw record length = wfp crc32(3) + file md5(16) + line(2) = 21 bytes */
	int raw_ln = 21;

	/* First three bytes are bytes 2nd-4th of the wfp) */
	int rec_ln = raw_ln - 3;

	/* First byte of the wfp is the file name */
	uint8_t key1 = first_byte(filename);

	/* Load blacklisted wfps into boolean array */
    long size = sizeof(BLACKLISTED_WFP);
	bool *bl = calloc(256*256*256,1);
	for (int i = 0; i < size; i += 4)
		if (BLACKLISTED_WFP[i] == key1)
			bl[BLACKLISTED_WFP[i+1]+BLACKLISTED_WFP[i+2]*256+BLACKLISTED_WFP[i+3]*256*256] = true;

	FILE *in, *out;
	out = NULL;

	in = fopen(filename, "rb");
	if (in == NULL) return false;

	/* Lock DB */
	ldb_lock();

	uint64_t wfp_counter = 0;
	uint64_t bl_counter = 0;
	
	/* We keep the last read key to group wfp records */
	uint8_t last_wfp[4] = "\0\0\0\0";
	uint8_t tmp_wfp[4] = "\0\0\0\0";
	*last_wfp = key1;
	*tmp_wfp = key1;

	/* This will store the LDB record, which cannot be larger than 65535 md5s(16)+line(2) (>1Mb) */
	uint8_t *record = malloc(256 * 256 * rec_ln);
	uint32_t record_ln = 0;

	/* 
		The first byte of the wfp crc32(4) is the actual file name containing the records
		We'll read 50 million wfp at a time (making a buffer of about 1Gb) 
	*/
	uint32_t buffer_ln = 50000000 * raw_ln;
	uint8_t *buffer = malloc(buffer_ln);

	/* Create table if it doesn't exist */
	if (!ldb_database_exists("oss"))           ldb_create_database("oss");
	if (!ldb_table_exists("oss", "wfp"))       ldb_create_table("oss", "wfp", 4, rec_ln);

	/* Open ldb */
	out = ldb_open(oss_wfp, last_wfp, "r+");

	bool first_read = true;
	uint32_t bytes_read = 0;

	while ((bytes_read = fread(buffer, 1, buffer_ln, in)) > 0)
	{
		for (int i = 0; (i + raw_ln) <= bytes_read; i += raw_ln)
		{
			uint8_t *wfp = buffer + i;
			uint8_t *rec = buffer + i + 3;

			if (bl[wfp[0]+wfp[1]*256+wfp[2]*256*256])
			{
				bl_counter++;
				continue;
			}

			bool new_key = (!reverse_memcmp(last_wfp + 1, wfp, 3));
			bool full_node = ((record_ln / rec_ln) >= 65535);

			/* Do we have a new key, or is the buffer full? */
			if (new_key || full_node || first_read)
			{
				first_read = false;

				/* If there is a buffer, write it */
				if (record_ln) ldb_node_write(oss_wfp, out, last_wfp, record, record_ln, (uint16_t) (record_ln/rec_ln));
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
				if (record_ln > 0) if (!reverse_memcmp(record + record_ln - rec_ln, rec, 16))
				{
					memcpy(record + record_ln, rec, rec_ln);
					record_ln += rec_ln;
					wfp_counter++;
				}
			}
			if (wfp_counter % 100000 == 0) printf ("%lu wfp imported, %lu blacklisted\n", wfp_counter, bl_counter);
		}
	}

	if (record_ln) ldb_node_write(oss_wfp, out, last_wfp, record, record_ln, (uint16_t) (record_ln/rec_ln));
	if (out) fclose (out);

	fclose(in);
	free(record);

	/* Lock DB */
	ldb_unlock ();

	return true;

}

/* Count number of comma delimited fields in data */
int csv_fields(char *data)
{
    int commas = 0;
    while (*data) if (*data++ == ',') commas++;
    return commas + 1;
}

/* Returns a pointer to field n in data */
char *field_n(int n, char *data)
{
    int commas = 0;
    while (*data) if (*data++ == ',') if (++commas == n-1) return data;
    return NULL;
}

bool ldb_import_generic(char *filename, char *table)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t lineln;

	uint8_t *componentid  = calloc (16,1);
	uint32_t component_count = 0;
	uint32_t component_skipped = 0;
	uint8_t  *component_buf = malloc (ldb_max_nodeln);
	uint16_t  component_ptr = 0;
	uint8_t  *component_lastid = calloc (33, 1);
	long      component_lastsector = -1;
	FILE     *component_sector = NULL;
	uint16_t  component_rg_start   = 0; // record group size
	uint16_t  component_rg_size   = 0; // record group size

	uint64_t totalbytes = file_size(filename);
	size_t bytecounter = 0;

	/* Create table if it doesn't exist */
	if (!ldb_database_exists("oss"))           ldb_create_database("oss");
	if (!ldb_table_exists("oss", table)) ldb_create_table("oss", table, 16, 0);

	struct ldb_table oss_bulk;
	strcpy(oss_bulk.db, "oss");
	strcpy(oss_bulk.table, table);
	oss_bulk.key_ln = 4;
	oss_bulk.rec_ln = 0;
	oss_bulk.ts_ln = 2;
	oss_bulk.tmp = false;

	fp = fopen (filename, "r");
	if (fp == NULL) return false;

	/* Lock DB */
	ldb_lock ();

	while ((lineln = getline (&line, &len, fp)) != -1)
	{

		if (lineln > 4096) continue;

		/* Trim trailing chr(10) */
		if (line[lineln - 1] == 10)
		{
			lineln--;
			line[lineln] = 0;
		}

		char *data = field_n(2, line);

		if (*data)
		{

			/* Calculate record size */
			uint16_t r_size = strlen (data);

			/* Convert file md5 to binary */
			hex_to_bin (line, 32, componentid);

			/* Check if we have a whole new key (first 4 bytes), or just a new subkey (last 12 bytes) */
			bool new_key = (memcmp (componentid, component_lastid, 4) != 0);
			bool new_subkey = new_key ? true : (memcmp (componentid, component_lastid, 16) != 0);

			/* If we have a new main key, or we exceed node size, we must flush and and initialize buffer */
			if (new_key || (component_ptr + 5 + 5 + 12 + 2 + 2 + r_size + 1) >= ldb_max_nodeln)
			{
				/* Write buffer to disk and initialize buffer */
				if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
				if (component_ptr) ldb_node_write(oss_bulk, component_sector, component_lastid, component_buf, component_ptr, 0);
				component_ptr = 0;
				component_rg_start  = 0;
				component_rg_size   = 0;

				/* Open new sector if needed */
				if (*componentid != component_lastsector)
				{
					if (component_sector) fclose (component_sector);
					component_sector = ldb_open(oss_bulk, componentid, "r+");
				}
			}

			/* New file id, start a new record group */
			if (new_subkey)
			{
				/* Write size of previous record group */
				if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
				component_rg_start  = component_ptr;

				/* K: Add remaining part of key to buffer */
				memcpy (component_buf + component_ptr, componentid + 4, 12);
				component_ptr += 12;

				/* GS: Add record group size (zeroed) */
				uint16_t zero = 0;
				uint16_write(component_buf + component_ptr, zero);
				component_ptr += 2;

				/* Update component_lastid */
				memcpy (component_lastid, componentid, 16);

				/* Update variables */
				component_rg_size   = 0;
			}

			/* Add record length to record */
			uint16_write(component_buf + component_ptr, r_size);
			component_ptr += 2;

			/* Add record to buffer */
			memcpy (component_buf + component_ptr, data, r_size);
			component_ptr += r_size;
			component_rg_size += (2 + r_size);
			component_count++;
		}
		bytecounter += lineln;

		progress ("Importing: ", bytecounter, totalbytes, true);
	}

	/* Flush buffer */
	if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
	if (component_ptr) ldb_node_write(oss_bulk, component_sector, component_lastid, component_buf, component_ptr, 0);
	if (component_sector) fclose (component_sector);

	printf ("Skipped: %u/%u component\n", component_skipped, component_count);
	fclose (fp);

	if (line) free (line);
	free (componentid);
	free (component_buf);
	free (component_lastid);

	/* Lock DB */
	ldb_unlock ();

	return true;

}

bool ldb_import_components(char *filename)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t lineln;

	/* Table definition */
	struct ldb_table oss_component_bulk;
	strcpy(oss_component_bulk.db, "oss");
	strcpy(oss_component_bulk.table, "component");
	oss_component_bulk.key_ln = 4;
	oss_component_bulk.rec_ln = 0;
	oss_component_bulk.ts_ln = 2;
	oss_component_bulk.tmp = false;

	uint8_t *componentid  = calloc (16,1);
	uint32_t component_count = 0;
	uint32_t component_skipped = 0;
	uint8_t  *component_buf = malloc (ldb_max_nodeln);
	uint16_t  component_ptr = 0;
	uint8_t  *component_lastid = calloc (33, 1);
	long      component_lastsector = -1;
	FILE     *component_sector = NULL;
	uint16_t  component_rg_start   = 0; // record group size
	uint16_t  component_rg_size   = 0; // record group size

	uint64_t totalbytes = file_size(filename);
	size_t bytecounter = 0;

	/* Create table if it doesn't exist */
	if (!ldb_database_exists("oss"))           ldb_create_database("oss");
	if (!ldb_table_exists("oss", "component")) ldb_create_table("oss", "component", 16, 0);

	fp = fopen (filename, "r");
	if (fp == NULL) return false;

	/* Lock DB */
	ldb_lock ();

	while ((lineln = getline (&line, &len, fp)) != -1)
	{

		if (lineln > 4096) continue;

		/* Trim trailing chr(10) */
		if (line[lineln - 1] == 10)
		{
			lineln--;
			line[lineln] = 0;
		}

		char *data = field_n(2, line);

		if (csv_fields(line) == 5 && data)
		{

			/* Calculate record size */
			uint16_t r_size = strlen (data);

			/* Convert file md5 to binary */
			hex_to_bin (line, 32, componentid);

			/* Check if we have a whole new key (first 4 bytes), or just a new subkey (last 12 bytes) */
			bool new_key = (memcmp (componentid, component_lastid, 4) != 0);
			bool new_subkey = new_key ? true : (memcmp (componentid, component_lastid, 16) != 0);

			/* If we have a new main key, or we exceed node size, we must flush and and initialize buffer */
			if (new_key || (component_ptr + 5 + 5 + 12 + 2 + 2 + r_size + 1) >= ldb_max_nodeln)
			{
				/* Write buffer to disk and initialize buffer */
				if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
				if (component_ptr) ldb_node_write(oss_component_bulk, component_sector, component_lastid, component_buf, component_ptr, 0);
				component_ptr = 0;
				component_rg_start  = 0;
				component_rg_size   = 0;

				/* Open new sector if needed */
				if (*componentid != component_lastsector)
				{
					if (component_sector) fclose (component_sector);
					component_sector = ldb_open(oss_component_bulk, componentid, "r+");
				}
			}

			/* New file id, start a new record group */
			if (new_subkey)
			{
				/* Write size of previous record group */
				if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
				component_rg_start  = component_ptr;

				/* K: Add remaining part of key to buffer */
				memcpy (component_buf + component_ptr, componentid + 4, 12);
				component_ptr += 12;

				/* GS: Add record group size (zeroed) */
				uint16_t zero = 0;
				uint16_write(component_buf + component_ptr, zero);
				component_ptr += 2;

				/* Update component_lastid */
				memcpy (component_lastid, componentid, 16);

				/* Update variables */
				component_rg_size   = 0;
			}

			/* Add record length to record */
			uint16_write(component_buf + component_ptr, r_size);
			component_ptr += 2;

			/* Add record to buffer */
			memcpy (component_buf + component_ptr, data, r_size);
			component_ptr += r_size;
			component_rg_size += (2 + r_size);
			component_count++;
		}
		bytecounter += lineln;

		progress ("Importing: ", bytecounter, totalbytes, true);
	}

	/* Flush buffer */
	if (component_rg_size > 0) uint16_write(component_buf + component_rg_start + 12, component_rg_size);
	if (component_ptr) ldb_node_write(oss_component_bulk, component_sector, component_lastid, component_buf, component_ptr, 0);
	if (component_sector) fclose (component_sector);

	printf ("Skipped: %u/%u component\n", component_skipped, component_count);
	fclose (fp);

	if (line) free (line);
	free (componentid);
	free (component_buf);
	free (component_lastid);

	/* Lock DB */
	ldb_unlock ();

	return true;

}

/* Imports a csv with: File_MD5(#2-16, since #1 is in the filename), Component_MD5(16), File_length, File_path  */
bool ldb_import_files(char *filename)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t lineln;

	/* Table definition */
	struct ldb_table oss_file_bulk;
	strcpy(oss_file_bulk.db, "oss");
	strcpy(oss_file_bulk.table, "file");
	oss_file_bulk.key_ln = 4;
	oss_file_bulk.rec_ln = 0;
	oss_file_bulk.ts_ln = 2;
	oss_file_bulk.tmp = false;

	uint8_t *componentid  = calloc (16,1);
	uint8_t *fileid = calloc (16,1);

	uint32_t file_count = 0;

	/* File table buffer */
	uint8_t  *file_buf = malloc (ldb_max_nodeln);
	uint16_t  file_ptr = 0;
	uint8_t  *file_lastid = calloc (33, 1);
	long      file_lastsector = -1;
	FILE     *file_sector = NULL;
	uint16_t  file_rg_start   = 0; // record group size
	uint16_t  file_rg_size   = 0; // record group size

	uint64_t totalbytes = file_size(filename);
	size_t bytecounter = 0;

	/* Create table if it doesn't exist */
	if (!ldb_database_exists("oss"))           ldb_create_database("oss");
	if (!ldb_table_exists("oss", "file"))      ldb_create_table("oss", "file", 16, 0);

	fp = fopen (filename, "r");
	if (fp == NULL) return false;

	/* Lock DB */
	ldb_lock ();

	while ((lineln = getline (&line, &len, fp)) != -1)
	{

		if (lineln > 4096 || lineln < 69) continue;

		/* Trim trailing chr(10) */
		if (line[lineln - 1] == 10)
		{
			lineln--;
			line[lineln] = 0;
		}

		char *data = field_n(3, line);
		char *path = field_n(4, line);

		bool skip = false; 
		if (blacklisted(path)) skip = true;
		if (!skip) if (strlen(path) > DISCARD_PATH_IF_LONGER_THAN) skip = true;

		if (data && path && !skip)
		{
			/* File ID is last 15 bytes */
			if (line[30] == ',')
			{
				hex_to_bin(basename(filename), 2, fileid);
				hex_to_bin(line, 30, fileid + 1);
				hex_to_bin(line + 31, 32, componentid);
			}
			/* Entire file ID in line */
			else
			{
				hex_to_bin(line, 32, fileid);
				hex_to_bin(field_n(2, line), 32, componentid);
			}

			/* Calculate record size */
			uint16_t r_size = strlen(data);

			/* Check if we have a whole new key (first 4 bytes), or just a new subkey (last 12 bytes) */
			bool new_key = (memcmp (fileid, file_lastid, 4) != 0);
			bool new_subkey   = new_key ? true : (memcmp (fileid + 4, file_lastid + 4, 12) != 0);
            bool node_size_exceeded = (file_ptr > 60000);

			/* If we have a new main key, or we exceed node size, we must flush and and initialize buffer */
			if (new_key || node_size_exceeded)
			{
				/* Write buffer to disk and initialize buffer */
				if (file_rg_size > 0) uint16_write(file_buf + file_rg_start + 12, file_rg_size);
				if (file_ptr) ldb_node_write(oss_file_bulk, file_sector, file_lastid, file_buf, file_ptr, 0);
				file_ptr = 0;
				file_rg_start  = 0;
				file_rg_size   = 0;

				/* Open new sector if needed */
				if (*fileid != file_lastsector)
				{
					if (file_sector) fclose (file_sector);
					file_sector = ldb_open(oss_file_bulk, fileid, "r+");
				}
			}

			/* New file id, start a new record group */
			if (new_subkey)
			{

				/* Write size of previous record group */
				if (file_rg_size > 0) uint16_write(file_buf + file_rg_start + 12, file_rg_size);
				file_rg_start  = file_ptr;

				/* K: Add remaining part of key to buffer */
				memcpy (file_buf + file_ptr, fileid + 4, 12);
				file_ptr += 12;

				/* GS: Add record group size (zeroed) */
				uint16_t zero = 0;
				uint16_write(file_buf + file_ptr, zero);
				file_ptr += 2;

				/* Update file_lastid */
				memcpy (file_lastid, fileid, 16);

				/* Update variables */
				file_rg_size   = 0;

			}

			/* Add record length to record */
			uint16_write(file_buf + file_ptr, r_size + 16);
			file_ptr += 2;

			/* Add component id to record */
			memcpy (file_buf + file_ptr, componentid, 16);
			file_ptr += 16;

			/* Add record to buffer */
			memcpy (file_buf + file_ptr, data, r_size);
			file_ptr += r_size;
			file_rg_size += (16 + 2 + r_size);
			file_count++;
		}
		bytecounter += lineln;

		progress("Importing: ", bytecounter, totalbytes, true);
	}
	progress("Importing: ", totalbytes, totalbytes, true);
	printf("\n");

	/* Flush file buffer */
	if (file_ptr)
	{
		/* update GS */
		if (file_rg_size > 0) uint16_write(file_buf + file_rg_start + 12, file_rg_size);
		ldb_node_write(oss_file_bulk, file_sector, file_lastid, file_buf, file_ptr, 0);
	}
	if (file_sector) fclose (file_sector);

	fclose (fp);
	if (line) free (line);
	free (fileid);
	free (file_buf);
	free (file_lastid);
	free (componentid);

	/* Lock DB */
	ldb_unlock ();

	return true;

}

bool csv_sort(char *file_path, bool skip_sort)
{
	if (!file_size(file_path)) return false;
	if (skip_sort) return true;

	/* Assemble command */
	char command[MAX_PATH_LEN] = "\0";
	sprintf(command,"sort -u -o %s %s", file_path, file_path);

	FILE *p = popen(command, "r");
	if (p) pclose(p);
	else
	{
		printf("Cannot execute %s\n", command);
		return false;
	}
	return true;
}

bool bin_sort(char * file_path, bool skip_sort)
{
	if (!file_size(file_path)) return false;
	if (skip_sort) return true;

	return bsort(file_path);
}

void mined_import(char *mined_path, bool skip_sort)
{
	char file_path[MAX_PATH_LEN] = "\0";

	/* Import components */
	sprintf(file_path, "%s/components.csv", mined_path);
	if (csv_sort(file_path, skip_sort)) ldb_import_components(file_path);

	/* Import files */
	for (int i = 0; i < 256; i++)
	{
		sprintf(file_path, "%s/files/%02x.csv", mined_path, i);
		if (csv_sort(file_path, skip_sort))
		{
			printf("%s\n", file_path);
			ldb_import_files(file_path);
		}
	}

	/* Import snippets */
	for (int i = 0; i < 256; i++)
	{
		sprintf(file_path, "%s/snippets/%02x.bin", mined_path, i);
		if (bin_sort(file_path, skip_sort))
		{
			printf("%s\n", file_path);
			ldb_import_snippets(file_path);
		}
	}
}
