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

#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <string>
//#include <winscard.h>

#include "prprf.h"
#include "nss.h"
#include "pk11func.h"
#include "ssl.h"
#include "p12plcy.h"
#include "secmod.h"
#include "secerr.h"

#include "prthread.h"
#include "prcvar.h"
#include "prlock.h"

#include "CoolKeyID.h"
#include "CoolKey.h"
#include "CoolKeyPref.h"
#include "cky_base.h"
#include "cky_applet.h"

#include "NSSManager.h"
#include "CoolKeyHandler.h"
#include "SlotUtils.h"


//static  char *test_extended_login = "s=325&msg_type=13&invalid_login=0&blocked=0&error=&required_parameter0=id%3DUSER%5FID%26name%3DUser+ID%26desc%3DUser+ID%26type%3Dstring%26option%3Doption1%2Coption2%2Coption3&required_parameter1=id%3DUSER%5FPWD%26name%3DUser+Password%26desc%3DUser+Password%26type%3Dpassword%26option%3D&required_parameter2=id%3DUSER%5FPIN%26name%3DPIN%26desc%3DOne+time+PIN+received+via+mail%26type%3Dpassword%26option%3D";


#include <string>

#ifndef CKO_MOZILLA_READER
#define CKO_MOZILLA_READER     (CKO_NETSCAPE+5)
#define CKA_MOZILLA_IS_COOL_KEY (CKO_NETSCAPE+24)
#define CKA_MOZILLA_ATR        (CKO_NETSCAPE+25)
#endif

static PRLogModuleInfo *coolKeyLogHN = PR_NewLogModule("netkey");

void NotifyEndResult(CoolKeyHandler* context, int operation, int result, int description);


struct AutoCKYBuffer : public CKYBuffer
{
  AutoCKYBuffer() { CKYBuffer_InitEmpty(this); };
  AutoCKYBuffer(const char *aHex) { CKYBuffer_InitFromHex(this, aHex); };
  AutoCKYBuffer(unsigned const char *aData, unsigned int aLength) { CKYBuffer_InitFromData(this, aData, aLength); };
  AutoCKYBuffer(unsigned long aLength) { CKYBuffer_InitFromLen(this, aLength); };
  ~AutoCKYBuffer() { CKYBuffer_FreeData(this); };
};

struct KHHttpEvent
{
  KHHttpEvent(CoolKeyHandler *aKeyHandler,NSS_HTTP_HANDLE handle)
    : mKeyHandler(aKeyHandler), mHttpHandle(handle)
  {
  }

  virtual ~KHHttpEvent()
  {
  }

  virtual HRESULT Execute() = 0;

  CoolKeyHandler *mKeyHandler;
  NSS_HTTP_HANDLE mHttpHandle;
  
};

struct KHOnConnectEvent : public KHHttpEvent
{
  KHOnConnectEvent(CoolKeyHandler *aKeyHandler, NSS_HTTP_HANDLE handle)
    : KHHttpEvent(aKeyHandler, handle)
  {
  }

  HRESULT Execute()
  {
       PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("KHOnConnectEvent::Execute:\n"));
	  HRESULT res;
    res =  mKeyHandler->OnConnectImpl();
	if(res == E_FAIL)
	{
		mKeyHandler->OnDisConnectImpl();
	}

	return res;
  }
};

class PDUWriterThread
{
public:

  PDUWriterThread(CoolKeyHandler *aHandler) {mAccepting=PR_FALSE; mHandler = aHandler;};
 ~PDUWriterThread() ;

  HRESULT Init();

  HRESULT QueueKHHttpEvent(KHHttpEvent *aEvent);
  HRESULT QueueOnConnectEvent(CoolKeyHandler *aKeyHandler,
                              NSS_HTTP_HANDLE handle);
 
  HRESULT Shutdown();

  static void ThreadRun(void* arg);

  PRLock    *mLock;
  PRCondVar *mCondVar;
  PRThread  *mThread;
  PRBool     mAccepting;

  CoolKeyHandler *mHandler;

  std::list<KHHttpEvent*> mPendingEvents;
};

PDUWriterThread::~PDUWriterThread() 
{

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::~PDUWriterThread:\n"));

    if (mCondVar ) {

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::~PDUWriterThreade about to destroy mCondVar.\n"));

    PR_DestroyCondVar(mCondVar);
    mCondVar = NULL;
  }

  if (mLock ) {
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::~PDUWriterThread  about to destroy mLock.\n"));

    PR_DestroyLock(mLock);
    mLock = NULL;
  }

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::~PDUWriterThread leaving....\n"));


}

void
PDUWriterThread::ThreadRun(void *arg)
{

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun:\n"));
  PDUWriterThread *pn = (PDUWriterThread*) arg;

  while (pn->mAccepting && pn->mLock && pn->mCondVar) {

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: top of while loop accepting %d lock %p cond %p\n",pn->mAccepting,pn->mLock,pn->mCondVar));
    PR_Lock(pn->mLock);

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: done PR_Lock()\\n"));

    if ( pn->mCondVar && pn->mPendingEvents.empty())
      PR_WaitCondVar(pn->mCondVar, PR_INTERVAL_NO_TIMEOUT);

      PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: done waiting on cond var\n"));

    if (pn->mPendingEvents.empty()) {
      PR_Unlock(pn->mLock);
      continue;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: have eventsg\n"));

    // pull the events we will fire off of the pending event list.
    std::list<KHHttpEvent*> events(pn->mPendingEvents);

    // clear the pending event queue.
    pn->mPendingEvents.clear();

    // let the lock go so the other threads can add their events.

     PR_Unlock(pn->mLock);

    // do the firing...
    while (!events.empty()) {
      // Ordering of events is important.  We push_back when inserting.
      KHHttpEvent *e = events.front();
      events.pop_front();

      e->Execute();

      delete e;

    }
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: bottom of while loop\n"));
  }

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::ThreadRun: no longer accepting\n"));

  if(pn)
  {
      delete pn;
  }
}

HRESULT
PDUWriterThread::Init()
{

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("PDUWriterThread::Init:\n"));
  mLock = PR_NewLock();
  if (!mLock)
    return E_FAIL;
  
  mCondVar = PR_NewCondVar(mLock);
  if (!mCondVar)
    return E_FAIL;
  
  mAccepting = PR_TRUE;
  
  mThread = PR_CreateThread(PR_SYSTEM_THREAD,
                            ThreadRun,
                            (void*)this,
                            PR_PRIORITY_NORMAL,
                            PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD,
                            0 /* Default stack size */);
  
  return S_OK;
}

HRESULT
PDUWriterThread::Shutdown()
{
  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown.mThread %p\n",mThread));
  mAccepting = PR_FALSE;
 
  int same_thread = 0;
 
  if (PR_GetCurrentThread() != mThread) {
   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown.mThread  About to attempt to interrupt and  join mThread %p\n",mThread));
   PRStatus status = PR_Interrupt(mThread);

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown. Result of interrupt Thread %d\n",status));
    
    status = PR_JoinThread(mThread);

     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown. done attempt join, result %d thread  %p\n",status,mThread));
  }
  else
  {

      PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown. PR_CurrentThread is equal to PDUWriterThread" ));

      same_thread = 1;

  }
  
  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::PDUWriterThread::Shutdown.mThread %p leaving....\n",mThread));

  if(same_thread)
      return E_FAIL;
  else 
      return S_OK;
}

HRESULT
PDUWriterThread::QueueKHHttpEvent(KHHttpEvent *aEvent)
{
  PR_Lock(mLock);

  mPendingEvents.push_back(aEvent);

  PR_NotifyCondVar(mCondVar);
  PR_Unlock(mLock);
  return S_OK;
}


HRESULT
PDUWriterThread::QueueOnConnectEvent(CoolKeyHandler *aKeyHandler, NSS_HTTP_HANDLE handle)
{
  if (!mAccepting) return E_FAIL;

  KHHttpEvent *e = (KHHttpEvent*)new KHOnConnectEvent(aKeyHandler, handle);
  if (!e) return E_FAIL;

  return  QueueKHHttpEvent(e);
}

CoolKeyHandler::CoolKeyHandler() 
  :mDataLock(NULL), mDataCondVar(NULL), m_dwRef(0), mCardContext(0),
    mCardConnection(0), mReceivedEndOp(false),mAppDir(NULL), mPort(0), mPDUWriter(0), 
	mCharScreenName(NULL),mCharPIN(NULL),mCharScreenNamePwd(NULL),mCharHostName(NULL),mCharTokenType(NULL),mCharTokenCode(NULL),mHttpRequestTimeout(30),mSSL(0),mRAUrl(NULL),mHttp_handle(0)
{
}

CoolKeyHandler::~CoolKeyHandler()
{

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler:\n"));

  if (mPDUWriter) {
      mPDUWriter->Shutdown();
  }

  if (mDataLock) {
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: about to destroy mDataLock\n"));
    PR_DestroyLock(mDataLock);
    mDataLock = NULL;
  }

  if (mDataCondVar) {
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: about to destroy mDataCondVar\n"));
    PR_DestroyCondVar(mDataCondVar);

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: about done destroying mDataCondVar\n"));

    mDataCondVar = NULL;
  }
 
  if(mCharTokenType)
  {
	  free(mCharTokenType);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharTokenType\n"));

  }

  if(mCharScreenName)
  {
	  free(mCharScreenName);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharScreenName\n"));

  }

  if(mCharPIN)
  {
	  free(mCharPIN);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharPIN\n"));

  }

  if(mCharHostName)
  {
	  free(mCharHostName);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharHostName\n"));

  }

  if(mRAUrl)
  {
	  free(mRAUrl);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mRAUrl\n"));

  }

  if(mCharScreenNamePwd)
  {
          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: attempt to  free mCharScreenNamePwd\n"));  

	  free(mCharScreenNamePwd);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharScreenNamePwd\n"));


  }
  if(mCharTokenCode)
  {
          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: attempt to free mCharTokenCode\n"));
	  free(mCharTokenCode);

          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done free mCharTokenCode\n"));

  }

 
  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: attempt mReqParamList.Cleanup %p\n",&mReqParamList)); 
  mReqParamList.CleanUp();

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done mReqParamList.CleanUp\n"));


  DisconnectFromReader();

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: done DisconnectFromReader\n"));
 
 
  assert(m_dwRef == 0);

if(mHttp_handle)
	{
		httpDestroyClient(mHttp_handle);
	}
  

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::~CoolKeyHandler: leaving\n"));
}

void CoolKeyHandler::AddRef()
{
   ++m_dwRef;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::AddRef count now %d:\n",m_dwRef));
}

void CoolKeyHandler::Release()
{
  assert(m_dwRef > 0);
  if (--m_dwRef == 0)
    {

      PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Release count now %d:\n",m_dwRef));
      delete this;
    } else
    {
  
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Release count now %d:\n",m_dwRef));

    }

}

HRESULT CoolKeyHandler::Init(const CoolKey *aKey,
                                           const char *screenName,
                                           const char *pin,const char *screenNamePwd,
										   const char *tokenCode,int op) {

  int error_no = 0;
  int config_error_no = 44;

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init:\n"));

  bool connected = false;
  PRThread*  cThread = NULL;
  const char *readerName =  NULL;

  if (!aKey || aKey->mKeyType != eCKType_CoolKey ||  !aKey->mKeyID) {
      PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 1\n"));
    goto done;
  }
  
  readerName = GetReaderNameForKeyID(aKey);

  cThread = PR_GetCurrentThread();


   mKey = *aKey;

  
  if (!readerName) {
     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 2\n"));
    goto done;
  }
 
   mDataLock = PR_NewLock();
  if (!mDataLock)
  {
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure Can't initialize Lock for data.\n"));
    return E_FAIL;

  }

  mDataCondVar = PR_NewCondVar(mDataLock);
  if (!mDataCondVar)
  {
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure Can't initialize Cond Var for data.\n"));
      return E_FAIL;

  }

  CollectPreferences();


  mHttpDisconnected = false;
  mCancelled = false;

  //CoolKeyGetPref("TPS_HOST_USES_SSL", &temp);
 

  if(!mCharHostName || !mRAUrl)
  {
      PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: Didn't collect proper config info..\n"));
      error_no = config_error_no;
      goto done;
  }
 

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: Past configuration tests, about to attempt operation.\n"));  

  mCardContext = CKYCardContext_Create(SCARD_SCOPE_USER);
  if (!mCardContext) {
     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 3\n"));
    error_no = 45;
    goto done;
  }
  
  mPDUWriter = new PDUWriterThread(this);
  if (!mPDUWriter) {
     error_no = 46;
     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 4\n"));
    goto done;
  }

  mPDUWriter->Init(); 


  mHttp_handle = httpAllocateClient();

  if(mHttp_handle <= 0)
  {
           PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 5\n"));
          error_no = 47;
	  goto done;
  }

  
  connected = ConnectToReader(readerName);
  
  if (!connected) {
     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 6\n"));
    error_no = 48;
    goto done;
  }
  
  if(screenName)
	mCharScreenName = strdup(screenName);

  if(pin)
	 mCharPIN = strdup(pin);

  if(screenNamePwd)
	  mCharScreenNamePwd = strdup(screenNamePwd);

  if(tokenCode)
  {
          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: token code: %s\n",tokenCode));
	  mCharTokenCode = strdup(tokenCode);
  }

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: mCharTokenCode %s \n",mCharTokenCode));

  mStatusRequest = true;

 done:
  
  if (!connected) {
    
    if (mCardContext) {
      CKYCardContext_Destroy(mCardContext);
      mCardContext = 0;
    }
     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure 7\n"));


     NotifyEndResult(this, op, 1, error_no);

     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Init: failure attempted to NotifyEndResult error no %d\n",error_no));

    return E_FAIL;
  }
  
  return S_OK;
}

void CoolKeyHandler::CollectPreferences()
{
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences !\n"));


    int httpMessageTimeout = 30;


    //Quickly grab the configurable http message timeout


    const char *msg_timeout = CoolKeyGetConfig("esc.tps.message.timeout");


    if(msg_timeout)
    {

        httpMessageTimeout = atoi(msg_timeout);


        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG,("CoolKeyHandler::CollectPreferences! Message timeout %d\n",httpMessageTimeout));

    }

    mHttpRequestTimeout = httpMessageTimeout;
 
    // Now grab the url for the tps server from config store.

    const char *tps_url = CoolKeyGetConfig("esc.tps.url");

    if(!tps_url)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences Can't find value for  esc.tps.url \n"));
        return;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences esc.tps.url %s\n",tps_url));

    string tps_url_str = tps_url;

    // determine whether or not we are SSL

    string ssl_str =     "https://";
    string non_ssl_str = "http://";

    size_t pos = tps_url_str.find(ssl_str,0);

    mSSL = 0;

    if(pos == 0)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences SSL on for tps url\n"));
        pos += ssl_str.length();
        mSSL= 1;
    }
    else
    {
        pos = tps_url_str.find(non_ssl_str,0);
        if(pos == string::npos)
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences esc.tps.url illegal protocol! \n")); 
            return;
        }

        pos+= non_ssl_str.length();

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences SSL off for tps url.\n"));
    }

    // Now grab the host name and host port from the tps url

    string host_name_port_str = "";
    string slash_str = "/";

    size_t end_host_port_pos = tps_url_str.find(slash_str,pos);
    size_t  end_host_port_count = 0;

    if(end_host_port_pos == string::npos)
    {
        end_host_port_count  = tps_url_str.length() - pos ;
    }
    else
    {
        end_host_port_count = end_host_port_pos - pos;
    } 

    string tps_url_offset = tps_url_str.substr(end_host_port_pos);

    if(!tps_url_offset.length())
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences NULL tps_url_offset string!.\n"));
        return;
    }


    mRAUrl = strdup(tps_url_offset.c_str());

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences  tps_url_offset string! %s.\n",tps_url_offset.c_str()));

    host_name_port_str = tps_url_str.substr(pos,end_host_port_count);

    if(!host_name_port_str.length())
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences Bad hostname and port sttring!.\n"));
        return;
     }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences host_name_port %s.\n",host_name_port_str.c_str())); 

    string delim = ":";
    string port_num_str = "";

    size_t delimPos = host_name_port_str.find(delim,0);

    if(delimPos == string::npos)
    {
        mPort = 80;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences no port number assuming 80!.\n"));

        mCharHostName = strdup(host_name_port_str.c_str());
    }
    else
    {
        port_num_str = host_name_port_str.substr(delimPos + 1);
        string host_name_str = host_name_port_str.substr(0, delimPos);

        
        if(host_name_str.length())
        {
            mCharHostName = strdup(host_name_str.c_str());
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences mCharHostName %s!.\n",mCharHostName));
        }
    }
   
    if(port_num_str.length())
    {
        mPort = atoi(port_num_str.c_str());

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences port_num_str %s.\n",port_num_str.c_str()));
    } 

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CollectPreferences port number %d.\n",mPort));
}


HRESULT CoolKeyHandler::SetAuthParameter(const char *param_id, const char *value)
{

    PR_Lock(mDataLock);

    string pId = "";

    if(param_id)
        pId = param_id;

        nsNKeyREQUIRED_PARAMETER *param = mReqParamList.GetById(pId);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::SetAuthParameter :result of GetById %p",param));

        if(param)
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::SetAuthParameter found and setting id %s value %s:\n",param_id,value));

            string val = "";

            if(value)
                val = value;

            param->setValue(val);

            if(mReqParamList.AreAllParametersSet())
            {
                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler :All auth parameters set, notify enrollment"));

                PR_NotifyCondVar(mDataCondVar);
            }
        }

     PR_Unlock(mDataLock);
 
     return S_OK;
}

HRESULT CoolKeyHandler::SetScreenName(const char *screenName)
{

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::SetScreenName:\n"));

   PR_Lock(mDataLock);
   if(!mCharScreenName)
   {

       mCharScreenName = strdup(screenName);

   }

   if(mCharScreenName && mCharScreenNamePwd)
   {
       PR_NotifyCondVar(mDataCondVar);
   }

   PR_Unlock(mDataLock);
   return S_OK;    

}

HRESULT CoolKeyHandler::SetTokenPin(const char *pin)
{

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::SetTokenPin:\n"));

   PR_Lock(mDataLock);
   if(!mCharPIN)
   {
        mCharPIN = strdup(pin);
   }

   if(mCharPIN)    {
       PR_NotifyCondVar(mDataCondVar);
   }

   PR_Unlock(mDataLock);

   return S_OK;
}

HRESULT CoolKeyHandler::SetPassword(const char *password)
{

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CloseConnection:\n"));

   PR_Lock(mDataLock);
   if(!mCharScreenNamePwd)
   {
        mCharScreenNamePwd = strdup(password);
   }

   if(mCharScreenName && mCharScreenNamePwd)
   {
       PR_NotifyCondVar(mDataCondVar);
   }
   
   PR_Unlock(mDataLock);

   return S_OK;
}

HRESULT CoolKeyHandler::CloseConnection()
{
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::CloseConnection:\n"));
    if(mHttp_handle)
    {
        ::httpCloseConnection(mHttp_handle);
    }

    return S_OK;
}

HRESULT CoolKeyHandler::Enroll(const char *aTokenType)
{
  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Enroll:\n"));
  mState = ENROLL;

  if(aTokenType)
	mCharTokenType = strdup(aTokenType);

  HRESULT res = S_OK;

  if(mHttp_handle > 0)
  {
	  if(!mPDUWriter)
	  {
		res = HttpBeginOpRequest();
		return res;

	  }
	  else
	  {
		  return mPDUWriter->QueueOnConnectEvent(this,mHttp_handle);
	  }
  }
  return E_FAIL;
}

HRESULT CoolKeyHandler::ResetPIN()
{
  mState = RESET_PIN;

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::ResetPIN:\n"));
  HRESULT res = S_OK;

  if(mHttp_handle > 0)
  {
	  if(!mPDUWriter)
	  {
		res = HttpBeginOpRequest();
		return res;
	  }
	  else
	  {
		  return mPDUWriter->QueueOnConnectEvent(this,mHttp_handle);
	  }
  }
  
  return E_FAIL; 
}

HRESULT CoolKeyHandler::Renew()
{
  mState = RENEW;
  
  return S_OK;
}

HRESULT CoolKeyHandler::Format( const char *aTokenType)
{
  mState = FORMAT;

  HRESULT res = S_OK;

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::Format:\n"));  

  
  if(aTokenType)
	mCharTokenType = strdup(aTokenType);


  if(mHttp_handle > 0)
  {
	  if(!mPDUWriter)
	  {
		  res = HttpBeginOpRequest();
		  return res;
	  }
	  else
	  {
		return mPDUWriter->QueueOnConnectEvent(this,mHttp_handle);
	  }
  }

  return E_FAIL;
}

HRESULT CoolKeyHandler::GetAuthDataFromUser(const char* ui)
{

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetAuthDataFromUser\n"));

    if(!ui)
    {
        return E_FAIL;
    }

     CoolKeyNotify(&mKey, eCKState_NeedAuth, 0,ui);

    while (1) {
        PR_Lock(mDataLock);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetAuthDataFromUser before PR_WaitCondVar\n"));

        PR_WaitCondVar(mDataCondVar, PR_INTERVAL_NO_TIMEOUT);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetAuthDataFromUser after PR_WaitCondVar\n"));
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetAuthDataFromUser got our required auth data,unlocking lock.\n"));
            PR_Unlock(mDataLock);
            break;
        }


   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetAuthDataFromUser got notification from user.\n"));

   return S_OK;
}

/* HRESULT CoolKeyHandler::GetTokenPinFromUser()
{

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetTokenPinFromUser\n"));

    CoolKeyNotify(&mKey, eCKState_NeedTokenPin, 0);

    while (1) {
        PR_Lock(mDataLock);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetTokenPinFromUser before PR_WaitCondVar\n"));         

        PR_WaitCondVar(mDataCondVar, PR_INTERVAL_NO_TIMEOUT);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetTokenPinFromUser after PR_WaitCondVar\n"));         
        if(mCharPIN)
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetTokenPinFromUser got uid and pword unlocking lock.\n"));
            PR_Unlock(mDataLock);
            break;
        }

   }

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser got notification from user.\n"));

   return S_OK;

}

HRESULT CoolKeyHandler::GetUidPwordFromUser()
{

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser\n"));

    CoolKeyNotify(&mKey, eCKState_NeedUidPword, 0);

    while (1) {
        PR_Lock(mDataLock);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser before PR_WaitCondVar\n"));
        
        PR_WaitCondVar(mDataCondVar, PR_INTERVAL_NO_TIMEOUT);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser after PR_WaitCondVar\n"));
        if(mCharScreenName && mCharScreenNamePwd)
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser got uid and pword unlocking lock.\n"));
            PR_Unlock(mDataLock);
            break;
        }

   }

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::GetUidPwordFromUser got notification from user.\n")); 

   return S_OK;

}

*/
HRESULT CoolKeyHandler::Disconnect()
{
  return S_OK;
}

HRESULT CoolKeyHandler::OnConnectImpl()
{
	return HttpBeginOpRequest();

}

HRESULT CoolKeyHandler::OnDisConnectImpl()
{
	return HttpOnDisconnect();

}

HRESULT CoolKeyHandler::HttpOnDisconnect()
{

     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpOnDisconnect:\n"));

	 int present = 0;

	 present = IsNodeInActiveKeyList(&mKey);

  if(!present)
	  return S_OK;

  
	
	if(mHttpDisconnected )
		return S_OK;


	mHttpDisconnected = true;

  // Clean up the smartcard objects //
  DisconnectFromReader();
  
  if (!mReceivedEndOp && !isCancelled()) {
    NotifyEndResult(this, mState, 1, 28 /*disconnect */);
  }
  else
    CloseConnection();
  

  //RemoveKeyFromActiveKeyList(&mKey);
  return S_OK;// ProxyEventToUIThread(new RemoveKeyFromActiveKeyListEvent(&mKey));
}

HRESULT CoolKeyHandler::ProcessMessageHttp(eCKMessage *msg)
{
	HRESULT rv = S_OK;
  
	eCKMessage::sntype type = msg->getMessageType();

         PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::ProcessMessageHttp: type %d\n",(int)type));
	switch(type)
	{
            case eCKMessage::EXTENDED_LOGIN_REQUEST:
               HttpSendAuthResponse((eCKMessage_EXTENDED_LOGIN_REQUEST *) msg);
            break;
          	
            case eCKMessage::LOGIN_REQUEST:
                HttpSendUsernameAndPW();
            break;
          
	    case eCKMessage::NEW_PIN_REQUEST:
              HttpSendNewPin((eCKMessage_NEWPIN_REQUEST *) msg);
            break;
          
            case eCKMessage::SECURID_REQUEST:
			HttpSendSecurID((eCKMessage_SECURID_REQUEST *) msg);
          //SendSecurID(""); // XXX: We'll need to handle this at some point!
            break;
          
	    case eCKMessage::TOKEN_PDU_REQUEST:
               HttpProcessTokenPDU(this,(eCKMessage_TOKEN_PDU_REQUEST *) msg);
            break;
          
	    case eCKMessage::END_OP:
                HttpProcessEndOp(this, (eCKMessage_END_OP *) msg);
            break;

            case eCKMessage::STATUS_UPDATE_REQUEST:
                HttpProcessStatusUpdate((eCKMessage_STATUS_UPDATE_REQUEST *) msg);
	    break;

            default:
                rv = E_FAIL;
            break;
        break;

	}
   
  
  return rv;
}

HRESULT CoolKeyHandler::HttpBeginOpRequest()
{
        int regular_login = 0;

	if(mHttp_handle <= 0)
	{
	  return E_FAIL;
	}

	char ascii_port[50];
	char host_port[200];
	char *method = "POST";

        if(mCharScreenName && mCharScreenNamePwd)
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpBeginOpRequest Attempting regular login, no extended login capabilities.n"));

            regular_login = 1;

        }

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpBeginOpRequest.n"));
	sprintf(ascii_port,"%d",mPort);
	sprintf(host_port,"%s:%s",mCharHostName,ascii_port);
		
	eCKMessage_BEGIN_OP begin_op;
	
	begin_op.setOperation(mState);
	char buffer[2048];

	if(!mRAUrl)
	{
		return E_FAIL;
	}

	if(mState == ENROLL || mState == FORMAT)
	{
		sprintf(buffer,"tokenType=%s",mCharTokenType);

                string buffer_str = buffer;
		begin_op.AddExtensionValue(buffer_str);
	} 

        string ext_buffer = "";

	char *clientVer = "ESC 1.1";
	sprintf(buffer,"clientVersion=%s",clientVer);

        ext_buffer = buffer;
	 
	begin_op.AddExtensionValue(ext_buffer); // The client version string
  
  
	const char *atr = GetATRForKeyID(&mKey);
	if (!atr)
		return E_FAIL;

	sprintf(buffer,"tokenATR=%s",atr);
  
 
        ext_buffer = buffer;
 
	begin_op.AddExtensionValue(ext_buffer);

	char *status = (char *) ((mStatusRequest) ? "true" : "false");
  
	sprintf(buffer,"statusUpdate=%s",status);

        ext_buffer = buffer;

	begin_op.AddExtensionValue(ext_buffer);

        if(!regular_login)
        { 
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpBeginOpRequest Attempting extended login.n"));

            sprintf(buffer,"extendedLoginRequest=%s","true");
            ext_buffer = buffer;
            begin_op.AddExtensionValue(ext_buffer);
        }

        string output = "";

        begin_op.encode(output);
 
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpBeginOpRequest hostport %s, data %s\n",host_port,output.c_str())); 
	int result = httpSendChunked(host_port,
	    mRAUrl,method,(char *)output.c_str(),HttpChunkedEntityCB,(void *)this,mHttp_handle,mSSL,mHttpRequestTimeout);


	if(!result)
	{
                HttpOnDisconnect();

                RemoveKeyFromActiveKeyList(&mKey);
		return E_FAIL;
	}
	else
	{
                RemoveKeyFromActiveKeyList(&mKey);
		return S_OK;
	}
}

void CoolKeyHandler::HttpProcessTokenPDU(CoolKeyHandler *context,eCKMessage_TOKEN_PDU_REQUEST *req)
{

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::ProcessTokenPDU:\n"));
	if(!req)
	{
		return;
	}

        int size = 4096;
        unsigned char pduData[4096];


        req->getPduData(pduData,&size);


	if(size == 0)
	{
		return;
	}
  
  // Send the PDU to the token
  
  // XXX: We don't have an CKYAPDU_InitFromData() method that
  //      allows us to set up an APDU structure from raw APDU data.
  //      This is a temp hack that works for now because the CKYAPDU
  //      structure is a C struct that contains a single member
  //      which just so happens to be an CKYBuffer.
  
  AutoCKYBuffer pduBuffer(pduData, size);
  CKYAPDU *requestAPDU = (CKYAPDU*)((CKYBuffer*)&pduBuffer);
  
  // XXX
  
   
  AutoCKYBuffer response;
  
  CKYStatus status = CKYCardConnection_ExchangeAPDU(context->GetCardConnection(),
                                                  requestAPDU, &response);
    
  if (status != CKYSUCCESS) {
    return;
  }
  
	eCKMessage_TOKEN_PDU_RESPONSE pdu_response;

	int pduSizeRet = (MESSAGE_u08) CKYBuffer_Size(&response);
	MESSAGE_byte *pduDataRet = (MESSAGE_byte *) CKYBuffer_Data(&response);

	if(pduSizeRet == 0 || !pduDataRet)
	{
		return;
	}

	pdu_response.setPduData(pduDataRet,pduSizeRet);

        string output = "";

        pdu_response.encode(output);
  
	NSS_HTTP_HANDLE handle = context->getHttpHandle();

	if(handle && output.size())
	{
                 PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::sending to RA: %s \n",output.c_str()));
		NSS_HTTP_RESULT res =  sendChunkedEntityData(output.size(),(unsigned char *) output.c_str(),handle);

                if(res == 0)
                {
                     PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::write back to RA failed , disconnecting: \n"));

                     context->CloseConnection();
                     context->HttpOnDisconnect();
                    
                }

	}
	return;
  }
  
HRESULT CoolKeyHandler::HttpProcessStatusUpdate(eCKMessage_STATUS_UPDATE_REQUEST * msg)
{
	HRESULT rv = S_OK;

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpProcessStatusUpdate:  \n"));

	if(!msg)
	{
		return E_FAIL;
	}

	MESSAGE_u08 current_state = msg->getCurrentState();
	string next_task_name = msg->getNextTaskName();

	CoolKeyNotify(&mKey, eCKState_StatusUpdate, current_state);
	eCKMessage_STATUS_UPDATE_RESPONSE response;

	response.setCurrentState(0);

        string output = "";

        response.encode(output);

	int len = output.size();

	NSS_HTTP_HANDLE handle = mHttp_handle;

	if(len && handle)
	{
                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler:: next task name %s sending to RA: %s \n",next_task_name.c_str(),output.c_str()));

		NSS_HTTP_RESULT res =  sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

		if(!res)
			rv = E_FAIL;
			
	}

	return rv;
}

HRESULT CoolKeyHandler::HttpSendSecurID(eCKMessage_SECURID_REQUEST *req)
 {
	 HRESULT rv = S_OK;

         PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendSecurID:  \n"));

	 char* pin = NULL;

	 if(!req)
	 {
		 rv = E_FAIL;
		 return rv;
	 }

	 eCKMessage_SECURID_RESPONSE response;

	 if(req->getPinRequired())
	 {
		 pin = (char *) mCharTokenCode;

	 }

         string pin_str = "";
         string value_str = "";


         if(pin)
             value_str = pin;


	 response.setPin(pin_str);
	 response.setValue(value_str);

         string output = "";

         response.encode(output);


	int len = output.size();

	NSS_HTTP_HANDLE handle = mHttp_handle;

	if(len && handle)
	{
                PR_LOG(coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::sending to RA: %s \n",output.c_str()));

		sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);
	}

	return rv;
    
}

HRESULT CoolKeyHandler::HttpSendUsernameAndPW()
{
	HRESULT rv = S_OK;

	eCKMessage_LOGIN_RESPONSE response;

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendUsernameAndPW:  \n"));

        string screen_name_str = "";

        if(mCharScreenName)
            screen_name_str = mCharScreenName;


        string sn_pwd_str = "";

        if(mCharScreenNamePwd)
            sn_pwd_str = mCharScreenNamePwd;


	response.setScreenName(screen_name_str);
	response.setPassWord(sn_pwd_str);

        string output = "";

        response.encode(output);

	int len = output.size();
  
	NSS_HTTP_HANDLE handle = mHttp_handle;

	if(len && handle)
	{
                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::sending to RA: %s \n",output.c_str()));

		NSS_HTTP_RESULT res =  sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

		if(!res)
		{
			rv = E_FAIL;
		}
	}
	else
	{
		rv = E_FAIL;
	}

	return rv;
}

HRESULT CoolKeyHandler::HttpSendAuthResponse(eCKMessage_EXTENDED_LOGIN_REQUEST *req)
{

        HRESULT rv = S_OK;

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendAuthResponse: \n"));
        if(!req)
        {
                return E_FAIL;
        }

        eCKMessage_EXTENDED_LOGIN_RESPONSE response;

        nsNKeyREQUIRED_PARAMETERS_LIST *param_list = req->GetReqParametersList();

        string pListBuffer = "";

        string buff_final_str = "";

        if(param_list)
        { 
            response.SetReqParametersList(param_list);

            param_list->EmitToBuffer(pListBuffer);
        }

        string titleStr = req->getTitle();

        string descStr = req->getDescription();

        string titleStrDec = "";
        string descStrDec = "";

        URLDecode_str(titleStr,titleStrDec);
        URLDecode_str(descStr,descStrDec);


        if(titleStrDec.size())
        {

            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendAuthResponse:  title %s\n",titleStrDec.c_str()));

            {

                buff_final_str = "title=" + titleStrDec + "&&";

                if(descStrDec.size())
                {
                     buff_final_str += "description=" + descStrDec + "&&";

                }

                buff_final_str += pListBuffer;
            }

        }

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendAuthResponse:  ui buffer %s\n",(char *) buff_final_str.c_str()));


        GetAuthDataFromUser((const char *)buff_final_str.c_str());

        string output = "";

        response.encode(output);

        int len = output.size();

        mReqParamList.CleanUp();

        NSS_HTTP_HANDLE handle = mHttp_handle;


        if(handle)
        {
                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::sending to RA: %s \n",output.c_str()));

                NSS_HTTP_RESULT res =  sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

                if(!res)
                {
                        rv = E_FAIL;
                }
        }
        else
        {
                rv = E_FAIL;
        }

        return rv;

}

HRESULT CoolKeyHandler::HttpSendNewPin(eCKMessage_NEWPIN_REQUEST  *req)
{
	HRESULT rv = S_OK;

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpSendNewPin: \n"));

	if(!req)
	{
		return E_FAIL;
	}

	eCKMessage_NEWPIN_RESPONSE response;

        string pin_str = "";

	if(mCharPIN)
	{
                pin_str = mCharPIN;
		response.setNewPin(pin_str);
	}

        string output = "";

        response.encode(output);


        int len = output.size();
  
	NSS_HTTP_HANDLE handle = mHttp_handle;

	if(len && handle)
	{
                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::sending to RA: %s \n",output.c_str()));

		NSS_HTTP_RESULT res =  sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

		if(!res)
		{
			rv = E_FAIL;
		}
	}
	else
	{
		rv = E_FAIL;
	}

	return rv;
}

void CoolKeyHandler::HttpProcessEndOp(CoolKeyHandler* context, eCKMessage_END_OP *end)
{

	if(!context || !end)
	{
		return;
	}

	MESSAGE_s32 operation = end->getOperation();
	MESSAGE_s32 result = end->getResult();
	MESSAGE_s32 message = end->getMessage();

	context->mReceivedEndOp = true;

	if ((operation == (int) eCKMessage_BEGIN_OP::ENROLL || operation == (int) eCKMessage_BEGIN_OP::UPDATE) && (result == 0)) {
    // Reset the card.
	CKYCardConnection_Reset(context->mCardConnection);
    
    // do we need to tell anything?
  }
  	
  NotifyEndResult(context, operation, result,message);

  
  context->HttpOnDisconnect();

}

void NotifyEndResult(CoolKeyHandler* context, int operation, int result, int description)
{
  RefreshInfoFlagsForKeyID(context->GetAutoCoolKey());

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::NotifyEndResult context %p op %d result %d description %d:\n",context,operation,result,description)); 
 
  switch (operation) {
  case ENROLL:
    if (result == 0) {
      CoolKeyAuthenticate(context->GetAutoCoolKey(), context->GetPIN());
      CoolKeyNotify(context->GetAutoCoolKey(), eCKState_EnrollmentComplete,
                   context->GetScreenName() == NULL ? 1 : 0);
    } else {
		CoolKeyNotify(context->GetAutoCoolKey(), eCKState_EnrollmentError, description); // XXX: Need INIT_FAILED error code!
    }
    break;
  case RESET_PIN:
    if (result == 0) {
      CoolKeyAuthenticate(context->GetAutoCoolKey(), context->GetPIN());
      CoolKeyNotify(context->GetAutoCoolKey(), eCKState_PINResetComplete, 0);
    } else {
      CoolKeyNotify(context->GetAutoCoolKey(), eCKState_PINResetError, description); // XXX: Need PIN_RESET_FAILED error code!
    }
    break;
  case FORMAT:
    if (result == 0) {
      CoolKeyNotify(context->GetAutoCoolKey(), eCKState_FormatComplete, 0);
    } else {
      CoolKeyNotify(context->GetAutoCoolKey(), eCKState_FormatError, description); // XXX: Need FORMAT_FAILED error code!
    }
    break;
  default:
    break;
  }
}

bool CoolKeyHandler::ConnectToReader(const char* readerName)
{
  bool connected = false;

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::ConnectToReader:\n")); 
  CoolKeyInfo *info = NULL;
  CKYStatus status;
 
  mCardConnection = CKYCardConnection_Create(mCardContext);
  assert(mCardConnection);
  if (!mCardConnection) {
    goto done;
  }
  
  status = CKYCardConnection_Connect(mCardConnection, readerName);
  if (status != CKYSUCCESS) {
    goto done;
  }
  
  info = GetCoolKeyInfoByReaderName(readerName);
  if (!info) {
    goto done;
  }
  
  if (HAS_ATR(info->mInfoFlags)) {
    // If the token has an applet, select it.
    if (HAS_APPLET(info->mInfoFlags)) {
      CKYISOStatus apduRC = 0;
      status = CKYApplet_SelectCoolKeyManager(mCardConnection, &apduRC);
      if (status != CKYSUCCESS) {
        goto done;
      }
      if (apduRC == CKYISO_SUCCESS) {
        connected = true;
      }
    }
    else {
      // We allow connections to tokens that don't have
      // applets so that we can FORMAT them.
      connected = true;
    }
  }
  
 done:
  
  if (!connected) {
    if (mCardConnection) {
      CKYCardConnection_Disconnect(mCardConnection);
      CKYCardConnection_Destroy(mCardConnection);
      mCardConnection = 0;
    }
  }
  
  return connected;
}

//Gets called when a chunk is available from the server

bool
CoolKeyHandler::HttpChunkedEntityCB(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status)
{

	if(uw)
	{
		return	((CoolKeyHandler *) uw)->HttpChunkedEntityCBImpl(entity_data,entity_data_len,uw,status);

	}

	return false;
	
}

bool
CoolKeyHandler::HttpChunkedEntityCBImpl(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status)
{

         PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpChunkedEntityCBImpl: data %s\n",(char *) entity_data));
	CoolKeyHandler *handler = NULL;
	handler = (CoolKeyHandler *) uw;
	HRESULT res = E_FAIL;
	bool ret = false;
	
	if(!handler || !entity_data)
	{
		return ret;
	}

	if(status == NSS_HTTP_CHUNKED_EOF && entity_data_len == 0)  //Check for end of data from server
	{
		if(!handler->mReceivedEndOp)
		{
			handler->HttpOnDisconnect();
			return ret;
		}
		else
		{
			handler->HttpOnDisconnect();
			ret = true;
			return ret;

		}
	}
	
	//We have some data, see what message

    eCKMessage *sn = NULL;
	eCKMessage::sntype t = eCKMessage::UNKNOWN_MESSAGE;

	if((status == NSS_HTTP_CHUNK_COMPLETE || status == NSS_HTTP_CHUNKED_EOF) && entity_data_len > 0)
	{
                string message_str = "";

                if(entity_data)
                    message_str = (char *) entity_data;

		t = eCKMessage::decodeMESSAGEType( message_str);

                PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpChunkedEntiryCB, message type %d,\n",(int)t));

		if(t == eCKMessage::UNKNOWN_MESSAGE)
		{
			handler->HttpOnDisconnect();
			return ret;
		}
		sn = handler->AllocateMessage(t,entity_data,entity_data_len);
	
		if(!sn)
		{
			handler->HttpOnDisconnect();
			return ret;
		}

		res = handler->ProcessMessageHttp(sn);
	}

	if(sn)
	{
		delete sn;
	}

	if(res == S_OK)
	{
		ret = true;
		
	}
	else
	{
		handler->HttpOnDisconnect();
	}

	return ret;
}

eCKMessage *CoolKeyHandler::AllocateMessage(eCKMessage::sntype type,unsigned char *data, unsigned size)
{

    unsigned char *lData = data;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::AllocateMessage %d :\n",(int)type));
    eCKMessage *sn = NULL;
    eCKMessage::sntype t = type;

    int input_name_values = 0;

    if(data != NULL && size != 0)
    {
        input_name_values = 1;
    }

    switch (t) {
        case eCKMessage::BEGIN_OP:
                 
	        sn = new eCKMessage_BEGIN_OP();
                    
        break;

        case eCKMessage::LOGIN_REQUEST:

            sn = new eCKMessage_LOGIN_REQUEST();

        break;
        case eCKMessage::EXTENDED_LOGIN_REQUEST:

            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::AllocateMessage,got EXTENDED_LOGIN_REQUEST mReqParamList %p\n" ,&mReqParamList));

            sn = new eCKMessage_EXTENDED_LOGIN_REQUEST();

                ((eCKMessage_EXTENDED_LOGIN_REQUEST *)sn)->SetReqParametersList(&mReqParamList);
                    

//            lData = (unsigned char *)test_extended_login;
//           lSize = strlen((char *)test_extended_login);

        break;

        case eCKMessage::LOGIN_RESPONSE:
         
	        sn = new eCKMessage_LOGIN_RESPONSE();
            
        break;

        case eCKMessage::SECURID_REQUEST:
            
	        sn = new eCKMessage_SECURID_REQUEST();
            
        break;

        case eCKMessage::SECURID_RESPONSE:

		    sn = new eCKMessage_SECURID_RESPONSE();
          
        break;

/*       case eCKMessage::ASQ_REQUEST:
            
			sn = new eCKMessage_ASQ_REQUEST();
            
        break;

        case eCKMessage::ASQ_RESPONSE:
            
	        sn = new eCKMessage_ASQ_RESPONSE();
            
        break;
*/   
        case eCKMessage::TOKEN_PDU_REQUEST:
           
	        sn = new eCKMessage_TOKEN_PDU_REQUEST();
            
        break;

        case eCKMessage::TOKEN_PDU_RESPONSE:
            
	        sn = new eCKMessage_TOKEN_PDU_RESPONSE();
            
        break;

        case eCKMessage::NEW_PIN_REQUEST:
            
	        sn = new eCKMessage_NEWPIN_REQUEST();
            
        break;

        case eCKMessage::NEW_PIN_RESPONSE:
            
	        sn = new eCKMessage_NEWPIN_RESPONSE();
            
        break;

        case eCKMessage::END_OP:
            
	        sn = new eCKMessage_END_OP();
            
        break;

        case eCKMessage::STATUS_UPDATE_REQUEST:
            
	        sn = new eCKMessage_STATUS_UPDATE_REQUEST();
            
        break;

        case eCKMessage::STATUS_UPDATE_RESPONSE:
            
	        sn = new eCKMessage_STATUS_UPDATE_RESPONSE();
            
        break;

        case eCKMessage::UNKNOWN_MESSAGE:
                        // FREAK OUT
                default:
                        break;
        }

		if(input_name_values)
		{
			int result = 0;

			if(sn)
			{

                                string input = "";

                                if(lData)
                                    input = (char *) lData;
                                sn->decode(input);

				if(result != MESSAGE_SUCCESS)
				{
					delete sn;
					return NULL;
				}
			}

		}


        return sn;
}

void
CoolKeyHandler::DisconnectFromReader()
{

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::DisconnectFromReader:\n"));
  if (mCardConnection) {
    CKYCardConnection_Disconnect(mCardConnection);
    CKYCardConnection_Destroy(mCardConnection);
    mCardConnection = 0;
  }

  if (mCardContext) {
    CKYCardContext_Destroy(mCardContext);
    mCardContext = 0;
  }
}

/*
 * make PKCS 11 token info to COOLKEY_INFO flags
 */
static unsigned int
MapGetFlags(CK_TOKEN_INFO *tokenInfo)
{
  unsigned int mask = COOLKEY_INFO_HAS_ATR_MASK; /*always true if we're found*/

  if (tokenInfo->firmwareVersion.major >= 1) {
    mask |= COOLKEY_INFO_HAS_APPLET_MASK;
  }
  if (tokenInfo->flags & CKF_TOKEN_INITIALIZED) {
    mask |= COOLKEY_INFO_IS_PERSONALIZED_MASK;
  }
  return mask;

}

//
// simple little function to format a serial number in
// standardized 'storage' form. Returns a pointer to
// the next position in 'dest'
//
static char *
copySerialNumber(char *dest, const char *src, int srcLen)
{
  char *cp = dest;
  int i;

  for (i=0; i < srcLen; i++) {
    char c = src[i];
    // strip the dashes & spaces
    if ((c == '-') || (c == ' ')) {
      continue;
    }
    if (isupper(c)) {
      c = tolower(c);
    }
    *cp++ = c;
  }
  return cp;
}

//
// Sigh This is not yet finalized. This is coded defensively with
// an eye towards forward compatibility.
// There are two ways the CUID is stored in the token info structure.
//  1) 2 CUID bytes (4 hex digits) stored in the manufacturer (this is
// the manufacturer portion of the CUID. The remaining 8 bytes (16 hex digits)
// are stored in the model number field. This left the serial field to
// display the MSN if it was available.
//  2) Some top portion of the CUID is stored in the model, and the
// remainder in the Serial number. Since this is displayed to the user
// it will probably have Dashes and store in lower case some time in the
// future.
//
static HRESULT
getCUIDFromTokenInfo(CK_TOKEN_INFO *tokenInfo, char *tokenSerialNumber)
{
  char *cp = tokenSerialNumber;

  if (isxdigit(tokenInfo->manufacturerID[0]) &&
      isxdigit(tokenInfo->manufacturerID[1]) &&
      isxdigit(tokenInfo->manufacturerID[2]) &&
      isxdigit(tokenInfo->manufacturerID[3]) ) {
    // one format has top 2 bytes of CUID (4 hex digits) in manufacturer,
    // and the rest in the module 
    cp = copySerialNumber(cp, (const char *)tokenInfo->manufacturerID, 4);
    cp = copySerialNumber(cp,(const char *)tokenInfo->model,
                                              sizeof(tokenInfo->model)); 
  } else {
    // otherwise it's just the concatenation of the model and serial
    // fields 
    cp = copySerialNumber(cp, (const char *)tokenInfo->model,
                                              sizeof(tokenInfo->model));
    cp = copySerialNumber(cp, (const char *)tokenInfo->serialNumber, 
                                              sizeof(tokenInfo->serialNumber));
  }
  *cp=0;

  return S_OK;
}

unsigned int 
CKHGetInfoFlags(PK11SlotInfo *aSlot)
{
  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CKHGetInfoFlags:\n"));
  CK_TOKEN_INFO tokenInfo;
  SECStatus status = PK11_GetTokenInfo(aSlot, &tokenInfo);
  if (status != SECSuccess) {
    return 0;
  }
  return MapGetFlags(&tokenInfo);
}

CoolKeyInfo * 
CKHGetCoolKeyInfo(PK11SlotInfo *aSlot)
{

  PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CKHGetCoolKeyInfo:\n"));
  PK11GenericObject *obj = NULL;
  SECItem label, ATR;
  CK_TOKEN_INFO tokenInfo;
  CoolKeyInfo *info = NULL;
  SECStatus status;
  HRESULT hres;
  int atrSize;
  char *atrString;

  ATR.data = NULL; // initialize for error processing
  label.data = NULL; // initialize for error processing

  /* if it's one of "ours" it'll have a reader object */
  obj = PK11_FindGenericObjects(aSlot, CKO_MOZILLA_READER);
  if (obj == NULL) {
    goto failed;
  }

  // get the reader name (though we probably don't need it anymore 
  status = PK11_ReadRawAttribute(PK11_TypeGeneric, obj, CKA_LABEL, &label); 
  if (status != SECSuccess) {
    goto  failed;
  }

  // get the ATR (though, again, we probably don't need it 
  status = PK11_ReadRawAttribute(PK11_TypeGeneric, obj, CKA_MOZILLA_ATR, &ATR); 
  PK11_DestroyGenericObjects(obj);
  obj = NULL;
  if (status != SECSuccess) {
    goto failed;
  }
  // get the CUID/Serial number (we *WILL* continue to need it )
  status = PK11_GetTokenInfo(aSlot,&tokenInfo);
  if (status != SECSuccess) {
    goto failed;
  }

  // OK, we have everything we need, now build the COOLKEYInfo structure.
  info = new CoolKeyInfo();
  if (!info) {
    goto failed;
  }

  atrSize = ATR.len*2+5;
  atrString = (char *)malloc(atrSize);
  hres = CoolKeyBinToHex(ATR.data, ATR.len, 
				(unsigned char *) atrString, atrSize, true);
  /* shouldn't the be != S_SUCCESS? */
  if (hres == E_FAIL) {
    free(atrString);
    goto failed;
  }
  SECITEM_FreeItem(&ATR,PR_FALSE);
  ATR.data = NULL;

  info->mATR = atrString;
  info->mReaderName= (char *)malloc(label.len+1);
  if (!info->mReaderName) {
    goto failed;
  }
  memcpy(info->mReaderName, label.data, label.len);
  info->mReaderName[label.len] = 0;
  info->mInfoFlags = MapGetFlags(&tokenInfo);
  info->mCUID = (char *)malloc(35); /* should be a define ! */
  if (!info->mCUID) {
    goto failed;
  }
  hres = getCUIDFromTokenInfo(&tokenInfo, info->mCUID);
  /* shouldn't the be != S_SUCCESS? */
  if (hres == E_FAIL) {
    goto failed;
  }

  SECITEM_FreeItem(&ATR,PR_FALSE);
  SECITEM_FreeItem(&label,PR_FALSE);
  info->mSlot = PK11_ReferenceSlot(aSlot);
  info->mSeries = PK11_GetSlotSeries(aSlot);
  return info;

failed:
  if (ATR.data) {
    SECITEM_FreeItem(&ATR,PR_FALSE);
  }
  if (label.data) {
    SECITEM_FreeItem(&label,PR_FALSE);
  }
  if (obj) {
    PK11_DestroyGenericObjects(obj);
  }
  if (info) {
    delete info;
  }
  return NULL;
}
