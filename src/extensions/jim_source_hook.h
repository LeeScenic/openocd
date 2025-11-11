/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Jim Tcl 'source' command hook for OpenOCD Memory VFS                  *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifndef OPENOCD_HELPER_JIM_SOURCE_HOOK_H
#define OPENOCD_HELPER_JIM_SOURCE_HOOK_H

#include <jim.h>

/**
 * @file jim_source_hook.h
 * @brief Hook for Jim Tcl 'source' command to support memory VFS
 *
 * This module replaces Jim Tcl's built-in 'source' command with a version
 * that checks memory VFS before accessing the file system.
 */

/**
 * Install the source command hook into Jim Tcl interpreter.
 * This replaces the built-in 'source' command with our VFS-aware version.
 *
 * @param interp  Jim Tcl interpreter
 * @return 0 on success, -1 on error
 */
int jim_source_hook_install(Jim_Interp *interp);

#endif /* OPENOCD_HELPER_JIM_SOURCE_HOOK_H */
