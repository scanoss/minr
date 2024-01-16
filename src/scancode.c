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

#include "license.h"
#include "scancode.h"
#include "file.h"
#define TMP_DIR "/tmp/minr"

/**
 * @brief Preparare the directory for temporary file
 *
 * @param id string id used as directory name
 */
bool scancode_prepare_tmp_dir(char *id)
{
    FILE *sc_file;
    char *command;
    asprintf(&command, "mkdir -p %s/%s && rm -f %s/%s/*", TMP_DIR, id, TMP_DIR, id); // create tmp or erase content
    sc_file = popen(command, "r");

    fclose(sc_file);
    free(command);
    return true;
}

/**
 * @brief Remove the directory for temporary file
 *
 * @param id string id used as directory name
 */
bool scancode_remove_tmp_dir(char *id)
{
    char *path;
    asprintf(&path, "%s/%s", TMP_DIR, id); // create tmp or erase content
    rm_dir(path);
    free(path);
    return true;
}

/**
 * @brief Copy a file to the tmp dir
 *
 * @param path path to the file
 * @param id id of the tmp dir
 */
bool scancode_copy_to_tmp(char *path, char *id)
{
    FILE *sc_file;
    char *command;
    asprintf(&command, "cp %s %s/%s/", path, TMP_DIR, id); // copy
    sc_file = popen(command, "r");

    fclose(sc_file);
    free(command);
    return true;
}

/**
 * @brief Call scancode process
 * @param id id of the tmp dir
 * @param csv_file result file
 */
bool scancode_run(char *id, char *csv_file)
{
    char *command;
    /* execute scancode, process the json output, and erase tmp */
    if (csv_file)
        asprintf(&command, "scancode -cl --quiet -n 6 --timeout 2 --json %s/%s/scancode.json %s/%s  2> scancode_error.txt &&\
	 	    				jq -r '.files[] | \"\\(.path),5,\\(.detected_license_expression_spdx)\"' %s/%s/scancode.json | sort -u | grep -v ',null$' | sed 's/\\/[^:/:,]*,/,/g' 1>> %s",
                 TMP_DIR, id, TMP_DIR, id, TMP_DIR, id, csv_file);

    FILE *sc_file = popen(command, "r");
    fclose(sc_file);
    free(command);
    return true;
}

/**
 * @brief Check is a version of scancode is present in the system
 * @return true if it is present
 */
bool scancode_check(void)
{
    FILE *sc_file;
    sc_file = popen("scancode --version 2>scancode_error.txt && jq --version 2>scancode_error.txt", "r");
    char *line = NULL;
    size_t len = 0;
    int read = 0;
    bool scancode_present = false;
    bool jq_present = false;

    while ((read = getline(&line, &len, sc_file)) != -1)
    {
        if (read > 0 && strstr(line, "ScanCode version"))
        {
            scancode_present = true;
        }

        if (read > 0 && strstr(line, "jq-"))
        {
            jq_present = true;
        }
    }
    free(line);
    fclose(sc_file);

    return (scancode_present && jq_present);
}