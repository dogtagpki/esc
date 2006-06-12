/** BEGIN COPYRIGHT BLOCK
 * This Program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 of the License.
 *
 * This Program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/

#ifndef pathsub_h___
#define pathsub_h___
/*
** Pathname subroutines.
**
** Brendan Eich, 8/29/95
*/
#include <limits.h>
#include <sys/types.h>

#if SUNOS4
#include "sunos4.h"
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*
 * Just keep sane lengths
 */
#undef NAME_MAX
#define NAME_MAX 256

extern char *program;

extern void fail(char *format, ...);
extern char *getcomponent(char *path, char *name);
extern char *ino2name(ino_t ino, char *dir);
extern void *xmalloc(size_t size);
extern char *xstrdup(char *s);
extern char *xbasename(char *path);
extern void xchdir(char *dir);

/* Relate absolute pathnames from and to returning the result in outpath. */
extern int relatepaths(char *from, char *to, char *outpath);

/* NOTE: changes current working directory -- caveat emptor */
extern void reversepath(char *inpath, char *name, int len, char *outpath);

/* stats every directory in path, reports results. */
extern void diagnosePath(const char * path);

#endif /* pathsub_h___ */
