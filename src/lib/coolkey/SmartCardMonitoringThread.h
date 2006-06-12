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

#ifndef SMARTCARDMONITORINGTHREAD_H
#define SMARTCARDMONITORINGTHREAD_H

#include "prthread.h"
#include "secmodt.h"
#include "CoolKeyHandler.h"
#include <list>

class SmartCardMonitoringThread
{
 public:
  SmartCardMonitoringThread(SECMODModule *mod);
  ~SmartCardMonitoringThread();
  
  void Start();
  void Stop();
  
  void Execute();
  void Interrupt();
  
  void Release();
  void OnComplete();
  void OnDisconnect();

 private:

  void Insert(PK11SlotInfo *slot);
  void Remove(CoolKeyInfo *info);

  static void LaunchExecute(void *arg);
  
  SECMODModule *mModule;
  PRThread* mThread;
  char* mCurrentActivation;
};

#endif
