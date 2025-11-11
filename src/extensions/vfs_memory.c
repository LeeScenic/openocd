/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Memory Virtual File System for OpenOCD                                *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vfs_memory.h"
#include "vfs_crypto.h"
#include <helper/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

/* Hash table for fast file lookup */
#define VFS_HASH_SIZE 256

struct vfs_file_entry {
	char *virtual_path;      /* Virtual path (key) */
	char *content;           /* File content */
	size_t content_size;     /* Size of content */
	struct vfs_file_entry *next;  /* Next entry in hash bucket */
};

static struct vfs_file_entry *vfs_hash_table[VFS_HASH_SIZE];
static int vfs_file_count = 0;
static size_t vfs_total_size = 0;
static bool vfs_initialized = false;
static bool vfs_debug = false;

/* Simple hash function */
static unsigned int vfs_hash(const char *str)
{
	unsigned int hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash % VFS_HASH_SIZE;
}

/* Normalize path: convert backslashes to forward slashes, remove ./ and trailing slashes */
static char *vfs_normalize_path(const char *path)
{
	if (!path)
		return NULL;

	size_t len = strlen(path);
	char *normalized = malloc(len + 1);
	if (!normalized)
		return NULL;

	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		if (path[i] == '\\')
			normalized[j++] = '/';
		else if (path[i] == '/' && i + 1 < len && path[i + 1] == '/')
			continue; /* Skip duplicate slashes */
		else
			normalized[j++] = path[i];
	}
	normalized[j] = '\0';

	/* Remove trailing slash */
	if (j > 1 && normalized[j - 1] == '/')
		normalized[j - 1] = '\0';

	/* Remove leading ./ */
	if (j > 2 && normalized[0] == '.' && normalized[1] == '/') {
		char *temp = strdup(normalized + 2);
		free(normalized);
		return temp;
	}

	return normalized;
}

int vfs_memory_init(void)
{
	if (vfs_initialized) {
		LOG_WARNING("VFS memory already initialized");
		return 0;
	}

	memset(vfs_hash_table, 0, sizeof(vfs_hash_table));
	vfs_file_count = 0;
	vfs_total_size = 0;
	vfs_initialized = true;

	if (vfs_debug)
		LOG_INFO("Memory VFS initialized");

	return 0;
}

void vfs_memory_cleanup(void)
{
	if (!vfs_initialized)
		return;

	for (int i = 0; i < VFS_HASH_SIZE; i++) {
		struct vfs_file_entry *entry = vfs_hash_table[i];
		while (entry) {
			struct vfs_file_entry *next = entry->next;
			free(entry->virtual_path);
			free(entry->content);
			free(entry);
			entry = next;
		}
		vfs_hash_table[i] = NULL;
	}

	vfs_file_count = 0;
	vfs_total_size = 0;
	vfs_initialized = false;

	if (vfs_debug)
		LOG_INFO("Memory VFS cleaned up");
}

int vfs_memory_add_file(const char *virtual_path, const char *content, size_t content_size)
{
	if (!vfs_initialized) {
		LOG_ERROR("VFS memory not initialized");
		return -1;
	}

	if (!virtual_path || !content) {
		LOG_ERROR("Invalid arguments to vfs_memory_add_file");
		return -1;
	}

	char *normalized_path = vfs_normalize_path(virtual_path);
	if (!normalized_path) {
		LOG_ERROR("Failed to normalize path: %s", virtual_path);
		return -1;
	}

	unsigned int hash = vfs_hash(normalized_path);

	/* Check if file already exists */
	struct vfs_file_entry *entry = vfs_hash_table[hash];
	while (entry) {
		if (strcmp(entry->virtual_path, normalized_path) == 0) {
			/* File exists, update content */
			char *new_content = malloc(content_size);
			if (!new_content) {
				free(normalized_path);
				LOG_ERROR("Out of memory");
				return -1;
			}
			memcpy(new_content, content, content_size);

			vfs_total_size -= entry->content_size;
			free(entry->content);
			entry->content = new_content;
			entry->content_size = content_size;
			vfs_total_size += content_size;

			if (vfs_debug)
				LOG_DEBUG("VFS: Updated file '%s' (%zu bytes)", normalized_path, content_size);

			free(normalized_path);
			return 0;
		}
		entry = entry->next;
	}

	/* Create new entry */
	struct vfs_file_entry *new_entry = malloc(sizeof(struct vfs_file_entry));
	if (!new_entry) {
		free(normalized_path);
		LOG_ERROR("Out of memory");
		return -1;
	}

	new_entry->content = malloc(content_size);
	if (!new_entry->content) {
		free(new_entry);
		free(normalized_path);
		LOG_ERROR("Out of memory");
		return -1;
	}

	memcpy(new_entry->content, content, content_size);
	new_entry->virtual_path = normalized_path;
	new_entry->content_size = content_size;
	new_entry->next = vfs_hash_table[hash];
	vfs_hash_table[hash] = new_entry;

	vfs_file_count++;
	vfs_total_size += content_size;

	if (vfs_debug)
		LOG_DEBUG("VFS: Added file '%s' (%zu bytes)", normalized_path, content_size);

	return 0;
}

int vfs_memory_read_file(const char *virtual_path, const char **content_out, size_t *size_out)
{
	if (!vfs_initialized)
		return -1;

	if (!virtual_path || !content_out || !size_out)
		return -1;

	char *normalized_path = vfs_normalize_path(virtual_path);
	if (!normalized_path)
		return -1;

	unsigned int hash = vfs_hash(normalized_path);
	struct vfs_file_entry *entry = vfs_hash_table[hash];

	while (entry) {
		if (strcmp(entry->virtual_path, normalized_path) == 0) {
			*content_out = entry->content;
			*size_out = entry->content_size;
			free(normalized_path);

			if (vfs_debug)
				LOG_DEBUG("VFS: Read file '%s' (%zu bytes)", virtual_path, entry->content_size);

			return 0;
		}
		entry = entry->next;
	}

	free(normalized_path);
	return -1;
}

bool vfs_memory_file_exists(const char *virtual_path)
{
	if (!vfs_initialized)
		return false;

	if (!virtual_path)
		return false;

	char *normalized_path = vfs_normalize_path(virtual_path);
	if (!normalized_path)
		return false;

	unsigned int hash = vfs_hash(normalized_path);
	struct vfs_file_entry *entry = vfs_hash_table[hash];

	while (entry) {
		if (strcmp(entry->virtual_path, normalized_path) == 0) {
			free(normalized_path);
			return true;
		}
		entry = entry->next;
	}

	free(normalized_path);
	return false;
}

int vfs_memory_remove_file(const char *virtual_path)
{
	if (!vfs_initialized)
		return -1;

	if (!virtual_path)
		return -1;

	char *normalized_path = vfs_normalize_path(virtual_path);
	if (!normalized_path)
		return -1;

	unsigned int hash = vfs_hash(normalized_path);
	struct vfs_file_entry *entry = vfs_hash_table[hash];
	struct vfs_file_entry *prev = NULL;

	while (entry) {
		if (strcmp(entry->virtual_path, normalized_path) == 0) {
			if (prev)
				prev->next = entry->next;
			else
				vfs_hash_table[hash] = entry->next;

			vfs_total_size -= entry->content_size;
			vfs_file_count--;

			free(entry->virtual_path);
			free(entry->content);
			free(entry);
			free(normalized_path);

			if (vfs_debug)
				LOG_DEBUG("VFS: Removed file '%s'", virtual_path);

			return 0;
		}
		prev = entry;
		entry = entry->next;
	}

	free(normalized_path);
	return -1;
}

static int vfs_load_file_from_disk(const char *file_path, const char *base_path)
{
	FILE *fp = fopen(file_path, "rb");
	if (!fp) {
		LOG_ERROR("Failed to open file: %s", file_path);
		return -1;
	}

	/* Get file size */
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (file_size < 0) {
		fclose(fp);
		LOG_ERROR("Failed to get file size: %s", file_path);
		return -1;
	}

	/* Read file content */
	char *content = malloc(file_size);
	if (!content) {
		fclose(fp);
		LOG_ERROR("Out of memory");
		return -1;
	}

	size_t read_size = fread(content, 1, file_size, fp);
	fclose(fp);

	if (read_size != (size_t)file_size) {
		free(content);
		LOG_ERROR("Failed to read file: %s", file_path);
		return -1;
	}

	/* Check if file is encrypted (.bin extension) and decrypt if necessary */
	size_t final_size = file_size;
	char *final_content = content;
	bool is_encrypted_file = false;

	/* Check if file has .bin extension */
	const char *ext = strrchr(file_path, '.');
	if (ext && strcmp(ext, ".bin") == 0) {
		is_encrypted_file = true;
		LOG_DEBUG("VFS: Detected encrypted file: %s", file_path);

		/* Try to decrypt if crypto is enabled */
		if (vfs_crypto_is_enabled()) {
			unsigned char *decrypted = NULL;
			size_t decrypted_len = 0;

			if (vfs_crypto_decrypt((unsigned char *)content, file_size,
			                       &decrypted, &decrypted_len) == 0) {
				/* Decryption successful */
				LOG_INFO("VFS: Successfully decrypted file: %s (%zu -> %zu bytes)",
				         file_path, file_size, decrypted_len);
				
				/* Replace content with decrypted data */
				free(content);
				final_content = (char *)decrypted;
				final_size = decrypted_len;
			} else {
				/* Decryption failed */
				LOG_WARNING("VFS: Failed to decrypt file: %s (will try to use as-is)", file_path);
				/* Continue with original content */
			}
		} else {
			LOG_WARNING("VFS: Encrypted file detected but crypto not enabled: %s", file_path);
			/* Continue with original content */
		}
	}

	/* Calculate virtual path (relative to base_path) */
	if (base_path) {
		size_t base_len = strlen(base_path);
		if (strncmp(file_path, base_path, base_len) == 0) {
			/* Extract the directory name from base_path */
			const char *dir_name = strrchr(base_path, '/');
			if (!dir_name)
				dir_name = strrchr(base_path, '\\');
			if (!dir_name)
				dir_name = base_path;
			else
				dir_name++;  /* Skip the slash */
			
			/* Get the relative path after base_path */
			const char *rel_path = file_path + base_len;
			while (*rel_path == '/' || *rel_path == '\\')
				rel_path++;
			
			/* Build virtual path as: dir_name/rel_path */
			/* If encrypted, remove .bin extension from virtual path */
			char *vpath_rel = strdup(rel_path);
			if (is_encrypted_file && vfs_crypto_is_enabled()) {
				char *bin_ext = strstr(vpath_rel, ".bin");
				if (bin_ext && bin_ext[4] == '\0') {
					*bin_ext = '\0';  /* Remove .bin extension */
				}
			}
			
			size_t vpath_len = strlen(dir_name) + strlen(vpath_rel) + 2;
			char *vpath = malloc(vpath_len);
			if (vpath) {
				snprintf(vpath, vpath_len, "%s/%s", dir_name, vpath_rel);
				int result = vfs_memory_add_file(vpath, final_content, final_size);
				free(vpath);
				free(vpath_rel);
				free(final_content);
				return result;
			} else {
				free(vpath_rel);
				free(final_content);
				LOG_ERROR("Out of memory");
				return -1;
			}
		}
	}

	/* Fallback: use file_path as is */
	/* If encrypted, remove .bin extension from virtual path */
	const char *vpath_to_use = file_path;
	char *vpath_copy = NULL;
	if (is_encrypted_file && vfs_crypto_is_enabled()) {
		vpath_copy = strdup(file_path);
		if (vpath_copy) {
			char *bin_ext = strstr(vpath_copy, ".bin");
			if (bin_ext && bin_ext[4] == '\0') {
				*bin_ext = '\0';  /* Remove .bin extension */
				vpath_to_use = vpath_copy;
			}
		}
	}
	
	int result = vfs_memory_add_file(vpath_to_use, final_content, final_size);
	free(final_content);
	if (vpath_copy)
		free(vpath_copy);

	return result;
}

static int vfs_load_directory_recursive(const char *dir_path, const char *base_path)
{
	DIR *dir = opendir(dir_path);
	if (!dir) {
		LOG_ERROR("Failed to open directory: %s", dir_path);
		return -1;
	}

	struct dirent *entry;
	int loaded = 0;

	while ((entry = readdir(dir)) != NULL) {
		/* Skip . and .. */
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		/* Build full path */
		size_t path_len = strlen(dir_path) + strlen(entry->d_name) + 2;
		char *full_path = malloc(path_len);
		if (!full_path) {
			LOG_ERROR("Out of memory");
			closedir(dir);
			return -1;
		}
		snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);

		struct stat st;
		if (stat(full_path, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				/* Recursively load subdirectory */
				if (vfs_load_directory_recursive(full_path, base_path ? base_path : dir_path) == 0)
					loaded++;
			} else if (S_ISREG(st.st_mode)) {
				/* Load regular file */
				if (vfs_load_file_from_disk(full_path, base_path ? base_path : dir_path) == 0)
					loaded++;
			}
		}

		free(full_path);
	}

	closedir(dir);
	return loaded > 0 ? 0 : -1;
}

int vfs_memory_load_directory(const char *directory_path)
{
	if (!vfs_initialized) {
		LOG_ERROR("VFS memory not initialized");
		return -1;
	}

	if (!directory_path) {
		LOG_ERROR("Invalid directory path");
		return -1;
	}

	struct stat st;
	if (stat(directory_path, &st) != 0) {
		LOG_ERROR("Directory does not exist: %s", directory_path);
		return -1;
	}

	if (!S_ISDIR(st.st_mode)) {
		LOG_ERROR("Not a directory: %s", directory_path);
		return -1;
	}

	LOG_INFO("Loading directory into memory VFS: %s", directory_path);

	int result = vfs_load_directory_recursive(directory_path, directory_path);

	if (result == 0) {
		LOG_INFO("VFS: Loaded directory '%s' (%d files, %zu bytes total)",
			directory_path, vfs_file_count, vfs_total_size);
	}

	return result;
}

void vfs_memory_get_stats(int *file_count_out, size_t *total_size_out)
{
	if (file_count_out)
		*file_count_out = vfs_file_count;
	if (total_size_out)
		*total_size_out = vfs_total_size;
}

void vfs_memory_set_debug(bool enable)
{
	vfs_debug = enable;
	if (enable)
		LOG_INFO("VFS memory debug enabled");
}
