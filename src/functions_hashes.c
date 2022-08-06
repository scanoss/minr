// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/function_hashes.c
 *
 * SCANOSS Functions hash 
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

/* Calls libbinhash library to create functions hash layer */

/**
  * @file functions_hash.c
  * @date 7 Feb 2021 
  * @brief Contains functions used to mine quality
  */

#include "minr.h"
#include "libbinhash.h"
#include "functions_hashes.h"

/**
 * Calls libbinhahes library and retrieves hashes from source
 * @param source_name Original filename incluing extension
 * @param mined_path Path to place the mined results
 * @param md5 MD5 checksum of the mined file
 * @param src Source code text of the mined file
 * @param size Length of the mined size. Unused, just only to keep prototype compatibility
 */
void mine_functions_hashes(char *source_name, char *mined_path, char *md5, char *src, long size)
{    
  Load_functions_blacklist();
  char *hashes=HashFileContents(source_name,mined_path,md5,src);
  free(hashes);
}
