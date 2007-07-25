/* ***** BEGIN COPYRIGHT BLOCK *****
 ** * Copyright (C) 2005 Red Hat, Inc.
 ** * All rights reserved.
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

/* MAC & Linux */

#ifndef WIN32


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fts.h>
#include <sys/sysctl.h>

#include "instlib.h"

#define LOWER(c) (isalpha(c) ? tolower(c) : c)
int HasString(const char *target,const char *search) {
    char c = LOWER(*search);
    int len = strlen(search);
    for  ( ; *target; target++) {
	if (LOWER(*target) == c) {
	    if (strncasecmp(target,search,len) == 0) {
		return 1;
	    }
	}
    }
    return 0;
}

int IsDirectory(const char *dir)
{
    struct stat sbuf;
    int rc;

    rc = stat(dir,&sbuf);
    if (rc == 0) {
	return ((sbuf.st_mode & S_IFDIR) == S_IFDIR);
    }
    return 0;
}


int FileExists(const char *file)
{
    struct stat sbuf;
    int rc;

    rc = stat(file,&sbuf);
    if (rc == 0) {
	return 1;
    }
    return 0;
}

#define MAX_RECURSE_LEVEL 15
#define DIR_MODE 0755
/*
 * Create a directory. Create any missing or broken 
 * components we need along the way. If we already have a
 * directory, return success.
 */
int Makedir(const char *directory, int level, int mode)
{
   int rc;
   char *buf, *cp;

   /* prevent arbitrary stack overflow */
   if (level > MAX_RECURSE_LEVEL) {
	errno = ENAMETOOLONG;
	return -1;
   }
   umask(0);

   /* just try it first */
   rc = mkdir(directory,DIR_MODE);
   if (rc != 0) {
	if (errno == EEXIST) {
	    if (IsDirectory(directory)) {
		/* we have a directory, use it */
		return 0;
	    } else  { /* must be a file */
		/* remove the file and try again */
		rc = RmFile(directory);
		if (rc == 0) {
			rc = mkdir(directory, mode);
		}
		return rc;
	    }
	}
	/* if we fail because on of the subdirectory entries was a
	 * file, or one of the subdirectory entries didn't exist,
	 * move back one component and try the whole thing again
	 */
	if ((errno != ENOENT) && (errno != ENOTDIR)) {
	    return rc;
	}
	buf = (char *)malloc(strlen(directory)+1);
	strcpy(buf,directory);
	if (cp = strrchr(buf,'/')) {
	    *cp = 0;
	    rc = Makedir(buf,level+1, mode);
	    if (rc == 0) {
	     	rc = mkdir(directory, mode);
	     }
	 }
	free(buf);
    }
    return rc;
}

#define INFO_FILE "/Contents/Info.plist"
#define MAX_VERSION_TYPES 3
/* try to find the version number. NOTE: this is not a full xml parser. A full
 * xml parser would be a bit better to use */
unsigned long GetBundleVersion(const char *bundle)
{
#ifdef xMAC
    CFBundleRef bundle;
    CURLRef	url;
    unsigned long version;

    url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, bundle, 
					kCFURLPOSIXPathStyle, true );
    if (!url) {
	return 0;
    }

    bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(url);
    if (!bundle) {
	return 0;
    }

    version = (unsigned long) CFBundleGetVersionNumber(bundle);
    CFRelease(bundle);

    return version;
#else
    FILE *infoFile;
    char *bundleFile;
    char *str;
    char buf[256];
    int versions[MAX_VERSION_TYPES];
    int i, currentType;
    unsigned long version;

    bundleFile = (char *)malloc(strlen(bundle) + strlen(INFO_FILE)+1);

    if (bundleFile == NULL) {
	return 0;
    }
    
    sprintf(bundleFile,"%s"INFO_FILE,bundle);
    infoFile = fopen(bundleFile,"r");
    free(bundleFile);
    if (infoFile == NULL) {
	return 0;
    }

    while ((str=fgets(buf, sizeof(buf),infoFile)) != NULL) {
	if (HasString(str,"CFBundleVersion")) {
    	    str=fgets(buf, sizeof(buf),infoFile);
	    break;
	}
    }
    fclose(infoFile);
    if (str == NULL) {
	return 0;
    }
    while (*str && !isdigit(*str)) {
	str++;
    }
    if (*str == 0) {
	return 0;
    }
    for (i=0; i < MAX_VERSION_TYPES; i++) {
    	versions[i] = 0;
    }
    currentType = 0;
    for (; currentType < MAX_VERSION_TYPES &&
		*str && (isdigit(*str) || *str == '.'); str++) {
	if (*str == '.') {
	   currentType++;
	   if (currentType >= MAX_VERSION_TYPES) {
		break;
	   }
	   continue;
	} 
	/* only record up to 2 digits for minor versions */
        if ((currentType != 0) && (versions[currentType] >= 10)) {
	    versions[currentType] = 99;
	    continue;
	}
	
	versions[currentType] =
	   versions[currentType] * 10 + (*str) - '0';
    }

    /* now assemble the decoded version number */
    version = 0;
    for (i=0; i < MAX_VERSION_TYPES; i++) {
	version = version * 100 + versions[i];
    }

    return version;
#endif
}

#define BUFSIZE 32*1024
int Copy(const char *src, const char *target, int mode)
{
    int fdsrc,fdtarget;
    int bytes,wbytes,rc = 0;
    char buf[BUFSIZE];

    fdsrc = open(src,O_RDONLY);
    if (fdsrc < 0) {
	return fdsrc;
    }
    fdtarget = open(target, O_WRONLY|O_CREAT|O_TRUNC, mode);
    if (fdtarget < 0) {
	close(fdsrc);
	return fdtarget;
    }

    while ((bytes = read(fdsrc,buf,sizeof(buf))) > 0) {
	wbytes = write(fdtarget,buf, bytes);
	if (wbytes != bytes) {
	    rc = -1;
	    break;
	}
    }
    if (bytes < 0) {
	rc = bytes;
    }

    close(fdtarget);
    close(fdsrc);
    return rc;
}

    

int Cpdir(const char *source,const char *target)
{
    FTS *dir;
    FTSENT *entry;
    int rc = 0;
    char buf[MAXPATHLEN];
    char *files[2];
    int mode;
    int cdir;

    files[0]=".";
    files[1]= 0;

    cdir = open(".",O_RDONLY);
    rc = chdir(source);
    if (rc != 0) {
	close(cdir);
	return rc;
    }

    rc = Makedir(target, 0, DIR_MODE);
    if (rc != 0) {
	return rc;
    }

    dir = fts_open(files,FTS_NOCHDIR|FTS_LOGICAL,NULL);
    if (dir == NULL) {
	return -1;
    }

    while (entry = fts_read(dir)) {
	sprintf(buf,"%s/%s",target,entry->fts_accpath);
	mode = DIR_MODE;
        if (entry->fts_statp) {
	   mode = entry->fts_statp->st_mode;
	}
	switch (entry->fts_info) {
	case FTS_D:
	   /* need more .... */
	   rc = Makedir(buf, 0, mode);
	   break;
	case FTS_F:
	   rc = Copy(entry->fts_path, buf, mode);
	   break;
	}
	if (rc != 0) break;
    }
    fchdir(cdir);
    close(cdir);
    fts_close(dir);
    return rc;
}

int RmFile(const char *fileName) 
{
    int rc = unlink(fileName);
    if ((rc < 0) && (errno == EPERM)) {
	chmod(fileName,0644);
	rc = unlink(fileName);
    }
    return rc;
}

int RmDir(const char *dirName) 
{
    int rc = 0;
    int count = 0;
    FTS *dir;
    FTSENT *entry;
    char *files[2];

    files[0] = (char *) dirName;
    files[1] = 0;
    if (FileExists(dirName)) {
	if (IsDirectory(dirName)) {
	    dir = fts_open(files,FTS_NOCHDIR|FTS_PHYSICAL,NULL);
	    if (dir == NULL) {
		return -1;
	    }

	    while (entry = fts_read(dir)) {
		switch (entry->fts_info) {
		case FTS_DP:
		    count++;
		    rc = rmdir(entry->fts_path);
		    break;
		case FTS_DEFAULT:
		case FTS_SL:
		case FTS_SLNONE:
		case FTS_NS:
		case FTS_NSOK:
		case FTS_F:
		    rc = RmFile(entry->fts_path);
		    break;
	        }
		if (rc != 0) break;
	   }
	   fts_close(dir);
	   if (count == 0) {
		rc = rmdir(dirName);
	   }
	   return rc;
	} else {
	   return RmFile(dirName);
	}
     }
     return 0;
}


char *
GetFullPath(const char *prog, int includeProg) {
    char *instDir, *end;
    char *cwd = NULL;
    char *new = NULL;

    /* get the install directory */
    instDir = strdup(prog);
    if ((end = strrchr(instDir, '/')) == 0) {
	if (includeProg) {
	    cwd = getcwd(NULL, 0);
	    new = (char *)malloc(strlen(cwd)+strlen(instDir)+2);
	    sprintf(new,"%s/%s",cwd,instDir);
	} else {
	    new = getcwd(NULL,0);
	}
    } else {
	if (!includeProg) {
	    *end = 0;
	}
	if (*instDir != '/') {
	    cwd = getcwd(NULL,0);
	    new = (char *)malloc(strlen(cwd)+strlen(instDir)+2);
	    char *instPtr = instDir;

	    /* handle . and .. prefixes */
	    if (*instDir == '.') {
		if ((instDir[1] = 0)  ||  /* . only */
		    ((instDir[1] == '.')  &&  /* .. & ../ */
			((instDir[2] == '/') || (instDir[2] == 0))) || 
		    (instDir[1] == '/') ) { /* ./ */

		    instPtr++;
		    if (*instPtr == '.') {
		        /* back up one element */
		        if ((end = strrchr(cwd,'/')) == 0) {
			    *end = 0;
		        } else {
			    *cwd = 0;
		        }
		        instPtr++;
		    }
		    if (*instPtr == '/') {
		        instPtr ++;
		    }
	        }
	    }
	    sprintf(new,"%s/%s",cwd,instPtr);
	}
    }
    if (cwd) {
	free(cwd);
    }
    if (new) {
	free(instDir);
	instDir = new;
    }
    return instDir;
}


#ifdef MAC
typedef struct kinfo_proc kinfo_proc;
static const int    listProc[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
static const int    listProcLen=(sizeof(listProc)/sizeof(int)) -1;
#endif

/*
 * find the PCSCD process
 */
#define MAX_RETRIES 100

int FindProcessByName(const char *procName)
{
#ifdef MAC
    int                 ret;
    kinfo_proc *        pList;
    size_t              length;
    int 		processCount;
    int			proc;
    int			i;
    int 		pid = -1;
    /*
     * the process table is always changing. we may need to loop several times
     * before we actually get the process list. We start with a large static
     * buffer hoping that we don't have to actually allocate anything in the
     * normal case 
     */ 
    pList = NULL; 
    length =  0;
    /* don't hang forever */
    for (i=0; i< MAX_RETRIES; i++) {
        length = 0;
        ret = sysctl( (int *) listProc, listProcLen, NULL, &length, NULL, 0);
        if (ret < 0) {
	    break;
	}
	pList = malloc(length);
	if (pList == NULL) {
	    break;
	}

	ret = sysctl( (int *) listProc, listProcLen, pList, &length, NULL, 0);
	if (ret < 0) {
	    if (errno == ENOMEM) {
		free(pList);
		pList = NULL;
		continue;
	    }
	}

	break;
    }

    /* just couldn't get our process list */
    if (pList == NULL) {
	perror("process list failed:");
	return -1;
    }

    /* now loop through lookin for our process */
    processCount = length / sizeof(kinfo_proc);
    for (proc=0; proc < processCount; proc++) {
	if (strcmp(pList[proc].kp_proc.p_comm,procName) == 0) {
	    pid = pList[proc].kp_proc.p_pid;
	    break;
	}
    }

    free(pList);
    return pid;
#else
    return -1;
#endif
}

int
WaitForProcessDeath(int pid, int retries)
{
    int rc;
    int i;

    /* loop for our retries until we can't send a signal to the process */
    for (i=0; i < retries; i++) {
	rc = kill(pid, 0);
	if ( rc!= 0 ) {
	    break;
	}
	sleep(1);
    }
    if (rc == 0) {
	/* process is still around past the retries */
	return PROCESS_ALIVE;
    }
    if (errno == EPERM) {
        /* we don't even have permission to kill the process */
	return PROCESS_IMPERVIOUS;
    }
    /* process has been killed */
    return PROCESS_DEAD;
}

#ifndef O_EXLOCK
#define O_EXLOCK 0
#endif

int
Setuid(const char *path) {
    /* NULL for now */
    struct stat statbuf;
    int fd;
    int rc, mode; 
    int ret = 0;

    fd = open(path, O_NONBLOCK|O_RDONLY|O_EXLOCK, 0);
    if (fd < 0) {
	return errno;
    }

    /* Get info about the file */
    rc = fstat(fd, &statbuf);
    if (rc < 0) {
	close(fd);
  	return errno;
    }

    /* make sure the file is owned by root */
    if (statbuf.st_uid != 0) {
 	rc = fchown(fd, 0, statbuf.st_gid); 
	if (rc < 0)  {
	    close(fd);
  	    return errno;
	}
    }

    /* turn off group and other write permission so the 
     * file can't be replaced */
    mode = statbuf.st_mode & ~(S_IWGRP|S_IWOTH);
    rc = fchmod(fd, mode | S_ISUID); /* set the uid bit */
    if (rc < 0) ret = errno;

    close(fd);
    return ret;
}

const char smartCardServices[] = {
"#!/bin/sh\n"
"\n"
"##\n"
"# Start PC/SC\n"
"##\n"
"\n"
". /etc/rc.common\n"
"\n"
"	ConsoleMessage \"Starting SmartCard Services\"\n"
"	/usr/sbin/pcscd\n"
"\n" };

/* SmartcardServices */
/* StartupParameters.plist */
const char startupParametersPList[] = {
"{\n"
" Description     = \"PC/SC Daemon\";\n"
" Provides        = (\"SmartCardServices\");\n"
" Requires        = ();\n"
" OrderPreference = \"None\";\n"
" Messages =\n"
" {\n"
"   start = \"Starting SmartCard Services\";\n"
"   stop  = \"Stopping SmartCard Services\";\n"
" };\n"
"}\n"
"\n" };


#define STARTUP_DIR "/System/Library/StartupItems"

int 
RemoveSystemBoot()
{

    int rc;

    if (IsDirectory(STARTUP_DIR"/SmartCardServices")) {

        /*Blow it away **/
        rc = RmDir(STARTUP_DIR"/SmartCardServices");
        if (rc < 0) {
            return -1;
        }
    }

    return rc;
}

int
SetupSystemBoot()
{
    int rc,fd; 

    if (!IsDirectory(STARTUP_DIR"/SmartCardServices")) {

	/*check for errors and clean up so we don't have any partial installs*/
	rc = Makedir(STARTUP_DIR"/SmartCardServices", 0, DIR_MODE);
	if (rc < 0) {
	    return -1;
	}
    }
    if (!FileExists(STARTUP_DIR"/SmartCardServices/SmartcardServices")) {
	fd = open(STARTUP_DIR"/SmartCardServices/_SmartcardServices", 
						O_CREAT|O_TRUNC|O_WRONLY);
	if (fd < 0) {
	    return -1;
	}
  	rc = write(fd,smartCardServices,sizeof(smartCardServices)-1);
	if (rc < 0) {
	    return -1;
	}
	if (rc < sizeof(smartCardServices)-1) {
	    return -1;
	    errno = ENOSPC;
	    return -1;
	}
	close(fd);
	rc = rename(STARTUP_DIR"/SmartCardServices/_SmartcardServices", 
		    STARTUP_DIR"/SmartCardServices/SmartcardServices");
	if (rc < 0) {
	    return -1;
	}
    }
    if (!FileExists(STARTUP_DIR"/SmartCardServices/StartupParameters.plist")) {
	fd = open(STARTUP_DIR"/SmartCardServices/_StartupParameters.plist", 
						O_CREAT|O_TRUNC|O_WRONLY);
	if (fd < 0) {
	    return -1;
	}
  	rc = write(fd, startupParametersPList, sizeof(startupParametersPList)-1);
	if (rc < 0) {
	    return -1;
	}
	if (rc < sizeof(startupParametersPList)-1) {
	    errno = ENOSPC;
	    return -1;
	}
	close(fd);
	rc = rename(STARTUP_DIR"/SmartCardServices/_StartupParameters.plist", 
		    STARTUP_DIR"/SmartCardServices/StartupParameters.plist"); 
	if (rc < 0) {
	    return -1;
	}
   }
   return 0;
}

#endif
