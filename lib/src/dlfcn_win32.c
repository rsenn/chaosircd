/* chaosircd - Chaoz's IRC daemon daemon
 *
 * Copyright (C) 2003-2006  Roman Senn <r.senn@nexbyte.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * $Id: dlfcn_win32.c,v 1.2 2006/09/28 08:38:31 roman Exp $
 */

#include "libchaos/defs.h"

#ifdef WIN32
#include <windows.h>
#endif


#ifndef PATH_MAX
#define PATH_MAX MAX_PATH 
#endif
/*#include "dlfcn_win32.h"*/

static void dl_convert_path(char *buf, size_t n, const char *path)
{
  while(--n)
  {
    *buf++ = (*path == '/' ? '\\' : *path);
    path++;
  }

  *buf = '\0';
}

void *dlopen(const char *filename, int flag)
{
  void *handle;
  char path[MAX_PATH];

  dl_convert_path(path, MAX_PATH, filename);

  return LoadLibrary(path);
}

void *dlsym(void *handle, const char *symbol)
{
  return GetProcAddress(handle, symbol);
}

void dlclose(void *handle)
{
  FreeLibrary(handle);
}

const char *dlerror()
{
  static char msg[256];
  DWORD error = GetLastError();

  if(error == 0)
    return NULL;

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)msg, sizeof(msg), NULL);

  SetLastError(0);
  return msg;
}
