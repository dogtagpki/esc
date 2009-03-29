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
#define LINE_BUF_SIZE           512

#include "SlotUtils.h"

#include "prthread.h"
#include "pk11func.h"
#include "cky_base.h"
#include "cky_applet.h"

#include "NSSManager.h"
#include "CoolKeyHandler.h"
#include "CoolKeyID.h"

#include <assert.h>
#include <list>
#include <algorithm>
#include <prlog.h>
#include <time.h>
#include "CoolKey.h"
static NSSManager* g_NSSManager = NULL;

static PRLogModuleInfo *coolKeyLog = PR_NewLogModule("coolKeyLib");

static std::list<CoolKeyListener*> g_Listeners;

struct ActiveKeyNode;

HRESULT AddNodeToActiveKeyList(ActiveKeyNode *aNode);
HRESULT ClearActiveKeyList(void);
ActiveKeyNode *GetNodeInActiveKeyList(const CoolKey *aKey);

class CoolKeyLogger {
public:

    CoolKeyLogger(char *logFileName, int maxNumLines);
    ~CoolKeyLogger();

    void LogMsg(int logLevel, const char *fmt, ...);
    void LogMsg(int logLevel,const char *msg, va_list argp);

    void init();

    int IsInitialized() { return initialized; }

private:

    void LockLog();
    void UnlockLog();

    PRLock *logLock;

    int maxLines;

    char *pathName;
    PRFileDesc *fd;

    int initialized;

};

CoolKeyLogger::CoolKeyLogger(char *logFileName, int maxNumLines)
{
    fd = NULL;
    logLock = NULL;

    maxLines = maxNumLines;
    if(logFileName)
        pathName = strdup(logFileName);
    initialized = 0;
}

CoolKeyLogger::~CoolKeyLogger()
{
   char tBuff[56];

   PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s ~CoolKeyLogger:\n",GetTStamp(tBuff,56)));
   LockLog();

   PR_Close(fd);

   fd = NULL;

   UnlockLog(); 

   PR_DestroyLock(logLock);

   logLock = NULL;

   if(pathName)
       free(pathName);

   pathName = NULL;
}

void CoolKeyLogger::LockLog()
{
    PR_Lock(logLock);
}

void CoolKeyLogger::UnlockLog()
{
   PR_Unlock(logLock); 
}

void CoolKeyLogger::init()
{
    char tBuff[56];

    PRFileInfo info;

    if( !pathName)
         return;

    logLock = PR_NewLock();

    PRStatus rv = PR_GetFileInfo(pathName,&info);

    int fileSize = 0;

    if(rv == PR_SUCCESS)
    {
        fileSize = info.size;
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s File info size %d! \n",GetTStamp(tBuff,56),fileSize));
    }
  
    //Assume average line size of about 40
 
    if((fileSize / 40) > maxLines)
    {

       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s Number of lines too big, truncate file %d! \n",GetTStamp(tBuff,56),fileSize / 80));

        fd = PR_Open(pathName, PR_WRONLY |  PR_CREATE_FILE | PR_TRUNCATE, 0600);
    }
    else
    {
        fd = PR_Open(pathName, PR_WRONLY |  PR_CREATE_FILE | PR_APPEND, 0600);
    }

    if(!fd)
        return; 

    initialized = 1;

    return;
}

void CoolKeyLogger::LogMsg(int logLevel, const char *fmt, ...)
{
    va_list ap;
    char line[LINE_BUF_SIZE]; 

    if(!initialized)
        return;

    va_start(ap, fmt);

    int end = PR_vsnprintf(line, sizeof(line)-1, fmt, ap);

    LockLog();

    PR_Write(fd,line,end);

    UnlockLog();

    va_end(ap);
}

void CoolKeyLogger::LogMsg(int logLevel, const char *msg, va_list argp)
{
    char line[LINE_BUF_SIZE];

    if(!initialized)
        return;

    int end = PR_vsnprintf(line, sizeof(line)-1, msg, argp);

    LockLog();

    PR_Write(fd,line,end);

    UnlockLog();
}

static CoolKeyLogger *g_Log = NULL;

COOLKEY_API HRESULT CoolKeyInit(const char *aAppDir)
{
    char tBuff[56];

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyInit: appDir %s \n",GetTStamp(tBuff,56),aAppDir));

    if (g_NSSManager) 
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ( "%s CoolKeyInit:g_NSSManager already exists. \n",GetTStamp(tBuff,56)));
        return E_FAIL;
    }

    InitCoolKeyList();

    g_NSSManager = new NSSManager();
  
    if (!g_NSSManager) 
    {
      PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s CoolKeyInit:Failed to create NSSManager.\n",GetTStamp(tBuff,56)));
      return E_FAIL;
    }
  
    HRESULT rv = g_NSSManager->InitNSS(aAppDir);
    if (rv == E_FAIL)
    {
         PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s Failed to initialize Crypto library! \n",GetTStamp(tBuff,56)));
         return rv;
    }

    return S_OK;
}

COOLKEY_API HRESULT CoolKeyShutdown()
{ 
    char tBuff[56];

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyShutdown:\n",GetTStamp(tBuff,56)));

    DestroyCoolKeyList();
 
    if (g_NSSManager) {
        g_NSSManager->Shutdown();
        delete g_NSSManager;
        g_NSSManager = 0;
    }

    if(g_Log)
        delete g_Log ;    

    return S_OK;
}

static CoolKeyDispatch g_Dispatch = NULL;
static CoolKeyReference g_Reference = NULL;
static CoolKeyRelease g_Release = NULL;
static CoolKeyGetConfigValue g_GetConfigValue = NULL;
static CoolKeySetConfigValue g_SetConfigValue = NULL;

char* CoolKeyVerifyPassword(PK11SlotInfo *,PRBool,void *);

COOLKEY_API HRESULT CoolKeySetCallbacks(CoolKeyDispatch dispatch,
	CoolKeyReference reference, CoolKeyRelease release,
        CoolKeyGetConfigValue getconfigvalue,CoolKeySetConfigValue setconfigvalue)
{
    char tBuff[56];
    g_Dispatch = dispatch;
    g_Reference = reference;
    g_Release = release;
    g_GetConfigValue = getconfigvalue;
    g_SetConfigValue = setconfigvalue;

    char * suppressPINPrompt =(char*) CoolKeyGetConfig("esc.security.url");

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeySetCallbacks: prompt %s \n", GetTStamp(tBuff,56), suppressPINPrompt));

    if(!suppressPINPrompt)
    {
        PK11_SetPasswordFunc( CoolKeyVerifyPassword);
    }
    // Set the verify password callback here, no params needed we know what it is
    return 0;
}

#define REFERENCE_LISTENER(list) \
  if (list) { \
    (*g_Reference)(list); \
  }

#define RELEASE_LISTENER(list) \
  if (list) { \
    (*g_Reference)(list); \
  }

char *CoolKeyVerifyPassword(PK11SlotInfo *slot,PRBool retry,void *arg)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyVerifyPassword: \n",GetTStamp(tBuff,56)));
    return NULL;
}

COOLKEY_API HRESULT CoolKeyRegisterListener(CoolKeyListener* aListener)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyRegisterListener: aListener %p\n",GetTStamp(tBuff,56),aListener));

    if (!aListener)
        return -1;

    REFERENCE_LISTENER(aListener);
    g_Listeners.push_back(aListener);
  
    return 0;
}

COOLKEY_API HRESULT CoolKeyUnregisterListener(CoolKeyListener* aListener)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyUnregisterListener:\n",GetTStamp(tBuff,56)));
    if (!aListener)
      return -1;
  
    std::list<CoolKeyListener*>::iterator it = 
                   find(g_Listeners.begin(), g_Listeners.end(), aListener);
  
    if (it != g_Listeners.end()) {

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
             ("%s CoolKeyUnregisterListener: erasing listener %p \n",GetTStamp(tBuff,56),*it));
        g_Listeners.erase(it);
        RELEASE_LISTENER(aListener);
    }
    return 0;
}

struct BlinkTimerParams
{
    BlinkTimerParams(const CoolKey *aKey) : mKey(*aKey), mSlot(0), mRate(0),
                                         mEnd(0), mThread(0), mActive(false)
    {
    }

    ~BlinkTimerParams()
    {
        mActive = false;

        if (mThread && mThread != PR_GetCurrentThread())
        PR_JoinThread(mThread);
    }

    AutoCoolKey mKey;
    PK11SlotInfo *mSlot;
    unsigned long mRate;
    PRIntervalTime mEnd;
    PRThread *mThread;
    bool mActive;
};

struct ActiveKeyNode
{
    ActiveKeyNode(const CoolKey *aKey) : mKey(*aKey)
    {
    }

    virtual ~ActiveKeyNode()
    {
    }

    virtual HRESULT OnRemoval() = 0;

    AutoCoolKey mKey;
};

struct ActiveKeyHandler : public ActiveKeyNode
{
    ActiveKeyHandler(const CoolKey *aKey, CoolKeyHandler *aHandler)
         : ActiveKeyNode(aKey)
    {
        char tBuff[56];
        PR_LOG( coolKeyLog, PR_LOG_DEBUG,
          ("%s ActiveKeyHandler::ActiveKeyHandler  \n",GetTStamp(tBuff,56)));

        assert(aHandler);
        mHandler = aHandler;
        mHandler->AddRef();
    }

    ~ActiveKeyHandler()
    {
        char tBuff[56];
        if (mHandler)
        {
            PR_LOG( coolKeyLog, PR_LOG_DEBUG,
                ("%s ActiveKeyHandler::~ActiveKeyHandler  \n",GetTStamp(tBuff,56)));

        
            mHandler->Release();
        }
    }

    HRESULT OnRemoval() { return S_OK; }

    CoolKeyHandler *mHandler;
};

struct ActiveBlinker : public ActiveKeyNode
{
    ActiveBlinker(const CoolKey *aKey, BlinkTimerParams *aParams)
        : ActiveKeyNode(aKey)
    {
        assert(aParams);
        mParams = aParams;
    }

    ~ActiveBlinker()
    {
        if (mParams)
            delete mParams;
    }

    HRESULT OnRemoval()
    {
        if (mParams) {
            mParams->mActive = false;

            if (mParams->mThread && mParams->mThread != PR_GetCurrentThread()) {
                PR_JoinThread(mParams->mThread);
                mParams->mThread = 0;
            }

            delete mParams;
            mParams = 0;
       }

       return S_OK;
    }

    BlinkTimerParams *mParams;
};

HRESULT CoolKeyNotify(const CoolKey *aKey, CoolKeyState aKeyState,
                                                int aData,const char *strData)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG,
        ("%s CoolKeyNotify: key %s state %d data %d strData %s",GetTStamp(tBuff,56),
         aKey->mKeyID,aKeyState,aData,strData));
    if(aKeyState == eCKState_KeyRemoved)
    {
        ActiveKeyHandler  *node = (ActiveKeyHandler *) GetNodeInActiveKeyList(aKey);
        if(node)
        {
            if(node->mHandler)
            {
                node->mHandler->CancelAuthParameters();
            }
        }
    }
    std::list<CoolKeyListener*>::iterator it;
    for(it=g_Listeners.begin(); it!=g_Listeners.end(); ++it)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG,
          ("%s CoolKeyNotify: About to notify listener %p",GetTStamp(tBuff,56),*it));
        if (g_Dispatch) {
            (*g_Dispatch)(*it, aKey->mKeyType, aKey->mKeyID,
                 aKeyState, aData, strData);
        }
    }
    return S_OK;
}



static std::list<ActiveKeyNode *> g_ActiveKeyList;

static void PR_CALLBACK BlinkTimer(void *arg)
{
    BlinkTimerParams *params = (BlinkTimerParams*)arg;
  
    while(params->mActive && PR_IntervalNow() < params->mEnd) {
        CKYBuffer ATR;
        CKYBuffer_InitEmpty(&ATR);
        CKYCardConnection *conn = NULL;
        CKYISOStatus apduRC = 0;
        CKYStatus status;
        const char *readerName = NULL;
  
        CKYCardContext *cardCtxt = CKYCardContext_Create(SCARD_SCOPE_USER);
        assert(cardCtxt);
        if (!cardCtxt) {
            goto done;
        }
  
        conn = CKYCardConnection_Create(cardCtxt);
        assert(conn);
        if (!conn) {
            goto done;
        }

        readerName = GetReaderNameForKeyID(&params->mKey);
        assert(readerName);
        if (!readerName) {
            goto done;
        }

        status = CKYCardConnection_Connect(conn, readerName);
        if (status != CKYSUCCESS) {
            goto done;
        }
        unsigned long state;
  
        status = CKYCardConnection_GetStatus(conn, &state, &ATR);
        if (status != CKYSUCCESS) {
            goto done;
        }

        apduRC = 0;
        status = CKYApplet_SelectCardManager(conn, &apduRC);
        if (status != CKYSUCCESS) {
            goto done;
        }
    
        done:

        if (conn) {
             CKYCardConnection_Disconnect(conn);
             CKYCardConnection_Destroy(conn);
        }
        if (cardCtxt) {
            CKYCardContext_Destroy(cardCtxt);
        }

        CKYBuffer_FreeData(&ATR);
     
        PR_Sleep(PR_MillisecondsToInterval(params->mRate));
    }

  PK11_FreeSlot(params->mSlot);    

  // The assumption we're making here is that if we get here
  // and params->mActive is false, the user cancelled the operation.
  // Note that it's up to the cancel code to remove the key from
  // the active list and do the notify.
  //
  // If params->mActive is true, then we completed blinking for
  // the entire interval.

  if (params->mActive) {
    CoolKeyNotify(&params->mKey, eCKState_BlinkComplete, 0);
      RemoveKeyFromActiveKeyList(&params->mKey);
  }
}

HRESULT
CoolKeyBlinkToken(const CoolKey *aKey, unsigned long aRate, unsigned long aDuration)
{

    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyBlinkToken:\n",GetTStamp(tBuff,56)));
    BlinkTimerParams* params = new BlinkTimerParams(aKey);

    if (!params)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
             ("%s CoolKeyBlinkToken: Can't create BlinkTimerParams.\n",GetTStamp(tBuff,56)));
        return E_FAIL;
    }
  
    params->mSlot = GetSlotForKeyID(aKey);

    if (!params->mSlot) {

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
          ("%s CoolKeyBlinkToken:Can't get Slot for key.\n",GetTStamp(tBuff,56)));
        delete params;
        return E_FAIL;
    }
  
    params->mRate = aRate;
    params->mEnd = PR_IntervalNow() 
                   + PR_MillisecondsToInterval(aDuration + /*fudge*/ 200);
    params->mActive = true;
  
  
    ActiveBlinker *node = new ActiveBlinker(aKey, params);

    if (!node) {
        delete params;
        return E_FAIL;
    }

    HRESULT hres = AddNodeToActiveKeyList(node);

    if (hres == E_FAIL) {
        delete params;
        return E_FAIL;
    }

    params->mThread = PR_CreateThread(PR_SYSTEM_THREAD, 
                                    BlinkTimer, 
                                    (void*)params,
                                    PR_PRIORITY_NORMAL, 
                                    PR_GLOBAL_THREAD, 
                                    PR_JOINABLE_THREAD, 
                                    0);

    CoolKeyNotify(aKey, eCKState_BlinkStart, 0);

    return S_OK;
}

HRESULT
AddNodeToActiveKeyList(ActiveKeyNode *aNode)
{
    char tBuff[56];

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s AddNodeToActiveKeyList:\n",GetTStamp(tBuff,56)));  

    g_ActiveKeyList.push_back(aNode);
    return S_OK;
}

HRESULT
RemoveKeyFromActiveKeyList(const CoolKey *aKey)
{
    char tBuff[56]; 
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s RemoveKeyFromActiveKeyList:\n",GetTStamp(tBuff,56)));
    std::list<ActiveKeyNode *>::iterator it;

    for (it = g_ActiveKeyList.begin(); it != g_ActiveKeyList.end(); ++it) {
        if ((*it)->mKey == *aKey) {
            ActiveKeyNode *node = *it;
            g_ActiveKeyList.erase(it);
            node->OnRemoval();
            delete node;
            return S_OK;
        }
    }

  // XXX: Do we want to return E_FAIL instead?

    return S_OK;
}

HRESULT
ClearActiveKeyList()
{
    std::list<ActiveKeyNode *>::iterator it;

    for (it = g_ActiveKeyList.begin(); it != g_ActiveKeyList.end(); ++it) {
        if (*it) {
            ActiveKeyNode *node = *it;
            delete node;
        }
    }

    g_ActiveKeyList.clear();

    return S_OK;
}

HRESULT
IsNodeInActiveKeyList(const CoolKey *aKey)
{
    ActiveKeyNode *test = NULL;

    test = GetNodeInActiveKeyList(aKey);

    if(test)
        return 1;

    return 0;

}

ActiveKeyNode *
GetNodeInActiveKeyList(const CoolKey *aKey)
{
    std::list<ActiveKeyNode *>::iterator it;

    for (it = g_ActiveKeyList.begin(); it != g_ActiveKeyList.end(); ++it) {
        if ((*it)->mKey == *aKey)
            return *it;
    }

    return NULL;
}

HRESULT CoolKeyEnrollToken(const CoolKey *aKey, const char *aTokenType, 
                           const char *aScreenName, const char *aPIN,
                           const char *aScreenNamePWord,const char *aTokenCode)
{

    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
         ("%s CoolKeyEnrollToken: aTokenCode %s\n",GetTStamp(tBuff,56),aTokenCode));
    if (!aKey || !aKey->mKeyID)
        return E_FAIL;
  
    CoolKeyHandler *handler = new CoolKeyHandler();
  
    if (!handler)
        return E_FAIL;
  
    ActiveKeyHandler *node = new ActiveKeyHandler(aKey, handler);

    if (!node) {
        delete handler;
        return E_FAIL;
    }

  // node now has the only reference to the
  // key handler we just created. It will automatically
  // destroy the key handler when it is removed from
  // the active key list.

    HRESULT hres = AddNodeToActiveKeyList(node);

    if (hres == E_FAIL) {
        delete handler;
        return hres;
    }

    hres = handler->Init(aKey, aScreenName, aPIN,aScreenNamePWord,aTokenCode,ENROLL);
  
    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    hres = handler->Enroll(aTokenType);
  
    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    CoolKeyNotify(aKey, eCKState_EnrollmentStart, aScreenName ? 1 : 0);
  
    return S_OK;
}

HRESULT CoolKeyResetTokenPIN(const CoolKey *aKey, const char *aScreenName, const char *aPIN,const char *aScreenNamePwd)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyResetTokenPIN:\n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return E_FAIL;
  
    CoolKeyHandler *handler = new CoolKeyHandler();
  
    if (!handler)
        return E_FAIL;
  
    ActiveKeyHandler *node = new ActiveKeyHandler(aKey, handler);

    if (!node) {
        delete handler;
        return E_FAIL;
    }

  // node now has the only reference to the
  // key handler we just created. It will automatically
  // destroy the key handler when it is removed from
  // the active key list.

    HRESULT hres = AddNodeToActiveKeyList(node);

    if (hres == E_FAIL) {
        delete handler;
        return E_FAIL;
    }
  
    hres = handler->Init(aKey, aScreenName, aPIN,aScreenNamePwd,NULL,RESET_PIN);
  
    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    hres = handler->ResetPIN();

    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    CoolKeyNotify(aKey, eCKState_PINResetStart, aScreenName ? 1 : 0);
  
    return S_OK;
}

HRESULT CoolKeyFormatToken(const CoolKey *aKey, const char *aTokenType, const char *aScreenName, const char *aPIN,const char *aScreenNamePWord,
									 const char *aTokenCode)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyFormatToken:\n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return E_FAIL;
  
    CoolKeyHandler *handler = new CoolKeyHandler();
  
    if (!handler)
        return E_FAIL;
  
    ActiveKeyHandler *node = new ActiveKeyHandler(aKey, handler);

    if (!node) {
        delete handler;
        return E_FAIL;
    }

  // node now has the only reference to the
  // key handler we just created. It will automatically
  // destroy the key handler when it is removed from
  // the active key list.

    HRESULT hres = AddNodeToActiveKeyList(node);

    if (hres == E_FAIL) {
        delete handler;
        return E_FAIL;
    }

    hres = handler->Init(aKey, aScreenName, aPIN,aScreenNamePWord,aTokenCode,FORMAT);

    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    hres = handler->Format(aTokenType);
  
    if (hres == E_FAIL) {
        RemoveKeyFromActiveKeyList(aKey);
        return hres;
    }
  
    CoolKeyNotify(aKey, eCKState_FormatStart, 0);
  
    return S_OK;
}

COOLKEY_API HRESULT CoolKeySetDataValue(const CoolKey *aKey,const char *name, const char *value)
{

    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeySetDataValue: name %s value %s\n",GetTStamp(tBuff,56),name,value));  

    if (!aKey || !aKey->mKeyID)
        return E_FAIL; 

    ActiveKeyHandler *node = (ActiveKeyHandler *) GetNodeInActiveKeyList(aKey);

    if (node) {
       if(node->mHandler)
       {
           node->mHandler->SetAuthParameter(name,value);
       }
    }

    return S_OK;
}

HRESULT CoolKeyCancelTokenOperation(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyCancelTokenOperation:\n",GetTStamp(tBuff,56))); 
    if (!aKey || !aKey->mKeyID)
        return E_FAIL;

    ActiveKeyHandler *node = (ActiveKeyHandler *) GetNodeInActiveKeyList(aKey);

    if (node) {

        if(node->mHandler)
        {
            node->mHandler->setCancelled();
            node->mHandler->CloseConnection();

        }
    RemoveKeyFromActiveKeyList(aKey);
    RefreshInfoFlagsForKeyID(aKey);
    CoolKeyNotify(aKey, eCKState_OperationCancelled, 0);
    
    }

    return S_OK;
} 

bool
CoolKeyHasApplet(const CoolKey  *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyHasApplet:\n",GetTStamp(tBuff,56)));
    bool hasApplet = false;
  
    if (aKey && aKey->mKeyID) {
        CoolKeyInfo *info = GetCoolKeyInfoByKeyID(aKey);
        if (info)
        {
            hasApplet = (HAS_APPLET(info->mInfoFlags) != 0);

            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyHasApplet: hasApplet: %d info flags %x\n",GetTStamp(tBuff,56),hasApplet,info->mInfoFlags));

        }
    }
  
    return hasApplet;
}

bool
CoolKeyIsEnrolled(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyIsEnrolled:\n",GetTStamp(tBuff,56)));
    bool isEnrolled = false;
  
    if (aKey && aKey->mKeyID) {
        CoolKeyInfo *info = GetCoolKeyInfoByKeyID(aKey);
        if (info)
        {
            isEnrolled = (IS_PERSONALIZED(info->mInfoFlags) != 0);

            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyIsEnrolled: enrolled: %d info flags %x\n",GetTStamp(tBuff,56),isEnrolled,info->mInfoFlags));
        }
    }
  
    return isEnrolled;
}

bool
CoolKeyAuthenticate(const CoolKey *aKey, const char *aPIN)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyAuthenticate:\n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return false;
  
    return NSSManager::AuthenticateCoolKey(aKey, aPIN);
}

HRESULT
CoolKeyGenerateRandomData(unsigned char *aBuf, int aBufLen)
{
    if (!aBuf || aBufLen < 1)
         return E_FAIL;
  
    SECStatus status = PK11_GenerateRandom(aBuf, aBufLen);
    return status;
}

HRESULT CoolKeyGetSignatureLength(const CoolKey *aKey, int *aLength)
{
    return NSSManager::GetSignatureLength(aKey, aLength);
}

HRESULT
CoolKeySignData(const CoolKey *aKey, const unsigned char *aData, int aDataLen, unsigned char *aSignedData, int *aSignedDataLen)
{
    if (!aKey || !aKey->mKeyID || !aData || aDataLen < 1 ||
        !aSignedData || !aSignedDataLen)
        return E_FAIL;
  
    return NSSManager::SignDataWithKey(aKey, aData, aDataLen, aSignedData, aSignedDataLen);
}
HRESULT
CoolKeyGetCertNicknames( const CoolKey *aKey , std::vector<std::string> & aNames)
{
    if(!aKey )
        return E_FAIL;

    HRESULT res = NSSManager::GetKeyCertNicknames(aKey,aNames); 

    return res;
}

HRESULT 
CoolKeyGetCertInfo(const CoolKey *aKey, char *aCertNickname, std::string & aCertInfo)
{
    if(!aKey || !aCertNickname)
    {
        return E_FAIL;
    }

    return NSSManager::GetKeyCertInfo(aKey,aCertNickname,aCertInfo);
}

HRESULT
CoolKeyGetPolicy(const CoolKey *aKey, char *aBuf, int aBufLen)
{
    if (!aKey || !aKey->mKeyID || !aBuf || aBufLen < 1)
        return E_FAIL;
  
    return NSSManager::GetKeyPolicy(aKey, aBuf, aBufLen);
}
HRESULT
CoolKeyGetIssuedTo(const CoolKey *aKey, char *aBuf, int aBufLength)
{
    if (!aKey || !aKey->mKeyID || !aBuf || aBufLength < 1)
        return E_FAIL;

    return NSSManager::GetKeyIssuedTo(aKey,aBuf,aBufLength);
}

HRESULT
CoolKeyGetIssuer(const CoolKey *aKey, char *aBuf, int aBufLength)
{
    if (!aKey || !aKey->mKeyID || !aBuf || aBufLength < 1)
        return E_FAIL;

    return NSSManager::GetKeyIssuer(aKey,aBuf,aBufLength);
}


HRESULT CoolKeyGetATR(const CoolKey *aKey, char *aBuf, int aBufLen)
{
    char tBuff[56];
    if (!aKey || !aKey->mKeyID || !aBuf || aBufLen < 1)
         return E_FAIL;
    aBuf[0] = 0;
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyGetATR::\n",GetTStamp(tBuff,56)));

    HRESULT result = S_OK;

    const char *atr = GetATRForKeyID(aKey);

    if(!atr)
        return result;

    if((int) strlen(atr) < aBufLen)
    {
        sprintf(aBuf,"%s",(char *) atr);
    }

    return result;
}

HRESULT CoolKeyGetIssuerInfo(const CoolKey *aKey, char *aBuf, int aBufLen)
{
    char tBuff[56];
 
    if (!aKey || !aKey->mKeyID || !aBuf || aBufLen < 1)
         return E_FAIL;

     aBuf[0] = 0;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyGetIssuerInfo::\n",GetTStamp(tBuff,56)));

    CKYBuffer ISSUER_INFO;
    CKYBuffer_InitEmpty(&ISSUER_INFO);
    CKYCardConnection *conn = NULL;
    CKYISOStatus apduRC = 0;
    CKYStatus status;
    const char *readerName = NULL;
    const CKYByte *infoData = NULL;
    CKYSize infoSize = 0;

    HRESULT result = S_OK;

    CKYCardContext *cardCtxt = CKYCardContext_Create(SCARD_SCOPE_USER);

    assert(cardCtxt);
    if (!cardCtxt) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info. Can't create Card Context !.\n",GetTStamp(tBuff,56));
        result = E_FAIL;
        goto done;
    }

    conn = CKYCardConnection_Create(cardCtxt);
    assert(conn);
    if (!conn) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info.  Can't create Card Connection!\n",GetTStamp(tBuff,56));
        result = E_FAIL;
        goto done;
    }

    readerName = GetReaderNameForKeyID(aKey);
    assert(readerName);
    if (!readerName) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info.  Can't get reader name!\n",GetTStamp(tBuff,56));
        result = E_FAIL;
        goto done;
    }

    status = CKYCardConnection_Connect(conn, readerName);
    if (status != CKYSUCCESS) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info. Can't connect to Card!\n",GetTStamp(tBuff,56));

        result = E_FAIL;
        goto done;
    }

#ifndef DARWIN
CKYCardConnection_BeginTransaction(conn);
#endif
    apduRC = 0;
    status = CKYApplet_SelectCoolKeyManager(conn, &apduRC);
    if (status != CKYSUCCESS) {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info.  Can't select CoolKey manager!\n",GetTStamp(tBuff,56));
        goto done;
    }

    status = CKYApplet_GetIssuerInfo(conn, &ISSUER_INFO,
                        &apduRC);
    if(status != CKYSUCCESS)
    {
        CoolKeyLogMsg( PR_LOG_ERROR, "%s Attempting to get key issuer info.  Error actually getting IssuerInfo!\n",GetTStamp(tBuff,56));
        result = E_FAIL;
        goto done;
    }

    infoSize =  CKYBuffer_Size(&ISSUER_INFO);

    if(infoSize == 0)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyGetIssuerInfo:: IssuerInfo buffer size is zero!\n",GetTStamp(tBuff,56)));
        result = E_FAIL;
        goto done;
    }

    if(infoSize >= (CKYSize ) aBufLen)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyGetIssuerInfo:: Insufficient space to put Issuer Info!\n",GetTStamp(tBuff,56)));

        result = E_FAIL;
        goto done;
    }

    infoData = CKYBuffer_Data(&ISSUER_INFO);

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyGetIssuerInfo:: IssuerInfo actual data %s!\n",GetTStamp(tBuff,56),(char *) infoData));
    if(infoData)
    {
        strcpy((char *) aBuf, (char *) infoData);
    }

    done:

    if (conn) {
#ifndef DARWIN
        CKYCardConnection_EndTransaction(conn);
#endif
        CKYCardConnection_Disconnect(conn);
        CKYCardConnection_Destroy(conn);
    }
    if (cardCtxt) {
        CKYCardContext_Destroy(cardCtxt);
    }

    CKYBuffer_FreeData(&ISSUER_INFO);

    return result;
}

bool    CoolKeyIsReallyCoolKey(const CoolKey *aKey)
{
    bool res = false;

    if(!aKey)
        return res;

    CoolKeyInfo *info =
        GetCoolKeyInfoByKeyID(aKey);

    if(!info)
        return res;

    if( IS_REALLY_A_COOLKEY(info->mInfoFlags))
        res = true;

    return res;
}

int CoolKeyGetAppletVer(const CoolKey *aKey, const bool isMajor)
{
    int result = -1;
    if(!aKey)
        return result;

    CoolKeyInfo *info =
        GetCoolKeyInfoByKeyID(aKey);

    if(!info)
        return result;

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);

    if(!slot)
        return result;

    CK_TOKEN_INFO tokenInfo;
    PK11_GetTokenInfo(slot, &tokenInfo);

    if(isMajor)
        result = (int) tokenInfo.firmwareVersion.major;
    else
        result = (int) tokenInfo.firmwareVersion.minor;

    return result;
}

bool
CoolKeyHasReader(const CoolKey *aKey)
{
    bool res = false;

    if(!aKey)
         return res;

    const char *readerName =  GetReaderNameForKeyID(aKey);

    if(readerName)
         res = true;

    return res;
}


bool
CoolKeyRequiresAuthentication(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyRequiresAuthentication:\n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return false;
    return NSSManager::RequiresAuthentication(aKey);
}

bool
CoolKeyIsAuthenticated(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyIsAuthenticated:\n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return false;
  
    return NSSManager::IsAuthenticated(aKey);
}

#define NIBBLE_TO_HEX(nibble, caps) ((nibble) < 10) ? ((nibble) + '0') : ((nibble) - 10 + ((caps) ? 'A' : 'a'));

HRESULT
CoolKeyBinToHex(const unsigned char *aInput,
               unsigned long aInputLength,
               unsigned char *aOutput,
               unsigned long aOutputLength,
               bool aCaps)
{
  // Check if there's enough room to accomodate our output.
  
    if (aOutputLength < ((aInputLength*2) + 1))
        return E_FAIL;
  
    unsigned long outIdx = 0;
    unsigned long i;
  
    for (i = 0; i < aInputLength; i++) {
        unsigned char hbits = aInput[i] >> 4;
        unsigned char lbits = aInput[i] & 0x0F;
    
        aOutput[outIdx++] = NIBBLE_TO_HEX(hbits, aCaps);
        aOutput[outIdx++] = NIBBLE_TO_HEX(lbits, aCaps);
    }
  
    aOutput[outIdx] = '\0';
  
    return S_OK;
}

const char *
CoolKeyGetTokenName(const CoolKey *aKey)
{
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
    const char *tokenName = PK11_GetTokenName(slot);
    PK11_FreeSlot(slot);  // depending 
    return tokenName;
}

const char *
CoolKeyGetKeyID(const char *tokenName, int *aKeyType)
{
    CoolKeyInfo *keyInfo = GetCoolKeyInfoByTokenName(tokenName);
    const char *aCUID = keyInfo->mCUID;

    *aKeyType = eCKType_CoolKey;
    return aCUID;
}

const char *CoolKeyGetConfig(const char *aValue)
{
    if(!g_GetConfigValue || ! aValue)
    {
        return NULL;
    }

    const char *res = (*g_GetConfigValue)(aValue);

    return res;
}

HRESULT     CoolKeySetConfig(const char *aName,const char *aValue)
{
    if( !aName || !aValue)
    {
        return E_FAIL;

    }

   HRESULT res = (*g_SetConfigValue)(aName,aValue);

   return res;
}

HRESULT CoolKeyInitializeLog(char *logFileName, int maxNumLines)
{
   if(g_Log)
       return S_OK;
   
   g_Log = new  CoolKeyLogger(logFileName,maxNumLines);

   if(g_Log)
       g_Log->init();
   else
       return E_FAIL;
       
   if(g_Log->IsInitialized())
   {
      CoolKeyLogNSSStatus();
      return S_OK;
   }
   else
      return E_FAIL;
}

HRESULT CoolKeyLogMsg(int logLevel, const char *fmt, ...)
{

    if(!g_Log)
        return S_OK;

    va_list ap;


    va_start(ap, fmt);

    g_Log->LogMsg(logLevel,fmt,ap);

    va_end(ap);

    return S_OK;
}

COOLKEY_API HRESULT CoolKeyLogNSSStatus()
{

    char tBuff[56];
    if (g_NSSManager)
    {
        unsigned int error = g_NSSManager->GetLastInitError();

        if(error == NSS_NO_ERROR)
        {
            CoolKeyLogMsg( PR_LOG_ALWAYS, "%s NSS system intialized successfully!\n",GetTStamp(tBuff,56));
            return S_OK;
        }

        if(error == NSS_ERROR_LOAD_COOLKEY)
        {
           CoolKeyLogMsg( PR_LOG_ERROR, "%s Failed to load CoolKey module! Keys will not be recognized!\n",GetTStamp(tBuff,56));
        }
 
        if(error == NSS_ERROR_SMART_CARD_THREAD) 
        {
            CoolKeyLogMsg( PR_LOG_ERROR, "%s Problem initializing the Smart Card thread! Keys will not be recognized!\n",GetTStamp(tBuff,56));
        }   
    }

    return S_OK;

}

//Utility function to get Time Stamp
char *GetTStamp(char *aTime,int aSize)
{
    if(!aTime)
        return NULL;
    int maxSize = 55;
    if(aSize < maxSize)
        return NULL;

    char *tFormat = "[%c]";
    time_t tm = time(NULL);
    struct tm *ptr = localtime(&tm);
    strftime(aTime ,maxSize ,tFormat,ptr);
    return aTime;
}

