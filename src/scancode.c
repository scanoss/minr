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

#include "scancode.h"

bool scancode_run(char * path)
{
	FILE *sc_file;
	char * command;
	asprintf(&command, "scancode -cl --quiet -n 6 --timeout 2 --json scancode.json %s", path);
	printf(command);
	sc_file = popen( command, "r");
	/*
	char c;
	while ((c = getc(sc_file)) != EOF)
    putchar(c);*/
    fclose(sc_file);
	free(command);
    return true;
}
