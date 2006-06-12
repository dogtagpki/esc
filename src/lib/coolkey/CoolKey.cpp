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

//#ifdef WIN32
//#include "windows.h"
//#include "CoolKeyCSP.h"
//#define ENABLE_CSP
//#endif

#include "CoolKey.h"
#include "CoolKeyPref.h"
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

static NSSManager* g_NSSManager = NULL;

static PRLogModuleInfo *coolKeyLog = PR_NewLogModule("netkey");

static std::list<CoolKeyListener*> g_Listeners;

struct ActiveKeyNode;

HRESULT AddNodeToActiveKeyList(ActiveKeyNode *aNode);
//extern HRESULT RemoveKeyFromActiveKeyList(const CoolKey *aKey);
HRESULT ClearActiveKeyList(void);
ActiveKeyNode *GetNodeInActiveKeyList(const CoolKey *aKey);



COOLKEY_API HRESULT CoolKeyInit(const char *aAppDir)
{

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyInit:\n appDir %s",aAppDir));
  if (g_NSSManager) 
  {
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyInit:g_NSSManager already exists. \n"));
    return E_FAIL;
  }

  InitCoolKeyList();

  g_NSSManager = new NSSManager();
  
  if (!g_NSSManager) 
  {
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyInit:Failed to create NSSManager.\n"));
    return E_FAIL;
  }
  
  HRESULT rv = g_NSSManager->InitNSS(aAppDir);
  if (rv == E_FAIL)
  {
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyInit:Failed to Init NSSManager. \n"));
       return rv;
  }


  return S_OK;
}

COOLKEY_API HRESULT CoolKeyShutdown()
{ 
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyShutdown:\n"));

//  ShutdownUIThreadProxyService();

  std::list<CoolKeyListener*>::iterator it;
  for (it=g_Listeners.begin(); it!=g_Listeners.end(); ++it)
  {
      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyShutdown: listener still in list %p\n",(*it)));
  }
 
  DestroyCoolKeyList();
 
  if (g_NSSManager) {
    g_NSSManager->Shutdown();
    delete g_NSSManager;
    g_NSSManager = 0;
  }

  //DestroyCoolKeyList();

  CoolKeyPrefShutdown();

  return S_OK;
}

static CoolKeyDispatch g_Dispatch = NULL;
static CoolKeyReference g_Reference = NULL;
static CoolKeyRelease g_Release = NULL;
static CoolKeyGetConfigValue g_GetConfigValue = NULL;
static CoolKeySetConfigValue g_SetConfigValue = NULL;

COOLKEY_API HRESULT CoolKeySetCallbacks(CoolKeyDispatch dispatch,
	CoolKeyReference reference, CoolKeyRelease release,
        CoolKeyGetConfigValue getconfigvalue,CoolKeySetConfigValue setconfigvalue)
{
   g_Dispatch = dispatch;
   g_Reference = reference;
   g_Release = release;
   g_GetConfigValue = getconfigvalue;
   g_SetConfigValue = setconfigvalue;
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


COOLKEY_API HRESULT CoolKeyRegisterListener(CoolKeyListener* aListener)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyRegisterListener: aListener %p\n",aListener));


  if (!aListener)
    return -1;

  REFERENCE_LISTENER(aListener);
  g_Listeners.push_back(aListener);
  
  return 0;
}

COOLKEY_API HRESULT CoolKeyUnregisterListener(CoolKeyListener* aListener)
{

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyUnregisterListener:\n"));
  if (!aListener)
    return -1;
  
  std::list<CoolKeyListener*>::iterator it = 
                   find(g_Listeners.begin(), g_Listeners.end(), aListener);
  
  if (it != g_Listeners.end()) {

      PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
             ("CoolKeyUnregisterListener: erasing listener %p \n",*it));
      //CoolKeyListener *listener = *it;
      g_Listeners.erase(it);
      RELEASE_LISTENER(aListener);
  }
  return 0;
}


HRESULT CoolKeyNotify(const CoolKey *aKey, CoolKeyState aKeyState,
                                                int aData,const char *strData)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
        ("CoolKeyNotify: key %s state %d data %d strData %s",
	 aKey->mKeyID,aKeyState,aData,strData));

  std::list<CoolKeyListener*>::iterator it;
  for(it=g_Listeners.begin(); it!=g_Listeners.end(); ++it)
  {
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
          ("CoolKeyNotify: About to notify listener %p",*it));

    if (g_Dispatch) {
    	(*g_Dispatch)(*it, aKey->mKeyType, aKey->mKeyID,
		 aKeyState, aData, strData);
    }
  }

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
        ("CoolKeyNotify: leaving: key %s state %d data %d",
        aKey->mKeyID, aKeyState, aData));
  return S_OK;
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
    PR_LOG( coolKeyLog, PR_LOG_DEBUG,
          ("ActiveKeyHandler::ActiveKeyHandler  \n"));

    assert(aHandler);
    mHandler = aHandler;
    mHandler->AddRef();
  }

  ~ActiveKeyHandler()
  {
    if (mHandler)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG,
          ("ActiveKeyHandler::~ActiveKeyHandler  \n"));

        
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

//  while(params->mActive && PR_IntervalNow() < params->mEnd) {
    /* call some function that requires communication with the card */
  //  CK_TOKEN_INFO dummy;
    //PK11_GetTokenInfo(params->mSlot, &dummy);

/*	CoolKeyInfo *	
			CKHGetCoolKeyInfo("dummy"); */
//	tokenName = PK11_GetTokenName(params->mSlot);
    
  //  PR_Sleep(PR_MillisecondsToInterval(params->mRate));
 // }
  
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
//    ProxyEventToUIThread(new RemoveKeyFromActiveKeyListEvent(&params->mKey));
      RemoveKeyFromActiveKeyList(&params->mKey);
  }
}

HRESULT
CoolKeyBlinkToken(const CoolKey *aKey, unsigned long aRate, unsigned long aDuration)
{

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyBlinkToken:\n"));
  BlinkTimerParams* params = new BlinkTimerParams(aKey);

  if (!params)
  {
      PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
             ("CoolKeyBlinkToken: Can't create BlinkTimerParams.\n"));
    return E_FAIL;
  }
  
  params->mSlot = GetSlotForKeyID(aKey);

  if (!params->mSlot) {

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
          ("CoolKeyBlinkToken:Can't get Slot for key.\n"));
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

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("AddNodeToActiveKeyList:\n"));  

  g_ActiveKeyList.push_back(aNode);
  return S_OK;
}

HRESULT
RemoveKeyFromActiveKeyList(const CoolKey *aKey)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("RemoveKeyFromActiveKeyList:\n"));
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

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, 
         ("CoolKeyEnrollToken: aTokenCode %s\n",aTokenCode));
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
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyResetTokenPIN:\n"));
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
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyFormatToken:\n"));
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

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeySetDataValue: name %s value %s\n",name,value));  

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

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyCancelTokenOperation:\n")); 
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

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyHasApplet:\n"));
  bool hasApplet = false;
  
  if (aKey && aKey->mKeyID) {
    CoolKeyInfo *info = GetCoolKeyInfoByKeyID(aKey);
    if (info)
    {
      hasApplet = (HAS_APPLET(info->mInfoFlags) != 0);

      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyHasApplet: hasApplet: %d info flags %x\n",hasApplet,info->mInfoFlags));

    }
  }
  
  return hasApplet;
}

bool
CoolKeyIsEnrolled(const CoolKey *aKey)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyIsEnrolled:\n"));
  bool isEnrolled = false;
  
  if (aKey && aKey->mKeyID) {
    CoolKeyInfo *info = GetCoolKeyInfoByKeyID(aKey);
    if (info)
    {
      isEnrolled = (IS_PERSONALIZED(info->mInfoFlags) != 0);

      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyIsEnrolled: enrolled: %d info flags %x\n",isEnrolled,info->mInfoFlags));
    }
  }
  
  return isEnrolled;
}

bool
CoolKeyAuthenticate(const CoolKey *aKey, const char *aPIN)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyAuthenticate:\n"));
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

    return S_OK; // NSSManager::GetKeyCertInfo(aKey,aCertNickname,aCertInfo);

}

HRESULT
CoolKeyGetPolicy(const CoolKey *aKey, char *aBuf, int aBufLen)
{
  if (!aKey || !aKey->mKeyID || !aBuf || aBufLen < 1)
    return E_FAIL;
  
  return NSSManager::GetKeyPolicy(aKey, aBuf, aBufLen);
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
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyRequiresAuthentication:\n"));
  if (!aKey || !aKey->mKeyID)
    return false;
  
  return NSSManager::RequiresAuthentication(aKey);
}

bool
CoolKeyIsAuthenticated(const CoolKey *aKey)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyIsAuthenticated:\n"));
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
