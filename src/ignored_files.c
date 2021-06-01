// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/ignore_file.c
 *
 * Big blob with file IDs to ignore
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

#include <stdbool.h>
#include "minr.h"

char IGNORE_FILE[] = "\
0962291d6d367570bee5454721c17e11\
0f3006e20d30987b4fdd49c4a0adf342\
2c58c801945b382ae168280eb80092b6\
327711a461a30ba31b918dc30a08ccae\
34ea47c60598f102697b4476b742f873\
375ddea382b6c56a7be2a967a20e0ab5\
3a55d95595a6f9e37dee53826b4daff2\
41876349cb12d6db992f1309f22df3f0\
42679f369206e3da1495ed8abf870cee\
443141a90f711430c1423855b273d2fa\
448c34a56d699c29117adc64c43affeb\
4a010b8264c9873452f055748133bb29\
54542dc54ebae0de4502a1319144538f\
72b0b8f922dc7f125c7aaed79f9d274a\
80cb6cf2fe634fe0eaa8a32ce59bee73\
8111d301bde6dd4875417be5a9d12fd5\
8f68037672effa2917b28650c7cbce1f\
93fb6cea8fc778af4030da1ceb7d4f60\
96ba0a444d087ae06f32319ca4f0a3e4\
97471e1a0beb9650905b574145ae4638\
9c64fec71a86faf860632aca64da7ef1\
a670520d9d36833b3e28d1e4b73cbe22\
abd1ac26aaea8b55ff1eefd6caca1e55\
b03f60eea43420aef3ecd1f96c455d97\
c9eb740112b98a4ffa4b6e8965caa81f\
d3e3748e06793d488ae92138dd974ef8\
de8ffdb592ab304b14832467aef77cb3\
e24114dd1624e26140c00c0d49ba9837\
e909b282419acd3b63f5fb68db3bbc9b\
f10f4e8a24a6f29f60f73caf7341f44d\
fdb446fc9fbdb44dd511ce3d449504b0";

bool ignored_file(char *file_id)
{
	char *ignore_id = IGNORE_FILE;
	while (*ignore_id)
	{
		if (!memcmp(file_id, ignore_id, MD5_LEN_HEX)) return true;
		ignore_id += MD5_LEN_HEX;
	}
	return false;
}
