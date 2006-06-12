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

//#include "XptlBase.h"
//#include "CoolBucky.h"
//#include "CoolSec.h"
//#include "CoolBos.h"
//#include "ILocateManager.h"
//#include "ICertificateBlob.h"
//#include "atlbase.h"

#include "nspr.h"

#include "pk11func.h"

#include "CoolKey.h"
#include "SmartCardMonitoringThread.h"
#include "NSSManager.h"
#include "CoolKeyID.h"
#include "SlotUtils.h"
//#include "CoolKeyThreadEventService.h"


#include <assert.h>

static PRLogModuleInfo *coolKeyLogSC = PR_NewLogModule("coolKey");

//WINOLEAPI  CoInitializeEx(IN LPVOID pvReserved, IN DWORD dwCoInit);

SmartCardMonitoringThread::SmartCardMonitoringThread(SECMODModule *aModule)
  : mModule(aModule), mThread(NULL)
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
            ("SmartCardMonitoringThread::SmartCardMonitoringThread : \n"));
}

SmartCardMonitoringThread::~SmartCardMonitoringThread()
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
          ("SmartCardMonitoringThread::~SmartCardMonitoringThread : \n"));
  Stop();
}

void SmartCardMonitoringThread::Start()
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
          ("SmartCardMonitoringThread::Start : \n"));

  if (!mThread) {
    mThread = PR_CreateThread(PR_SYSTEM_THREAD, LaunchExecute, this,
                              PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                              PR_JOINABLE_THREAD, 0);
  }
}

void SmartCardMonitoringThread::Stop()
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
          ("SmartCardMonitoringThread::Stop : \n"));

  Interrupt();
}

void SmartCardMonitoringThread::Insert(PK11SlotInfo *aSlot)
{

  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
          ("SmartCardMonitoringThread::Insert  pig: \n"));

  CoolKeyInfo *info = CKHGetCoolKeyInfo(aSlot);
  if (info) {
    if (!InsertCoolKeyInfoIntoCoolKeyList(info)) {
      AutoCoolKey key(eCKType_CoolKey, info->mCUID);
      CoolKeyNotify(&key, eCKState_KeyInserted, 0);
    } else {
      delete info;
    }
  } 
}

void SmartCardMonitoringThread::Remove(CoolKeyInfo *info)
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
          ("SmartCardMonitoringThread::Remove : \n"));

  info->mInfoFlags = 0;
  AutoCoolKey key(eCKType_CoolKey, info->mCUID);
  CoolKeyNotify(&key, eCKState_KeyRemoved, 0);
  RemoveCoolKeyInfoFromCoolKeyList(info);
}

void SmartCardMonitoringThread::Execute()
{
  PK11SlotInfo *slot;

  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
         ("SmartCardMonitoringThread::Execute.\n"));
  //
  // populate token names for already inserted tokens.
  //
  PK11SlotList *sl =
	PK11_FindSlotsByNames(mModule->dllName, NULL, NULL, PR_TRUE);
  PK11SlotListElement *sle;
 
  if (sl) {

    for (sle=PK11_GetFirstSafe(sl); sle; 
				      sle=PK11_GetNextSafe(sl,sle,PR_FALSE)) {

      Insert(sle->slot);

      PK11_FreeSlot(sle->slot);
    }
    PK11_FreeSlotList(sl);
  }

  // loop starts..
  do {

     PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
            ("SmartCardMonitoringThread::Execute.Waiting for TokenEvent\n"));
    slot = SECMOD_WaitForAnyTokenEvent(mModule, 0, PR_SecondsToInterval(1)  );


    PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
           ("SmartCardMonitoringThread::Execute Token Event fired :"
            " slot %p \n", slot));

    if (slot == NULL) {

       PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
              ("SmartCardMonitoringThread::Execute. slot is NULL\n"));
      break;
    }

    // now we have a potential insertion or removal event see if the slot
    // is present to determine which it is...
    CoolKeyInfo *info = GetCoolKeyInfoBySlot(slot);

    PR_LOG( coolKeyLogSC, PR_LOG_DEBUG,
             ("SmartCardMonitoringThread::info %p : \n",info));

    PRBool isPresent = PK11_IsPresent(slot);


    PR_LOG( coolKeyLogSC, PR_LOG_DEBUG,
             ("SmartCardMonitoringThread::isPresent %d : \n",isPresent));


    /* if we think we have a token, but it's not the right one, or it's 
     * not there, then send a removal event */
    if (info && (!isPresent || (info->mSeries != PK11_GetSlotSeries(slot))) ) {

      PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
             ("SmartCardMonitoringThread::Execute Token Removed : \n"));

      Remove(info);
      delete info;
      info = NULL;
    }
    /* if the there is a token and we don't have info for it, send an 
     * insertion event */
    if (isPresent && !info) {

      PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
             ("SmartCardMonitoringThread::Execute Token Inserted : \n"));

      Insert(slot);
    }
    PK11_FreeSlot(slot);

  } while (1);

}

void SmartCardMonitoringThread::Interrupt()
{
  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
         ("SmartCardMonitoringThread::Interrupt: mThread %p mModule %p\n",mThread,mModule));

  if(mThread)
  {
    SECStatus rv;
    rv = SECMOD_CancelWait(mModule);
    if (rv !=SECSuccess) {
      PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
              ("SmartCardMonitoringThread::Interrupt:"
               " Can't join thread. result of CancelWait"
               " %d error %d \n",rv,PORT_GetError()));

      return; // don't hang if we couldn't wake up thread!!
    }
    PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
           ("SmartCardMonitoringThread::Interrupt: "
            "About to join smart card thread. \n"));

    PRStatus rs = PR_JoinThread(mThread);

    PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
           ("SmartCardMonitoringThread::Interrupt: result"
            " of PR_JoinThread %d error %d \n",rs,PORT_GetError()));

    mThread = 0; 
  }

  if(mModule)
  {
       PR_LOG( coolKeyLogSC, PR_LOG_DEBUG,
           ("SmartCardMonitoringThread::Interrupt: about to unload  module"
            ));

       mModule = 0;
  }
}

void SmartCardMonitoringThread::Release() {

  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
         ("SmartCardMonitoringThread::Release : \n"));

  if (mCurrentActivation)
    free(mCurrentActivation); 
  
  mCurrentActivation= NULL;
}

void SmartCardMonitoringThread::OnComplete() 
{

  PR_LOG( coolKeyLogSC, PR_LOG_DEBUG, 
         ("SmartCardMonitoringThread::OnComplete : \n"));

  AutoCoolKey key(eCKType_CoolKey, mCurrentActivation);
  CoolKeyNotify(&key, eCKState_KeyInserted, 0);
}

void SmartCardMonitoringThread::OnDisconnect() {}


void SmartCardMonitoringThread::LaunchExecute(void *arg)
{
  ((SmartCardMonitoringThread*)arg)->Execute();
}

