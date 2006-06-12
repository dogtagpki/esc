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

#ifndef RH_COOLKEY_H 
#define RH_COOLKEY_H

#include "rhICoolKey.h"
#include "nsIGenericFactory.h"
#include "nsEmbedString.h"
#include <list>
#include "CoolKey.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"

#ifdef XP_WIN32
#include "CoolKeyCSP.h"
#endif

// generate unique ID here with uuidgen
#define COOLKEY_CID \
{ 0x777f7150, 0x4a2b, 0x4301, \
{ 0xad, 0x10, 0x5e, 0xab, 0x25, 0xb3, 0x22, 0xaa}}

typedef enum {
  eAKS_Unavailable = 0,             // Key is not present.
  eAKS_AppletNotFound,              // Key has a matching ATR but no applet.
  eAKS_Uninitialized,               // Key has an applet, but no certs.
  eAKS_Unknown,                     // We're confused.
  eAKS_Available,                   // Key has either a Phase 1 or Phase 2 cert.
  eAKS_EnrollmentInProgress,        // Key is undergoing enrollment.
  eAKS_UnblockInProgress,           // Key is processing an unblock request.
  eAKS_PINResetInProgress,          // Key is undergoing a PIN reset.
  eAKS_RenewInProgress,             // Key is undergoing certificate renewal
  eAKS_FormatInProgress,            // Key is being reformated.
  eAKS_BlinkInProgress              // Key is blinking.
} CoolKeyStatus;


using namespace std;

class CoolKeyNode
{
  public:

  CoolKeyNode(unsigned long aKeyType, const char * aKeyID, CoolKeyStatus aStatus) {
    mKeyType = aKeyType;
    mKeyID   = aKeyID;
    mStatus  = aStatus;
    mPin     = "";
  }

  ~CoolKeyNode() {
  }

  unsigned long mKeyType;
  nsEmbedCString mKeyID;
  CoolKeyStatus mStatus;
  nsEmbedCString mPin;
};

class rhCoolKey : public rhICoolKey
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_RHICOOLKEY


    rhCoolKey();

    void ShutDownInstance();
    ~rhCoolKey();
private:
    static HRESULT Dispatch( rhICoolKey *listener,
    unsigned long keyType, const char *keyID, unsigned long keyState,
    unsigned long data, const char *strData);
    static HRESULT Reference( rhICoolKey *listener );
    static HRESULT Release( rhICoolKey *listener );

    static HRESULT doSetCoolKeyConfigValue(const char *aName, const char *aValue); 
    static const char *doGetCoolKeyConfigValue(const char *aName );

protected:
  /* additional members */

    nsCOMPtr<rhIKeyNotify> mJsNotify;


    #ifdef XP_WIN32
    CoolKeyCSPKeyListener*  mCSPListener;
    #endif

    static std::list<CoolKeyNode*> gASCAvailableKeys;

    static std::list< nsCOMPtr <rhIKeyNotify> > gNotifyListeners;

    rhICoolKey* mProxy;

    static PRBool      gAutoEnrollBlankTokens;

    nsCOMPtr<nsISupports> nssComponent;
    PRBool InitInstance();

    HRESULT ASCSetCoolKeyPin(unsigned long aKeyType, const char * aKeyID, const char * aPin);
    PRBool ASCCoolKeyIsAvailable(unsigned long aKeyType, char * aKeyID);

    CoolKeyNode* GetCoolKeyInfo(unsigned long aKeyType, const char * aKeyID);
    HRESULT  ASCGetAvailableCoolKeyAt(unsigned long aIndex,
                                            unsigned long *aKeyType,
                                            nsEmbedCString *aKeyID);

    int   ASCGetNumAvailableCoolKeys(void);


    void InsertKeyIntoAvailableList(unsigned long aKeyType, const char * aKeyID,
                                       CoolKeyStatus aStatus);

    void RemoveKeyFromAvailableList(unsigned long aKeyType, const char * aKeyID);

    rhIKeyNotify* GetNotifyKeyListener(rhIKeyNotify *listener);

    int GetNotifyKeyListenerListSize();
    void AddNotifyKeyListener(rhIKeyNotify *listener);
    void RemoveNotifyKeyListener(rhIKeyNotify *listener);
    void ClearNotifyKeyList();

    void ClearAvailableList();

    rhICoolKey* CreateProxyObject();

};

#endif




