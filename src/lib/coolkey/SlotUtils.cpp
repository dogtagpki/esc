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
#include <time.h>

#include "SlotUtils.h"
#include <list>
#include "CoolKeyHandler.h"

#include "zlib.h"

#include "SAOL_OIDS.h"

static std::list<CoolKeyInfo*> gCoolKeyList;
PRLock *gCoolKeyListLock = NULL;

static PRLogModuleInfo *coolKeyLogSU = PR_NewLogModule("coolKeySlot");
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s DestroyCoolKeyList:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s LockCoolKeyList:\n gCoolKeyListLock %p",GetTStamp(tBuff,56),gCoolKeyListLock));

    if(!gCoolKeyListLock)
    {
        gCoolKeyListLock = PR_NewLock();
    }

    if (gCoolKeyListLock) {

    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s LockCoolKeyList:\n gCoolKeyListLock %p about to lock gCoolKeyListLock",GetTStamp(tBuff,56),gCoolKeyListLock));

    PR_Lock(gCoolKeyListLock);

    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s LockCoolKeyList:\n gCoolKeyListLock %p obtained gCoolKeyListLock",GetTStamp(tBuff,56),gCoolKeyListLock));

#ifdef DEBUG
    assert(gCoolKeyListLockCount == 0);
    ++gCoolKeyListLockCount;
#endif // DEBUG
  }
}

void
UnlockCoolKeyList()
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s UnLockCoolKeyList:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoByReaderName:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoBySlotName:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoBySlot:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoByKeyIDInternal:\n",GetTStamp(tBuff,56)));
    std::list<CoolKeyInfo*>::iterator it;

    if(!aKey)
        return NULL;

    for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
    {

        PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoByKeyIDInternal id %s:\n",GetTStamp(tBuff,56),(*it)->mCUID));
    // XXX_KEY_TYPE_CASUALTY: We need to check the keyType here too!
        if (!PL_strcasecmp((*it)->mCUID, aKey->mKeyID))
            return *it;
    }

    return NULL;
}

CoolKeyInfo *
GetCoolKeyInfoByKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoByKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;
    return GetCoolKeyInfoByKeyIDInternal(aKey);
}

CoolKeyInfo *
GetCoolKeyInfoByTokenName(const char *aTokenName)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCoolKeyInfoByTokenName:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetReaderNameForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if (!info)
        return 0;

    return info->mReaderName;
}

const char *
GetSlotNameForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetSlotNameForKeyID:\n",GetTStamp(tBuff,56)));
    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);
    if (!info)
        return NULL;

    return PK11_GetSlotName(info->mSlot);
}

const char *
GetATRForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetSlotNameForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if (!info)
        return 0;

    return info->mATR;
}

const char *
GetCUIDForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetCUIDForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if (!info)
        return 0;

    return info->mCUID;
}

const char *
GetMSNForKeyIDInternal(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetMSNForKeyIDInternal:\n",GetTStamp(tBuff,56)));
    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if (!info)
        return 0;

    return info->mMSN;
}

const char *
GetMSNForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetMSNForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;
    return GetMSNForKeyIDInternal(aKey);
}


unsigned int
GetInfoFlagsForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetInfoFlagsForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    const CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if (!info)
        return 0;

    return info->mInfoFlags;
}

int
RefreshInfoFlagsForKeyID(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s RefreshInfoFlagsForKeyID:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    CoolKeyInfo *info = GetCoolKeyInfoByKeyIDInternal(aKey);

    if(!info)
        return -1;
    int alreadyCoolKey = 0;
    if( IS_REALLY_A_COOLKEY(info->mInfoFlags))
        alreadyCoolKey = 1;

    if (!info)
        return -1;

    if (!HAS_ATR(info->mInfoFlags))
        return -1;

    info->mInfoFlags = CKHGetInfoFlags(info->mSlot);

    if(alreadyCoolKey)
    {
        info->mInfoFlags |= COOLKEY_INFO_IS_REALLY_A_COOLKEY_MASK;
    }

    return 0;
}

int
InsertCoolKeyInfoIntoCoolKeyList(CoolKeyInfo *aInfo)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s InsertCoolKeyInfoIntoCoolKeyList:\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    if (!aInfo)
        return -1;

    gCoolKeyList.push_back(aInfo);

    return 0;
}

int
RemoveCoolKeyInfoFromCoolKeyList(CoolKeyInfo *aInfo)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s RemoveCoolKeyInfoFromCoolKeyList:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s ClearCoolKeyList:entering\n",GetTStamp(tBuff,56)));
    AutoCoolKeyListLock autoLock;

    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s ClearCoolKeyList:\n",GetTStamp(tBuff,56)));
    std::list<CoolKeyInfo*>::iterator it;

    for (it = gCoolKeyList.begin(); it != gCoolKeyList.end(); ++it)
    {
        PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s ClearCoolKeyList: clearing %p \n",GetTStamp(tBuff,56),*it));
        CoolKeyInfo *info = *it;
        delete info;
    }

    gCoolKeyList.clear();

    return 0;
}

PK11SlotInfo *GetSlotForKeyID(const CoolKey *aKey)
{
    AutoCoolKeyListLock autoLock;

    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetSlotForKeyID:\n",GetTStamp(tBuff,56)));  

    if (!aKey)
    {
        PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetSlotForKeyID: null CoolKey.\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetAuthKey:\n",GetTStamp(tBuff,56)));
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
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetAuthenticationPrivateKey:\n",GetTStamp(tBuff,56)));
    return (SECKEYPrivateKey*) GetAuthKey(eGetPrivateKey, slot);
}

SECItem* GetAuthenticationPublicKey(PK11SlotInfo* slot)
{
    char tBuff[56];
    PR_LOG( coolKeyLogSU, PR_LOG_DEBUG, ("%s GetAuthenticationPrivateKey:\n",GetTStamp(tBuff,56)));
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
