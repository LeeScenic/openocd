/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Jim Tcl 'source' command hook for OpenOCD Memory VFS                  *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jim_source_hook.h"
#include "vfs_memory.h"
#include "../helper/log.h"
#include <string.h>

/* Reference to the original Jim 'source' command */
static Jim_Cmd *original_source_cmd = NULL;

/**
 * VFS-aware 'source' command implementation
 */
static int jim_source_vfs_command(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	if (argc != 2) {
		Jim_WrongNumArgs(interp, 1, argv, "fileName");
		return JIM_ERR;
	}

	const char *filename = Jim_String(argv[1]);
	const char *content;
	size_t content_size;

	/* Try to read from memory VFS first */
	if (vfs_memory_read_file(filename, &content, &content_size) == 0) {
		LOG_DEBUG("VFS: Sourcing '%s' from memory (%zu bytes)", filename, content_size);

		/* Create a Jim object from the content */
		Jim_Obj *scriptObj = Jim_NewStringObj(interp, content, content_size);
		Jim_IncrRefCount(scriptObj);

		/* Set source info for better error messages */
		Jim_Obj *filenameObj = Jim_NewStringObj(interp, filename, -1);
		Jim_IncrRefCount(filenameObj);
		Jim_SetSourceInfo(interp, scriptObj, filenameObj, 1);
		Jim_DecrRefCount(interp, filenameObj);

		/* Push current filename */
		Jim_Obj *oldFilenameObj = interp->currentFilenameObj;
		interp->currentFilenameObj = Jim_NewStringObj(interp, filename, -1);
		Jim_IncrRefCount(interp->currentFilenameObj);

		/* Evaluate the script */
		int retcode = Jim_EvalObj(interp, scriptObj);

		/* Restore filename */
		Jim_DecrRefCount(interp, interp->currentFilenameObj);
		interp->currentFilenameObj = oldFilenameObj;

		Jim_DecrRefCount(interp, scriptObj);

		/* Handle JIM_RETURN */
		if (retcode == JIM_RETURN) {
			if (--interp->returnLevel <= 0) {
				retcode = interp->returnCode;
				interp->returnCode = JIM_OK;
				interp->returnLevel = 0;
			}
			return JIM_OK;
		}

		return retcode;
	}

	/* File not in memory VFS, call original source command */
	LOG_DEBUG("VFS: File '%s' not in memory, using original source command", filename);

	if (original_source_cmd && original_source_cmd->u.native.cmdProc) {
		return original_source_cmd->u.native.cmdProc(interp, argc, argv);
	}

	/* Fallback: try Jim_EvalFile directly */
	int retval = Jim_EvalFile(interp, filename);
	if (retval == JIM_RETURN)
		return JIM_OK;
	return retval;
}

int jim_source_hook_install(Jim_Interp *interp)
{
	if (!interp) {
		LOG_ERROR("Invalid Jim interpreter");
		return -1;
	}

	/* Get the original source command */
	original_source_cmd = Jim_GetCommand(interp, Jim_NewStringObj(interp, "source", -1), JIM_NONE);
	if (!original_source_cmd) {
		LOG_WARNING("Could not find original 'source' command");
		/* Not a fatal error, we can still create our own */
	} else {
		LOG_DEBUG("VFS: Found original 'source' command, saving reference");
	}

	/* Create our VFS-aware source command */
	Jim_CreateCommand(interp, "source", jim_source_vfs_command, NULL, NULL);

	LOG_INFO("Memory VFS source hook installed");

	return 0;
}
