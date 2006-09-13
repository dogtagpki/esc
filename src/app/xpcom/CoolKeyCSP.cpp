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

#include <string.h>
#include <windows.h>
//#include <wincrypt.h>
#include <prlog.h>
#include "openkey.h"
#include "CoolKeyCSP.h"

#define MAX_CONTAINER_NAME 128
#define MAX_KEY_ID 128

static PRLogModuleInfo *coolKeyCSPLog = PR_NewLogModule("coolKeyCSP");

HCRYPTPROV CoolKeyCSPKeyListener::mCryptProv = NULL;

CoolKeyCSPKeyListener::CoolKeyCSPKeyListener() 
{
};

CoolKeyCSPKeyListener:: ~CoolKeyCSPKeyListener() {};


HCRYPTPROV CoolKeyCSPKeyListener::GetCryptHandle()
{

    if(!CoolKeyCSPKeyListener::mCryptProv)
    {

       CryptAcquireContext(&CoolKeyCSPKeyListener::mCryptProv, NULL, 
           OPENKEY_PROV, PROV_RSA_FULL,CRYPT_VERIFYCONTEXT);

    }


    return CoolKeyCSPKeyListener::mCryptProv;

}

// AddCert
//
// Add the binary encoded cert (in buffer pbCert of length cbCert)
// to the cert store hCertStore.  OPENKEY_PROV_W, wszContainer, and
// dwKeySpec are used to create the key provider info for the cert.
// We add the keyID as a user-defined cert property so we can find
// such certs later.
//
// Return TRUE on success, FALSE on failure.

static BOOL
AddCert(
  const BYTE *pbCert,
  DWORD cbCert,
  WCHAR *wszContainer,
  DWORD dwKeySpec,
  const char *keyID,
  HCERTSTORE hCertStore)
{
  BOOL rv = TRUE;
  PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
                                X509_ASN_ENCODING, pbCert, cbCert);
  if (pCertContext == NULL)
  {
    return FALSE;
  }

  // Set three cert properties:
  // 1. friendly name
  // 2. key provider info, which uniquely identifies the cert's
  //    private key
  // 3. our own user-defined KeyID property

  CRYPT_DATA_BLOB blob;

  // The friendly name property is a Unicode string.
  blob.cbData = (DWORD) (wcslen(OPENKEY_NAME_W)+1) * sizeof(wchar_t);
  blob.pbData = (BYTE *) OPENKEY_NAME_W;
  if (!CertSetCertificateContextProperty(pCertContext,
      CERT_FRIENDLY_NAME_PROP_ID, 0, &blob))
  {
    rv = FALSE;
    goto failed;
  }

  CRYPT_KEY_PROV_INFO keyProvInfo;
  memset(&keyProvInfo, 0, sizeof keyProvInfo);
  keyProvInfo.pwszContainerName = wszContainer;
  keyProvInfo.pwszProvName = OPENKEY_PROV_W;
  keyProvInfo.dwProvType = PROV_RSA_FULL;
  // I wonder if we can set keyProvInfo.dwFlags to
  // CERT_SET_KEY_PROV_HANDLE_PROP_ID.
  keyProvInfo.dwKeySpec = dwKeySpec;
  if (!CertSetCertificateContextProperty(pCertContext,
      CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo))
  {
    rv = FALSE;
    goto failed;
  }

  blob.cbData = (DWORD) (strlen(keyID) + 1);
  blob.pbData = (BYTE *) keyID;
  if (!CertSetCertificateContextProperty(pCertContext,
      OPENKEY_CERT_KEY_ID_PROP_ID, 0, &blob))
  {
    rv = FALSE;
    goto failed;
  }

  if (!CertAddCertificateContextToStore(hCertStore, pCertContext,
      CERT_STORE_ADD_REPLACE_EXISTING, // Is CERT_STORE_ADD_ALWAYS better?
      NULL))
  {
    rv = FALSE;
  }

failed:
  CertFreeCertificateContext(pCertContext); // always returns TRUE
  return rv; 
}

// GetCert
//
// Get the binary encoded cert corresponding to the private key hKey.
// On successful return, *ppbCert points to a buffer that holds the
// encoded cert, and *pcbCert is the length of the encoded cert. The
// caller is responsible for freeing *ppbCert with a free() call.
//
// Return TRUE on success, FALSE on failure.

static BOOL
GetCert(
  HCRYPTKEY hKey,
  BYTE **ppbCert,
  DWORD *pcbCert)
{
  BYTE *pbCert;
  DWORD cbCert;
  if (!CryptGetKeyParam(hKey, KP_CERTIFICATE, NULL,  &cbCert, 0))
  {
    return FALSE;
  }
  pbCert = (BYTE *) malloc(cbCert);
  if (!pbCert)
  {
    return FALSE;
  }
  if (!CryptGetKeyParam(hKey, KP_CERTIFICATE, pbCert, &cbCert, 0))
  {
    free(pbCert);
    return FALSE;
  }
  *ppbCert = pbCert;
  *pcbCert = cbCert;
  return TRUE;
}

// PropCertsInContainer
//
// Propagate the certs in the key container named szContainer to
// the cert store hCertStore.  These certs will have our own
// user-defined key ID property with the value keyID.
//
// Return TRUE on success, FALSE on failure.

static BOOL
PropCertsInContainer(
  const char *keyID,
  const char *szContainer,
  HCERTSTORE hCertStore)
{
  BOOL rv = TRUE;

  HCRYPTPROV hCryptProv = CoolKeyCSPKeyListener::GetCryptHandle();
  
  if (!hCryptProv)
  {
    return FALSE;
  }

  size_t containerLen = strlen(szContainer) + 1;
  wchar_t *wszContainer = (wchar_t *) malloc(containerLen*sizeof(wchar_t));
  if (!wszContainer)
  {
    rv = FALSE;
    goto failed;
  }
  mbstowcs(wszContainer, szContainer, containerLen);

  // Iterate through the private keys in this key container.

  const DWORD dwKeySpec[] = {AT_KEYEXCHANGE, AT_SIGNATURE};
  const DWORD dwNumKeySpec = sizeof(dwKeySpec)/sizeof(dwKeySpec[0]);
  DWORD i;

  // If anything fails, we go on to propagate the next cert.

  for (i = 0; i < dwNumKeySpec; i++)
  {
    HCRYPTKEY hUserKey = NULL;
    if (!CryptGetUserKey(hCryptProv, dwKeySpec[i], &hUserKey))
    {
      // NTE_NO_KEY means there is no key of this type and is
      // not a real error.
      if (GetLastError() != NTE_NO_KEY)
      {
        rv = FALSE;
      }
      continue;
    }
    PBYTE pbCert = NULL;
    DWORD cbCert = 0;
    BOOL bOK = GetCert(hUserKey, &pbCert, &cbCert);
    if (!CryptDestroyKey(hUserKey))
    {
      // Should not happen.
      rv = FALSE;
    }
    if (!bOK)
    {
      rv = FALSE;
      continue;
    }

    bOK = AddCert(pbCert, cbCert, wszContainer, dwKeySpec[i],
        keyID, hCertStore);
    free(pbCert);
    if (!bOK)
    {
      rv = FALSE;
    }
  }

failed:
  free(wszContainer);
/*  if (!CryptReleaseContext(hContext, 0))
  {
    rv = FALSE;
  }

*/
  return rv;
}

// PropCerts
//
// Propagate the certs on the token aKey to the local "My" cert
// store so that CryptoAPI applications such as Internet Explorer
// can use those certs.
//
// Return TRUE on success, FALSE on failure.

static BOOL
PropCerts( CoolKey *aKey)
{
  BOOL rv = TRUE;
  BOOL hasReader = (BOOL) CoolKeyHasReader(aKey);
  if (!hasReader)
  {
    return FALSE;
  }

   HCRYPTPROV hCryptProv = CoolKeyCSPKeyListener::GetCryptHandle();
  if (!hCryptProv)
  {
    return FALSE;
  }
  HCERTSTORE hCertStore = CertOpenSystemStore(NULL, "My");
  if (!hCertStore)
  {
    rv = FALSE;
    goto failed;
  }

  // Enumerate the key containers in our CSP and acquire
  // context for each of them.

  char szContainer[MAX_CONTAINER_NAME];
  DWORD dwContainerLen = sizeof szContainer;
  DWORD dwFlags = CRYPT_FIRST;
  // XXX Microsoft documentation says that all of the available
  // container names might not be enumerated if this function is
  // used in a multithreaded context.  We should find out exactly
  // what that means and if what we are doing here is safe.
  while (CryptGetProvParam(hCryptProv, PP_ENUMCONTAINERS, 
      (BYTE *) szContainer, &dwContainerLen, dwFlags))
  {
    // XXX the container name should be fully qualified:
    //     \\.\reader\container
    // But our CSP only recognizes simple container names.
    PropCertsInContainer(aKey->mKeyID, szContainer, hCertStore);
    dwContainerLen = sizeof szContainer;
    dwFlags = 0;
  }

failed:
  if (!CertCloseStore(hCertStore, CERT_CLOSE_STORE_CHECK_FLAG))
  {
    rv = FALSE;
  }
/*  if (!CryptReleaseContext(hCryptProv, 0))
  {
    rv = FALSE;
  }

*/
  return rv;
}

// RemoveCerts
//
// Remove the certs on the token aKey from the local "My" cert store.
//
// Return TRUE on success, FALSE on failure.

static BOOL
RemoveCerts(CoolKey *aKey)
{
  BOOL rv = TRUE;
  HCERTSTORE hCertStore = CertOpenSystemStore(NULL, "My");
  if (!hCertStore)
  {
    return FALSE;
  }
  PCCERT_CONTEXT pCertContext = NULL;
  DWORD dwFindProperty = OPENKEY_CERT_KEY_ID_PROP_ID;
  while ((pCertContext = CertFindCertificateInStore(hCertStore,
      X509_ASN_ENCODING, 0, CERT_FIND_PROPERTY, &dwFindProperty,
      pCertContext)) != NULL)
  {
    char keyID[MAX_KEY_ID];
    DWORD cbKeyID = sizeof keyID;
    if (!CertGetCertificateContextProperty(pCertContext,
        OPENKEY_CERT_KEY_ID_PROP_ID, keyID, &cbKeyID))
    {
      rv = FALSE;
      continue;
    }
    if (!strcmp(keyID, aKey->mKeyID))
    {
      // Both CertDeleteCertificateFromStore and
      // CertFindCertificateInStore free the cert
      // context, so we have to duplicate the cert
      // context, pass the duplicate to CertDeleteCertificateFromStore,
      // and pass the original to CertFindCertificateInStore.
      PCCERT_CONTEXT pDupCertContext;
      if ((pDupCertContext = CertDuplicateCertificateContext(pCertContext))
          == NULL)
      {
        rv = FALSE;
        continue;
      }
      if (!CertDeleteCertificateFromStore(pDupCertContext))
      {
        rv = FALSE;
      }
    }
  }

  if (!CertCloseStore(hCertStore, CERT_CLOSE_STORE_CHECK_FLAG))
  {
    rv = FALSE;
  }
  return rv;
}

NS_IMETHODIMP CoolKeyCSPKeyListener::RhNotifyKeyStateChange(PRUint32 aKeyType,const char *aKeyID, PRUint32 aKeyState, PRUint32 aData, const char* strData)
{
  BOOL bOK = TRUE;


   
   PR_LOG( coolKeyCSPLog, PR_LOG_DEBUG, ("CoolKeyCSPListener::RhNotifyStateChange state %d \n",aKeyState));
   AutoCoolKey key(aKeyType, aKeyID);


  switch (aKeyState)
  {
    //case eCKState_KeyInserted:
    case eCKState_EnrollmentComplete:

      bOK = PropCerts(&key);
      break;

    //case eCKState_KeyRemoved:
    case eCKState_FormatComplete:
      bOK = RemoveCerts(&key);

      break;

    default:
      break;
  };

  return NS_OK;
}


NS_IMPL_ISUPPORTS1(CoolKeyCSPKeyListener,rhIKeyNotify)
