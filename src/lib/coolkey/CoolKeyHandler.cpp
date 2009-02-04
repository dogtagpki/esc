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

#define FORCE_PR_LOG 1

#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <time.h>

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
#include "cky_base.h"
#include "cky_applet.h"

#include "NSSManager.h"
#include "CoolKeyHandler.h"
#include "SlotUtils.h"


//static  char *test_extended_login = "s=325&msg_type=13&invalid_login=0&blocked=0&error=&required_parameter0=id%3DUSER%5FID%26name%3DUser+ID%26desc%3DUser+ID%26type%3Dstring%26option%3Doption1%2Coption2%2Coption3&required_parameter1=id%3DUSER%5FPWD%26name%3DUser+Password%26desc%3DUser+Password%26type%3Dpassword%26option%3D&required_parameter2=id%3DUSER%5FPIN%26name%3DPIN%26desc%3DOne+time+PIN+received+via+mail%26type%3Dpassword%26option%3D";

#include <string>
#ifndef CKO_NETSCAPE
#define CKO_NETSCAPE CKO_NSS
#endif
#ifndef CKO_MOZILLA_READER
#define CKO_MOZILLA_READER     (CKO_NETSCAPE+5)
#define CKA_MOZILLA_IS_COOL_KEY (CKO_NETSCAPE+24)
#define CKA_MOZILLA_ATR        (CKO_NETSCAPE+25)
#endif

static PRLogModuleInfo *coolKeyLogHN = PR_NewLogModule("coolKeyHandler");

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
        char tBuff[56];
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s KHOnConnectEvent::Execute:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::~PDUWriterThread:\n",GetTStamp(tBuff,56)));

    if (mCondVar ) {

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::~PDUWriterThreade about to destroy mCondVar.\n",GetTStamp(tBuff,56)));

    PR_DestroyCondVar(mCondVar);
    mCondVar = NULL;
    }

    if (mLock ) {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::~PDUWriterThread  about to destroy mLock.\n",GetTStamp(tBuff,56)));

        PR_DestroyLock(mLock);
        mLock = NULL;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::~PDUWriterThread leaving....\n",GetTStamp(tBuff,56)));
}

void
PDUWriterThread::ThreadRun(void *arg)
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun:\n",GetTStamp(tBuff,56)));
    PDUWriterThread *pn = (PDUWriterThread*) arg;

    while (pn->mAccepting && pn->mLock && pn->mCondVar) {

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: top of while loop accepting %d lock %p cond %p\n",GetTStamp(tBuff,56),pn->mAccepting,pn->mLock,pn->mCondVar));
        PR_Lock(pn->mLock);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: done PR_Lock()\\n",GetTStamp(tBuff,56)));

        if ( pn->mCondVar && pn->mPendingEvents.empty())
            PR_WaitCondVar(pn->mCondVar, PR_INTERVAL_NO_TIMEOUT);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: done waiting on cond var\n",GetTStamp(tBuff,56)));

        if (pn->mPendingEvents.empty()) {
            PR_Unlock(pn->mLock);
            continue;
        }

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: have eventsg\n",GetTStamp(tBuff,56)));

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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: bottom of while loop\n",GetTStamp(tBuff,56)));
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::ThreadRun: no longer accepting\n",GetTStamp(tBuff,56)));

    if(pn)
    {
        delete pn;
    }
}

HRESULT
PDUWriterThread::Init()
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s PDUWriterThread::Init:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown.mThread %p\n",GetTStamp(tBuff,56),mThread));
    mAccepting = PR_FALSE;
 
    int same_thread = 0;
 
    if (PR_GetCurrentThread() != mThread) {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown.mThread  About to attempt to interrupt and  join mThread %p\n",GetTStamp(tBuff,56),mThread));
        PRStatus status = PR_Interrupt(mThread);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown. Result of interrupt Thread %d\n",GetTStamp(tBuff,56),status));
    
        status = PR_JoinThread(mThread);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown. done attempt join, result %d thread  %p\n",GetTStamp(tBuff,56),status,mThread));
    }
    else
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown. PR_CurrentThread is equal to PDUWriterThread",GetTStamp(tBuff,56)));

        same_thread = 1;
    }
  
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::PDUWriterThread::Shutdown.mThread %p leaving....\n",GetTStamp(tBuff,56),mThread));

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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler:\n",GetTStamp(tBuff,56)));

    if (mPDUWriter) {
        mPDUWriter->Shutdown();
    }

    if (mDataLock) {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: about to destroy mDataLock\n",GetTStamp(tBuff,56)));
        PR_DestroyLock(mDataLock);
        mDataLock = NULL;
    }

    if (mDataCondVar) {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: about to destroy mDataCondVar\n",GetTStamp(tBuff,56)));
        //PR_NotifyCondVar(mDataCondVar);
        PR_DestroyCondVar(mDataCondVar);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: about done destroying mDataCondVar\n",GetTStamp(tBuff,56)));

        mDataCondVar = NULL;
    }
 
    if(mCharTokenType)
    {
        free(mCharTokenType);
        mCharTokenType = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharTokenType\n",GetTStamp(tBuff,56)));

    }

    if(mCharScreenName)
    {
        free(mCharScreenName);
        mCharScreenName = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharScreenName\n",GetTStamp(tBuff,56)));

    }

    if(mCharPIN)
    {
        free(mCharPIN);
        mCharPIN = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharPIN\n",GetTStamp(tBuff,56)));

    }

    if(mCharHostName)
    {
        free(mCharHostName);
        mCharHostName = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharHostName\n",GetTStamp(tBuff,56)));

    }

    if(mRAUrl)
    {
        free(mRAUrl);
        mRAUrl = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mRAUrl\n",GetTStamp(tBuff,56)));

    }

    if(mCharScreenNamePwd)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: attempt to  free mCharScreenNamePwd\n",GetTStamp(tBuff,56)));  
        mCharScreenNamePwd = NULL;
        free(mCharScreenNamePwd);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharScreenNamePwd\n",GetTStamp(tBuff,56)));

    }
    if(mCharTokenCode)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: attempt to free mCharTokenCode\n",GetTStamp(tBuff,56)));
        free(mCharTokenCode);
        mCharTokenCode = NULL;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done free mCharTokenCode\n",GetTStamp(tBuff,56)));

    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: attempt mReqParamList.Cleanup %p\n",GetTStamp(tBuff,56),&mReqParamList)); 
    mReqParamList.CleanUp();

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done mReqParamList.CleanUp\n",GetTStamp(tBuff,56)));


    DisconnectFromReader();

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: done DisconnectFromReader\n",GetTStamp(tBuff,56)));
 
 
    assert(m_dwRef == 0);

    if(mHttp_handle)
    {
        httpDestroyClient(mHttp_handle);
        mHttp_handle = 0;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::~CoolKeyHandler: leaving\n",GetTStamp(tBuff,56)));
}

void CoolKeyHandler::AddRef()
{
    char tBuff[56];
    ++m_dwRef;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::AddRef count now %d:\n",GetTStamp(tBuff,56),m_dwRef));
}

void CoolKeyHandler::Release()
{
    char tBuff[56];
    assert(m_dwRef > 0);
    if (--m_dwRef == 0)
    {

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Release count now %d:\n",GetTStamp(tBuff,56),m_dwRef));
        delete this;
    } else
    {
  
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Release count now %d:\n",GetTStamp(tBuff,56),m_dwRef));

    }

}

HRESULT CoolKeyHandler::Init(const CoolKey *aKey,
                                           const char *screenName,
                                           const char *pin,const char *screenNamePwd,
										   const char *tokenCode,int op) {

    char tBuff[56];
    int error_no = 0;
    int config_error_no = CONFIG_ERROR;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Init:\n",GetTStamp(tBuff,56)));

    bool connected = false;
    PRThread*  cThread = NULL;
    const char *readerName =  NULL;

    if (!aKey || aKey->mKeyType != eCKType_CoolKey ||  !aKey->mKeyID) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Insuficient input parameters. \n",GetTStamp(tBuff,56));
      goto done;
    }
  
    readerName = GetReaderNameForKeyID(aKey);

    cThread = PR_GetCurrentThread();


    mKey = *aKey;

  
    if (!readerName) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Cannot locate card reader name! \n",GetTStamp(tBuff,56));
        goto done;
    }
 
    mDataLock = PR_NewLock();
    if (!mDataLock)
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation.  Cannnot initialize internal locking mechanism.\n",GetTStamp(tBuff,56));
        return E_FAIL;

    }

    mDataCondVar = PR_NewCondVar(mDataLock);
    if (!mDataCondVar)
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Cannot initialize internal syncronization mechanism.\n",GetTStamp(tBuff,56));
        return E_FAIL;

    }

    CollectPreferences();

    mHttpDisconnected = false;
    mCancelled = false;

    if(!mCharHostName || !mRAUrl)
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Didn't collect proper config information.\n",GetTStamp(tBuff,56));
        error_no = config_error_no;
        goto done;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Init: Past configuration tests, about to attempt operation.\n",GetTStamp(tBuff,56)));  

    mCardContext = CKYCardContext_Create(SCARD_SCOPE_USER);
    if (!mCardContext) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Cannot create card context! \n",GetTStamp(tBuff,56));
        error_no = CARD_CONTEXT_ERROR;
        goto done;
    }
  
    mPDUWriter = new PDUWriterThread(this);
    if (!mPDUWriter) {
        error_no = PDU_WRITER_ERROR;
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Cannot begin CoolKey operation. Cannot  create internal PDU writer thread!\n",GetTStamp(tBuff,56));
        goto done;
    }

    mPDUWriter->Init(); 


    mHttp_handle = httpAllocateClient();

    if(mHttp_handle <= 0)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s Cannot begin CoolKey operation. Can't create internal Http Client!\n",GetTStamp(tBuff,56)));
        error_no = HTTP_CLIENT_ERROR;
        goto done;
    }
  
    connected = ConnectToReader(readerName);
  
    if (!connected) {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s Cannot begin CoolKey operation. Can't connect to card reader!\n",GetTStamp(tBuff,56)));
        error_no = CONN_READER_ERROR;
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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Init: token code: %s\n",GetTStamp(tBuff,56),tokenCode));
        mCharTokenCode = strdup(tokenCode);
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Init: mCharTokenCode %s \n",GetTStamp(tBuff,56),mCharTokenCode));

    mStatusRequest = true;

 done:
  
    if (!connected) {
    
        if (mCardContext) {
            CKYCardContext_Destroy(mCardContext);
            mCardContext = 0;
        }

        NotifyEndResult(this, op, 1, error_no);

        return E_FAIL;
    }
  
    return S_OK;
}

void CoolKeyHandler::CollectPreferences()
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences !\n",GetTStamp(tBuff,56)));

    //Grab the keyID which we will need

    const char *keyID = mKey.mKeyID;

    if(!keyID)
    {
        CoolKeyLogMsg( PR_LOG_ERROR,"%s Collecting CoolKey preferences. Cannot get keyID , cannot proceed. \n",GetTStamp(tBuff,56));

        return;
    }

    int httpMessageTimeout = 30;

    //Quickly grab the configurable http message timeout

    const char *msg_timeout = CoolKeyGetConfig("esc.tps.message.timeout");

    if(msg_timeout)
    {
        httpMessageTimeout = atoi(msg_timeout);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG,("%s CoolKeyHandler::CollectPreferences! Message timeout %d\n",GetTStamp(tBuff,56),httpMessageTimeout));

    }

    mHttpRequestTimeout = httpMessageTimeout;
 
    // Now grab the url for the tps server from config store.

    string tps_operation = "Operation";
  
    string tps_url_for_key =  tps_operation + "-" + keyID;

    const char *tps_url_for_key_str = tps_url_for_key.c_str();

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG,("%s CoolKeyHandler::CollectPreferences! tps_url %s\n",GetTStamp(tBuff,56),tps_url_for_key_str)); 
    const char *tps_url = CoolKeyGetConfig(tps_url_for_key_str);

    if(!tps_url)
    {
        //now try to get the hard coded entry out of the config file.

        tps_url = CoolKeyGetConfig("esc.tps.url");

        if(!tps_url)
        {
            CoolKeyLogMsg( PR_LOG_ERROR, "%s Collecting CoolKey preferences. Cannot find value for the TPS URL. \n",GetTStamp(tBuff,56));

            return;
        }
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences esc.tps.url %s\n",GetTStamp(tBuff,56),tps_url));

    string tps_url_str = tps_url;

    // determine whether or not we are SSL

    string ssl_str =     "https://";
    string non_ssl_str = "http://";

    size_t pos = tps_url_str.find(ssl_str,0);

    mSSL = 0;

    if(pos == 0)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences SSL on for tps url\n",GetTStamp(tBuff,56)));
        pos += ssl_str.length();
        mSSL= 1;
    }
    else
    {
        pos = tps_url_str.find(non_ssl_str,0);
        if(pos == string::npos)
        {
            CoolKeyLogMsg( PR_LOG_ERROR, "%s Collecting CoolKey preferences.  TPS URL has specified an illegal protocol! \n",GetTStamp(tBuff,56)); 
            return;
        }

        pos+= non_ssl_str.length();

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences SSL off for tps url.\n",GetTStamp(tBuff,56)));
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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences NULL tps_url_offset string!.\n",GetTStamp(tBuff,56)));
        return;
    }
    mRAUrl = strdup(tps_url_offset.c_str());

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences  tps_url_offset string! %s.\n",GetTStamp(tBuff,56),tps_url_offset.c_str()));

    host_name_port_str = tps_url_str.substr(pos,end_host_port_count);

    if(!host_name_port_str.length())
    {
        CoolKeyLogMsg(PR_LOG_ERROR, "%s Collecting CoolKey preferences.  Bad hostname and port value!.\n",GetTStamp(tBuff,56));
        return;
     }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences host_name_port %s.\n",GetTStamp(tBuff,56),host_name_port_str.c_str())); 

    string delim = ":";
    string port_num_str = "";

    size_t delimPos = host_name_port_str.find(delim,0);

    if(delimPos == string::npos)
    {
        mPort = 80;
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences no port number assuming 80!.\n",GetTStamp(tBuff,56)));

        mCharHostName = strdup(host_name_port_str.c_str());
    }
    else
    {
        port_num_str = host_name_port_str.substr(delimPos + 1);
        string host_name_str = host_name_port_str.substr(0, delimPos);
        
        if(host_name_str.length())
        {
            mCharHostName = strdup(host_name_str.c_str());
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences mCharHostName %s!.\n",mCharHostName,GetTStamp(tBuff,56)));
        }
    }
   
    if(port_num_str.length())
    {
        mPort = atoi(port_num_str.c_str());

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences port_num_str %s.\n",GetTStamp(tBuff,56),port_num_str.c_str()));
    } 

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CollectPreferences port number %d.\n",GetTStamp(tBuff,56),mPort));
}

HRESULT CoolKeyHandler::CancelAuthParameters()
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CancelAuthParameters. \n",GetTStamp(tBuff,56)));
    if(mDataLock)
        PR_Lock(mDataLock);

    if(mDataCondVar)
    {
          PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CancelAuthParameters. About to notify mDataCondVar. \n",GetTStamp(tBuff,56)));
         PR_NotifyCondVar(mDataCondVar);
    }

    PR_Unlock(mDataLock);

    return S_OK;
}

HRESULT CoolKeyHandler::SetAuthParameter(const char *param_id, const char *value)
{
    char tBuff[56];
    PR_Lock(mDataLock);

    string pId = "";

    if(param_id)
        pId = param_id;

    nsNKeyREQUIRED_PARAMETER *param = mReqParamList.GetById(pId);

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::SetAuthParameter :result of GetById %p",GetTStamp(tBuff,56),param));

    if(param)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::SetAuthParameter found and setting id %s value %s:\n",GetTStamp(tBuff,56),param_id,value));

        string val = "";

        if(value)
            val = value;

        param->setValue(val);

        if(mReqParamList.AreAllParametersSet())
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler :All auth parameters set, notify enrollment",GetTStamp(tBuff,56)));

            PR_NotifyCondVar(mDataCondVar);
        }
    }

    PR_Unlock(mDataLock);
 
    return S_OK;
}

HRESULT CoolKeyHandler::SetScreenName(const char *screenName)
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::SetScreenName:\n",GetTStamp(tBuff,56)));

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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::SetTokenPin:\n",GetTStamp(tBuff,56)));

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

    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::SetPassword:\n",GetTStamp(tBuff,56)));

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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::CloseConnection:\n",GetTStamp(tBuff,56)));
    if(mHttp_handle)
    {
        ::httpCloseConnection(mHttp_handle);
    }

    return S_OK;
}

HRESULT CoolKeyHandler::Enroll(const char *aTokenType)
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Enroll:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    mState = RESET_PIN;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::ResetPIN:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    mState = FORMAT;

    HRESULT res = S_OK;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::Format:\n",GetTStamp(tBuff,56)));  
  
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser\n",GetTStamp(tBuff,56)));

    if(!ui)
    {
        return E_FAIL;
    }

    CoolKeyNotify(&mKey, eCKState_NeedAuth, 0,ui);

    while (1) {
        PR_Lock(mDataLock);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser before PR_WaitCondVar\n",GetTStamp(tBuff,56)));

        PR_WaitCondVar(mDataCondVar, PR_INTERVAL_NO_TIMEOUT);

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser after PR_WaitCondVar\n",GetTStamp(tBuff,56)));
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser got our required auth data,unlocking lock.\n",GetTStamp(tBuff,56)));
            PR_Unlock(mDataLock);
            break;
        }


   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser got notification from user.\n",GetTStamp(tBuff,56)));

   if (! mReqParamList.AreAllParametersSet())
   {
       PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::GetAuthDataFromUser ,not all params set, returing E_FAIL.\n",GetTStamp(tBuff,56)));

       return E_FAIL;

   }

   return S_OK;
}

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
    return HttpDisconnect();
}

HRESULT CoolKeyHandler::HttpDisconnect(int reason)
{
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpDisconnect:\n",GetTStamp(tBuff,56)));
    int present = 0;

    //Assume lost connection
    int error = 28;

    if(reason)
        error = reason;

    present = IsNodeInActiveKeyList(&mKey);

    if(!present)
        return S_OK;
	
    if(mHttpDisconnected )
        return S_OK;

    mHttpDisconnected = true;

  // Clean up the smartcard objects //
    DisconnectFromReader();
  
    if (!mReceivedEndOp && !isCancelled()) {
        CloseConnection();
        NotifyEndResult(this, mState, 1, error /*disconnect */);
    }
    else
        CloseConnection();

    return S_OK;
}

HRESULT CoolKeyHandler::ProcessMessageHttp(eCKMessage *msg)
{
    char tBuff[56];
    HRESULT rv = S_OK;
  
    eCKMessage::sntype type = msg->getMessageType();

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::ProcessMessageHttp: type %d\n",GetTStamp(tBuff,56),(int)type));
    switch(type)
    {
            case eCKMessage::EXTENDED_LOGIN_REQUEST:
               HttpSendAuthResponse(this,(eCKMessage_EXTENDED_LOGIN_REQUEST *) msg);
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
    char tBuff[56];
    int regular_login = 0;


    if(mHttp_handle <= 0 )
    {
        HttpDisconnect();
        RemoveKeyFromActiveKeyList(&mKey);
        return E_FAIL;
    }

    char ascii_port[50];
    char host_port[200];
    char *method = "POST";

    if(mCharScreenName && mCharScreenNamePwd)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpBeginOpRequest Attempting regular login, no extended login capabilities.n",GetTStamp(tBuff,56)));

        regular_login = 1;
    }

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpBeginOpRequest.n",GetTStamp(tBuff,56)));
    sprintf(ascii_port,"%d",mPort);
    sprintf(host_port,"%s:%s",mCharHostName,ascii_port);
		
    eCKMessage_BEGIN_OP begin_op;
	
    begin_op.setOperation(mState);
    char buffer[2048];

    if(!mRAUrl)
    {
        HttpDisconnect();
        RemoveKeyFromActiveKeyList(&mKey);
        return E_FAIL;
    }

    if(mState == ENROLL || mState == FORMAT)
    {
        sprintf(buffer,"tokenType=%s",mCharTokenType);

        string buffer_str = buffer;
        begin_op.AddExtensionValue(buffer_str);
    } 

    string ext_buffer = "";

    char *clientVer = "ESC 1.0.1";
    sprintf(buffer,"clientVersion=%s",clientVer);

    ext_buffer = buffer;
	 
    begin_op.AddExtensionValue(ext_buffer); // The client version string
  
    const char *atr = GetATRForKeyID(&mKey);
    if (!atr )
    {
        HttpDisconnect();
        RemoveKeyFromActiveKeyList(&mKey);
        return E_FAIL;
    }

    sprintf(buffer,"tokenATR=%s",atr);
 
    ext_buffer = buffer;
 
    begin_op.AddExtensionValue(ext_buffer);

    char *status = (char *) ((mStatusRequest) ? "true" : "false");
  
    sprintf(buffer,"statusUpdate=%s",status);

    ext_buffer = buffer;

    begin_op.AddExtensionValue(ext_buffer);

    if(!regular_login)
    { 
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpBeginOpRequest Attempting extended login.n",GetTStamp(tBuff,56)));

        sprintf(buffer,"extendedLoginRequest=%s","true");
        ext_buffer = buffer;
        begin_op.AddExtensionValue(ext_buffer);
    }

    string output = "";

    begin_op.encode(output);
 
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpBeginOpRequest hostport %s, data %s\n",host_port,output.c_str(),GetTStamp(tBuff,56))); 
    int result = httpSendChunked(host_port,
     mRAUrl,method,(char *)output.c_str(),HttpChunkedEntityCB,(void *)this,mHttp_handle,mSSL,mHttpRequestTimeout);

    if(!result)
    {
        HttpDisconnect();
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::ProcessTokenPDU:\n",GetTStamp(tBuff,56)));
    if(!req || !context)
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Processing HTTP message.  Bad input data. \n",GetTStamp(tBuff,56));
        return;
    }

    int size = 4096;
    unsigned char pduData[4096];
    int ERR_CONN_TOKEN=8;

    req->getPduData(pduData,&size);

    if(size == 0)
    {
        CoolKeyLogMsg(PR_LOG_ERROR, "%s Processing HTTP message.  Can't extract PDU data from message! \n",GetTStamp(tBuff,56));
        context->HttpDisconnect();
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
  
    AutoCKYBuffer response;
  
    CKYStatus status = CKYCardConnection_ExchangeAPDU(context->GetCardConnection(),
                                                  requestAPDU, &response);
    if (status != CKYSUCCESS) {
        CoolKeyLogMsg( PR_LOG_ERROR, 
            "%s Processing HTTP message.  Can't write apdu to card! status %d response[0] %x response[1] %x error %d \n"
         ,GetTStamp(tBuff,56)   ,status,CKYBuffer_GetChar(&response,0),CKYBuffer_GetChar(&response,1),
        CKYCardConnection_GetLastError(context->GetCardConnection()));

        context->HttpDisconnect(ERR_CONN_TOKEN);

        return;
    }
  
    eCKMessage_TOKEN_PDU_RESPONSE pdu_response;

    int pduSizeRet = (MESSAGE_u08) CKYBuffer_Size(&response);
    MESSAGE_byte *pduDataRet = (MESSAGE_byte *) CKYBuffer_Data(&response);

    if(pduSizeRet == 0 || !pduDataRet )
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Processing HTTP message. No PDU response from card! \n",GetTStamp(tBuff,56));
        context->HttpDisconnect(ERR_CONN_TOKEN);
        return;
    }

    pdu_response.setPduData(pduDataRet,pduSizeRet);
    string output = "";

    pdu_response.encode(output);
  
    NSS_HTTP_HANDLE handle = context->getHttpHandle();

    if(handle && output.size())
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::sending to RA: %s \n",GetTStamp(tBuff,56),output.c_str()));
        NSS_HTTP_RESULT res =  sendChunkedEntityData(output.size(),(unsigned char *) output.c_str(),handle);

        if(res == 0)
        {
            CoolKeyLogMsg( PR_LOG_ERROR, "%s Processing HTTP message. Write back to TPS failed , disconnecting. \n",GetTStamp(tBuff,56));
            context->HttpDisconnect();
        }
        else
        {
            PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler:ProcessTokenPDU data written to RA .\n",GetTStamp(tBuff,56)));
        }

    }

}
  
HRESULT CoolKeyHandler::HttpProcessStatusUpdate(eCKMessage_STATUS_UPDATE_REQUEST * msg)
{
    char tBuff[56];
    HRESULT rv = S_OK;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpProcessStatusUpdate:  \n",GetTStamp(tBuff,56)));

    if(!msg)
    {
        HttpDisconnect();
        return E_FAIL;
    }

    MESSAGE_u08 current_state = msg->getCurrentState();
    string next_task_name = msg->getNextTaskName();


    CoolKeyNotify(&mKey, eCKState_StatusUpdate, current_state);

    eCKMessage_STATUS_UPDATE_RESPONSE response;

    response.setCurrentState(current_state);

    string output = "";

    response.encode(output);

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpProcessStatusUpdat response encoded \n"));
    int len = output.size();

    NSS_HTTP_HANDLE handle = mHttp_handle;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("CoolKeyHandler::HttpProcessStatusUpdate len %d output %s",len,output.c_str()));

    if(len && handle)
    {
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler:: next task name %s sending to RA: %s \n",GetTStamp(tBuff,56),next_task_name.c_str(),output.c_str()));

        NSS_HTTP_RESULT res =  sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

        if(!res)
            rv = E_FAIL;
    }

    if(rv == E_FAIL)
        HttpDisconnect();

    return rv;
}

HRESULT CoolKeyHandler::HttpSendSecurID(eCKMessage_SECURID_REQUEST *req)
{
    char tBuff[56];
    HRESULT rv = S_OK;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendSecurID:  \n",GetTStamp(tBuff,56)));
    char* pin = NULL;

     if(!req)
     {
         HttpDisconnect();
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
        PR_LOG(coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::sending to RA: %s \n",GetTStamp(tBuff,56),output.c_str()));
        NSS_HTTP_RESULT res= sendChunkedEntityData(len,(unsigned char *) output.c_str(),handle);

        if(!res)
            rv = E_FAIL;
    }

    if(rv == E_FAIL)
        HttpDisconnect();

    return rv;
}

HRESULT CoolKeyHandler::HttpSendUsernameAndPW()
{
    char tBuff[56];
    HRESULT rv = S_OK;

    eCKMessage_LOGIN_RESPONSE response;

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendUsernameAndPW:  \n",GetTStamp(tBuff,56)));

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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::sending to RA: %s \n",GetTStamp(tBuff,56),output.c_str()));

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

    if(rv == E_FAIL)
        HttpDisconnect();

    return rv;
}

HRESULT CoolKeyHandler::HttpSendAuthResponse(CoolKeyHandler *context,eCKMessage_EXTENDED_LOGIN_REQUEST *req)
{
    char tBuff[56];
    HRESULT rv = S_OK;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendAuthResponse: \n",GetTStamp(tBuff,56)));
    if(!req || !context)
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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendAuthResponse:  title %s\n",GetTStamp(tBuff,56),titleStrDec.c_str()));

        {
            buff_final_str = "title=" + titleStrDec + "&&";

            if(descStrDec.size())
            {
                buff_final_str += "description=" + descStrDec + "&&";
            }
            buff_final_str += pListBuffer;
        } 

   }

   PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendAuthResponse:  ui buffer %s\n",GetTStamp(tBuff,56),(char *) buff_final_str.c_str()));

   HRESULT res = GetAuthDataFromUser((const char *)buff_final_str.c_str());
   int ERR_CONN_TOKEN = 8;
   if(res == E_FAIL)
   {
        context->HttpDisconnect(ERR_CONN_TOKEN);
        return E_FAIL;
   }
 
   string output = "";

   response.encode(output);

   int len = output.size();

   mReqParamList.CleanUp();

   NSS_HTTP_HANDLE handle = mHttp_handle;

   if(handle)
   {
       PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::sending to RA: %s \n",GetTStamp(tBuff,56),output.c_str()));

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

  if(rv == E_FAIL)
      HttpDisconnect();

  return rv;
}

HRESULT CoolKeyHandler::HttpSendNewPin(eCKMessage_NEWPIN_REQUEST  *req)
{
    char tBuff[56];
    HRESULT rv = S_OK;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpSendNewPin: \n",GetTStamp(tBuff,56)));

    if(!req)
    {
        HttpDisconnect();
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
        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::sending to RA: %s \n",GetTStamp(tBuff,56),output.c_str()));

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

    if(rv == E_FAIL)
        HttpDisconnect();

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
  
    context->HttpDisconnect();	
    NotifyEndResult(context, operation, result,message);
}

void NotifyEndResult(CoolKeyHandler* context, int operation, int result, int description)
{
    char tBuff[56];
    RefreshInfoFlagsForKeyID(context->GetAutoCoolKey());

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::NotifyEndResult context %p op %d result %d description %d:\n",GetTStamp(tBuff,56),context,operation,result,description)); 

    if(!context)
        return;
 
    switch (operation) {
    case ENROLL:
        if (result == 0) {

            CoolKeyLogMsg(PR_LOG_ALWAYS,"%s Key Enrollment success.\n",GetTStamp(tBuff,56));
            CoolKeyAuthenticate(context->GetAutoCoolKey(), context->GetPIN());
            CoolKeyNotify(context->GetAutoCoolKey(), eCKState_EnrollmentComplete,
                   context->GetScreenName() == NULL ? 1 : 0);
        } else {
            CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Key Enrollment failure. Error: %d.\n",GetTStamp(tBuff,56),description);
	    CoolKeyNotify(context->GetAutoCoolKey(), eCKState_EnrollmentError, description); // XXX: Need INIT_FAILED error code!
        }
        break;
    case RESET_PIN:
        if (result == 0) {
     
            CoolKeyLogMsg(PR_LOG_ALWAYS,"%s Key Reset Password success.\n",GetTStamp(tBuff,56));

            CoolKeyAuthenticate(context->GetAutoCoolKey(), context->GetPIN());
            CoolKeyNotify(context->GetAutoCoolKey(), eCKState_PINResetComplete, 0);
        } else {
            CoolKeyLogMsg(PR_LOG_ALWAYS, "%s Key Reset Password failure. Error: %d.\n",GetTStamp(tBuff,56),description);
            CoolKeyNotify(context->GetAutoCoolKey(), eCKState_PINResetError, description); // XXX: Need PIN_RESET_FAILED error code!
        }
        break;
    case FORMAT:
        if (result == 0) {
            CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Key Format success.\n",GetTStamp(tBuff,56));
            CoolKeyNotify(context->GetAutoCoolKey(), eCKState_FormatComplete, 0);
        } else {
            CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Key Format failure. Error: %d.\n",GetTStamp(tBuff,56),description);
            CoolKeyNotify(context->GetAutoCoolKey(), eCKState_FormatError, description); // XXX: Need FORMAT_FAILED error code!
        }
        break;
        default:
        break;
    }
}

bool CoolKeyHandler::ConnectToReader(const char* readerName)
{
    char tBuff[56];
    bool connected = false;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::ConnectToReader:\n",GetTStamp(tBuff,56))); 
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpChunkedEntityCBImpl: data %s\n",GetTStamp(tBuff,56),(char *) entity_data));
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
            handler->HttpDisconnect();
            return ret;
        }
        else
        {
            handler->HttpDisconnect();
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

        PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::HttpChunkedEntiryCB, message type %d,\n",GetTStamp(tBuff,56),(int)t));

        if(t == eCKMessage::UNKNOWN_MESSAGE)
        {
            handler->HttpDisconnect();
            return ret;
        }
        sn = handler->AllocateMessage(t,entity_data,entity_data_len);
	
        if(!sn)
        {
            handler->HttpDisconnect();
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
        handler->HttpDisconnect();
    }

    return ret;
}

eCKMessage *CoolKeyHandler::AllocateMessage(eCKMessage::sntype type,unsigned char *data, unsigned size)
{
    char tBuff[56];
    unsigned char *lData = data;

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::AllocateMessage %d :\n",GetTStamp(tBuff,56),(int)type));
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

           PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::AllocateMessage,got EXTENDED_LOGIN_REQUEST mReqParamList %p\n",GetTStamp(tBuff,56) ,&mReqParamList));

           sn = new eCKMessage_EXTENDED_LOGIN_REQUEST();

           ((eCKMessage_EXTENDED_LOGIN_REQUEST *)sn)->SetReqParametersList(&mReqParamList);
                    
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

    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CoolKeyHandler::DisconnectFromReader:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetInfoFlags:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo:\n",GetTStamp(tBuff,56)));
    PK11GenericObject *obj = NULL;
    SECItem label, ATR;
    CK_TOKEN_INFO tokenInfo;
    CoolKeyInfo *info = NULL;
    SECStatus status;
    HRESULT hres;
    int atrSize;
    char *atrString;
    SECItem isCOOLKey;

    memset((void *) &tokenInfo,0,sizeof(tokenInfo));
    ATR.data = NULL; // initialize for error processing
    label.data = NULL; // initialize for error processing
    isCOOLKey.data = NULL;


    int isACOOLKey = 0;

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
 // PK11_DestroyGenericObjects(obj);
    if (status != SECSuccess) {
        goto failed;
    }
  // get the CUID/Serial number (we *WILL* continue to need it )
    status = PK11_GetTokenInfo(aSlot,&tokenInfo);
    if (status != SECSuccess) {
        goto failed;
    }

  //get the are we a CoolKey value

    status = PK11_ReadRawAttribute(PK11_TypeGeneric, obj, CKA_MOZILLA_IS_COOL_KEY, &isCOOLKey);

    PK11_DestroyGenericObjects(obj);
    obj = NULL;

    if (status != SECSuccess) {
        goto  failed;
    }

    if(isCOOLKey.len == 1)
    {
         PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: CKA_MOZILLA_IS_COOL_KEY  %d.\n",GetTStamp(tBuff,56),(int) isCOOLKey.data[0]));

         isACOOLKey=(int) isCOOLKey.data[0]; 
    } 

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->flags %u.\n",GetTStamp(tBuff,56),tokenInfo.flags));
  
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->label %s.\n",GetTStamp(tBuff,56),(char *)tokenInfo.label));
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->manufacturerID %s.\n",GetTStamp(tBuff,56),(char *)tokenInfo.manufacturerID));
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->model %s.\n",GetTStamp(tBuff,56),(char *)tokenInfo.model));
    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->serialNumber %s.\n",GetTStamp(tBuff,56),(char *)tokenInfo.serialNumber));

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->firmwareVersion.major %d info->firmwareVersion.minor %d \n",GetTStamp(tBuff,56),(int)tokenInfo.firmwareVersion.major,(int) tokenInfo.firmwareVersion.minor));


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


    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: info->atr %s.\n",GetTStamp(tBuff,56),(char *)atrString));


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

    PR_LOG( coolKeyLogHN, PR_LOG_DEBUG, ("%s CKHGetCoolKeyInfo: tokenInfo.label length %d.\n",GetTStamp(tBuff,56),strlen((char *) tokenInfo.label)));

    // Give the CAC card some sort of unique key ID

    if(strlen(info->mCUID) == 0)
    {
        strncpy(info->mCUID,(char *)tokenInfo.label,35);
        info->mCUID[34] = 0;
        isACOOLKey = 0;
    }

    //Handle the isCOOLKey flag
    if(isACOOLKey) {
        info->mInfoFlags |= COOLKEY_INFO_IS_REALLY_A_COOLKEY_MASK;
    }

    SECITEM_FreeItem(&ATR,PR_FALSE);
    SECITEM_FreeItem(&label,PR_FALSE);
    SECITEM_FreeItem(&isCOOLKey,PR_FALSE);

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
