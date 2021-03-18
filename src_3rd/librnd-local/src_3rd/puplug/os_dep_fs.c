/*
    libporty - file system related calls
    Copyright (C) 2011..2012  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/libporty
    Author: libporty [-at-] igor2.repo.hu
*/

#define PUP_WANTS_READDIR
#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "os_dep_fs.h"

#ifdef WIN32
char pup_path_sep = '\\';
#else
char pup_path_sep = '/';
#endif

#ifdef PUP_HAVE_READDIR

void *pup_open_dir(const char *dirname)
{
	return opendir(dirname);
}

const char *pup_read_dir(void *handle)
{
	struct dirent *dp;
	
	dp = readdir((DIR *)handle);
	if (dp == NULL)
		return NULL;
	return dp->d_name;
}
 
void pup_close_dir(void *handle)
{
	closedir((DIR *)handle);
}

#else
#ifdef PUP_HAVE_FINDNEXTFILE

#include <windows.h>

typedef struct {
	HANDLE h;
	char *first;
	char *free;
} pup_dirent_t;

void *pup_open_dir(const char *dirname)
{
	WIN32_FIND_DATA fd;
	HANDLE h;
	pup_dirent_t *p;
	char *s;

	s = malloc(strlen(dirname)+10);
	sprintf(s, "%s\\*", dirname);
	h = FindFirstFile(s, &fd);
	free(s);
	if (h == INVALID_HANDLE_VALUE)
		return NULL;
	p = malloc(sizeof(pup_dirent_t));
	p->h = h;
	p->first = strdup(fd.cFileName);
	p->free = NULL;
	return p;
}

const char *pup_read_dir(void *handle)
{
	WIN32_FIND_DATA fd;
	pup_dirent_t *p = handle;

	if (p->first != NULL) {
		p->free  = p->first;
		p->first = NULL;
		return p->free;
	}

	if (p->free != NULL) {
		free(p->free);
		p->free = NULL;
	}

	if (FindNextFile(p->h, &fd) != 0)
		return strdup(fd.cFileName);

	return NULL;
}

void pup_close_dir(void *handle)
{
	pup_dirent_t *p = handle;

	if (p->first != NULL)
		free(p->first);
	else if (p->free != NULL)
		p->free = NULL;
	
	FindClose(p->h);
	free(p);
}
#else
#error no suitable readdir implementation
#endif
#endif
