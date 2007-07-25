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

#ifndef __COOLKEY_H__
#define __COOLKEY_H__

// The following ifdef block is the windows way of creating macros which make
// exporting from a DLL simpler. All files within this DLL are compiled with
// the COOLKEY_EXPORTS symbol defined on the command line. 
// This only works with windows, so the more standard way is to use a .def
// file. Since we have core conf, we can use a single .def file for all
// platforms (coreconf will do the appropriate processing.
#define COOLKEY_API

#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>

////////////////////////////////////////////////////////////////////////
//
// Public Structures and Functions:
//
////////////////////////////////////////////////////////////////////////

#ifndef WIN32
#define S_OK 0
#define E_FAIL -1
typedef int HRESULT;
#else
#include <windows.h>
#endif

typedef enum {
  eCKState_KeyInserted = 1000,
  eCKState_KeyRemoved,
  eCKState_EnrollmentStart,
  eCKState_EnrollmentComplete,
  eCKState_EnrollmentError,
  eCKState_UnblockStart,
  eCKState_UnblockComplete,
  eCKState_UnblockError,
  eCKState_PINResetStart,
  eCKState_PINResetComplete,
  eCKState_PINResetError,
  eCKState_RenewStart,
  eCKState_RenewComplete,
  eCKState_RenewError,
  eCKState_FormatStart,
  eCKState_FormatComplete,
  eCKState_FormatError,
  eCKState_BlinkStart,
  eCKState_BlinkComplete,
  eCKState_BlinkError,
  eCKState_OperationCancelled,
  eCKState_StatusUpdate,
  eCKState_NeedAuth
} CoolKeyState;

typedef enum {
  eCKType_Invalid = 0,
  eCKType_CoolKey,
  eCKType_VeriSignKey
} CoolKeyType;

typedef struct CoolKey {
  unsigned long mKeyType;
  char *mKeyID;
} CoolKey;

/*#ifdef WIN32
class IWindowsKeyListener;
#define CoolKeyListener IWindowsKeyListener
#else
*/
class rhICoolKey;
#define CoolKeyListener rhICoolKey 
/*
#endif
*/
typedef HRESULT (*CoolKeyDispatch)(CoolKeyListener *listener,
   unsigned long aKeyType, const char *aKeyID, unsigned long aKeyState, 
   unsigned long aData, const char *aStrData); 
typedef HRESULT (*CoolKeyReference)(CoolKeyListener *listener);
typedef HRESULT (*CoolKeyRelease)(CoolKeyListener *listener);

typedef HRESULT (*CoolKeySetConfigValue)(const char *name,const char *value);
typedef const char * (*CoolKeyGetConfigValue)(const char *name);



extern "C" {


COOLKEY_API HRESULT CoolKeyInit(const char *aAppDir);
COOLKEY_API HRESULT CoolKeyShutdown();
COOLKEY_API HRESULT CoolKeyRegisterListener(CoolKeyListener* aListener);
COOLKEY_API HRESULT CoolKeyUnregisterListener(CoolKeyListener* aListener);
COOLKEY_API HRESULT CoolKeySetCallbacks(CoolKeyDispatch dispatch,
                        CoolKeyReference reference, CoolKeyRelease release,
                        CoolKeyGetConfigValue getconfigvalue,CoolKeySetConfigValue setconfigvalue);

COOLKEY_API bool    CoolKeyRequiresAuthentication(const CoolKey *aKey);
COOLKEY_API bool    CoolKeyHasApplet(const CoolKey *aKey);
COOLKEY_API bool    CoolKeyIsEnrolled(const CoolKey *aKey);
COOLKEY_API bool    CoolKeyHasReader(const CoolKey *aKey);
COOLKEY_API bool    CoolKeyIsReallyCoolKey(const CoolKey *aKey);

COOLKEY_API bool    CoolKeyAuthenticate(const CoolKey *aKey, const char *aPIN);
COOLKEY_API HRESULT CoolKeyGenerateRandomData(unsigned char *aBuf, int aBufLen);
COOLKEY_API HRESULT CoolKeyGetSignatureLength(const CoolKey *aKey, int *aLength);
COOLKEY_API HRESULT CoolKeySignData(const CoolKey *aKey,
                                  const unsigned char *aData,
                                  int aDataLen, unsigned char *aSignedData,
                                  int *aSignedDataLen);
COOLKEY_API HRESULT CoolKeyGetPolicy(const CoolKey *aKey, char *aBuf, int aBufLen);

COOLKEY_API HRESULT CoolKeyGetCertNicknames( const CoolKey *aKey , std::vector<std::string> & aNames);

COOLKEY_API HRESULT CoolKeyGetCertInfo(const CoolKey *aKey, char *aCertNickname, std::string & aCertInfo);

COOLKEY_API HRESULT CoolKeyGetIssuedTo(const CoolKey *aKey, char *aBuf, int aBufLength);
COOLKEY_API HRESULT CoolKeyGetIssuer(const CoolKey *aKey, char *aBuf, int aBufLength);

COOLKEY_API bool    CoolKeyRequiresAuthentication(const CoolKey *aKey);
COOLKEY_API bool    CoolKeyIsAuthenticated(const CoolKey *aKey);

COOLKEY_API HRESULT CoolKeyEnrollToken(const CoolKey *aKey,
                     const char *aTokenType, const char *aScreenName, 
                     const char *aPIN,const char *screenNamePWord,
		     const char *tokenCode);
COOLKEY_API HRESULT CoolKeyUnblockToken(const CoolKey *aKey);
COOLKEY_API HRESULT CoolKeyResetTokenPIN(const CoolKey *aKey,
                                       const char *aScreenName,
                                       const char *aPIN,
                                       const char *aScreenNamePwd);
COOLKEY_API HRESULT CoolKeyRenewToken(const CoolKey *aKey);
COOLKEY_API HRESULT CoolKeyFormatToken(const CoolKey *aKey, 
                     const char *aTokenType, const char *aScreenName, 
                     const char *aPIN,const char *screenNamePWord,
		     const char *tokenCode);
COOLKEY_API HRESULT CoolKeyCancelTokenOperation(const CoolKey *aKey);
COOLKEY_API HRESULT CoolKeyBlinkToken(const CoolKey *aKey,
                                    unsigned long aRate,
                                    unsigned long aDuration);
COOLKEY_API HRESULT CoolKeyBinToHex(const unsigned char *aInput,
                                  unsigned long aInputLength,
                                  unsigned char *aOutput,
                                  unsigned long aOutputLength, bool aCaps);

COOLKEY_API HRESULT RemoveKeyFromActiveKeyList(const CoolKey *aKey);

COOLKEY_API HRESULT IsNodeInActiveKeyList(const CoolKey *aKey);


COOLKEY_API HRESULT CoolKeySetDataValue(const CoolKey *aKey,const char *name, const char *value);

COOLKEY_API HRESULT CoolKeyGetIssuerInfo(const CoolKey *aKey, char *aBuf, int aBufLen);

COOLKEY_API HRESULT CoolKeyGetATR(const CoolKey *aKey, char *aBuf, int aBufLen);


COOLKEY_API int CoolKeyGetAppletVer(const CoolKey *aKey, const bool isMajor);

COOLKEY_API HRESULT CoolKeyInitializeLog(char *logFileName, int maxNumLines);

COOLKEY_API HRESULT CoolKeyLogMsg(int logLevel, const char *fmt, ...);

COOLKEY_API HRESULT CoolKeyLogNSSStatus();

//Utility time function
char *GetTStamp(char *aTime,int aSize);
}

////////////////////////////////////////////////////////////////////////
//
// Private Structures and Functions:
//
////////////////////////////////////////////////////////////////////////

struct AutoCoolKey : public CoolKey
{
  AutoCoolKey(void)
  {
    mKeyType = eCKType_Invalid;
    mKeyID = NULL;
  }

  AutoCoolKey(unsigned long aKeyType, const char *aKeyID)
  {
    mKeyType = aKeyType;
    mKeyID = NULL;

    if (aKeyID)
      mKeyID = strdup(aKeyID);
  }

  AutoCoolKey(const CoolKey &aKey)
  {
    mKeyType = aKey.mKeyType;
    mKeyID = NULL;

    if (aKey.mKeyID)
      mKeyID = strdup(aKey.mKeyID);
  }

  ~AutoCoolKey(void)
  {
    if (mKeyID)
      free(mKeyID);
  }

  AutoCoolKey& operator=(const CoolKey &aKey)
  {
    if (mKeyID)
      free(mKeyID);

    mKeyType = aKey.mKeyType;
    mKeyID = NULL;

    if (aKey.mKeyID)
      mKeyID = strdup(aKey.mKeyID);

    return *this;
  }

  bool operator==(const CoolKey &aKey)
  {
    if (mKeyType == aKey.mKeyType) {
      if (mKeyID && aKey.mKeyID)
        return !strcmp(mKeyID, aKey.mKeyID);
    }

    return false;
  }
};

extern "C" {
HRESULT CoolKeyNotify(const CoolKey *aKey, CoolKeyState aKeyState, int aData,const char *strData=NULL);

const char *CoolKeyGetTokenName(const CoolKey *aKey);
const char *CoolKeyGetKeyID(const char *tokenName, int *aKeyType);

const char *CoolKeyGetConfig(const char *aName);
HRESULT     CoolKeySetConfig(const char *aName,const char *aValue);

}

#endif  // __COOLKEY_H__
