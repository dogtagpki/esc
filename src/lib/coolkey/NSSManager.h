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

#ifdef DARWIN 
#define COOLKEY_PKCS11_LIBRARY   "/Library/Application Support/CoolKey/PKCS11/libcoolkeypk11.dylib"
#else
#define COOLKEY_PKCS11_LIBRARY   DLL_PREFIX"coolkeypk11."DLL_SUFFIX
#endif

#define COOLKEY_NAME             "COOL Key Module"
#define MUSCLE_NAME             "SLB PKCS #11 module"
#define PROMISCUOUS_PARAMETER   "noAppletOK=yes"
#define NSS_PUBLIC_CERTS	"slotFlags=PublicCerts"

#define NSS_NO_ERROR 0
#define NSS_ERROR_LOAD_COOLKEY 1
#define NSS_ERROR_SMART_CARD_THREAD 2

#ifndef NSSMANAGER_H
#define NSSMANAGER_H

#include "SmartCardMonitoringThread.h"
#include "keythi.h"
#include "certt.h"
#include "cert.h"

#define NICKNAME_EXPIRED_STRING " (expired)"
#define NICKNAME_NOT_YET_VALID_STRING " (not yet valid)"
//#include <vector>
//#include <string>

//using namespace std;

class NSSManager
{  
 public:
  NSSManager(); 
  virtual ~NSSManager();


  HRESULT InitNSS(const char *aAppDir);
  void Shutdown();

  static bool AuthenticateCoolKey(const CoolKey *aKey, const char *aPIN);
  static bool RequiresAuthentication(const CoolKey *aKey);
  static bool IsAuthenticated(const CoolKey *aKey);

  static HRESULT GetSignatureLength(const CoolKey *aKey, int *aLength);
  static HRESULT SignDataWithKey(const CoolKey *aKey, 
                                 const unsigned char *aData, int aDataLen, 
                                 unsigned char *aSignedData, int *aSignedDataLen);
  static HRESULT GetKeyPolicy(const CoolKey *aKey, char *aBuf, int aBufLength);


  static HRESULT GetKeyCertInfo(const CoolKey *aKey, char *aCertNickname, string & aCertInfo);

  static HRESULT  GetKeyCertNicknames( const CoolKey *aKey,  vector<string> & aStrings  ); 

  static HRESULT GetKeyIssuedTo(const CoolKey *aKey, char *aBuf, int aBufLength);

  static HRESULT GetKeyIssuer(const CoolKey *aKey, char *aBuf, int aBufLength);

  static unsigned int GetLastInitError() { return lastError;}

 private:

   static bool IsCACert(CERTCertificate *cert);

   static unsigned int lastError;

#ifdef LINUX
  PK11SlotInfo *systemCertDB;
#endif
  SmartCardMonitoringThread *mpSCMonitoringThread;
};


#endif
