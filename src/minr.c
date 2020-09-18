// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/minr.c
 *
 * Main minr functions
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
char *downloaded_file(char *tmp_component)
{
	DIR *dp;
	struct dirent *ent;
	char *out = calloc(MAX_PATH_LEN, 1);

	if ((dp = opendir(tmp_component)) != NULL)
	{
		bool found=false;

		while ((ent = readdir(dp)) != NULL)
		{
			sprintf(out,"%s", ent->d_name);
			if (not_a_dot(out))
			{
				sprintf(out,"%s/%s", tmp_component, ent->d_name);
				found=true;
				break;
			}
		}

		if (!found)
			printf("Download directory is empty\n");

	}
	else
		printf("Cannot access %s\n", tmp_component);

	closedir(dp);
	return out;

}

/* Download "url" into "tmp_component" */
bool download(char *tmp_component, char *url, char *md5)
{
	FILE *fp;

	/* Assemble download/copy command */
	char *command = malloc(MAX_PATH_LEN);
	if (is_file(url))
		sprintf(command, "cp \"%s\" \"%s\"", url, tmp_component);
	else
		sprintf(command, "cd %s && curl -LsJkO -f \"%s\"", tmp_component, url);

	/* Execute command */
	fp = popen(command, "r");
	free(command);
	if (fp == NULL)
	{
		perror("Curl error");
		exit(1);
	}

	uint32_t response = pclose(fp);
	if (response != 0)
	{
		printf("Curl returned %d\n", response);
		return false;
	}

	char *tmp_file = downloaded_file(tmp_component);
	if (!*tmp_file)
	{
		free(tmp_file);
		return false;
	}

	uint8_t *bin_md5 = file_md5(tmp_file);
	char *hex_md5 = bin_to_hex(bin_md5, 16);
	strcpy(md5, hex_md5);
	free(hex_md5);
	free(bin_md5);

	/* Expand file */
	char *unzipcommand = decompress(tmp_file);

	/* Assemble directory change and unzip command */
	if (*unzipcommand)
	{
		char *command = malloc(MAX_PATH_LEN);
		
		/* cd into tmp */
		sprintf(command, "cd %s", tmp_component);

		/* add the unzip command */		
		sprintf(command + strlen(command), " ; %s ", unzipcommand);

		/* add the downloaded file name */
		sprintf(command + strlen(command), " \"%s\" > /dev/null", tmp_file);

		fp = popen(command, "r");
		free(command);

		if (fp == NULL)
		{
			perror("Error decompressing");
			free(tmp_file);
			exit(1);
		}
		pclose(fp);
		remove(tmp_file);
	}

	if (unzipcommand) free(unzipcommand);
	free(tmp_file);

	return true;
}

/* Mine the given path */
void mine(char *src, uint8_t *zsrc, struct mz_cache_item *mz_cache, char *tmp_component, char *path, bool all_extensions, bool exclude_mz, char *urlid)
{

	/* File discrimination check #2: Is the extension blacklisted? */
	if (!all_extensions) if (blacklisted(path)) return;

	/* Open file and obtain file length */
	FILE *fp = fopen(path, "rb");
	fseeko64 (fp, 0, SEEK_END);
	uint64_t length = ftello64 (fp);

	/* File discrimination check #3: Is it under/over the threshold */
	if (length < min_file_size || length > MAX_FILE_SIZE)
	{
		if (fp) fclose (fp);
		return;
	}

	/* Read file contents into src and close it */
	fseeko64(fp, 0, SEEK_SET);
	if (!fread(src, 1, length, fp)) printf("Error reading %s\n", path);
	src[length] = 0;
	fclose(fp);

	/* Calculate MD5 */
	uint8_t md5[16] = "\0";
	calc_md5(src, length, md5);

	/* File discrimination check: Unwanted header? */
	if (unwanted_header(src)) return;

	/* Add to .mz */
	if (!exclude_mz)
	{
		/* File discrimination check: Binary? */
		int src_ln = strlen(src);
		if (length == src_ln) mz_add(md5, src, length, true, zsrc, mz_cache);
	}

	/* Output file information */
	char *hex_md5 = bin_to_hex(md5, 16);
	fprintf(out_file[*md5], "%s,%s,%lu,%s\n", hex_md5 + 2, urlid, length, path + strlen(tmp_component) + 1);
	free(hex_md5);
}

/* Recursive directory reading */
void recurse(char *component_record, char *tmp_component, char *tmp_dir, bool all_extensions, bool exclude_mz, char* urlid, char *src, uint8_t *zsrc, struct mz_cache_item *mz_cache)
{

	DIR *dp;
	struct dirent *entry;

	if (!(dp = opendir(tmp_dir))) return;

	while ((entry = readdir(dp)))
	{
		if (valid_path(tmp_dir, entry->d_name))
		{
			char *path = malloc(MAX_PATH_LEN);
			sprintf(path, "%s/%s", tmp_dir, entry->d_name);
			if (entry->d_type == DT_DIR && not_a_dot(entry->d_name)) 
				recurse(component_record, tmp_component, path, all_extensions, exclude_mz, urlid, src, zsrc, mz_cache);

			/* File discrimination check #1: Is this an actual file? */
			else if (is_file (path))
			{
				/* If it is the first file, then add the component record first */
				if (*component_record)
				{
					fprintf(out_component, "%s\n",component_record);
					*component_record = 0;
				}
				mine(src, zsrc, mz_cache, tmp_component, path, all_extensions, exclude_mz, urlid);
			}
			free(path);
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

bool valid_file_and_directory(char *file, char *directory)
{
	/* Does the file exist? */
	if (!is_file(file))
	{
		printf("File %s does not exist or cannot be accessed\n", file);
		return false;
	}

	/* Is the file a valid extension? */
	if (strcmp(extension(file), "bin") && strcmp(extension(file), "csv") && strcmp(extension(file), "mz"))
	{
		printf("Only .csv, .bin or .mz files can be joined by minr\n");
		return false;
	}

	if (!directory) if (!strcmp(extension(file), "mz"))
	{
		printf("Sorting support for .mz files is not yet implemented\n");
		return false;
	}

	/* Does the target directory exist? */
	if (directory) if (!is_dir(directory))
	{
		printf("Target directory %s does not exist or cannot be accessed\n", directory);
		return false;
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

void file_append(char *file, char *destination)
{
	FILE *f;
	uint64_t size = file_size(file);
	if (!size) return;

	/* Read source into memory */
	f = fopen(file, "r");
	if (!f)
	{
		printf("Cannot open source file %s\n", file);
		exit(EXIT_FAILURE);
	}
	char *buffer = malloc(size);
	uint64_t bytes = fread(buffer, 1, size, f);
	fclose(f);
	if (bytes != size)
	{
		free(buffer);
		printf("Failure reading source file %s\n", file);
		exit(EXIT_FAILURE);
	}

	/* Append data to destination */
	f = fopen(destination, "a");
	if (!f)
	{
		free(buffer);
		printf("Cannot write to destination file %s\n", destination);
		exit(EXIT_FAILURE);
	}
	bytes = fwrite(buffer, 1, size, f);
	fclose(f);
	free(buffer);	
}

void minr_join(char *file, char *directory)
{
	if (!valid_file_and_directory(file, directory)) exit(EXIT_FAILURE);

	/* Assemble and check destination file path */
	char *destination = malloc(MAX_PATH_LEN);
	sprintf(destination, "%s/%s", directory, basename(file));
	if (!strcmp(destination, file))
	{
		printf("Cannot join a file with itself\n");
		free(destination);
		return;
	}

	/* Validate source and destination files */	
	if (valid_source_destination(file, destination))
		file_append(file, destination);

	free(destination);
}
