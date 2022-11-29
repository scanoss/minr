

/**
  * @file join.c
  * @date 14 Sep 2021 
  * @brief Contains functions used for implentent minr join funtionality
  */


#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include "minr_log.h"
#include "minr.h"
#include "file.h"

/**
 * @brief  If the CSV file does not end with LF, eliminate the last line
 * 
 * @param path string path
 */
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

/**
 * @brief Creates a directory with 0755 permissions.
 * 
 * @param destination destination path
 */
void mkdir_if_not_exist(char *destination)
{
	char *dst_dir = strdup(destination);
	char *dir = dirname(dst_dir);
	if (is_dir(dir))
	{
		free(dst_dir);
		return;
	}
	create_dir(dir);
	if (!is_dir(dir))
	{
		printf("Cannot create directory %s\n", dst_dir);
		free(dst_dir);
		exit(EXIT_FAILURE);
	}

	free(dst_dir);
}

/**
 * @brief Move a file to a new location by copying byte by byte.
 * If the file already exist it is overwritten.
 * 
 * @param src src path
 * @param dst dst path 
 * @param skip_delete if true the src file is not deleted after the copy is done.
 * @return true success. False otherwise.
 */
static bool write_file(char *src, char *dst, char * mode, bool mkdir, bool skip_delete) {
		
	if (mkdir)
	{
		mkdir_if_not_exist(dst);
	}
		
	FILE *srcf = fopen(src, "rb");
	if (!srcf)
	{	
		printf("Cannot open source file %s\n", src);
		exit(EXIT_FAILURE);
	}

	FILE *dstf = fopen(dst, mode);
	if (!dstf)
	{	
		printf("Cannot open destinstion file %s\n", dst);
		exit(EXIT_FAILURE);
	}

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
	if (!skip_delete) unlink(src);
	return true;
}

bool move_file(char *src, char *dst, bool skip_delete)
{
	return write_file(src,dst, "wb", true, skip_delete);
}
/**
 * @brief Append the contents of a file to the end of another file.
 * 
 * @param file Origin of the data to be appended.
 * @param destination path to out file
 */

bool file_append(char *file, char *destination)
{
	return write_file(file,destination, "ab", false, true);
}



/**
 * @brief join two binary files
 * 
 * @param source path to the source file
 * @param destination path to destination file
 * @param snippets true if it is a snippet file
 * @param skip_delete true to avoid deletion
 */
void bin_join(char *source, char *destination, bool snippets, bool skip_delete)
{
	/* If source does not exist, no need to join */
	if (!is_file(source)) 
	{
		minr_log("Error: File %s does not exist\n", source);
		return;
	}

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
		if (!move_file(source, destination, skip_delete))
		{
			printf("Cannot move file\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	printf("Joining into %s\n", destination);
	file_append(source, destination);
	if (!skip_delete) unlink(source);
}

/**
 * @brief Join two csv files
 * 
 * @param source path to source file
 * @param destination path to destination file 
 * @param skip_delete true for skip deletion
 */
void csv_join(char *source, char *destination, bool skip_delete)
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
		if (!move_file(source, destination, skip_delete))
		{
			printf("Cannot move file\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	printf("Joining into %s\n", destination);
	file_append(source, destination);
	if (!skip_delete) unlink(source);
}

/**
 * @brief Join two mz sources
 * 
 * @param source paht to source
 * @param destination  path to destination
 * @param skip_delete true to skip deletion
 */
void minr_join_mz(char * table, char *source, char *destination, bool skip_delete, bool encrypted)
{
	char src_path[MAX_PATH_LEN] = "\0";
	char dst_path[MAX_PATH_LEN] = "\0";

	for (int i = 0; i < 65536; i++)
	{
		sprintf(src_path, "%s/%s/%04x.mz", source, table, i);
		sprintf(dst_path, "%s/%s/%04x.mz", destination, table, i);

		check_file_extension(src_path, encrypted);
		check_file_extension(dst_path, encrypted);

		bin_join(src_path, dst_path, false, skip_delete);
	}
	sprintf(src_path, "%s/%s", table, source);
	if (!skip_delete) rmdir(src_path);
}

/**
 * @brief  Join two snippets file
 * 
 * @param source path to source
 * @param destination path to destination
 * @param skip_delete true to skip deletion
 */
void minr_join_snippets(char *source, char *destination, bool skip_delete)
{
	char src_path[MAX_PATH_LEN] = "\0";
	char dst_path[MAX_PATH_LEN] = "\0";

	for (int i = 0; i < 256; i++)
	{
		sprintf(src_path, "%s/%s/%02x.bin", source, TABLE_NAME_WFP, i);
		sprintf(dst_path, "%s/%s/%02x.bin", destination, TABLE_NAME_WFP, i);
		bin_join(src_path, dst_path, true, skip_delete);
	}
	sprintf(src_path, "%s/%s", source, TABLE_NAME_WFP);
	if (!skip_delete) rmdir(src_path);
}

/**
 * @brief minr join function. Join the files specified in the job
 * 
 * @param job pointer to mnir job
 */
void minr_join(struct minr_job *job)
{
	char *source = job->join_from;
	char *destination = job->join_to;

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

	/* Join urls */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_URL);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_URL);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join files */
	for (int i = 0; i < 256; i++)
	{
		sprintf(src_path, "%s/%s/%02x.csv", source, TABLE_NAME_FILE, i);
		sprintf(dst_path, "%s/%s/%02x.csv", destination,TABLE_NAME_FILE, i);
		csv_join(src_path, dst_path, job->skip_delete);
	}
	sprintf(src_path, "%s/%s", source, TABLE_NAME_FILE);
	if (!job->skip_delete) rmdir(src_path);

	/* Join pivot */
	for (int i = 0; i < 256; i++)
	{
		sprintf(src_path, "%s/%s/%02x.csv", source, TABLE_NAME_PIVOT, i);
		sprintf(dst_path, "%s/%s/%02x.csv", destination, TABLE_NAME_PIVOT, i);
		csv_join(src_path, dst_path, job->skip_delete);
	}
	sprintf(src_path, "%s/%s", source, TABLE_NAME_PIVOT);
	if (!job->skip_delete) rmdir(src_path);

	/* Join snippets */
	minr_join_snippets(source, destination, job->skip_delete);

	/* Join MZ (sources/ and notices/) */
	minr_join_mz(TABLE_NAME_SOURCES, source, destination, job->skip_delete, job->bin_import);
	minr_join_mz(TABLE_NAME_NOTICES, source, destination, job->skip_delete, job->bin_import);

	/* Join licenses */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_LICENSE);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_LICENSE);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join dependencies */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_DEPENDENCY);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_DEPENDENCY);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join quality */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_QUALITY);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_QUALITY);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join copyright */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_COPYRIGHT);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_COPYRIGHT);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join quality */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_QUALITY);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_QUALITY);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join vulnerabilities */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_VULNERABILITY);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_VULNERABILITY);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join attribution */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_ATTRIBUTION);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_ATTRIBUTION);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join cryptography */
	sprintf(src_path, "%s/%s.csv", source, TABLE_NAME_CRYPTOGRAPHY);
	sprintf(dst_path, "%s/%s.csv", destination, TABLE_NAME_CRYPTOGRAPHY);
	csv_join(src_path, dst_path, job->skip_delete);

	/* Join Extra tables */
	sprintf(src_path, "%s/extra", source);
	sprintf(dst_path, "%s/extra", destination);

	if (is_dir(src_path) && is_dir(destination))
	{
		/* Join files */
		for (int i = 0; i < 256; i++)
		{
			sprintf(src_path, "%s/extra/%s/%02x.csv", source, TABLE_NAME_FILE, i);
			sprintf(dst_path, "%s/extra/%s/%02x.csv", destination, TABLE_NAME_FILE, i);
			csv_join(src_path, dst_path, job->skip_delete);
		}
		
		sprintf(src_path, "%s/%s", source, TABLE_NAME_FILE);
		if (!job->skip_delete) 
			rmdir(src_path);
		/* Join Pivot */
		for (int i = 0; i < 256; i++)
		{
			sprintf(src_path, "%s/extra/%s/%02x.csv", source, TABLE_NAME_PIVOT, i);
			sprintf(dst_path, "%s/extra/%s/%02x.csv", destination, TABLE_NAME_PIVOT, i);
			csv_join(src_path, dst_path, job->skip_delete);
		}

		sprintf(src_path, "%s/%s", source, TABLE_NAME_PIVOT);
		if (!job->skip_delete) 
			rmdir(src_path);

		/* Join Extra sources */
		for (int i = 0; i < 65536; i++)
		{
			sprintf(src_path, "%s/extra/%s/%04x.mz", source, TABLE_NAME_SOURCES, i);
			sprintf(dst_path, "%s/extra/%s/%04x.mz", destination, TABLE_NAME_SOURCES, i);
			bin_join(src_path, dst_path, false, job->skip_delete);
		}
		
		sprintf(src_path, "%s/%s", source, TABLE_NAME_SOURCES);
		if (!job->skip_delete) 
			rmdir(src_path);
	}

	if (!job->skip_delete) 
		rmdir(source);
}
