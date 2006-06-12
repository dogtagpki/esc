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

#include "nss.h"
#include "secmod.h"
#include "pk11func.h"
#include "ssl.h"
#include "p12plcy.h"
#include "secmod.h"
#include "secerr.h"
#include "certdb.h"
#include "secmodt.h"
#include "keythi.h"
#include "keyhi.h"

#include "prprf.h"
#include "prlock.h"
#include <assert.h>

#include "SlotUtils.h"
#include <list>
#include "CoolKeyHandler.h"

#include "zlib.h"

#include "SAOL_OIDS.h"

static std::list<CoolKeyInfo*> gCoolKeyList;
PRLock *gCoolKeyListLock = NULL;

static PRLogModuleInfo *coolKeyLogSU = PR_NewLogModule("coolKey");
#ifdef DEBUG
int gCoolKeyListLockCount = 0;
#endif // DEBUG

class AutoCoolKeyListLock
{
public:
  AutoCoolKeyListLock() { LockCoolKeyList(); }
  ~AutoCoolKeyListLock() { UnlockCoolKeyList(); }
};

void
InitCoolKeyList()
{

  gCoolKeyListLock = PR_NewLock();
}

void
DestroyCoolKeyList()
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("DestroyCoolKeyList:\n"));
  ClearCoolKeyList();

  if (gCoolKeyListLock)
  {
    PR_DestroyLock(gCoolKeyListLock);
    gCoolKeyListLock = NULL;
  }
}

void
LockCoolKeyList()
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("LockCoolKeyList:\n gCoolKeyListLock %p",gCoolKeyListLock));

  if(!gCoolKeyListLock)
  {
      gCoolKeyListLock = PR_NewLock();

  }

  if (gCoolKeyListLock) {

    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("LockCoolKeyList:\n gCoolKeyListLock %p about to lock gCoolKeyListLock",gCoolKeyListLock));

    PR_Lock(gCoolKeyListLock);

    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("LockCoolKeyList:\n gCoolKeyListLock %p obtained gCoolKeyListLock",gCoolKeyListLock));

#ifdef DEBUG
    assert(gCoolKeyListLockCount == 0);
    ++gCoolKeyListLockCount;
#endif // DEBUG
  }
}

void
UnlockCoolKeyList()
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("UnLockCoolKeyList:\n"));
  if (gCoolKeyListLock) {
#ifdef DEBUG
    --gCoolKeyListLockCount;
    assert(gCoolKeyListLockCount == 0);
#endif // DEBUG
    PR_Unlock(gCoolKeyListLock);
  }
}

CoolKeyInfo *
GetCoolKeyInfoByReaderName(const char *aReaderName)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoByReaderName:\n"));
  AutoCoolKeyListLock autoLock;

  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
    if (!PL_strcasecmp((*it)->mReaderName, aReaderName))
      return *it;
  }

  return NULL;
}

CoolKeyInfo *
GetCoolKeyInfoBySlotName(const char *aSlotName)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoBySlotName:\n"));
  AutoCoolKeyListLock autoLock;

  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
      const char* slotName = PK11_GetSlotName((*it)->mSlot);

      if (!PL_strcasecmp(slotName, aSlotName))
      {
        return (*it);
      }
  }

  return NULL;
}

CoolKeyInfo *
GetCoolKeyInfoBySlot(PK11SlotInfo *aSlot)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoBySlot:\n"));
  AutoCoolKeyListLock autoLock;

  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
      if ((*it)->mSlot == aSlot)
      {
        return (*it);
      }
  }

  return NULL;
}

CoolKeyInfo *
GetCoolKeyInfoByKeyIDInternal(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoByKeyIDInternal:\n"));
  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoByKeyIDInternal id %s:\n",(*it)->mCUID));
    // XXX_KEY_TYPE_CASUALTY: We need to check the keyType here too!
    if (!PL_strcasecmp((*it)->mCUID, aKey->mKeyID))
      return *it;
  }

  return NULL;
}

CoolKeyInfo *
GetCoolKeyInfoByKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoByKeyID:\n"));
  AutoCoolKeyListLock autoLock;
  return GetCoolKeyInfoByKeyIDInternal(aKey);
}


CoolKeyInfo *
GetCoolKeyInfoByTokenName(const char *aTokenName)
{

  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCoolKeyInfoByTokenName:\n"));
  AutoCoolKeyListLock autoLock;

  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
      char* tokenName = PK11_GetTokenName((*it)->mSlot);

      if (!PL_strcasecmp(tokenName, aTokenName))
      {
        return (*it);
      }
  }

  return NULL;
}

const char *
GetReaderNameForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetReaderNameForKeyID:\n"));
  AutoCoolKeyListLock autoLock;

  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return 0;

  return info->mReaderName;
}

const char *
GetSlotNameForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetSlotNameForKeyID:\n"));
  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);
  if (!info)
    return NULL;

  return PK11_GetSlotName(info->mSlot);
}

const char *
GetATRForKeyID(const CoolKey *aKey)
{

  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetSlotNameForKeyID:\n"));
  AutoCoolKeyListLock autoLock;

  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return 0;

  return info->mATR;
}

const char *
GetCUIDForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetCUIDForKeyID:\n"));
  AutoCoolKeyListLock autoLock;

  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return 0;

  return info->mCUID;
}

const char *
GetMSNForKeyIDInternal(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetMSNForKeyIDInternal:\n"));
  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return 0;

  return info->mMSN;
}

const char *
GetMSNForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetMSNForKeyID:\n"));
  AutoCoolKeyListLock autoLock;
  return GetMSNForKeyIDInternal(aKey);
}


unsigned int
GetInfoFlagsForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetInfoFlagsForKeyID:\n"));
  AutoCoolKeyListLock autoLock;

  const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return 0;

  return info->mInfoFlags;
}

int
RefreshInfoFlagsForKeyID(const CoolKey *aKey)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("RefreshInfoFlagsForKeyID:\n"));
  AutoCoolKeyListLock autoLock;

  CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

  if (!info)
    return -1;

  if (!HAS_ATR(info->mInfoFlags))
    return -1;

  info->mInfoFlags = CKHGetInfoFlags(info->mSlot);

  return 0;
}

int
InsertCoolKeyInfoIntoCoolKeyList(CoolKeyInfo *aInfo)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("InsertCoolKeyInfoIntoCoolKeyList:\n"));
  AutoCoolKeyListLock autoLock;

  if (!aInfo)
    return -1;

  gCoolKeyList.push_back(aInfo);

  return 0;
}

int
RemoveCoolKeyInfoFromCoolKeyList(CoolKeyInfo *aInfo)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("RemoveCoolKeyInfoFromCoolKeyList:\n"));
  AutoCoolKeyListLock autoLock;

  if (!aInfo)
    return -1;

  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
    if (*it == aInfo)
    {
      gCoolKeyList.erase(it);
      break;
    }
  }

  return 0;
}

int
ClearCoolKeyList()
{

  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("ClearCoolKeyList:entering\n"));
  AutoCoolKeyListLock autoLock;

  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("ClearCoolKeyList:\n"));
  std::list<CoolKeyInfo*>::iterator it;

  for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
  {
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("ClearCoolKeyList: clearing %p \n",*it));
    CoolKeyInfo *info = *it;
    delete info;
  }

  gCoolKeyList.clear();

  return 0;
}

PK11SlotInfo *GetSlotForKeyID(const CoolKey *aKey)
{
  AutoCoolKeyListLock autoLock;


   PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetSlotForKeyID:\n"));  

  if (!aKey)
  {
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetSlotForKeyID: null CoolKey.\n"));
    return NULL;
  }

  CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);
  if (!info)
    return NULL;

  return PK11_ReferenceSlot(info->mSlot);
}

enum {
  eGetPublicKey,
  eGetPrivateKey
};

void* GetAuthKey(int keyType, PK11SlotInfo* slot)
{
  PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetAuthKey:\n"));
  CERTCertListNode *node;
  CERTCertList *certs = PK11_ListCertsInSlot(slot);
  if (!certs)
    return NULL;

  for (node = CERT_LIST_HEAD(certs); !CERT_LIST_END(node,certs); node = CERT_LIST_NEXT(node))
  {
    SECItem policyItem;
    policyItem.data = 0;
    
    SECStatus s = CERT_FindCertExtension(node->cert, SEC_OID_X509_CERTIFICATE_POLICIES, &policyItem);
    
    if (s != SECSuccess || !policyItem.data) 
      continue;
    
    CERTCertificatePolicies *policies = CERT_DecodeCertificatePoliciesExtension(&policyItem);
    
    if (!policies)
    {
      PORT_Free(policyItem.data);
      continue;
    }
    
    CERTPolicyInfo **policyInfos = policies->policyInfos;
    
    while (*policyInfos)
    {
      char *policyID = CERT_GetOidString(&(*policyInfos)->policyID);
      if (!PL_strcasecmp(policyID, SAOL_OID_BRONZE))
      {
        PR_smprintf_free(policyID);
        PORT_Free(policyItem.data);

        if (keyType == eGetPrivateKey)
          return PK11_FindPrivateKeyFromCert(slot, node->cert, NULL);

        if (keyType == eGetPublicKey)
          return SECITEM_DupItem(&node->cert->derPublicKey);
      }
      policyInfos++;
      PR_smprintf_free(policyID);
    }

    PORT_Free(policyItem.data);
    CERT_DestroyCertificatePoliciesExtension(policies);
  }

  CERT_DestroyCertList(certs);
  return NULL;
}

SECKEYPrivateKey* GetAuthenticationPrivateKey(PK11SlotInfo* slot)
{
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetAuthenticationPrivateKey:\n"));
    return (SECKEYPrivateKey*) GetAuthKey(eGetPrivateKey, slot);
}

SECItem* GetAuthenticationPublicKey(PK11SlotInfo* slot)
{
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("GetAuthenticationPrivateKey:\n"));
    return (SECItem*) GetAuthKey(eGetPublicKey, slot);
}


#ifdef DUMP_CHALLENGE_RESPONSE
void DumpAuthenticationPublicKey(PK11SlotInfo* slot, FILE *f)
{
  CERTCertListNode *node;
  CERTCertList *certs = PK11_ListCertsInSlot(slot);
  if (!certs)
    return;

  for (node = CERT_LIST_HEAD(certs); !CERT_LIST_END(node,certs); node = CERT_LIST_NEXT(node))
  {
    SECItem policyItem;
    policyItem.data = 0;
    
    SECStatus s = CERT_FindCertExtension(node->cert, SEC_OID_X509_CERTIFICATE_POLICIES, &policyItem);
    
    if (s != SECSuccess || !policyItem.data) 
      continue;
    
    CERTCertificatePolicies *policies = CERT_DecodeCertificatePoliciesExtension(&policyItem);
    
    if (!policies)
    {
      PORT_Free(policyItem.data);
      continue;
    }
    
    CERTPolicyInfo **policyInfos = policies->policyInfos;
    
    while (*policyInfos)
    {
      char *policyID = CERT_GetOidString(&(*policyInfos)->policyID);
      if (!PL_strcasecmp(policyID, SAOL_OID_BRONZE))
      {
        PR_smprintf_free(policyID);
        PORT_Free(policyItem.data);  
        fwrite(node->cert->derCert.data, node->cert->derCert.len, 1, f);
        return; 
      }
      policyInfos++;
      PR_smprintf_free(policyID);
    }

    PORT_Free(policyItem.data);
    CERT_DestroyCertificatePoliciesExtension(policies);
  }

  CERT_DestroyCertList(certs);
  return;
}
#endif
