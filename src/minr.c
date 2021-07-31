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

#include "minr.h"
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"
#include "md5.h"
#include "hex.h"
#include "ignorelist.h"
#include "ignored_files.h"
#include "ldb.h"
#include "crypto.h"

/* Paths */
char mined_path[MAX_ARG_LEN] = ".";
char tmp_path[MAX_ARG_LEN] = "/dev/shm";
int min_file_size = MIN_FILE_SIZE;

char *ATTRIBUTION_NOTICES[] =
{
"COPYING",
"COPYING.lib",
"LICENSE",
"LICENSE.md",
"LICENSE.txt",
"LICENSE-Community.txt",
"LICENSES",
"NOTICE",
NULL
};

/* Return true if file in path is an attribution notice */
bool is_attribution_notice(char *path)
{
	char *file = basename(path);
	if (!file) return true;
	if (!*file) return true;

	int i=0;
	while (ATTRIBUTION_NOTICES[i])
		if (stricmp(ATTRIBUTION_NOTICES[i++], file))
			return true;

	return false;
}

/* Return true if data is binary */
bool is_binary(char *data, long len)
{
		/* Is it a zip? */
		if (*data == 'P' && data[1] == 'K' && data[2] < 9) return true;

		/* Does it contain a chr(0)? */
		return (len != strlen(data));
}

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

/* Returns the command needed to decompress the "url" */
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

	if (file_size(tmp_file) < min_file_size)
	{
		printf("Retrieved file is under min_file_size. Ignoring URL\n");
		return true;
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

bool load_file(struct minr_job *job, char *path)
{
	/* Open file and obtain file length */
	FILE *fp;

	 if ((fp = fopen(path,"rb")) == NULL){
	       printf("Error! Cannot load the definitions file");
	       exit(1);
	}

	fseeko64(fp, 0, SEEK_END);
	job->src_ln = ftello64(fp);

	/* File discrimination check #3: Is it under/over the threshold */
	if (job->src_ln < min_file_size || job->src_ln >= MAX_FILE_SIZE)
	{
		if (fp) fclose (fp);
		return false;
	}

	/* Read file contents into src and close it */
	fseeko64(fp, 0, SEEK_SET);
	//job->src=(char *)malloc(job->src_ln+1);
	if (!fread(job->src, 1, job->src_ln, fp)) printf("Error reading %s\n", path);
	job->src[job->src_ln] = 0;
	fclose(fp);

	/* Calculate file MD5 */
	calc_md5(job->src, job->src_ln, job->md5);
	ldb_bin_to_hex(job->md5, MD5_LEN, job->fileid);

	if (ignored_file(job->fileid)) return false;

	return true;
}

/* Extracts the "n"th value from the comma separated "in" string */
void extract_csv(char *out, char *in, int n, long limit)
{
	*out = 0;
	if (!in) return;
	if (!*in) return;

	int strln = strlen(in);
	if (strln < limit) limit = strln;

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

/* Write entry id to attribution.csv */
void attribution_add(struct minr_job *job)
{
	char path[MAX_PATH_LEN]="\0";
	sprintf(path, "%s/attribution.csv", job->mined_path);

	char notice_id[MD5_LEN * 2 + 1] = "\0";
	ldb_bin_to_hex(job->md5, MD5_LEN, notice_id);

	FILE *fp = fopen(path, "a");
	if (!fp)
	{
		printf("Cannot create file %s\n", path);
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "%s,%s\n", job->pairid, notice_id);
	fclose(fp);
}

/* Appends attribution notice to archive */
void mine_attribution_notice(struct minr_job *job, char *path)
{
	if (!load_file(job,path)) return;

	mine_license(job, job->pairid, true);

	/* Write entry to mined/attribution.csv */
	attribution_add(job);

	/* Create sources directory */
	char mzpath[LDB_MAX_PATH * 2];
	sprintf(mzpath, "%s/notices", job->mined_path);

	ldb_prepare_dir(mzpath);

	/* Compress data */
	job->zsrc_ln = compressBound(job->src_ln + 1);

	/* Save the first bytes of zsrc to accomodate the MZ header */
	compress(job->zsrc + MZ_HEAD, &job->zsrc_ln, (uint8_t *) job->src, job->src_ln + 1);
	uint32_t zln = job->zsrc_ln;

	/* Only the last 14 bytes of the MD5 go to the mz record (first two bytes are the file name) */
	memcpy(job->zsrc, job->md5 + 2, MZ_MD5);

	/* Add the 32-bit compressed file length */
	memcpy(job->zsrc + MZ_MD5, (char *) &zln, MZ_SIZE);

	int mzid = uint16(job->md5);
	int mzlen = job->zsrc_ln + MZ_HEAD;

	sprintf(mzpath, "%s/notices/%04x.mz", job->mined_path, mzid);
	FILE *f = fopen(mzpath, "a");
	if (f)
	{
		size_t written = fwrite(job->zsrc, mzlen, 1, f);
		if (!written)
		{
			printf("Error writing %s\n", mzpath);
			exit(EXIT_FAILURE);
		}
		fclose(f);
	}

	mine_copyright(job->mined_path, job->pairid, job->src, job->src_ln, true);
}

/* Mine the given path */
void mine(struct minr_job *job, char *path)
{
	/* Mine attribution notice */
	if (is_attribution_notice(path))
	{
		mine_attribution_notice(job, path);
		return;
	}

	/* File discrimination check #2: Is the extension ignored or path not wanted? */
	if (!job->all_extensions) if (ignored_extension(path)) return;
	if (unwanted_path(path)) return;

	/* Load file contents and calculate md5 */
	if (!load_file(job,path)) return;

	/* File discrimination check: Unwanted header? */
	if (unwanted_header(job->src)) return;

	/* Add to .mz */
	if (!job->exclude_mz)
	{
		/* File discrimination check: Binary? */
		if (is_binary(job->src, job->src_ln))
			job->exclude_detection = true;
		else
			mz_add(job->mined_path, job->md5, job->src, job->src_ln, true, job->zsrc, job->mz_cache);
	}

	/* Mine more */
	if (!job->exclude_detection)
	{
		mine_crypto(job->mined_path, job->fileid, job->src, job->src_ln);
		mine_license(job, job->fileid, false);
		mine_quality(job->mined_path, job->fileid, job->src, job->src_ln);
		mine_copyright(job->mined_path, job->fileid, job->src, job->src_ln, false);
	}

	/* Output file information */
	fprintf(out_file[*job->md5], "%s,%s,%s\n", job->fileid + 2, job->urlid, path + strlen(job->tmp_dir) + 1);
}

void mine_local_file(struct minr_job *job, char *path)
{
	job->src = calloc(MAX_FILE_SIZE + 1, 1);

	/* Load file contents and calculate md5 */
	if (!load_file(job,path))
	{
		free(job->src);
		return;
	}
	/* File discrimination check: Unwanted header? */
	if (unwanted_header(job->src) || is_binary(job->src, job->src_ln)) {
		free(job->src);
		return;
	}
	switch(job->local_mining)
	{
		case 1:
			mine_crypto(NULL,path, job->src, job->src_ln);
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
	free(job->src);
}

/**
  @brief Local mining
  @description Mines for Licenses, Crypto definitions and Copyrigths from a local directory. Results are presented via stdout.
  @since 2.1.2
  */

void mine_local_directory(struct minr_job *job, char* root){

	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(root)))
		return;

	while ((entry = readdir(dir)) != NULL) {
		char path[1024];
		if (entry->d_type == DT_DIR) {

			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
			mine_local_directory(job,path);
		} else {
			sprintf(path,"%s/%s",root,entry->d_name);
			mine_local_file(job,path);
		}
	}
	closedir(dir);

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

