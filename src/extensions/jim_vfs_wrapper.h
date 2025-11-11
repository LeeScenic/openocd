/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Jim Tcl VFS Wrapper for OpenOCD                                       *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifndef OPENOCD_HELPER_JIM_VFS_WRAPPER_H
#define OPENOCD_HELPER_JIM_VFS_WRAPPER_H

#include <jim.h>

/**
 * @file jim_vfs_wrapper.h
 * @brief Wrapper for Jim Tcl file operations to support memory VFS
 *
 * This module wraps Jim Tcl's file operations to check memory VFS first
 * before accessing the actual file system.
 */

/**
 * Wrapper for Jim_EvalFile that checks memory VFS first.
 * If the file exists in memory VFS, it will be evaluated from memory.
 * Otherwise, it falls back to the original Jim_EvalFile.
 *
 * @param interp    Jim Tcl interpreter
 * @param filename  File name to evaluate
 * @return Jim Tcl return code (JIM_OK, JIM_ERR, etc.)
 */
int Jim_EvalFile_VFS(Jim_Interp *interp, const char *filename);

/**
 * Wrapper for Jim_EvalFileGlobal that checks memory VFS first.
 *
 * @param interp    Jim Tcl interpreter
 * @param filename  File name to evaluate
 * @return Jim Tcl return code (JIM_OK, JIM_ERR, etc.)
 */
int Jim_EvalFileGlobal_VFS(Jim_Interp *interp, const char *filename);

#endif /* OPENOCD_HELPER_JIM_VFS_WRAPPER_H */
