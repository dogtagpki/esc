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

//#include "windows.h"
#include "CoolKey.h"
#include "CoolKeyPref.h"

#include <map>
#include <string>
#include <prlog.h>

#ifndef WIN32
#define _MAX_PATH 512
#endif
std::map<std::string, char*> sPrefMap;

static PRLogModuleInfo *coolKeyLogPR = PR_NewLogModule("coolKey");

void CoolKeyPrefInit(char *aPrefDir)
{
#ifdef WIN32
  wchar_t widePath[_MAX_PATH];
  GetModuleFileNameW(NULL, widePath, _MAX_PATH);
  wchar_t* doomed = wcsrchr(widePath, '\\');
  if (doomed) {
    doomed++;
    *doomed = '\0';
  }
  wcscat(widePath, L"ESC.cfg");

  FILE* pref = _wfopen(widePath, L"r");
  if (!pref) return;

#else
  char buff[_MAX_PATH];

  getcwd(buff, 512);

  PR_LOG( coolKeyLogPR, PR_LOG_DEBUG, ("CoolKeyPrefInit: CWD %s\n",buff));

  PR_LOG( coolKeyLogPR, PR_LOG_DEBUG, ("CoolKeyPrefInit:\n"));
  char prefPath[_MAX_PATH];

  if(aPrefDir)
  {
      strcpy(prefPath,aPrefDir);
      strcat(prefPath,"/ESC.cfg");
  }
  else
  {
      strcpy(prefPath,"./");
      strcat(prefPath,"ESC.cfg");

  }

  PR_LOG( coolKeyLogPR, PR_LOG_DEBUG, 
				("CoolKeyPrefInit: pref_path %s\n",prefPath));  
  FILE* pref = fopen(prefPath, "r");
  if (!pref) return;
#endif
  
  char line[200];
  while(fgets(line, 200, pref))
  {
    if (line[0] == '#')
      continue;
    
    char* pivot = strchr(line, '=');
    if (!pivot)
      continue;
    
    *pivot =(char) NULL;
    
    pivot++;
    
    // kill trailing ws.
    char*ws = pivot;
    while (ws && *ws != ' ' && *ws != '\r' && *ws != '\n')
      ws++;
    *ws = '\0';
    sPrefMap[line] = strdup(pivot);
  }
}

void CoolKeyGetPref(char* key, char** value)
{
  std::string skey = key;
  std::map<std::string, char*>::iterator it = sPrefMap.find(skey);
  if (it != sPrefMap.end())
    *value = strdup((*it).second);
  else
    *value = NULL;
}

void CoolKeyFreePref(char* value)
{
  if (value) {
    free(value);
  }
}

void CoolKeyPrefShutdown()
{

  PR_LOG( coolKeyLogPR, PR_LOG_DEBUG, ("CoolKeyPrefShutdown:\n"));
  std::map<std::string, char*>::iterator it;
  for (it=sPrefMap.begin(); it!=sPrefMap.end(); ++it)
    free((*it).second);
  sPrefMap.clear();
}
