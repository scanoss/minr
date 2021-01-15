
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include "minr.h"
#include "file.h"

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

/* If the CSV file does not end with LF, eliminate the last line */
void truncate_csv(char *path)
{

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
	if (!size)
	{
		fclose(file);
		return;
	}

	/* Read last byte */	
	uint8_t last_byte[1] = "\0";
	fseeko64(file, size - 1, SEEK_SET);
	fread(last_byte, 1, 1, file);

	/* Ends with chr(10), it is ok */
	if (*last_byte == 10)
	{
		fclose(file);
		return;
	}

	printf("Truncated %s\n", path);

	int i = 0;
	for (i = size - 1; i > 0; i--)
	{

		fseeko64(file, i, SEEK_SET);
		fread(last_byte, 1, 1, file);
		if (*last_byte == 10) break;
	}

	fclose(file);
	if (i > 0) truncate(path, i + 1);
	return;
}

void mkdir_if_not_exist(char *destination)
{
	char *dst_dir = strdup(destination);
	char *dir = dirname(dst_dir);

	if (is_dir(dir))
	{
		free(dst_dir);
		return;
	}

	mkdir(dir, 0755);
	if (!is_dir(dir))
	{
		printf("Cannot create directory %s\n", dst_dir);
		free(dst_dir);
		exit(EXIT_FAILURE);
	}

	free(dst_dir);
}

bool move_file(char *src, char *dst) {
		
	mkdir_if_not_exist(dst);
		
	FILE *srcf = fopen(src, "rb");
	if (!srcf) return false;

	FILE *dstf = fopen(dst, "wb");
	if (!dstf) return false;

	/* Copy byte by byte */
	int byte = 0;
	while (!feof(srcf))
	{
		byte = fgetc(srcf);
		if (feof(srcf)) break;
		fputc(byte, dstf);
	}

	fclose(srcf);
	fclose(dstf);
	unlink(src);
	return true;
}

void bin_join(char *source, char *destination, bool snippets)
{
	/* If source does not exist, no need to join */
	if (!is_file(source)) return;

	if (is_file(destination))
	{
		/* Snippet records should divide by 21 */
		if (snippets) if (file_size(destination) % 21)
		{
			printf("File %s does not contain 21-byte records\n", destination);
			exit(EXIT_FAILURE);
		}
	}

	/* If destination does not exist. Source is moved */
	else
	{
		printf("Moving %s into %s\n", source, destination);
		if (!move_file(source, destination))
		{
			printf("Cannot move file\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	printf("Joining into %s\n", destination);
	file_append(source, destination);
	unlink(source);
}

void csv_join(char *source, char *destination)
{
	/* If source does not exist, no need to join */
	if (is_file(source)) truncate_csv(source); else return;

	if (is_file(destination))
	{	
		truncate_csv(destination);
	}

	/* If destination does not exist. Source is moved */
	else
	{
		printf("Moving into %s\n", destination);
		if (!move_file(source, destination))
		{
			printf("Cannot move file\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	printf("Joining into %s\n", destination);
	file_append(source, destination);
	unlink(source);
}

/* Join mz sources */
void minr_join_mz(char *source, char *destination)
{
	char src_path[MAX_PATH_LEN] = "\0";
	char dst_path[MAX_PATH_LEN] = "\0";

	for (int i = 0; i < 65536; i++)
	{
		sprintf(src_path, "%s/sources/%04x.mz", source, i);
		sprintf(dst_path, "%s/sources/%04x.mz", destination, i);
		bin_join(src_path, dst_path, false);
	}
	sprintf(src_path, "%s/sources", source);
	rmdir(src_path);
}

/* Join snippets */
void minr_join_snippets(char *source, char *destination)
{
	char src_path[MAX_PATH_LEN] = "\0";
	char dst_path[MAX_PATH_LEN] = "\0";

	for (int i = 0; i < 256; i++)
	{
		sprintf(src_path, "%s/snippets/%02x.bin", source, i);
		sprintf(dst_path, "%s/snippets/%02x.bin", destination, i);
		bin_join(src_path, dst_path, true);
	}
	sprintf(src_path, "%s/snippets", source);
	rmdir(src_path);
}

void minr_join(char *source, char *destination)
{
	if (!is_dir(source) || !is_dir(destination))
	{
		printf("Source and destination must be mined/ directories\n");
		exit(EXIT_FAILURE);
	}

	if (!strcmp(source, destination))
	{
		printf("Source and destination cannot be the same\n");
		exit(EXIT_FAILURE);
	}

	char src_path[MAX_PATH_LEN] = "\0";
	char dst_path[MAX_PATH_LEN] = "\0";

	/* Join components */
	sprintf(src_path, "%s/components.csv", source);
	sprintf(dst_path, "%s/components.csv", destination);
	csv_join(src_path, dst_path);

	/* Join files */
	for (int i = 0; i < 256; i++)
	{
		sprintf(src_path, "%s/files/%02x.csv", source, i);
		sprintf(dst_path, "%s/files/%02x.csv", destination, i);
		csv_join(src_path, dst_path);
	}
	sprintf(src_path, "%s/files", source);
	rmdir(src_path);

	/* Join snippets */
	minr_join_snippets(source, destination);

	/* Join MZ */
	minr_join_mz(source, destination);

	/* Join licenses */
	sprintf(src_path, "%s/licenses.csv", source);
	sprintf(dst_path, "%s/licenses.csv", destination);
	csv_join(src_path, dst_path);

	/* Join dependencies */
	sprintf(src_path, "%s/dependencies.csv", source);
	sprintf(dst_path, "%s/dependencies.csv", destination);
	csv_join(src_path, dst_path);

	rmdir(source);
}
