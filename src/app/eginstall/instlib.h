/* ***** BEGIN COPYRIGHT BLOCK *****
 *** * Copyright (C) 2005 Red Hat, Inc.
 *** * All rights reserved.
 ***
 *** This library is free software; you can redistribute it and/or
 *** modify it under the terms of the GNU Lesser General Public
 *** License as published by the Free Software Foundation version
 *** 2.1 of the License.
 ***
 *** This library is distributed in the hope that it will be useful,
 *** but WITHOUT ANY WARRANTY; without even the implied warranty of
 *** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *** Lesser General Public License for more details.
 ***
 *** You should have received a copy of the GNU Lesser General Public
 *** License along with this library; if not, write to the Free Software
 *** Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *** ***** END COPYRIGHT BLOCK *****/

#ifndef _INSTLILB_H_
#define _INSTLILB_H_ 1

/* macros for encoding function return codes to pass to the calling
 * process so it can present reasonable UI for when things go wrong */
#define MK_ERR(type,val)   (((type)<<16)|((val)&0xffff))
#define _ABS(x)		((x) < 0 ? (x)*-1:(x))
#define _NORMALIZE(x,base,op) ((x) op (base) ? (x -(base)) : (x))

#define ARG_ERR(x)	MK_ERR(0, x) /* 1, 2, 3 for x */
#define PID_ERR(x)	MK_ERR(1, x) /* returned status */
#define AUTH_ERR(x)	MK_ERR(2, _ABS(_NORMALIZE(x,-6000,<))) /* Mac auth */
#define RDRCNF_ERR(x)	MK_ERR(3, x) /* errno */
#define EXEC_ERR(x)	MK_ERR(4, x) /* errno */
#define RESTART_ERR(x)	MK_ERR(5, x) /* errno */
#define CLEANUP_ERR(x)	MK_ERR(6, x) /* errno */


#define BUFSIZE 32*1024
/* does the target string exist in search string. Case insensitive compare */
int HasString(const char *target,const char *search);

/* is the given file a directory */
int IsDirectory(const char *dir);

/* does the given file exist */
int FileExists(const char *file);

/* Make a directory path. removes regular files found along the path, 
 * recursively builds all the directories needed along the path 
 * (like mkdir -p) */
int Makedir(const char *directory, int level, int mode);

/* find the version number of a bundle */
unsigned long GetBundleVersion(const char *bundle);

/* copy file src to file specified by target */
int Copy(const char *src, const char *target, int mode);

/* copy a directory and all it's contents from source to target.
 * symbolic links are followed */
int Cpdir(const char *source,const char *target);

/* Remove a File... force the removal with chown if necessary */
int RmFile(const char *file);

/* Remove a directory and all it's contents */
int RmDir(const char *directory);

/* expand the path name of a program name. if includeProg is true, the
 * path includes the program name, otherwise it's just the directory path
 * of the program */
char * GetFullPath(const char *prog, int includeProg);

/* find a process id from it's name. if more than on, only the first one is
 * returned */
int FindProcessByName(const char *procName);

#define PROCESS_ALIVE 1
#define PROCESS_IMPERVIOUS 2
#define PROCESS_DEAD 3
/* wait for a process to die */
int WaitForProcessDeath(int pid, int retries);

/* set the permissions and UID bits on a file */
int Setuid(const char *path);

/* setup pcscd to restart on system boot */
int SetupSystemBoot(void);

#endif /* !_INSTLIB_H_ */
