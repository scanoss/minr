// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/minr.c
 *
 * Main minr functions
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
 * @file minr.c
 * @date 28 Oct 2021
 * @brief Contains the main minr functionalities
 */

#include "minr.h"
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <zlib.h>
#include "attributions.h"
#include "file.h"
#include "hex.h"
#include "ignorelist.h"
#include "ignored_files.h"
#include <ldb.h>
#include "crypto.h"
#include "minr_log.h"

/* Paths */
char tmp_path[MAX_ARG_LEN] = "/dev/shm";
int min_file_size = MIN_FILE_SIZE;

/**
 * @brief Return true if data is binary
 *
 * @param data data buffer
 * @param len data size
 * @return true if it is binary
 */
bool is_binary(char *data, long len)
{
	/* Is it a zip? */
	if (*data == 'P' && data[1] == 'K' && data[2] < 9)
		return true;
		
	/* Does it contain a chr(0)? */
	return (len != strlen(data));
}

/**
 * @brief Execute the command specified by the string command. It shall create a pipe between the calling program
 *
 * @param command command string
 * @return uint32_t error code
 */
uint32_t execute_command(char *command)
{
	/* Execute command */
	FILE *fp = popen(command, "r");
	if (fp == NULL)
	{
		printf("[EXEC_ERROR] %s\n", command);
		exit(EXIT_FAILURE);
	}
	return pclose(fp);
}

/**
 * @brief Returns the command needed to decompress the "url"
 *
 * @param url url string
 * @return char* command to decompress
 */
char *decompress(char *url)
{

	char c[][2][MAX_ARG_LEN] = {
		{"7z", "7z e -p1 -y"},
		{"aar", "unzip -Psecret -n"},
		{"bz2", "tar -axf"},
		{"egg", "unzip -Psecret -n"},
		{"gem", "gem unpack"},
		{"jar", "unzip -Psecret -n"},
		{"rar", "unrar x -y"},
		{"tar.gz", "tar -axf"},
		{"tar", "tar -xf"},
		{"tar.xz", "tar -axf"},
		{"tgz", "tar -axf"},
		{"war", "unzip -Psecret -n"},
		{"whl", "unzip -Psecret -n"},
		{"xz", "xz -d"},
		{"zip", "unzip -Psecret -n"},
		{"gz", "gunzip"},
		{"nupkg", "unzip -Psecret -n"}};

	int commands = sizeof(c) / sizeof(c[0]);
	char *out = calloc(MAX_PATH_LEN, 1);

	for (int i = 0; i < commands; i++)
	{
		char *extension = c[i][0];
		char *command = c[i][1];

		/* If extension matches url, return command */
		if (!memcmp(extension, url + strlen(url) - strlen(extension), strlen(extension)))
		{
			strcpy(out, command);
			break;
		}
	}

	return out;
}

/**
 * @brief Returns path to the downloadad tmp file
 *
 * @param tmp_dir temp dir string
 * @return char* path to file
 */
char *downloaded_file(char *tmp_dir)
{
	DIR *dp;
	struct dirent *ent;
	char *out = calloc(MAX_PATH_LEN, 1);

	if ((dp = opendir(tmp_dir)) != NULL)
	{
		bool found = false;

		while ((ent = readdir(dp)) != NULL)
		{
			sprintf(out, "%s", ent->d_name);
			if (not_a_dot(out))
			{
				sprintf(out, "%s/%s", tmp_dir, ent->d_name);
				found = true;
				break;
			}
		}
		if (!found)
			printf("Download directory is empty\n");
	}
	else
		printf("Cannot access %s\n", tmp_dir);

	closedir(dp);
	return out;
}

/**
 * @brief Calculate the MD5 of tmp_file and load it into job->urlid
 *
 * @param job pointer to minr job
 * @param tmp_file path to temp file
 */
void load_urlid(struct minr_job *job, char *tmp_file)
{
	uint8_t *bin_md5 = md5_file(tmp_file);
	char *hex_md5 = bin_to_hex(bin_md5, 16);
	strcpy(job->urlid, hex_md5);
	free(hex_md5);
	free(bin_md5);
}

/**
 * @brief Launch command to either download (url) or copy (file) target
 *
 * @param job pointer to minr job
 * @return uint32_t error code
 */
uint32_t download_file(struct minr_job *job)
{
	/* Assemble download/copy command */
	char command[MAX_PATH_LEN] = "\0";
	if (is_file(job->url))
		sprintf(command, "cp \"%s\" \"%s\"", job->url, job->tmp_dir);
	else
		sprintf(command, "cd %s && curl --max-time 1800 -LsJkO --fail --show-error \"%s\"", job->tmp_dir, job->url);

	bool outcome = execute_command(command);

	if (outcome)
		printf("[CURL_ERROR] %s\n", command);

	return outcome;
}

/**
 * @brief Download and process URL
 *
 * @param job pointer to minr job
 * @return true if succed
 */
bool download(struct minr_job *job)
{
	/* Download file */
	uint32_t response = download_file(job);
	if (response)
		return false;

	/* Get the name of the downloaded file inside tmp_dir */
	char *tmp_file = downloaded_file(job->tmp_dir);
	if (!*tmp_file)
	{
		free(tmp_file);
		return false;
	}

	if (file_size(tmp_file) < min_file_size)
	{
		printf("Retrieved file is under min_file_size. Ignoring URL\n");
		free(tmp_file);
		return false;
	}

	/* Get urlid */
	load_urlid(job, tmp_file);

	/* Expand file */
	char *unzipcommand = decompress(tmp_file);

	/* Assemble directory change and unzip command */
	if (*unzipcommand)
	{
		char command[MAX_PATH_LEN] = "\0";

		/* cd into tmp */
		sprintf(command, "cd %s", job->tmp_dir);

		/* add the unzip command */
		sprintf(command + strlen(command), " ; %s ", unzipcommand);

		/* add the downloaded file name */
		sprintf(command + strlen(command), " \"%s\" > /dev/null", tmp_file);

		execute_command(command);
		remove(tmp_file);
	}

	if (unzipcommand)
		free(unzipcommand);
	/* Add de unziped folder to tmp path */
	char *ext = strrchr(tmp_file, '.');
	if (ext)
		ext[0] = '\0';
	/* check if it is a valid dir and replace the tmp*/
	if (is_dir(tmp_file))
		strcpy(job->tmp_dir, tmp_file);
	free(tmp_file);
	return true;
}

enum
{
	FILE_IGNORED = 0,
	FILE_ACCEPTED,
};

/**
 * @brief Open a file and fullyfil the minr job structure
 *
 * @param job pointer to minr job
 * @param path path to file
 * @return FILE_IGNORED 
 * @return FILE_ACCEPTED 
 */
int load_file(struct minr_job *job, char *path)
{
	int result = FILE_ACCEPTED;
	/* Open file and obtain file length */
	job->src_ln = 0;
	FILE *fp;

	if (!is_file(path))
		return FILE_IGNORED;

	if ((fp = fopen(path, "rb")) == NULL)
	{
		minr_log("Failed to open the file %s", path);
		return FILE_IGNORED;
	}

	fseeko64(fp, 0, SEEK_END);
	job->src_ln = ftello64(fp);
	fseeko64(fp, 0, SEEK_SET);

	if (job->src_ln <= 0)
		return FILE_IGNORED;
	
	job->src = calloc(job->src_ln + 2, 1);

	if (!job->src)
	{
		fprintf(stderr,"Not memory available to mine this file %s\n. File trunked to %d", path, MAX_FILE_SIZE);
		job->src_ln = MAX_FILE_SIZE;
		job->src = calloc(job->src_ln + 1, 1);
		if (!job->src)
		{
			fprintf(stderr, "Not memory available\n");
			exit(EXIT_FAILURE);
		}
	}

	/* File discrimination check #3: Is it under/over the threshold */
	if (job->src_ln < min_file_size)
	{
		result = FILE_IGNORED;
	}
		
	if (!fread(job->src, 1, job->src_ln, fp))
	{
		fclose(fp);
		return result;
	}

	/* Read file contents into src and close it */
	job->src[job->src_ln] = 0;
	fclose(fp);

	/* Calculate file MD5 */
	uint8_t * md5 = md5_file(path);
	memcpy(job->md5, md5, sizeof(job->md5));
	free(md5);
	ldb_bin_to_hex(job->md5, MD5_LEN, job->fileid);

	if (ignored_file(job->fileid))
	{
		return FILE_IGNORED;
	}
		
	return result;
}

/**
 * @brief Extracts the "n"th value from the comma separated "in" string
 *
 * @param out[out] pointer to the wanted field
 * @param in data buffer
 * @param n filed number
 * @param limit field len limit
 */
void extract_csv(char *out, char *in, int n, long limit)
{
	*out = 0;
	if (!in)
		return;
	if (!*in)
		return;

	int strln = strlen(in);
	if (strln < limit)
		limit = strln;

	limit--; // need an extra byte for chr(0)

	char *tmp = in;
	int n_counter = 1;
	int out_ptr = 0;

	do
	{
		if (*tmp == ',')
			n_counter++;
		else if (n_counter == n)
			out[out_ptr++] = *tmp;
	} while (*tmp++ && (n_counter <= n) && ((out_ptr + 1) < limit));

	out[out_ptr] = 0;
}

/**
 * @brief  Mine the given path
 *
 * @param job ponter to minr job
 * @param path path to be mined
 */
void mine(struct minr_job *job, char *path)
{
	bool extra_table = false;
	bool exclude_detection = job->exclude_detection;

	/* Mine attribution notice */
	job->is_attribution_notice = is_attribution_notice(path);
	if (job->is_attribution_notice)
	{
		mine_attribution_notice(job, path);
		minr_log("file %s processed as attribution notice\n", path);
		if (job->mine_all)
				extra_table = true;
		else
			return;
	}
	/* File discrimination check #2: Is the extension ignored or path not wanted? */
	if (!job->all_extensions)
		if (ignored_extension(path))
		{
			minr_log("Ignored due to: ignored_extension\n");
			if (job->mine_all)
				extra_table = true;
			else
				return;
		}


	if (unwanted_path(path))
	{
		minr_log("Ignored due to: unwanted_path\n");
		if (job->mine_all)
			extra_table = true;
		else
			return;
	}
	/* Load file contents and calculate md5 */
	int result = load_file(job, path);
	if (result == FILE_IGNORED)
	{
		minr_log("Ignoring empty file %s\n", path);
		if (job->src_ln <= 0 || !job->mine_all)
		{
			return;
		}
		else
			extra_table = true;
	}
	//Replace comma in path for '-'
	char * comma = strchr(path, ',');
	if (comma)
		*comma = '-';

	/* Add to .mz */
	if (!job->exclude_mz)
	{
		/* File discrimination check: Binary? */
		if (is_binary(job->src, job->src_ln))
		{
			exclude_detection = true;
			minr_log("Binary detected, excluded from sources\n");
		}
		else if ((job->zsrc = calloc(compressBound(job->src_ln +1) + MZ_HEAD, 1)) != NULL && job->src_ln > 0 && job->src)
		{
			if (extra_table)
			{	
				mz_add(job->mined_extra_path, job->md5, job->src, job->src_ln, true, job->zsrc, job->mz_cache_extra);
			}
			else
			{
				bool skip = false;
			
				/* Is the file extension supposed to be skipped for snippet hashing? */
				if (skip_mz_extension(path))
				{
					minr_log("Ignored due to: skip_mz_extension\n");
					skip = true;
				}
						
				if (!skip)
				{
					mz_add(job->mined_path, job->md5, job->src, job->src_ln, true, job->zsrc, job->mz_cache);
				}
				else if (job->mine_all)
				{
					minr_log("Por las dudas %s\n", path);
					extra_table = true;
					mz_add(job->mined_extra_path, job->md5, job->src, job->src_ln, true, job->zsrc, job->mz_cache_extra);
				}
			}
			free(job->zsrc);
		}
		else
		{
			minr_log("File %s was not added to sources\n", path);
		}
	}

	/* Mine more */
	if (!exclude_detection && job->src)
	{
		mine_crypto(job->mined_path, job->fileid, job->src, job->src_ln);
		mine_license(job, job->fileid, false);
		mine_quality(job->mined_path, job->fileid, job->src, job->src_ln);
		mine_copyright(job->mined_path, job->fileid, job->src, job->src_ln, false);
	}

	/* Output file information */

	if (extra_table)
	{
		minr_log("File %s proceesed as \"Extra\"\n", path);
		fprintf(job->out_file_extra[*job->md5], "%s,%s,%s\n", job->fileid + 2, job->urlid, path + strlen(job->tmp_dir) + 1);
		fflush(job->out_file_extra[*job->md5]);
		if (job->out_pivot_extra)
			fprintf(job->out_pivot_extra, "%s,%s\n", job->urlid + 2, job->fileid);
	}
	else
	{
		minr_log("File %s accepted\n", path);
		uint8_t url_md5_byte;
		ldb_hex_to_bin(job->urlid, 2, &url_md5_byte);
		fprintf(job->out_file[*job->md5], "%s,%s,%s\n", job->fileid + 2, job->urlid, path + strlen(job->tmp_dir) + 1);
		fflush(job->out_file[*job->md5]);
		ldb_hex_to_bin(job->urlid, 2, &url_md5_byte);
		if (job->out_pivot)
			fprintf(job->out_pivot, "%s,%s\n", job->urlid + 2, job->fileid);
	}

	free(job->src);
}

/**
 * @brief Mine a local file
 *
 * @param job pointer to minr job
 * @param path string path
 */
void mine_local_file(struct minr_job *job, char *path)
{
	//job->src = calloc(MAX_FILE_SIZE + 1, 1);
	bool skip = false;

	/* Load file contents and calculate md5 */
	if (!load_file(job, path))
		skip = true;

	/* File discrimination check: Unwanted header? */
	if (!skip)
		if (unwanted_header(job->src))
			skip = true;

	/* Is it binary */
	if (!skip)
		if (is_binary(job->src, job->src_ln))
			skip = true;

	if (!skip)
	{
		switch (job->local_mining)
		{
		case 1:
			mine_crypto(NULL, path, job->src, job->src_ln);
			break;

		case 2:
			mine_license(job, path, false);
			break;

		case 3:
			mine_quality(NULL,
						 path,
						 job->src,
						 job->src_ln);
			break;

		case 4:
			mine_copyright(NULL,
						   path,
						   job->src,
						   job->src_ln, false);
			break;
		}
	}
	free(job->src);
	job->src = NULL;
}

/**
 * @brief Local mining.
 * Mines for Licenses, Crypto definitions and Copyrigths from a local directory. Results are presented via stdout.
 * @since 2.1.2
 *
 * @param job pointer to minr job
 * @param root path to the root
 */
void mine_local_directory(struct minr_job *job, char *root)
{

	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(root)))
		return;

	while ((entry = readdir(dir)) != NULL)
	{
		char path[1024];
		if (entry->d_type == DT_DIR)
		{

			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
			mine_local_directory(job, path);
		}
		else
		{
			sprintf(path, "%s/%s", root, entry->d_name);
			mine_local_file(job, path);
		}
	}
	closedir(dir);
}

/**
 * @brief Recursive directory reading
 *
 * @param job pointer to minr job
 * @param path root path
 */
void recurse(struct minr_job *job, char *path)
{
	DIR *dp;
	struct dirent *entry;

	if (!(dp = opendir(path)))
		return;

	while ((entry = readdir(dp)))
	{
		if (valid_path(path, entry->d_name))
		{
			/* Assemble path */
			char tmp_path[MAX_PATH_LEN] = "\0";
			sprintf(tmp_path, "%s/%s", path, entry->d_name);

			/* Recurse if dir */
			if (entry->d_type == DT_DIR && not_a_dot(entry->d_name))
				recurse(job, tmp_path);

			/* File discrimination check #1: Is this an actual file? */
			else if (is_file(tmp_path))
				mine(job, tmp_path);
		}
	}

	if (dp)
		closedir(dp);
}

/**
 * @brief Verify that required binaries are installed
 *
 * @return true if succed
 */
bool check_dependencies()
{
	load_crypto_definitions();

	struct stat sb;
	char *dependencies[] =
		{
			"/bin/gunzip",
			"/bin/tar",
			"/usr/bin/7z",
			"/usr/bin/curl",
			"/usr/bin/sort",
			"/usr/bin/unrar",
			"/usr/bin/unzip",
			"/usr/bin/xz",
			"/usr/bin/gem"};

	for (int i = 0; i < (sizeof(dependencies) / sizeof(dependencies[0])); i++)
	{
		if (!(stat(dependencies[i], &sb) == 0 && sb.st_mode & S_IXUSR))
		{
			printf("Missing %s\n", dependencies[i]);
			return false;
		}
	}
	return true;
}

/**
 * @brief Returns true if file ends with LF or if it is empty
 *
 * @param path string path
 * @return true if succed
 */
bool ends_with_chr10(char *path)
{
	/* Read source into memory */
	FILE *file = fopen(path, "r");
	if (!file)
	{
		printf("Cannot open source file %s\n", path);
		exit(EXIT_FAILURE);
	}

	/* Obtain file size */
	fseeko64(file, 0, SEEK_END);
	uint64_t size = ftello64(file);

	/* Empty file, it is ok */
	if (size == 0)
	{
		fclose(file);
		return true;
	}

	/* Read last byte */
	uint8_t last_byte[1] = "\0";
	fseeko64(file, size - 1, SEEK_SET);
	fread(last_byte, 1, 1, file);

	/* Ends with chr(10), it is ok */
	if (*last_byte == 10)
	{
		fclose(file);
		return true;
	}

	/* Not empty, not ending with chr(10), it is not ok */
	fclose(file);
	return false;
}

/**
 * @brief Validate source and destination files for join and sort
 *
 * @param file path string
 * @param destination path string
 * @return true
 * @return false
 */
bool valid_source_destination(char *file, char *destination)
{
	if (strcmp(extension(file), extension(destination)))
	{
		printf("Source and destination should be the same file type %s != %s\n", file, destination);
		return false;
	}

	/* Validate .mz files by checking .mz integrity */
	if (!strcmp(extension(file), "mz"))
	{
		/* Check source file */
		if (!mz_check(file))
		{
			printf("Source %s is corrupted\n", file);
			return false;
		}

		/* Check destination file */
		if (destination)
		{
			if (is_file(destination))
				if (file_size(destination))
					if (!mz_check(destination))
					{
						printf("Destination %s is corrupted\n", file);
						return false;
					}
		}
	}

	/* CSV files should both end with chr(10) (or be empty) */
	if (!strcmp(extension(file), "csv"))
	{
		if (!ends_with_chr10(file))
		{
			printf("CSV file %s should end with LF\n", file);
			return false;
		}
		if (!ends_with_chr10(destination))
		{
			printf("CSV file %s should end with LF\n", destination);
			return false;
		}
	}

	/* Validate .bin files by checking if they divide by 21 (21-byte records) */
	if (!strcmp(extension(file), "bin"))
	{
		/* Check source file */
		uint64_t size = file_size(file);
		if (size)
			if (size % 21)
			{
				printf("Source file %s seems not to contain 21-byte records\n", file);
				return false;
			}

		/* Check destination */
		if (destination)
		{
			uint64_t size_d = file_size(destination);
			if (size_d)
				if (size_d % 21)
				{
					printf("Destination file %s seems not to contain 21-byte records\n", destination);
					return false;
				}
		}
	}

	if (!check_disk_free(file, file_size(file)))
		return false;

	return true;
}
