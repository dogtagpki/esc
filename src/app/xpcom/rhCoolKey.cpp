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
#include "prthread.h"
#include "nscore.h"
#include "content/nsCopySupport.h"
#include <vector>
#include <string>
#include <time.h>


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

//static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

#define NS_XPCOMPROXY_CONTRACTID "@mozilla.org/xpcomproxy;1"


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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyShutdownObserver::~CoolKeyShutdownObserver \n",GetTStamp(tBuff,56)));
 }
 
 NS_IMETHODIMP
 CoolKeyShutdownObserver::Observe(nsISupports *aSubject,
                                 const char *aTopic,
                                 const PRUnichar *someData)
 {
     char tBuff[56];
     if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
     {
         PR_LOG(coolKeyLog, PR_LOG_DEBUG, ("%s CoolKeyShutdownObserver::Observe shutting down",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::rhCoolKey: %p \n",GetTStamp(tBuff,56),this));

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
        PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s ESC InitInstance failed,exiting. CoolKey instance %p\n",GetTStamp(tBuff,56),coolKey_instance));
        exit(1);
     }

  /* member initializers and constructor code */ 
}

rhCoolKey::~rhCoolKey()
{
   /* destructor code */

    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::~rhCoolKey: %p \n",GetTStamp(tBuff,56),this));
}

void rhCoolKey::ShutDownInstance()
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ShutDownInstance. %p \n",GetTStamp(tBuff,56),this));    

    if (mProxy)
    {
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ShutDownInstance: About to dereference Proxy Object. Proxy %p \n",GetTStamp(tBuff,56),mProxy));

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
    char tBuff[56];
    PRBool ret = PR_TRUE;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::InitInstance %p \n",GetTStamp(tBuff,56),this));

    char xpcom_path[4096];
    xpcom_path[0] = 0;

    static const GREVersionRange greVersion = 
    {
    "1.9", PR_TRUE,
    "2", PR_TRUE
    };

    nsresult rv = GRE_GetGREPathWithProperties(&greVersion, 1, nsnull, 0, xpcom_path, 4096);
    if (NS_FAILED(rv)) {
        return PR_FALSE;
    }

    char *lib_name =(char *) XPCOM_LIB_NAME ;

  
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::xpcom_path %s \n",GetTStamp(tBuff,56),xpcom_path)); 

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
        PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s Can't create Proxy Object for ESC. \n",GetTStamp(tBuff,56)));
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
     PR_LOG(coolKeyLog,PR_LOG_ERROR,("%s Could not get an observer service.  We will leak on shutdown.",GetTStamp(tBuff,56)));
   }

  return ret;

}

rhICoolKey* rhCoolKey::CreateProxyObject()
{
    char tBuff[56];
    rhICoolKey *proxyObject = NULL;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::CreateProxyObject: \n",GetTStamp(tBuff,56)));

    nsCOMPtr<nsIProxyObjectManager> manager =
            do_GetService(NS_XPCOMPROXY_CONTRACTID);

    PR_ASSERT(manager);


    manager->GetProxyForObject(NULL, NS_GET_IID(rhICoolKey), this, NS_PROXY_SYNC | NS_PROXY_ALWAYS, (void**)&proxyObject);

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::CreateProxyObject: original: %p proxy %p  \n",GetTStamp(tBuff,56),this,proxyObject));

    return proxyObject;
   
}

CoolKeyNode* rhCoolKey::GetCoolKeyInfo(unsigned long aKeyType, const char * aKeyID)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyInfo: gASCAvailableKeys %p looking for key %s type %d \n",GetTStamp(tBuff,56),&gASCAvailableKeys,aKeyID,aKeyType));

    std::list<CoolKeyNode*>::const_iterator it;
    for(it=gASCAvailableKeys.begin(); it!=gASCAvailableKeys.end(); ++it) {

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyInfo: current key %s type %d, looking for key %s type %d \n",GetTStamp(tBuff,56),(*it)->mKeyID.get(),(*it)->mKeyType,aKeyID,aKeyType));

        if ((*it)->mKeyType == aKeyType && !strcmp((*it)->mKeyID.get(), aKeyID))
            return *it;
    }

    return 0;
}

// Internal methods

PRBool rhCoolKey::ASCCoolKeyIsAvailable(unsigned long aKeyType, char * aKeyID)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ASCCoolKeyIsAvailable type %d id %s \n",GetTStamp(tBuff,56),aKeyType,aKeyID));
    return GetCoolKeyInfo(aKeyType, aKeyID) ? PR_TRUE : PR_FALSE;
}

HRESULT  rhCoolKey::ASCGetAvailableCoolKeyAt(unsigned long aIndex,
                                           unsigned long *aKeyType,
                                           nsEmbedCString *aKeyID)
{
    char tBuff[56]; 
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ASCGetAvailableCoolKeyAt: index %d type %d id %s \n",GetTStamp(tBuff,56),aIndex,aKeyType,aKeyID)); 
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
    char tBuff[56];
    int size = (int) gASCAvailableKeys.size();
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ASCGetNumAvailableCoolKeys %d \n",GetTStamp(tBuff,56),size));
    return size;

}

int rhCoolKey::GetNotifyKeyListenerListSize()
{
    return gNotifyListeners.size();

}

rhIKeyNotify* rhCoolKey::GetNotifyKeyListener(rhIKeyNotify *listener){

    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetNotifyKeyListener: %p size %d \n",GetTStamp(tBuff,56),listener,gNotifyListeners.size() ));

    std::list<nsCOMPtr<rhIKeyNotify> >::const_iterator it;
    for(it=gNotifyListeners.begin(); it!=gNotifyListeners.end(); ++it) {

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetNotifyKeyListener:  cur %p looking for %p \n",GetTStamp(tBuff,56),(*it).get(),listener));

        if((*it) == listener)
        {
            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetNotifyKeyListener:   looking for %p returning %p \n",GetTStamp(tBuff,56),listener,(*it).get()));
            return (*it);
        }
    }

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetNotifyKeyListener:  looking for %p returning NULL. \n",GetTStamp(tBuff,56),listener));

  return nsnull;
}

void rhCoolKey::AddNotifyKeyListener(rhIKeyNotify *listener)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::AddNotifyKeyListener: %p \n",GetTStamp(tBuff,56),listener));

    if(GetNotifyKeyListener(listener ))
    {
         PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::AddNotifyKeyListener: %p listener already in list. \n",GetTStamp(tBuff,56),listener));

         return ;
    }

    gNotifyListeners.push_back(listener);
}

void rhCoolKey::RemoveNotifyKeyListener(rhIKeyNotify *listener)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RemoveNotifyKeyListener: %p \n",GetTStamp(tBuff,56),listener));

    if(!GetNotifyKeyListener(listener ))
    { 
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RemoveNotifyKeyListener: %p trying to remove listener not in list \n",GetTStamp(tBuff,56),listener));

        return ;
    }

    gNotifyListeners.remove(listener);

    listener = NULL;
}

void rhCoolKey::ClearNotifyKeyList()
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ClearNotifyKeyList: \n",GetTStamp(tBuff,56)));

    while (gNotifyListeners.size() > 0) {
        rhIKeyNotify * node = (gNotifyListeners.front()).get();

        node = NULL;

        gNotifyListeners.pop_front();
    }

}

void rhCoolKey::InsertKeyIntoAvailableList(unsigned long aKeyType, const char * aKeyID,CoolKeyStatus aStatus)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::InsertKeyIntoAvailableList: \n",GetTStamp(tBuff,56)));
    if (ASCCoolKeyIsAvailable(aKeyType, (char *)aKeyID))
    {
         PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::InsertKeyIntoAvailableList: Key Not Available \n",GetTStamp(tBuff,56)));

        return;
    }

    CoolKeyNode *node = new CoolKeyNode(aKeyType, aKeyID, aStatus);

    if (!node)
    {
        PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s Can't create new  CoolKey Data Structure. \n",GetTStamp(tBuff,56)));
        return;
    }

    gASCAvailableKeys.push_back(node);

}

void rhCoolKey::RemoveKeyFromAvailableList(unsigned long aKeyType, const char * aKeyID)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RemoveKeyFromAvailableList type %d id %s \n",GetTStamp(tBuff,56),aKeyType,aKeyID));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return;

    gASCAvailableKeys.remove(node);
    delete node;
}

void rhCoolKey::ClearAvailableList()
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ClearAvailableList \n",GetTStamp(tBuff,56)));
    while (gASCAvailableKeys.size() > 0) {
        CoolKeyNode *node = gASCAvailableKeys.front();
        delete node;
        gASCAvailableKeys.pop_front();
    }
}

HRESULT rhCoolKey::ASCSetCoolKeyPin(unsigned long aKeyType, const char * aKeyID, const char * aPin)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::ASCSetCoolKeyPin type %d id %s pin %s \n",GetTStamp(tBuff,56),aKeyType,aKeyID,aPin));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);
    if (!node)
        return E_FAIL;

    node->mPin = aPin;
    return S_OK;
}

// Interface method implementations

NS_IMETHODIMP rhCoolKey::RhNotifyKeyStateChange(PRUint32 aKeyType,const char *aKeyID, PRUint32 aKeyState, PRUint32 aData,const char* strData)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhNotifyKeyStateChange: id: %s type: %d state %d data: %d \n",GetTStamp(tBuff,56),aKeyID,aKeyType, aKeyState,aData));

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

             PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s Key Inserted. ID %s \n",GetTStamp(tBuff,56),aKeyID));
             InsertKeyIntoAvailableList(tempKey.mKeyType,aKeyID ,(CoolKeyStatus) keyStatus);
          break;
        }
        case eCKState_KeyRemoved:
            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s Key Removed. ID %s \n",GetTStamp(tBuff,56),aKeyID));
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
           
          PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhNotifyKeyStateChange after call to RhNotifyKeyStateChange listener: %p",GetTStamp(tBuff,56),(*it).get()));

     }

     return NS_OK;
}

NS_IMETHODIMP rhCoolKey::RhCoolKeyUnSetNotifyCallback(rhIKeyNotify *jsNotify)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: input %p  this %p \n",GetTStamp(tBuff,56),jsNotify,this));

    RemoveNotifyKeyListener(jsNotify);

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: removed listener, size now %d \n",GetTStamp(tBuff,56),GetNotifyKeyListenerListSize()));

   if(GetNotifyKeyListenerListSize() == 0)
   {
       PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeyUnSetNotifyCallback Object: input %p  this %p Listener size 0. \n",GetTStamp(tBuff,56),jsNotify,this));
   }

   return NS_OK;
 }

NS_IMETHODIMP rhCoolKey::RhCoolKeySetNotifyCallback(rhIKeyNotify *jsNotify)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeySetNotifyCallback Object: %p this %p\n",GetTStamp(tBuff,56),jsNotify,this));

    AddNotifyKeyListener(jsNotify);    

    return NS_OK;

}

/* void CoolKeyInitializeLog (in string aPathName, in unsigned long aMaxLines); */
NS_IMETHODIMP rhCoolKey::CoolKeyInitializeLog(const char *aPathName, PRUint32 aMaxLines)
{
    ::CoolKeyInitializeLog((char *)aPathName, aMaxLines);  

    return NS_OK;
}

/* void CoolKeyLogMsg (in unsigned long aLogLevel, in string aMessage); */
NS_IMETHODIMP rhCoolKey::CoolKeyLogMsg(PRUint32 aLogLevel, const char *aMessage)
{
    char tBuff[56];

    if(aMessage && ((PRLogModuleLevel) aLogLevel >=  PR_LOG_NONE && aLogLevel <= PR_LOG_MAX))
    {
        ::CoolKeyLogMsg((PRLogModuleLevel) aLogLevel, "%s %s \n",GetTStamp(tBuff,56),aMessage);
        PR_LOG( coolKeyLog, (PRLogModuleLevel) aLogLevel, ("%s %s",GetTStamp(tBuff,56),aMessage));
    }

    return NS_OK;
}

NS_IMETHODIMP rhCoolKey::BlinkCoolKey(PRUint32 aKeyType, const char *aKeyID, PRUint32 aRate, PRUint32 aDuration)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhBlinkCoolKey thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

   CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

  if (!node)
  {
      PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhBlinkCoolKey: GetCoolKeyInfo failed. \n",GetTStamp(tBuff,56)));
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

    char tBuff[56];
    ::CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Attempting to Enroll Key ,ID: %s \n",GetTStamp(tBuff,56),aKeyID);
    
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
    char tBuff[56];
    ::CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Attempting to Reset Key Password, ID: %s \n",GetTStamp(tBuff,56),aKeyID);
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
    {
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhResetCoolKeyPIN no node: thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhResetCoolKeyPIN thread: token unavailable %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_ERROR, ("%s rhCoolKey::RhRenewCoolKey (not implemented) thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void rhFormatCoolKey (in unsigned long aKeyType, in string aKeyID, in string aEnrollmentType, in string aScreenName, in string aPIN, in string aScreenNamePWord, in string aTokenCode); */

NS_IMETHODIMP rhCoolKey::FormatCoolKey(PRUint32 aKeyType, const char *aKeyID, const char *aEnrollmentType, const char *aScreenName, const char *aPIN, const char *aScreenNamePWord, const char *aTokenCode)
{
    char tBuff[56];
    ::CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Attempting to Format Key, ID: %s. ",GetTStamp(tBuff,56),aKeyID);
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
    char tBuff[56];
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if (!node)
        return NS_ERROR_FAILURE;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCancelCoolKeyOperation type %d id %s status %d: \n",GetTStamp(tBuff,56),aKeyType,aKeyID,node->mStatus));

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
    char tBuff[56];
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

        PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyCertNicknames  name %s  \n",GetTStamp(tBuff,56),tName));

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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetAvailableCoolKeys thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

    if(!count || !str)
    {
        return NS_ERROR_FAILURE;
    }
 
    char **array = NULL;

    long numKeys = ASCGetNumAvailableCoolKeys();

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s Attempting to get number of keys. Value:  %d \n",GetTStamp(tBuff,56),numKeys));

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
    char tBuff[56]; 
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetCoolKeyStatus thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
    CoolKeyNode *node = GetCoolKeyInfo(aKeyType, aKeyID);

    if(node)
    {
        *_retval = node->mStatus;
    }
    else
    {
        *_retval = eAKS_Unavailable;
    }
    
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetCoolKeyStatus retval: %d \n",GetTStamp(tBuff,56),*_retval));

    return NS_OK;
}


/* boolean GetCoolKeyIsReallyCoolKey (in unsigned long aKeyType, in string aKeyID); */


NS_IMETHODIMP rhCoolKey::GetCoolKeyIsReallyCoolKey(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyIsReallyCoolKey thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

    if (aKeyType && aKeyID && ASCCoolKeyIsAvailable(aKeyType, (char *) aKeyID)) {
        if (aKeyID) {
            AutoCoolKey key(aKeyType, aKeyID);
            PRBool isCool = CoolKeyIsReallyCoolKey(&key);
            PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyIsReallyCoolKey isCool: %d \n",GetTStamp(tBuff,56),(int) isCool));
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
    char tBuff[56];
    PR_LOG(coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyAppletVer thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

    AutoCoolKey key(aKeyType, aKeyID);

    int ver = CoolKeyGetAppletVer(&key, aIsMajor);

    *_retval = ver;

    return S_OK;
}

/* boolean rhCoolKeyIsEnrolled (in unsigned long aKeyType, in string aKeyID); */

NS_IMETHODIMP rhCoolKey::GetCoolKeyIsEnrolled(PRUint32 aKeyType, const char *aKeyID, PRBool *_retval)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeyIsEnrolled thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    string certInfo = "";
    *aCertInfo = NULL;

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyCertInfo thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

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

/* string GetCoolKeyATR (in unsigned long aKeyType, in string aKeyID); */
  NS_IMETHODIMP rhCoolKey::GetCoolKeyATR(PRUint32 aKeyType, const char *aKeyID, char **_retval)
{
    char tBuff[56];
    *_retval  = NULL;
    AutoCoolKey key(aKeyType, aKeyID);
    char atr[128];
    HRESULT res =   CoolKeyGetATR(&key, (char *)&atr,sizeof(atr));
     PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s Attempting to get the key's ATR: Key: %s, ATR  %s. \n",GetTStamp(tBuff,56),aKeyID, (char *) atr));
    if(res == S_OK)
    {
        char *temp =  (char *) nsMemory::Clone(atr,sizeof(char) * strlen((char *)atr) + 1);
        *_retval  = temp;
    }
      return NS_OK;
  }

/* string GetCoolKeyTokenName (in unsigned long aKeyType, in string aKeyID); */
 NS_IMETHODIMP rhCoolKey::GetCoolKeyTokenName(PRUint32 aKeyType, const char *aKeyID, char **_retval)
{
  char tBuff[56];

  *_retval = NULL;

  if(!aKeyType && !aKeyID)
      return NS_OK;

  AutoCoolKey key(aKeyType,aKeyID);
  
  char *tokenName = NULL;

  tokenName = (char *) CoolKeyGetTokenName(&key);

  PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyTokenName %s \n",GetTStamp(tBuff,56),tokenName));
  if(tokenName)
  {
      char *temp =  (char *) nsMemory::Clone(tokenName,sizeof(char) * strlen((char *)tokenName) + 1);
      *_retval  = temp;

  }

  return NS_OK;

}

/* string GetCoolKeyIssuerInfo (in unsigned long aKeyType, in string aKeyID); */  NS_IMETHODIMP rhCoolKey::GetCoolKeyIssuerInfo(PRUint32 aKeyType, const char *aKeyID, char **_retval)
{
    char tBuff[56];
    *_retval  = NULL;

    AutoCoolKey key(aKeyType, aKeyID);

    char issuerInfo[256];

    HRESULT res =  CoolKeyGetIssuerInfo(&key, (char *)&issuerInfo,256);

    ::CoolKeyLogMsg( PR_LOG_ALWAYS, "%s Attempting to get the key's Issuer: Key: %s, Issuer  %s. \n",GetTStamp(tBuff,56),aKeyID, (char *) issuerInfo);

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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetCoolKeyPolicy thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

    if (!aKeyID) {
        return NS_ERROR_FAILURE;
    }

    char policyChar[MAX_STR_LEN] ;
    policyChar[0] = 0;

    AutoCoolKey key(aKeyType, aKeyID);
    HRESULT hres =  CoolKeyGetPolicy(&key, policyChar, MAX_STR_LEN);

    PR_LOG(coolKeyLog,PR_LOG_DEBUG,("%s rhCoolKey::RhGetCoolKeyPolicy hres: %d \n",GetTStamp(tBuff,56),hres));
    if (hres == E_FAIL)
    {
        return NS_ERROR_FAILURE;

    }

    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetCoolKeyPolicy policy: %s \n",GetTStamp(tBuff,56),policyChar));

     char *temp =  (char *) nsMemory::Clone(policyChar,sizeof(char) * strlen(policyChar) + 1);

    *policy = temp;

    return NS_OK;
}

/* string GetCoolKeyIssuedTo (in unsigned long aKeyType, in string aKeyID); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyIssuedTo(PRUint32 aKeyType, const char *aKeyID, char **issuedTo)
{
    char tBuff[56];
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

    PR_LOG(coolKeyLog,PR_LOG_DEBUG,("%s rhCoolKey::RhGetCoolKeyGetIssuedTo  %s \n",GetTStamp(tBuff,56),(char *) buff));

    char *temp =  (char *) nsMemory::Clone(buff,sizeof(char) * strlen(buff) + 1);

    *issuedTo = temp;

    return NS_OK;

}

/* string GetCoolKeyIssuer (in unsigned long aKeyType, in string aKeyID); */
NS_IMETHODIMP rhCoolKey::GetCoolKeyIssuer(PRUint32 aKeyType, const char *aKeyID, char **issuer)
{
    char tBuff[56];
    if (!aKeyID) {
        return NS_ERROR_FAILURE;
    }

    AutoCoolKey key(aKeyType, ( char *)aKeyID);

  //  const char *keyName = CoolKeyGetTokenName(&key);

    char buff[512];
    int bufLength = 512;
    buff[0] = 0;
   
    CoolKeyGetIssuer(&key, (char *) buff, bufLength);

    if(!buff[0])
    {
        return NS_OK;
    }

    PR_LOG(coolKeyLog,PR_LOG_DEBUG,("%s rhCoolKey::RhGetCoolKeyGetIssuer  %s \n",GetTStamp(tBuff,56),(char *) buff));

    char *temp =  (char *) nsMemory::Clone(buff,sizeof(char) * strlen(buff) + 1);

    *issuer = temp;

    return NS_OK;

}

/* boolean SetCoolKeyConfigValue (in string aName, in string aValue); */
NS_IMETHODIMP rhCoolKey::SetCoolKeyConfigValue(const char *aName, const char *aValue, PRBool *_retval)
{
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("rhCoolKey::SetCoolKeyConfigValue thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyConfigValue thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));

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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhCoolKeyRequiresAuthentication thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhGetCoolKeyIsAuthenticated thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::RhAuthenticateCoolKey thread: %p \n",GetTStamp(tBuff,56),PR_GetCurrentThread()));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::SetCoolKeyDataValue \n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetCoolKeyVersion \n",GetTStamp(tBuff,56)));

    char *version = (char *) GETSTRING(ESC_VERSION);
    
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
char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::AddRef \n",GetTStamp(tBuff,56)));
++mRefCnt;
return mRefCnt;
}


NS_IMETHODIMP_(nsrefcnt)
rhCoolKeyModule::Release(void)
{
char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::Release : mRefCnt %d \n",GetTStamp(tBuff,56),mRefCnt - 1));
--mRefCnt;
if (mRefCnt == 0) {
mRefCnt = 1; /* stabilize */
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::Release deleting Module \n",GetTStamp(tBuff,56)));
delete this;
return 0;
}
return mRefCnt;
}


NS_IMETHODIMP
rhCoolKeyModule::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::QueryInterface \n",GetTStamp(tBuff,56)));
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
char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::GetClassObject \n",GetTStamp(tBuff,56)));
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

char tBuff[54];
nsIComponentRegistrar* compReg = nsnull;

PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::RegisterSelf \n",GetTStamp(tBuff,56)));
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
char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::UnregisterSelf \n",GetTStamp(tBuff,56)));
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

char tBuff[56];
PR_LOG( coolKeyLog, PR_LOG_DEBUG, ("%s rhCoolKeyModule::NSGetModule \n",GetTStamp(tBuff,56)));

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
