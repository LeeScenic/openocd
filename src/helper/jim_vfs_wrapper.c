/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Jim Tcl VFS Wrapper for OpenOCD                                       *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jim_vfs_wrapper.h"
#include "vfs_memory.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

int Jim_EvalFile_VFS(Jim_Interp *interp, const char *filename)
{
	if (!filename)
		return Jim_EvalFile(interp, filename);

	/* Try to read from memory VFS first */
	const char *content;
	size_t content_size;

	if (vfs_memory_read_file(filename, &content, &content_size) == 0) {
		/* File found in memory VFS, evaluate it */
		LOG_DEBUG("VFS: Evaluating '%s' from memory (%zu bytes)", filename, content_size);

		/* Create a Jim object from the content */
		Jim_Obj *scriptObj = Jim_NewStringObj(interp, content, content_size);
		Jim_IncrRefCount(scriptObj);

		/* Set source info for better error messages */
		Jim_Obj *filenameObj = Jim_NewStringObj(interp, filename, -1);
		Jim_IncrRefCount(filenameObj);
		Jim_SetSourceInfo(interp, scriptObj, filenameObj, 1);
		Jim_DecrRefCount(interp, filenameObj);

		/* Evaluate the script */
		int retcode = Jim_EvalObj(interp, scriptObj);
		Jim_DecrRefCount(interp, scriptObj);

		/* Handle JIM_RETURN */
		if (retcode == JIM_RETURN) {
			if (--interp->returnLevel <= 0) {
				retcode = interp->returnCode;
				interp->returnCode = JIM_OK;
				interp->returnLevel = 0;
			}
		}

		return retcode;
	}

	/* File not in memory VFS, fallback to disk */
	LOG_DEBUG("VFS: File '%s' not in memory, reading from disk", filename);
	return Jim_EvalFile(interp, filename);
}

int Jim_EvalFileGlobal_VFS(Jim_Interp *interp, const char *filename)
{
	if (!filename)
		return Jim_EvalFileGlobal(interp, filename);

	/* Try to read from memory VFS first */
	const char *content;
	size_t content_size;

	if (vfs_memory_read_file(filename, &content, &content_size) == 0) {
		/* File found in memory VFS, evaluate it in global scope */
		LOG_DEBUG("VFS: Evaluating '%s' from memory in global scope (%zu bytes)",
			filename, content_size);

		int savedepth = interp->framePtr->level;
		Jim_Obj *prevScriptObj;
		int retcode;

		Jim_Obj *scriptObj = Jim_NewStringObj(interp, content, content_size);
		Jim_IncrRefCount(scriptObj);

		/* Set source info */
		Jim_Obj *filenameObj = Jim_NewStringObj(interp, filename, -1);
		Jim_IncrRefCount(filenameObj);
		Jim_SetSourceInfo(interp, scriptObj, filenameObj, 1);
		Jim_DecrRefCount(interp, filenameObj);

		/* Evaluate in global frame */
		interp->framePtr = interp->topFramePtr;
		prevScriptObj = interp->currentScriptObj;
		interp->currentScriptObj = scriptObj;

		retcode = Jim_EvalObj(interp, scriptObj);

		interp->currentScriptObj = prevScriptObj;

		/* Restore the original frame */
		while (interp->framePtr->level != savedepth) {
			struct Jim_CallFrame *frame = interp->framePtr;
			interp->framePtr = frame->parent;
		}

		Jim_DecrRefCount(interp, scriptObj);

		/* Handle JIM_RETURN */
		if (retcode == JIM_RETURN) {
			if (--interp->returnLevel <= 0) {
				retcode = interp->returnCode;
				interp->returnCode = JIM_OK;
				interp->returnLevel = 0;
			}
		}

		return retcode;
	}

	/* File not in memory VFS, fallback to disk */
	LOG_DEBUG("VFS: File '%s' not in memory, reading from disk (global)", filename);
	return Jim_EvalFileGlobal(interp, filename);
}
