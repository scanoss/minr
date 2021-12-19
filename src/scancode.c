// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/quality.c
 *
 * SCANOSS Quality assessment subroutines
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

/* Make quality assessment on an individual file */

/**
 * @file scancode.c
 * @date 12 Dec 2021
 * @brief Contains functions used to call scancode external module
 */
#include <dirent.h>
#include <ctype.h>
#include "scancode.h"
#include "file.h"

#define TMP_DIR "/tmp/minr"

#define LICENSE_PATTERN_NUMBER 2

const char LICENSE_PATTERN[LICENSE_PATTERN_NUMBER][10] = {"LIC\0", "COPY\0"};

static bool prepare_tmp_dir(char * id)
{
    FILE *sc_file;
    char *command;
    asprintf(&command, "mkdir -p %s/%s && rm -f %s/%s/*", TMP_DIR, id, TMP_DIR, id); // create tmp or erase content
    sc_file = popen(command, "r");

    fclose(sc_file);
    free(command);
    return true;
}

static bool copy_to_tmp(char *path, char *id)
{
    FILE *sc_file;
    char *command;
    asprintf(&command, "cp %s %s/%s/", path, TMP_DIR, id); //copy
    sc_file = popen(command, "r");

    fclose(sc_file);
    free(command);
    return true;
}

static bool scancode_run(char * id, char *csv_file)
{
    char *command;
    if (csv_file)
        asprintf(&command, "scancode -cl --quiet -n 6 --timeout 2 --json %s/%s/scancode.json %s/%s  2> scancode_error.txt &&\
	 	    				jq -r '.files[] | \"\\(.path),5,\\(.licenses[].spdx_license_key)\"' %s/%s/scancode.json | sort -u | sed 's/\\/[^:/:,]*,/,/g' 1>> %s",
                 TMP_DIR, id, TMP_DIR, id, TMP_DIR, id,csv_file);

    FILE *sc_file = popen(command, "r");

    fclose(sc_file);
    free(command);
    return true;
}

static bool validate_file(char * path)
{
    if (!is_file(path))
        return false;
    
    char * file_name = strrchr(path, '/') + 1; //find the las /
    if (!file_name) //if there is not / then point to the path
        file_name = path;
    
    char upper_file_name[strlen(file_name)];
    memset(upper_file_name, 0, sizeof(upper_file_name));
    int i = 0;
     while (*file_name)
     {
         upper_file_name[i] = toupper(*file_name);
         i++;
         file_name++;
     }

    for (int j = 0; j < LICENSE_PATTERN_NUMBER; j++)
    {
        if (strstr(upper_file_name, LICENSE_PATTERN[j]))
        {
            fprintf(stderr,"\n%s - %s\n", path, upper_file_name);
            return true;
        }
    }

    return false;

}

bool find_files(char * path, char * id)
{
    DIR *dp;
	struct dirent *entry;

    bool found = false;
	if (!(dp = opendir(path))) 
        return found;
    printf(path);

	while ((entry = readdir(dp)))
	{
        if (valid_path(path, entry->d_name))
		{
			/* Assemble path */
			char tmp_path[MAX_PATH_LEN] = "\0";
			sprintf(tmp_path, "%s/%s", path, entry->d_name);

			/* File discrimination check #1: Is this an actual file? */
            if (validate_file(tmp_path))
            {
                copy_to_tmp(tmp_path, id);
                found = true;
            }
		}
	}

	if (dp) closedir(dp);
    return found;

}

bool scancode_mine_attribution_notice(struct minr_job *job)
{
    char *csv_path = NULL;
    /* Assemble csv path */
    if (!job->local_mining)
        asprintf(&csv_path, "%s/licenses.csv", job->mined_path);
    
    prepare_tmp_dir(job->purlid);
    
    if (find_files(job->tmp_dir, job->purlid))
        scancode_run(job->purlid, csv_path);

    free(csv_path);

    return true;
}