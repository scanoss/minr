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

/* Returns the command needed to decompress the "url" */
#include "minr.h"
#include <dirent.h>
#include <sys/stat.h>
#include "file.h"
#include "md5.h"
#include "hex.h"
#include "blacklist.h"
#include "ldb.h"

/* Paths */
char mined_path[MAX_ARG_LEN] = ".";
char tmp_path[MAX_ARG_LEN] = "/dev/shm";
int min_file_size = MIN_FILE_SIZE;

uint32_t execute_command(char *command)
{
	/* Execute command */
	FILE *fp = popen(command, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "Error executing %s\n", command);
		exit(1);
	}
	return pclose(fp);
}

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
		{"gz", "gunzip"}
	};

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

/* Returns path to the downloadad tmp file */
char *downloaded_file(char *tmp_dir)
{
	DIR *dp;
	struct dirent *ent;
	char *out = calloc(MAX_PATH_LEN, 1);

	if ((dp = opendir(tmp_dir)) != NULL)
	{
		bool found=false;

		while ((ent = readdir(dp)) != NULL)
		{
			sprintf(out,"%s", ent->d_name);
			if (not_a_dot(out))
			{
				sprintf(out,"%s/%s", tmp_dir, ent->d_name);
				found=true;
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

/* Calculate the MD5 of tmp_file and load it into job->urlid */
void load_urlid(struct minr_job *job, char *tmp_file)
{
	uint8_t *bin_md5 = file_md5(tmp_file);
	char *hex_md5 = bin_to_hex(bin_md5, 16);
	strcpy(job->urlid, hex_md5);
	free(hex_md5);
	free(bin_md5);
}

/* Launch command to either download (url) or copy (file) target */
uint32_t download_file(struct minr_job *job)
{
	/* Assemble download/copy command */
	char command[MAX_PATH_LEN] = "\0";
	if (is_file(job->url))
		sprintf(command, "cp \"%s\" \"%s\"", job->url, job->tmp_dir);
	else
		sprintf(command, "cd %s && curl -LsJkO -f \"%s\"", job->tmp_dir, job->url);

	return execute_command(command);
}

/* Download and process URL */
bool download(struct minr_job *job)
{
	/* Download file */
	uint32_t response = download_file(job);
	if (response != 0)
	{
		fprintf(stderr, "Curl returned %d\n", response);
		return false;
	}

	/* Get the name of the downloaded file inside tmp_dir */
	char *tmp_file = downloaded_file(job->tmp_dir);
	if (!*tmp_file)
	{
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

	if (unzipcommand) free(unzipcommand);
	free(tmp_file);

	return true;
}

/* Mine the given path */
void mine(struct minr_job *job, char *path)
{
	/* File discrimination check #2: Is the extension blacklisted or path not wanted? */
	if (!job->all_extensions) if (blacklisted_extension(path)) return;
	if (unwanted_path(path)) return;

	/* Open file and obtain file length */
	FILE *fp = fopen(path, "rb");
	fseeko64(fp, 0, SEEK_END);
	job->src_ln = ftello64(fp);

	/* File discrimination check #3: Is it under/over the threshold */
	if (job->src_ln < min_file_size || job->src_ln >= MAX_FILE_SIZE)
	{
		if (fp) fclose (fp);
		return;
	}

	/* Read file contents into src and close it */
	fseeko64(fp, 0, SEEK_SET);
	if (!fread(job->src, 1, job->src_ln, fp)) printf("Error reading %s\n", path);
	job->src[job->src_ln] = 0;
	fclose(fp);

	/* Calculate MD5 */
	uint8_t md5[16] = "\0";
	calc_md5(job->src, job->src_ln, md5);

	/* File discrimination check: Unwanted header? */
	if (unwanted_header(job->src)) return;

	/* Add to .mz */
	if (!job->exclude_mz)
	{
		/* File discrimination check: Binary? */
		int src_ln = strlen(job->src);
		if (job->src_ln == src_ln) mz_add(job->mined_path, md5, job->src, job->src_ln, true, job->zsrc, job->mz_cache);

	}

	/* Convert md5 to hex */
	char *hex_md5 = bin_to_hex(md5, 16);

	/* Mine more */
	if (!job->exclude_detection)
	{
		mine_license(job->mined_path, hex_md5, job->src, job->src_ln, job->licenses, job->license_count);
		mine_quality(job->mined_path, hex_md5, job->src, job->src_ln);
		mine_copyright(job->mined_path, hex_md5, job->src, job->src_ln);
	}

	/* Output file information */
	fprintf(out_file[*md5], "%s,%s,%s\n", hex_md5 + 2, job->urlid, path + strlen(job->tmp_dir) + 1);
	free(hex_md5);
}

/* Recursive directory reading */
void recurse(struct minr_job *job, char *path)
{
	DIR *dp;
	struct dirent *entry;

	if (!(dp = opendir(path))) return;

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

	if (dp) closedir(dp);
}

/* Verify that required binaries are installed */
bool check_dependencies()
{

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
		"/usr/bin/gem"
	};

	for (int i = 0; i < (sizeof(dependencies)/sizeof(dependencies[0])); i++)
	{
		if (!(stat(dependencies[i], &sb) == 0 && sb.st_mode & S_IXUSR))
		{
			printf("Missing %s\n", dependencies[i]);
			return false;
		}
	}
	return true;
}

/* Returns true if file ends with LF or if it is empty */
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

/* Validate source and destination files for join and sort */
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
			if (is_file(destination)) if (file_size(destination)) if (!mz_check(destination))
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
		if (size) if (size % 21)
		{
			printf("Source file %s seems not to contain 21-byte records\n", file);
			return false;
		}

		/* Check destination */
		if (destination)
		{
			uint64_t size_d = file_size(destination);
			if (size_d) if (size_d % 21)
			{
				printf("Destination file %s seems not to contain 21-byte records\n", destination);
				return false;
			}
		}
	}

	if (!check_disk_free(file, file_size(file))) return false;

	return true;
}

