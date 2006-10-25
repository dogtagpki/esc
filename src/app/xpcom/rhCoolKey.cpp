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

#include <stdio.h>
#include "rhCoolKey.h"
#include "CoolKey.h"
#include "nsMemory.h"

#include "nsXPCOM.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#ifdef XP_MACOSX
#include "nsServiceManagerUtils.h"
#else
#include "nsServiceManagerUtils.h"
#endif

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsCOMPtr.h"
#include "nsIProxyObjectManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsXPCOMGlue.h"
#include "prlink.h"
#include "nscore.h"
#include "content/nsCopySupport.h"
#include <vector>
#include <string>


#define STRINGIFY(x) #x
#define GETSTRING(x) STRINGIFY(x)

#ifndef ESC_VERSION
#define ESC_VERSION 1.0.0-0
#endif

#include <prlog.h>
#define COOL_MAX_PATH 1024
#define MAX_STR_LEN COOL_MAX_PATH

#ifdef XP_MACOSX
#define  XPCOM_LIB_NAME  "libxpcom.dylib"
#else
#ifdef XP_WIN32
#define  XPCOM_LIB_NAME 0 
#else
#define  XPCOM_LIB_NAME  "libxpcom.so"
#endif
#endif

#define PSM_COMPONENT_CONTRACTID "@mozilla.org/psm;1"

static const nsIID kIModuleIID = NS_IMODULE_IID;
static const nsIID kIFactoryIID = NS_IFACTORY_IID;
static const nsIID kISupportsIID = NS_ISUPPORTS_IID;
static const nsIID kIComponentRegistrarIID = NS_ICOMPONENTREGISTRAR_IID;

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);


static const nsCID kCoolKeyCID = COOLKEY_CID; 
static rhCoolKey *coolKey_instance = NULL;

// Util funcs , later move to another file

std::list<CoolKeyNode*>rhCoolKey::gASCAvailableKeys;

std::list< nsCOMPtr <rhIKeyNotify>  > rhCoolKey::gNotifyListeners;


PRBool rhCoolKey::gAutoEnrollBlankTokens = PR_FALSE; 

static PRLogModuleInfo *coolKeyLog = PR_NewLogModule("coolKey");


rhCoolKey *single = NULL;

class CoolKeyShutdownObserver : public nsIObserver
 {
 public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSIOBSERVER

 ~CoolKeyShutdownObserver(); 
 };

 NS_IMPL_ISUPPORTS1(CoolKeyShutdownObserver, nsIObserver)

 CoolKeyShutdownObserver::~CoolKeyShutdownObserver()
 {
     PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyShutdownObserver::~CoolKeyShutdownObserver \n"));

  
 }
 
 NS_IMETHODIMP
 CoolKeyShutdownObserver::Observe(nsISupports *aSubject,
                                 const char *aTopic,
                                 const PRUnichar *someData)
 {
   if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
   {

         PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("CoolKeyShutdownObserver::Observe shutting down"));

         if(single)
         {
             single->ShutDownInstance();

         }


   }
   return NS_OK;
 }





unsigned int
ASCCalcBase64DecodedLength(const char *aBase64Str)
{
  // The Base64 string might contain whitespace
  // formatting, so we need to scan the string and
  // count the non whitespace characters.

  unsigned int numValidChars = 0;
  unsigned int numEqualSigns = 0;

  const char *c = aBase64Str;

  while (c && *c) {
    if (!isspace(*c)) {
      if (*c == '=')
        numEqualSigns++;
      numValidChars++;
    }
    c++;
  }

  // The number of Base64 characters we have should
  // always be a divisible by 4, but we also need to
  // subtract off the '=' padding characters from our
  // final calculation.

  return ((numValidChars / 4) * 3) - numEqualSigns;
}

unsigned int
ASCCalcBase64EncodedLength(unsigned int aDataLength)
{
  // The Base64 data we generate will always be a
  // multiple of 4 in length since it includes no
  // whitespace!

  return ((aDataLength + 2) / 3) * 4;
}

rhCoolKey::rhCoolKey()
:mJsNotify(nsnull),mProxy(nsnull)
{
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::rhCoolKey: %p \n",this));

       if(!single)
       {
           single = this;

       }
       else
       {
           return;
       }

       #ifdef XP_WIN32 
           mCSPListener = nsnull;
       #endif

       PRBool res = InitInstance();

       if(res == PR_FALSE)
       {
            PR_LOG( coolKeyLog, PR_LOG_ERROR, ("ESC InitInstance failed,exiting. CoolKey instance %p\n",coolKey_instance));

            exit(1);
       }

  /* member initializers and constructor code */ 
}

rhCoolKey::~rhCoolKey()
{
   /* destructor code */

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::~rhCoolKey: %p \n",this));

}

void rhCoolKey::ShutDownInstance()
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ShutDownInstance. %p \n",this));    

    if (mProxy)
    {
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ShutDownInstance: About to dereference Proxy Object. Proxy %p \n",mProxy));

       CoolKeyUnregisterListener(mProxy);

       NS_RELEASE(mProxy);

       mProxy = nsnull;
    }

    #ifdef XP_WIN32

    if(mCSPListener)  {
        RemoveNotifyKeyListener(mCSPListener);
        mCSPListener = nsnull;
    }
    #endif
    ClearNotifyKeyList();

    CoolKeyShutdown();
}

HRESULT rhCoolKey::Dispatch( rhICoolKey *listener,
    unsigned long keyType, const char *keyID, unsigned long keyState,
    unsigned long data, const char *strData)
{
    return listener->RhNotifyKeyStateChange(keyType,keyID, keyState, 
			data, strData);
}

HRESULT rhCoolKey::Reference( rhICoolKey *listener )
{
    return S_OK;
}

HRESULT rhCoolKey::Release( rhICoolKey *listener )
{
    return S_OK;
}

HRESULT rhCoolKey::doSetCoolKeyConfigValue(const char *aName, const char *aValue) 
{

    if(!aName || !aValue)
    {
        return E_FAIL;
    }


    nsCOMPtr<nsIPrefService> pref;
    pref = do_GetService("@mozilla.org/preferences-service;1");

    if(!pref)
    {
        return E_FAIL;
    }


    nsCOMPtr<nsIPrefBranch> pBranch;

    pref->GetBranch(nsnull,getter_AddRefs(pBranch));

    if(pBranch)
    {
      pBranch->SetCharPref(aName, aValue);
      pref->SavePrefFile(nsnull);
    }

    return S_OK;

}          



const char *rhCoolKey::doGetCoolKeyConfigValue(const char *aName )
{

    if(!aName)
    {
        return NULL;
    }


    nsCOMPtr<nsIPrefBranch> pref;
    char * prefValue = NULL;;
    pref = do_GetService("@mozilla.org/preferences-service;1");

    if(!pref)
    {
        return NULL;
    }


    pref->GetCharPref(aName, &prefValue);


    return (const char *) prefValue;


}


PRBool rhCoolKey::InitInstance()
{
    PRBool ret = PR_TRUE;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::InitInstance %p \n",this));

    char *path = (char *) GRE_GetXPCOMPath();

     PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GREPath %s \n",path));

    char xpcom_path[512];



    char *lib_name = XPCOM_LIB_NAME ;

#ifdef XP_WIN32
   
    sprintf(xpcom_path,"%s",path); 
#else
    sprintf(xpcom_path,"%s/%s",path,lib_name);
#endif
  
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::xpcom_path %s \n",xpcom_path)); 

    XPCOMGlueStartup(xpcom_path);

    nssComponent
    = do_GetService(PSM_COMPONENT_CONTRACTID); 



    CoolKeySetCallbacks(Dispatch,Reference, Release,doGetCoolKeyConfigValue ,doSetCoolKeyConfigValue);

    mProxy = CreateProxyObject();

    if(mProxy)
    {
        CoolKeyRegisterListener(mProxy);

    }
    else
    {
        PR_LOG( coolKeyLog, PR_LOG_ERROR, ("Can't create Proxy Object for ESC. \n"));
    }


#ifdef XP_WIN32

     const char *doCapi = NULL;

     doCapi = doGetCoolKeyConfigValue("esc.windows.do.capi");

     if(!mCSPListener && doCapi && !strcmpi(doCapi,"yes"))
     {
         mCSPListener = new CoolKeyCSPKeyListener();

         if(mCSPListener)
         {
             AddNotifyKeyListener(mCSPListener);

         }

     }

#endif


  // Now setup CoolKey.

  // Get the application directory so CoolKey knows where to find
  // the files it needs. Note that according to MS documentation
  // TCHAR could be 2 bytes on some systems.
  //
  // XXX: We'll also need to add code to figure out where to
  //      place NSS databases before we ship!

  char appDir[COOL_MAX_PATH + 1];

  strcpy(appDir, "./");

  CoolKeyInit(NULL);

  // Add our shutdown observer.
   nsCOMPtr<nsIObserverService> observerService =
     do_GetService("@mozilla.org/observer-service;1");
 
   if (observerService) {
     CoolKeyShutdownObserver* observer =
       new CoolKeyShutdownObserver();
 
     if (!observer) {

          return PR_FALSE;
    
     }
 
     observerService->AddObserver(observer, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
   } else {
     PR_LOG(coolKeyLog,PR_LOG_ERROR,("Could not get an observer service.  We will leak on shutdown."));
   }

  return ret;

}

rhICoolKey* rhCoolKey::CreateProxyObject()
{

    rhICoolKey *proxyObject = NULL;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::CreateProxyObject: \n"));

    nsCOMPtr<nsIProxyObjectManager> manager =
            do_GetService(NS_XPCOMPROXY_CONTRACTID);

    PR_ASSERT(manager);


    manager->GetProxyForObject(NS_UI_THREAD_EVENTQ, NS_GET_IID(rhICoolKey), this, PROXY_SYNC | PROXY_ALWAYS, (void**)&proxyObject);

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::CreateProxyObject: original: %p proxy %p  \n",this,proxyObject));

    return proxyObject;
   
}

CoolKeyNode* rhCoolKey::GetCoolKeyInfo(unsigned long aKeyType, const char * aKeyID)
{

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyInfo: gASCAvailableKeys %p looking for key %s type %d \n",&gASCAvailableKeys,aKeyID,aKeyType));

  std::list<CoolKeyNode*>::const_iterator it;
  for(it=gASCAvailableKeys.begin(); it!=gASCAvailableKeys.end(); ++it) {

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyInfo: current key %s type %d, looking for key %s type %d \n",(*it)->mKeyID.get(),(*it)->mKeyType,aKeyID,aKeyType));

    if ((*it)->mKeyType == aKeyType && !strcmp((*it)->mKeyID.get(), aKeyID))
      return *it;
  }

  return 0;
}

// Internal methods

PRBool rhCoolKey::ASCCoolKeyIsAvailable(unsigned long aKeyType, char * aKeyID)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ASCCoolKeyIsAvailable type %d id %s \n",aKeyType,aKeyID));
    return GetCoolKeyInfo(aKeyType, aKeyID) ? PR_TRUE : PR_FALSE;
}


HRESULT  rhCoolKey::ASCGetAvailableCoolKeyAt(unsigned long aIndex,
                                           unsigned long *aKeyType,
                                           nsEmbedCString *aKeyID)
{
   
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ASCGetAvailableCoolKeyAt: index %d type %d id %s \n",aIndex,aKeyType,aKeyID)); 
    if (!aKeyType || !aKeyID)
        return E_FAIL;

    *aKeyType = 0;
    *aKeyID = "";

    if (!gASCAvailableKeys.empty() && 
		aIndex < (unsigned long) ASCGetNumAvailableCoolKeys()) {
        std::list<CoolKeyNode*>::const_iterator it;
        for(it=gASCAvailableKeys.begin(); it!=gASCAvailableKeys.end(); ++it) {
          if (aIndex-- == 0) {
              *aKeyType = (*it)->mKeyType;
              *aKeyID = (*it)->mKeyID;
               return S_OK;
          }
        }
    }

  return E_FAIL;

}

int   rhCoolKey::ASCGetNumAvailableCoolKeys(void)
{

    int size = (int) gASCAvailableKeys.size();
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ASCGetNumAvailableCoolKeys %d \n",size));
    return size;

}

int rhCoolKey::GetNotifyKeyListenerListSize()
{
    return gNotifyListeners.size();

}

rhIKeyNotify* rhCoolKey::GetNotifyKeyListener(rhIKeyNotify *listener){

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetNotifyKeyListener: %p size %d \n",listener,gNotifyListeners.size() ));

  std::list<nsCOMPtr<rhIKeyNotify> >::const_iterator it;
  for(it=gNotifyListeners.begin(); it!=gNotifyListeners.end(); ++it) {

      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetNotifyKeyListener:  cur %p looking for %p \n",(*it).get(),listener));

      if((*it) == listener)
      {
          PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetNotifyKeyListener:   looking for %p returning %p \n",listener,(*it).get()));
          return (*it);
      }
  }

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetNotifyKeyListener:  looking for %p returning NULL. \n",listener));

  return nsnull;
}


void rhCoolKey::AddNotifyKeyListener(rhIKeyNotify *listener)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::AddNotifyKeyListener: %p \n",listener));

    if(GetNotifyKeyListener(listener ))
    {

         PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::AddNotifyKeyListener: %p listener already in list. \n",listener));

         
         return ;

    }

    gNotifyListeners.push_back(listener);

}

void rhCoolKey::RemoveNotifyKeyListener(rhIKeyNotify *listener)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RemoveNotifyKeyListener: %p \n",listener));

    if(!GetNotifyKeyListener(listener ))
    { 

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RemoveNotifyKeyListener: %p trying to remove listener not in list \n",listener));

        return ;
    }

    gNotifyListeners.remove(listener);

    listener = NULL;
}

void rhCoolKey::ClearNotifyKeyList()
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ClearNotifyKeyList: \n"));

    while (gNotifyListeners.size() > 0) {
        rhIKeyNotify * node = (gNotifyListeners.front()).get();

        node = NULL;

        gNotifyListeners.pop_front();
    }

}

void rhCoolKey::InsertKeyIntoAvailableList(unsigned long aKeyType, const char * aKeyID,CoolKeyStatus aStatus)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::InsertKeyIntoAvailableList: \n"));
    if (ASCCoolKeyIsAvailable(aKeyType, (char *)aKeyID))
    {
         PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::InsertKeyIntoAvailableList: Key Not Available \n"));

        return;

    }


    CoolKeyNode *node = new CoolKeyNode(aKeyType, aKeyID, aStatus);

    if (!node)
    {
        PR_LOG( coolKeyLog, PR_LOG_ERROR, ("Can't create new  CoolKey Data Structure. \n"));
        return;
    }


    gASCAvailableKeys.push_back(node);

}

void rhCoolKey::RemoveKeyFromAvailableList(unsigned long aKeyType, const char * aKeyID)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RemoveKeyFromAvailableList type %d id %s \n",aKeyType,aKeyID));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return;

    gASCAvailableKeys.remove(node);
    delete node;
}

void rhCoolKey::ClearAvailableList()
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ClearAvailableList \n"));
    while (gASCAvailableKeys.size() > 0) {
        CoolKeyNode *node = gASCAvailableKeys.front();
        delete node;
        gASCAvailableKeys.pop_front();
    }
}

HRESULT rhCoolKey::ASCSetCoolKeyPin(unsigned long aKeyType, const char * aKeyID, const char * aPin)
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::ASCSetCoolKeyPin type %d id %s pin %s \n",aKeyType,aKeyID,aPin));
  CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);
  if (!node)
    return E_FAIL;

  node->mPin = aPin;
  return S_OK;
}

// Interface method implementations

NS_IMETHODIMP rhCoolKey::RhNotifyKeyStateChange(PRUint32 aKeyType,const char *aKeyID, PRUint32 aKeyState, PRUint32 aData,const char* strData)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhNotifyKeyStateChange: id: %s type: %d state %d data: %d \n",aKeyID,aKeyType, aKeyState,aData));

    CoolKeyNode tempKey(aKeyType, aKeyID,(CoolKeyStatus) aKeyState);
    CoolKeyNode *node = NULL;

    AutoCoolKey key(aKeyType, aKeyID);

     switch (aKeyState)
    {
        case eCKState_KeyInserted:
        {
         // The assumption we're making here is that we
         // will never get notified for a non CoolKey.

             CoolKeyStatus keyStatus = eAKS_AppletNotFound;
            if (CoolKeyIsEnrolled(&key))
                keyStatus = eAKS_Available;       else if (CoolKeyHasApplet(&key))                keyStatus = eAKS_Uninitialized;

              PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Key Inserted. ID %s \n",aKeyID));
              InsertKeyIntoAvailableList(tempKey.mKeyType,aKeyID ,(CoolKeyStatus) keyStatus);
          break;
        }
        case eCKState_KeyRemoved:
          PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Key Removed. ID %s \n",aKeyID));
          RemoveKeyFromAvailableList(tempKey.mKeyType, aKeyID);
          break;
        case eCKState_EnrollmentComplete:
        case eCKState_EnrollmentError:
        case eCKState_PINResetComplete:
        case eCKState_PINResetError:
        case eCKState_FormatComplete:
        case eCKState_FormatError:
        case eCKState_BlinkComplete:
        case eCKState_BlinkError:
        case eCKState_OperationCancelled:
{

          node = GetCoolKeyInfo(tempKey.mKeyType, aKeyID);

          if (node) {
            node->mStatus = eAKS_AppletNotFound;
            if (CoolKeyIsEnrolled(&key))
              node->mStatus = eAKS_Available;
            else if (CoolKeyHasApplet(&key))
              node->mStatus = eAKS_Uninitialized;
           }
          break;
        }
        case eCKState_EnrollmentStart:
        case eCKState_PINResetStart:
        case eCKState_FormatStart:
        case eCKState_BlinkStart:
        default:
          break;
      };

      //Now notify all the listeners of the event

      std::list< nsCOMPtr <rhIKeyNotify> >::const_iterator it;
      for(it=gNotifyListeners.begin(); it!=gNotifyListeners.end(); ++it) {

          ((rhIKeyNotify *) (*it))->RhNotifyKeyStateChange(aKeyType,aKeyID,aKeyState,aData,strData);
           
          PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhNotifyKeyStateChange after call to RhNotifyKeyStateChange listener: %p",(*it).get()));


     }

     return NS_OK;
}

NS_IMETHODIMP rhCoolKey::RhCoolKeyUnSetNotifyCallback(rhIKeyNotify *jsNotify)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: input %p  this %p \n",jsNotify,this));

    RemoveNotifyKeyListener(jsNotify);

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: removed listener, size now %d \n",GetNotifyKeyListenerListSize()));


   if(GetNotifyKeyListenerListSize() == 0)
   {
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: input %p  this %p Listener size 0. \n",jsNotify,this));

   }

    return NS_OK;
 }

NS_IMETHODIMP rhCoolKey::RhCoolKeySetNotifyCallback(rhIKeyNotify *jsNotify)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeySetNotifyCallback Object: %p this %p\n",jsNotify,this));

    AddNotifyKeyListener(jsNotify);    

    return NS_OK;

}

NS_IMETHODIMP rhCoolKey::BlinkCoolKey(PRUint32 aKeyType, const char *aKeyID, PRUint32 aRate, PRUint32 aDuration)
{

   PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhBlinkCoolKey thread: %p \n",PR_GetCurrentThread()));

   CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

  if (!node)
  {
      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhBlinkCoolKey: GetCoolKeyInfo failed. \n"));
      return NS_ERROR_FAILURE;
  }

  if (!aKeyID)
  {
    return NS_ERROR_FAILURE ;
  }

  AutoCoolKey key(aKeyType, aKeyID);
  HRESULT hres = CoolKeyBlinkToken(&key, aRate, aDuration);

  if (hres == S_OK)
  {
    node->mStatus = eAKS_BlinkInProgress;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;

}

/* void rhEnrollCoolKey (in unsigned long aKeyType, in string aKeyID, in string aEnrollmentType, in string aScreenName, in string aPin, in string aScreenNamePWord, in string aTokenCode); */

NS_IMETHODIMP rhCoolKey::EnrollCoolKey(PRUint32 aKeyType, const char *aKeyID, const char *aEnrollmentType, const char *aScreenName, const char *aPin, const char *aScreenNamePWord, const char *aTokenCode)
{

    PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Attempting to Enroll Key ,ID: %s \n",aKeyID));
    
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return NS_ERROR_FAILURE;

  // Prevent multiple windows from trying to enroll the same
  // key at the same time. If the key is already in the process
  // of being enrolled, just return S_OK.

    if (node->mStatus == eAKS_EnrollmentInProgress)
        return NS_OK;

    AutoCoolKey key(aKeyType, aKeyID);
    HRESULT hres = CoolKeyEnrollToken(&key, aEnrollmentType, aScreenName, aPin,aScreenNamePWord,aTokenCode);


    if (hres == S_OK)
    {
        node->mStatus = eAKS_EnrollmentInProgress;
        return NS_OK;
    }

    return NS_OK;

}

/* void rhResetCoolKeyPIN (in unsigned long aKeyType, in string aKeyID, in string aScreenName, in string aPIN, in string aScreenNamePwd); */

NS_IMETHODIMP rhCoolKey::ResetCoolKeyPIN(PRUint32 aKeyType, const char *aKeyID, const char *aScreenName, const char *aPIN, const char *aScreenNamePwd)
{
    PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Attempting to Reset Key PIN, ID: %s \n",aKeyID));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhResetCoolKeyPIN no node: thread: %p \n",PR_GetCurrentThread()));
        return NS_ERROR_FAILURE;

    }

  // Prevent multiple windows from trying to reset the pin for the same
  // key at the same time. If the key is already in the process
  // of being reset, just return S_OK.

    if (node->mStatus == eAKS_PINResetInProgress)
        return NS_OK;

  // Key must be available, or we throw an error!

    if (node->mStatus != eAKS_Available)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhResetCoolKeyPIN thread: token unavailable %p \n",PR_GetCurrentThread()));
        return NS_ERROR_FAILURE;

    }

    AutoCoolKey key(aKeyType, aKeyID);

    HRESULT hres = CoolKeyResetTokenPIN(&key, aScreenName, aPIN,aScreenNamePwd);


    if (hres == S_OK) 
    {
        node->mStatus = eAKS_PINResetInProgress;
        return NS_OK;
    }

    return NS_OK;
}

/* void rhRenewCoolKey (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::RenewCoolKey(PRUint32 aKeyType, const char *aKeyID)
{
    PR_LOG( coolKeyLog, PR_LOG_ERROR, ("rhCoolKey::RhRenewCoolKey (not implemented) thread: %p \n",PR_GetCurrentThread()));
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void rhFormatCoolKey (in unsigned long aKeyType, in string aKeyID, in string aEnrollmentType, in string aScreenName, in string aPIN, in string aScreenNamePWord, in string aTokenCode); */

NS_IMETHODIMP rhCoolKey::FormatCoolKey(PRUint32 aKeyType, const char *aKeyID, const char *aEnrollmentType, const char *aScreenName, const char *aPIN, const char *aScreenNamePWord, const char *aTokenCode)
{

    PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Attempting to Format Key, ID: %s. ",aKeyID));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return NS_ERROR_FAILURE;

  // Prevent multiple windows from trying to format the same
  // key at the same time. If the key is already in the process
  // of being formatted, just return S_OK.

    if (node->mStatus == eAKS_FormatInProgress)
        return NS_OK;

  // Throw an error if the key is already busy!
    if (node->mStatus != eAKS_AppletNotFound &&
        node->mStatus != eAKS_Uninitialized &&
        node->mStatus != eAKS_Available)
    {
        return NS_ERROR_FAILURE;
    }

  // Prevent multiple windows from trying to enroll the same
  // key at the same time. If the key is already in the process
  // of being enrolled, just return S_OK.

    AutoCoolKey key(aKeyType, aKeyID);

    HRESULT hres = CoolKeyFormatToken(&key, aEnrollmentType, aScreenName,aPIN,aScreenNamePWord,aTokenCode);

    if (hres == S_OK) 
    {
        node->mStatus = eAKS_FormatInProgress;
        return NS_OK;
    }

    return NS_OK;
}

/* void rhCancelCoolKeyOperation (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::CancelCoolKeyOperation(PRUint32 aKeyType, const char *aKeyID)
{

    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return NS_ERROR_FAILURE;


    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCancelCoolKeyOperation type %d id %s status %d: \n",aKeyType,aKeyID,node->mStatus));

  // If the key isn't busy, then there's nothing to do.

    if (node->mStatus != eAKS_EnrollmentInProgress &&
      node->mStatus != eAKS_UnblockInProgress &&
      node->mStatus != eAKS_PINResetInProgress &&
      node->mStatus != eAKS_RenewInProgress &&
      node->mStatus != eAKS_FormatInProgress) 
          return NS_OK;

    AutoCoolKey key(aKeyType, aKeyID);
    HRESULT hres = CoolKeyCancelTokenOperation(&key);

    if(hres == S_OK)
    {
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

/* void GetCoolKeyCertNicknames (in unsigned long aKeyType, in string aKeyID, out PRUint32 count, [array, size_is (count), retval] out string str); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyCertNicknames(PRUint32 aKeyType, const char *aKeyID, PRUint32 *count, char ***str)
{

    if(!aKeyID || !count)
    {
        return NS_ERROR_FAILURE;
    }

    vector<string>  nicknames;
    AutoCoolKey key(aKeyType, aKeyID);

    HRESULT res =  CoolKeyGetCertNicknames( &key,nicknames);
  
    if(res != S_OK)
    {
        return NS_OK;    

    }

    char **array = NULL;
    int num = nicknames.size();


    array = (char **) nsMemory::Alloc((sizeof(char *) *num));

    if(!array)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
 
    vector<string>::iterator i; 
    int j = 0;
    for(i = nicknames.begin(); i!= nicknames.end(); i++)
    {
        char *tName = (char *) (*i).c_str();

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyCertNicknames  name %s  \n",tName));

        array[j] = NULL;
        if(tName)
        {
            array[j] =(char *) nsMemory::Clone(tName,sizeof(char) * strlen(tName) + 1);
        }

        j++;
    }

    *count = num;
    *str = array;
    
    return NS_OK;
}


/* void rhGetAvailableCoolKeys (out PRUint32 count, [array, size_is (count), retval] out string str); */

NS_IMETHODIMP rhCoolKey::GetAvailableCoolKeys(PRUint32 *count, char ***str)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetAvailableCoolKeys thread: %p \n",PR_GetCurrentThread()));

    if(!count || !str)
    {
        return NS_ERROR_FAILURE;
    }
 
    char **array = NULL;

    long numKeys = ASCGetNumAvailableCoolKeys();

    PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Attempting to get number of keys. Value:  %d \n",numKeys));

    if(numKeys == 0)
    {
        return NS_OK;
    }

    array = (char **) nsMemory::Alloc((sizeof(char *) * numKeys));

    if(!array)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    int i;

    for (i = 0; i < numKeys; i++) {

        unsigned long keyType;
        nsEmbedCString keyID;

        ASCGetAvailableCoolKeyAt(i, &keyType, &keyID);

        const char *id = keyID.get();

        array[i] = NULL;

        if(id)
        {
            array[i] =(char *) nsMemory::Clone(id,sizeof(char) * strlen(id) + 1);

            if(!array[i])
            {
                return NS_ERROR_OUT_OF_MEMORY;

            }

        }
    }

    *count = numKeys;
    *str = array;
    return NS_OK;
}

/* unsigned long rhGetCoolKeyStatus (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::GetCoolKeyStatus(PRUint32 aKeyType, const char *aKeyID, PRUint32 *_retval)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetCoolKeyStatus thread: %p \n",PR_GetCurrentThread()));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if(node)
    {
        *_retval = node->mStatus;
    }
    else
    {
        *_retval = eAKS_Unavailable;

    }
    
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetCoolKeyStatus retval: %d \n",*_retval));

    return NS_OK;
}


/* boolean GetCoolKeyIsReallyCoolKey (in unsigned long aKeyType, in string aKeyID); */


NS_IMETHODIMP rhCoolKey::GetCoolKeyIsReallyCoolKey(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyIsReallyCoolKey thread: %p \n",PR_GetCurrentThread()));

    if (ASCCoolKeyIsAvailable(aKeyType, (char *) aKeyID)) {
        if (aKeyID) {
            AutoCoolKey key(aKeyType, aKeyID);
            PRBool isCool = CoolKeyIsReallyCoolKey(&key);
            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyIsReallyCoolKey isCool: %d \n",(int) isCool));
            *_retval= isCool;
            return NS_OK;
        }
    }
    *_retval = PR_FALSE;
    return NS_OK;
}

/* long GetCoolKeyGetAppletVer (in unsigned long aKeyType, in string aKeyID, in boolean aIsMajor); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyGetAppletVer(PRUint32 aKeyType, const char *aKeyID, PRBool aIsMajor, PRInt32 *_retval)
{

    PR_LOG(coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyAppletVer thread: %p \n",PR_GetCurrentThread()));

    AutoCoolKey key(aKeyType, aKeyID);

    int ver = CoolKeyGetAppletVer(&key, aIsMajor);

    *_retval = ver;

    return S_OK;

}

/* boolean rhCoolKeyIsEnrolled (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::GetCoolKeyIsEnrolled(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeyIsEnrolled thread: %p \n",PR_GetCurrentThread()));
    if (ASCCoolKeyIsAvailable(aKeyType, (char *) aKeyID)) {

    if (aKeyID) {
      AutoCoolKey key(aKeyType, aKeyID);
      PRBool isEnrolled = CoolKeyIsEnrolled(&key);
      *_retval= isEnrolled;
      return NS_OK;
    }
  }

  *_retval = PR_FALSE;

  return NS_OK;
}

/* string GetCoolKeyCertInfo (in unsigned long aKeyType, in string aKeyID, in string aCertNickname); */   
NS_IMETHODIMP rhCoolKey::GetCoolKeyCertInfo(PRUint32 aKeyType, const char *aKeyID, const char *aCertNickname, char **aCertInfo)
{

    string certInfo = "";
    *aCertInfo = NULL;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyCertInfo thread: %p \n",PR_GetCurrentThread()));


    AutoCoolKey key(aKeyType, aKeyID);


    HRESULT res =  CoolKeyGetCertInfo(&key,(char *) aCertNickname, certInfo);

    if(res == S_OK)
    {
        char *info = (char *) certInfo.c_str();

        char *temp =  (char *) nsMemory::Clone(info,sizeof(char) * strlen(info) + 1);
        *aCertInfo = temp;
    }

    return NS_OK;
}


/* string GetCoolKeyIssuerInfo (in unsigned long aKeyType, in string aKeyID); */  NS_IMETHODIMP rhCoolKey::GetCoolKeyIssuerInfo(PRUint32 aKeyType, const char *aKeyID, char **_retval)
  {

    *_retval  = NULL;

    AutoCoolKey key(aKeyType, aKeyID);

    char issuerInfo[256];

    HRESULT res =  CoolKeyGetIssuerInfo(&key, (char *)&issuerInfo,256);

     PR_LOG( coolKeyLog, PR_LOG_ALWAYS, ("Attempting to get the key's Issuer: Key: %s, Issuer  %s. \n",aKeyID, (char *) issuerInfo));

    if(res == S_OK)
    {
        char *temp =  (char *) nsMemory::Clone(issuerInfo,sizeof(char) * strlen((char *)issuerInfo) + 1);
        *_retval  = temp;

    }
      return NS_OK;

  }


/* void rhGetCoolKeyPolicy (in unsigned long aKeyType, in string aKeyID, out string policy); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyPolicy(PRUint32 aKeyType, const char *aKeyID, char **policy)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetCoolKeyPolicy thread: %p \n",PR_GetCurrentThread()));


    if (!aKeyID) {
        return NS_ERROR_FAILURE;
    }

    char policyChar[MAX_STR_LEN] ;
    policyChar[0] = 0;

    AutoCoolKey key(aKeyType, aKeyID);
    HRESULT hres =  CoolKeyGetPolicy(&key, policyChar, MAX_STR_LEN);

    PR_LOG(coolKeyLog,PR_LOG_DEBUG,("rhCoolKey::RhGetCoolKeyPolicy hres: %d \n",hres));
    if (hres == E_FAIL)
    {
        return NS_ERROR_FAILURE;

    }

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetCoolKeyPolicy policy: %s \n",policyChar));

     char *temp =  (char *) nsMemory::Clone(policyChar,sizeof(char) * strlen(policyChar) + 1);

    *policy = temp;

    return NS_OK;
}

/* string GetCoolKeyIssuedTo (in unsigned long aKeyType, in string aKeyID); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyIssuedTo(PRUint32 aKeyType, const char *aKeyID, char **issuedTo)
{
    if (!aKeyID) {
        return NS_ERROR_FAILURE;
    }

    AutoCoolKey key(aKeyType, ( char *)aKeyID);

  //  const char *keyName = CoolKeyGetTokenName(&key);

    char buff[512];
    int bufLength = 512;
    buff[0] = 0;
    
    CoolKeyGetIssuedTo(&key, (char *) buff, bufLength);


    if(!buff[0])
    {
        return NS_OK;
    }

    PR_LOG(coolKeyLog,PR_LOG_DEBUG,("rhCoolKey::RhGetCoolKeyGetIssuedTo  %s \n",(char *) buff));

    char *temp =  (char *) nsMemory::Clone(buff,sizeof(char) * strlen(buff) + 1);

    *issuedTo = temp;

    return NS_OK;

}
/* boolean SetCoolKeyConfigValue (in string aName, in string aValue); */
NS_IMETHODIMP rhCoolKey::SetCoolKeyConfigValue(const char *aName, const char *aValue, PRBool *_retval)
{
     PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::SetCoolKeyConfigValue thread: %p \n",PR_GetCurrentThread()));
    if(!aName || !aValue)
    {
        *_retval = 0;
        return NS_ERROR_FAILURE;
    }

    *_retval = (PRBool)  doSetCoolKeyConfigValue(aName,aValue);
   
    return NS_OK;
}

/* string GetCoolKeyConfigValue (in string aName); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyConfigValue(const char *aName, char **_retval)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyConfigValue thread: %p \n",PR_GetCurrentThread()));

    if(!aName)
    {
        return NS_ERROR_FAILURE;
    }

   *_retval = (char *) doGetCoolKeyConfigValue(aName);

   return NS_OK;   

}

/* boolean rhCoolKeyRequiresAuthentication (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::GetCoolKeyRequiresAuthentication(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhCoolKeyRequiresAuthentication thread: %p \n",PR_GetCurrentThread()));
    PRBool requiresAuth = PR_FALSE;

    *_retval = PR_TRUE;

    if (aKeyID) {
        AutoCoolKey key(aKeyType, aKeyID);
        requiresAuth = CoolKeyRequiresAuthentication(&key);

        *_retval = requiresAuth;
    }

    return NS_OK;
}

/* boolean rhGetCoolKeyIsAuthenticated (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::GetCoolKeyIsAuthenticated(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhGetCoolKeyIsAuthenticated thread: %p \n",PR_GetCurrentThread()));
    PRBool isAuthed = PR_FALSE;

    *_retval = PR_TRUE;

    if (aKeyID) {
        AutoCoolKey key(aKeyType, aKeyID);
        isAuthed = CoolKeyIsAuthenticated(&key);

        *_retval = isAuthed;
    }

    return NS_OK;
}

/* boolean rhAuthenticateCoolKey (in unsigned long aKeyType, in string aKeyID, in string aPIN); */

NS_IMETHODIMP rhCoolKey::AuthenticateCoolKey(PRUint32 aKeyType, const char *aKeyID, const char *aPIN, PRBool *_retval) 
{
  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::RhAuthenticateCoolKey thread: %p \n",PR_GetCurrentThread()));
  *_retval = PR_FALSE;

  if(!aKeyID || !aPIN)
  {
      return NS_ERROR_FAILURE;
  }

  AutoCoolKey key(aKeyType, aKeyID);

  PRBool didAuth = CoolKeyAuthenticate(&key, aPIN);

  if (didAuth)
    ASCSetCoolKeyPin(aKeyType, aKeyID, aPIN);

   *_retval = PR_TRUE;

    return NS_OK;
}

/* void SetCoolKeyDataValue (in unsigned long aKeyType, in string aKeyID, in string name, in string value); */

NS_IMETHODIMP rhCoolKey::SetCoolKeyDataValue(PRUint32 aKeyType, const char *aKeyID, const char *name, const char *value)
{

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::SetCoolKeyDataValue \n"));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
         return NS_ERROR_FAILURE;

    AutoCoolKey key(aKeyType, aKeyID);

    CoolKeySetDataValue(&key,name, value);

    return NS_OK;

}

/* string GetCoolKeyVersion (); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyVersion(char **_retval)
{
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::GetCoolKeyVersion \n"));

    char *version = GETSTRING(ESC_VERSION);
    
    char *versionVal =  (char *) nsMemory::Clone(version,sizeof(char) * strlen(version) +  1);
    
    *_retval = versionVal;   

    return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(rhCoolKey)

NS_IMPL_ISUPPORTS1(rhCoolKey,rhICoolKey) 

// Implement full module and factory dance so we can see what is going on 

class rhCoolKeyFactory: public nsIFactory{
private:
nsrefcnt mRefCnt;
public:
rhCoolKeyFactory();
virtual ~rhCoolKeyFactory();

NS_IMETHOD QueryInterface(const nsIID &aIID, void **aResult);
NS_IMETHOD_(nsrefcnt) AddRef(void);
NS_IMETHOD_(nsrefcnt) Release(void);

NS_IMETHOD CreateInstance(nsISupports *aOuter, const nsIID & iid, void * *result);
NS_IMETHOD LockFactory(PRBool lock);

};

rhCoolKeyFactory::rhCoolKeyFactory()
{
mRefCnt = 0;
}
rhCoolKeyFactory::~rhCoolKeyFactory()
{
}

NS_IMETHODIMP rhCoolKeyFactory::QueryInterface(const nsIID &aIID,
void **aResult)
{
if (aResult == NULL) {
return NS_ERROR_NULL_POINTER;
}
*aResult = NULL;
if (aIID.Equals(kISupportsIID)) {
*aResult = (void *) this;
}
else
if (aIID.Equals(kIFactoryIID)) {
*aResult = (void *) this;
}

if (*aResult == NULL) {
return NS_ERROR_NO_INTERFACE;
}

AddRef();
return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) rhCoolKeyFactory::AddRef()
{
return ++mRefCnt;
}


NS_IMETHODIMP_(nsrefcnt) rhCoolKeyFactory::Release()
{
if (--mRefCnt == 0) {
delete this;
return 0;
}
return mRefCnt;
}


NS_IMETHODIMP
rhCoolKeyFactory::CreateInstance(nsISupports *aOuter, const nsIID & iid, void * *result)
{
if (!result)
return NS_ERROR_INVALID_ARG;

rhCoolKey* sample = new rhCoolKey();
if (!sample)
return NS_ERROR_OUT_OF_MEMORY;

nsresult rv = sample->QueryInterface(iid, result);

if (NS_FAILED(rv)) {
*result = nsnull;
delete sample;
}

return rv;
}


NS_IMETHODIMP
rhCoolKeyFactory::LockFactory(PRBool lock)
{
return NS_ERROR_NOT_IMPLEMENTED;
}

// Module implementation
class rhCoolKeyModule : public nsIModule
{
public:
rhCoolKeyModule();
virtual ~rhCoolKeyModule();

// nsISupports methods:
NS_IMETHOD QueryInterface(const nsIID & uuid, void * *result);
NS_IMETHOD_(nsrefcnt) AddRef(void);
NS_IMETHOD_(nsrefcnt) Release(void);

// nsIModule methods:
NS_IMETHOD GetClassObject(nsIComponentManager *aCompMgr, const nsCID & aClass, const nsIID & aIID, void * *aResult);
NS_IMETHOD RegisterSelf(nsIComponentManager *aCompMgr, nsIFile *aLocation, const char *aLoaderStr, const char *aType);
NS_IMETHOD UnregisterSelf(nsIComponentManager *aCompMgr, nsIFile *aLocation, const char *aLoaderStr);
NS_IMETHOD CanUnload(nsIComponentManager *aCompMgr, PRBool *_retval);

private:
nsrefcnt mRefCnt;
};


//----------------------------------------------------------------------

rhCoolKeyModule::rhCoolKeyModule()
{
mRefCnt = 0;
}

rhCoolKeyModule::~rhCoolKeyModule()
{
}

// nsISupports implemention
NS_IMETHODIMP_(nsrefcnt)
rhCoolKeyModule::AddRef(void)
{
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::AddRef \n"));
++mRefCnt;
return mRefCnt;
}


NS_IMETHODIMP_(nsrefcnt)
rhCoolKeyModule::Release(void)
{
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::Release \n"));
--mRefCnt;
if (mRefCnt == 0) {
mRefCnt = 1; /* stabilize */
delete this;
return 0;
}
return mRefCnt;
}


NS_IMETHODIMP
rhCoolKeyModule::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::QueryInterface \n"));
if ( !aInstancePtr )
return NS_ERROR_NULL_POINTER;

nsISupports* foundInterface;

if ( aIID.Equals(kIModuleIID) )
foundInterface = (nsIModule*) this;

else if ( aIID.Equals(kISupportsIID) )
foundInterface = (nsISupports*) this;

else
foundInterface = 0;

if (foundInterface) {
foundInterface->AddRef();
*aInstancePtr = foundInterface;
return NS_OK;
}

*aInstancePtr = foundInterface;
return NS_NOINTERFACE;
}


// Create a factory object for creating instances of aClass.
NS_IMETHODIMP
rhCoolKeyModule::GetClassObject(nsIComponentManager *aCompMgr,
const nsCID& aClass,
const nsIID& aIID,
void** result)
{

PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::GetClassObject \n"));
if (!kCoolKeyCID.Equals(aClass))
return NS_ERROR_FACTORY_NOT_REGISTERED;

if (!result)
return NS_ERROR_INVALID_ARG;

rhCoolKeyFactory* factory = new rhCoolKeyFactory();
if (!factory)
return NS_ERROR_OUT_OF_MEMORY;

nsresult rv = factory->QueryInterface(aIID, result);

if (NS_FAILED(rv)) {
*result = nsnull;
delete factory;
}

return rv;
}


NS_IMETHODIMP
rhCoolKeyModule::RegisterSelf(nsIComponentManager *aCompMgr,
nsIFile* aPath,
const char* registryLocation,
const char* componentType)
{

nsIComponentRegistrar* compReg = nsnull;

PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::RegisterSelf \n"));
nsresult rv = aCompMgr->QueryInterface(kIComponentRegistrarIID, (void**)&compReg);
if (NS_FAILED(rv))
return rv;

rv = compReg->RegisterFactoryLocation(kCoolKeyCID,
"CoolKey",
"@redhat.com/rhCoolKey",
aPath,
registryLocation,
componentType);

compReg->Release();

return rv;
}

NS_IMETHODIMP
rhCoolKeyModule::UnregisterSelf(nsIComponentManager* aCompMgr,
nsIFile* aPath,
const char* registryLocation)
{

PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::UnregisterSelf \n"));
nsIComponentRegistrar* compReg = nsnull;

nsresult rv = aCompMgr->QueryInterface(kIComponentRegistrarIID, (void**)&compReg);
if (NS_FAILED(rv))
return rv;

rv = compReg->UnregisterFactoryLocation(kCoolKeyCID, aPath);

compReg->Release();

return rv;
}

NS_IMETHODIMP
rhCoolKeyModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
*okToUnload = PR_FALSE; // we do not know how to unload.
return NS_OK;
}


//----------------------------------------------------------------------

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,
nsIFile* location,
nsIModule** return_cobj)
{
nsresult rv = NS_OK;

PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKeyModule::NSGetModule \n"));

// Create and initialize the module instance
rhCoolKeyModule *m = new rhCoolKeyModule();
if (!m) {
return NS_ERROR_OUT_OF_MEMORY;
}

// Increase refcnt and store away nsIModule interface to m in return_cobj
rv = m->QueryInterface(kIModuleIID, (void**)return_cobj);
if (NS_FAILED(rv)) {
delete m;
}
return rv;
}


