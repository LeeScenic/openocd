/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Memory Virtual File System for OpenOCD                                *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifndef OPENOCD_HELPER_VFS_MEMORY_H
#define OPENOCD_HELPER_VFS_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file vfs_memory.h
 * @brief In-memory virtual file system for protecting TCL script content
 *
 * This module provides a simple in-memory file system that allows OpenOCD
 * to load TCL scripts into memory at startup and serve them from memory
 * instead of disk, preventing content leakage.
 */

/**
 * Initialize the memory VFS subsystem.
 * Must be called before using any other vfs_memory functions.
 *
 * @return 0 on success, -1 on error
 */
int vfs_memory_init(void);

/**
 * Cleanup and free all memory VFS resources.
 * Should be called during OpenOCD shutdown.
 */
void vfs_memory_cleanup(void);

/**
 * Load all files from a directory into memory VFS recursively.
 * 
 * @param directory_path Path to the directory to load
 * @return 0 on success, -1 on error
 */
int vfs_memory_load_directory(const char *directory_path);

/**
 * Add a single file to memory VFS.
 *
 * @param virtual_path  Virtual path for the file in memory (e.g., "tcl/target/stm32f4x.cfg")
 * @param content       File content (will be copied)
 * @param content_size  Size of the content in bytes
 * @return 0 on success, -1 on error
 */
int vfs_memory_add_file(const char *virtual_path, const char *content, size_t content_size);

/**
 * Read a file from memory VFS.
 *
 * @param virtual_path  Virtual path of the file
 * @param content_out   Output pointer to file content (do not free, owned by VFS)
 * @param size_out      Output pointer to content size
 * @return 0 on success, -1 if file not found
 */
int vfs_memory_read_file(const char *virtual_path, const char **content_out, size_t *size_out);

/**
 * Check if a file exists in memory VFS.
 *
 * @param virtual_path  Virtual path to check
 * @return true if file exists in memory, false otherwise
 */
bool vfs_memory_file_exists(const char *virtual_path);

/**
 * Remove a file from memory VFS.
 *
 * @param virtual_path  Virtual path of the file to remove
 * @return 0 on success, -1 if file not found
 */
int vfs_memory_remove_file(const char *virtual_path);

/**
 * Get statistics about memory VFS usage.
 *
 * @param file_count_out    Output pointer to number of files in VFS
 * @param total_size_out    Output pointer to total size of all files
 */
void vfs_memory_get_stats(int *file_count_out, size_t *total_size_out);

/**
 * Enable or disable VFS debug logging.
 *
 * @param enable  true to enable debug logging, false to disable
 */
void vfs_memory_set_debug(bool enable);

#endif /* OPENOCD_HELPER_VFS_MEMORY_H */
