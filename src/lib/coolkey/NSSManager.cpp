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

#include "NSSManager.h"
#include "SmartCardMonitoringThread.h"

#include "prprf.h"
#include "prsystem.h"
#include "prthread.h"
#include "prlink.h"
#include "prmem.h"

#include "nss.h"
#include "secmod.h"
#include "pk11func.h"
#include "ssl.h"
#include "p12plcy.h"
#include "secmod.h"
#include "secerr.h"
#include "secder.h"
#include "certdb.h"
#include "secmodt.h"
#include "keythi.h"
#include "keyhi.h"

#include <iostream>
#include <sstream>

#include "SlotUtils.h"

static PRLogModuleInfo *coolKeyLogNSS = PR_NewLogModule("coolKeyNSS");

unsigned int NSSManager::lastError;
NSSManager::NSSManager()
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::NSSManager:\n",GetTStamp(tBuff,56)));
    mpSCMonitoringThread = NULL;
    NSSManager::lastError = NSS_NO_ERROR;
#ifdef LINUX
    systemCertDB = NULL;
#endif
}

NSSManager::~NSSManager()
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::~NSSManager:\n",GetTStamp(tBuff,56)));
    if (mpSCMonitoringThread) {
        delete mpSCMonitoringThread;
        mpSCMonitoringThread = NULL;
    }

}

HRESULT NSSManager::InitNSS(const char *aAppDir)
{
  // Init NSS

    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_ALWAYS, ("%s Initializing the NSS Crypto Library. \n",GetTStamp(tBuff,56)));

    if(aAppDir)
    {
        SECStatus status =  NSS_Init(aAppDir);

        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS:\n",GetTStamp(tBuff,56)));
        if(status != SECSuccess)
        {
            PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS: db init failed try simple init.\n",GetTStamp(tBuff,56)));
	    status = NSS_NoDB_Init(NULL);

            PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS: tried NSS_NoDB_Init res %d .\n",GetTStamp(tBuff,56),status));

	    if(status != SECSuccess)
            {
                PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS:Simple init failed.\n",GetTStamp(tBuff,56)));
		return E_FAIL;
            }
        }
    }

    char *libName = COOLKEY_PKCS11_LIBRARY ;

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS: About to try SECMOD_AddNewModule :%s \n",GetTStamp(tBuff,56),libName));

    char modSpec[512];

    sprintf(modSpec,"library=\"%s\" name=\"%s\" parameters=\"%s\" NSS=\"slotParams={0x00000002=[slotFlags='PublicCerts']}\"\n",COOLKEY_PKCS11_LIBRARY,COOLKEY_NAME,PROMISCUOUS_PARAMETER);

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS: modSpec %s\n",GetTStamp(tBuff,56),modSpec));

    SECMODModule *userModule = SECMOD_LoadUserModule(modSpec,NULL,0);

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s InitNSS: Done SECMOD_LoadUserModule %p \n",GetTStamp(tBuff,56),userModule));

    if(!userModule || !userModule->loaded)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_ALWAYS, ("%s NSSManager::InitNSS problem loading PKCS11 module. No keys will be recognized!\n",GetTStamp(tBuff,56)));

        NSSManager::lastError = NSS_ERROR_LOAD_COOLKEY;
        return E_FAIL;
    }

#ifdef LINUX

    // Load our Linux only database


    const char *modspec = "configdir='/etc/pki/nssdb' tokenDescripton='SystemDB' flags='readOnly'";
    PK11SlotInfo *systemCertDB = SECMOD_OpenUserDB(modspec);

    if(!systemCertDB)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_ALWAYS, ("%s NSSManager::InitNSS problem loading Linux  System Cert Database!\n",GetTStamp(tBuff,56)));
    }

#endif

    mpSCMonitoringThread = new SmartCardMonitoringThread(userModule);
    if (!mpSCMonitoringThread) {
        SECMOD_UnloadUserModule(userModule);
        NSSManager::lastError =  NSS_ERROR_SMART_CARD_THREAD;
        return E_FAIL;
    }
    mpSCMonitoringThread->Start();

    return S_OK;
}

void NSSManager::Shutdown()
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::Shutdown \n",GetTStamp(tBuff,56)));
    if (mpSCMonitoringThread) {

        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::Shutdown Stopping Smart Thread %p \n",GetTStamp(tBuff,56),mpSCMonitoringThread));
        mpSCMonitoringThread->Stop();
    }
  
  // Logout all tokens.
    PK11_LogoutAll();

#ifdef LINUX
    if(systemCertDB)
    {
        SECMOD_CloseUserDB(systemCertDB);

        PK11_FreeSlot(systemCertDB);
        systemCertDB = NULL;
    }

#endif
}

bool 
NSSManager::AuthenticateCoolKey(const CoolKey *aKey, const char *aPIN)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::AuthenticateCoolKey \n",GetTStamp(tBuff,56)));
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
  
    if (!slot)
        return false;
  
    if (!PK11_IsPresent(slot)) {
        PK11_FreeSlot(slot);
        return false;
    }
  
    if (!PK11_NeedLogin(slot)) {
        PK11_FreeSlot(slot);
        return true;
    }
  
    SECStatus status = PK11_CheckUserPassword(slot, (char *)aPIN);
    PK11_FreeSlot(slot);
  
  // Note: SECWouldBlock means that the password was incorrect.
  //       SECFailure means NSS encountered a failure that couldn't
  //       be fixed by a retry.
  
    bool didAuth = (status == SECSuccess);
  
    return didAuth;
}

HRESULT 
NSSManager::GetSignatureLength(const CoolKey *aKey, int *aLength)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetSignatureLength \n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID || !aLength)
        return E_FAIL;
  
    *aLength = 0;
  
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
    if (!slot)
        return E_FAIL;
  
    SECKEYPrivateKey *privKey = GetAuthenticationPrivateKey(slot);
  
    if (!privKey)
        return E_FAIL;
  
    *aLength = PK11_SignatureLen(privKey);
  
    PK11_FreeSlot(slot);
    SECKEY_DestroyPrivateKey(privKey);
    return S_OK;
}

HRESULT 
NSSManager::SignDataWithKey(const CoolKey *aKey, 
                            const unsigned char *aData, int aDataLen, 
                            unsigned char *aSignedData, int *aSignedDataLen)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::SignDataWithKey \n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID || !aData || aDataLen < 1 ||
          !aSignedData || !aSignedDataLen)
        return E_FAIL;
  
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
    if (!slot)
        return E_FAIL;
  
    SECKEYPrivateKey *privKey = GetAuthenticationPrivateKey(slot);
  
    if (!privKey)
        return E_FAIL; 
  
  // Make sure the caller supplied us with a aSignedData
  // buffer that was large enough!
  
    int sigLen = PK11_SignatureLen(privKey);
  
    if (sigLen > *aSignedDataLen)
        return E_FAIL;
  
    unsigned char digest[1024]; // How do I dynamically check the size needed?
    unsigned int digestLen;
  
    PK11Context* DigestContext = PK11_CreateDigestContext(SEC_OID_SHA1);
    SECStatus s = PK11_DigestBegin(DigestContext);
    s = PK11_DigestOp(DigestContext, aData, aDataLen);
    s = PK11_DigestFinal(DigestContext, digest, &digestLen, sizeof digest);
  
    PK11_DestroyContext(DigestContext, PR_TRUE);
  
    SECItem sig, hash;
  
    sig.data = aSignedData;
    sig.len = *aSignedDataLen;
    hash.data = digest;
    hash.len = digestLen;
  
    s = PK11_Sign(privKey, &sig, &hash);
  
    PK11_FreeSlot(slot);
    SECKEY_DestroyPrivateKey(privKey);
    return S_OK;
}
HRESULT 
NSSManager::GetKeyCertNicknames( const CoolKey *aKey,  vector<string> & aStrings  )
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertNickNames \n",GetTStamp(tBuff,56)));

    if(!aKey )
    {
        return E_FAIL;
    }

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);

    if (!slot)
    {
        return E_FAIL;
    }

    CERTCertList *certs = PK11_ListCerts(PK11CertListAll,NULL);

    if (!certs)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetCertNicknames no certs found! \n",GetTStamp(tBuff,56)));
        PK11_FreeSlot(slot);
        return E_FAIL;
    }
    CERTCertListNode *node= NULL;
    for( node = CERT_LIST_HEAD(certs);
             ! CERT_LIST_END(node, certs);
             node = CERT_LIST_NEXT(node))
    {
        if(node->cert)
        {
            CERTCertificate *cert = node->cert;
            if(cert)
            {
                if(cert->slot != slot)
                {
                    CERT_RemoveCertListNode(node);
                }
            }
        }
    }

    if (!certs)
    {
        PK11_FreeSlot(slot);
        return E_FAIL;
    }

    CERTCertNicknames *nicknames =
    CERT_NicknameStringsFromCertList(certs,
                                     NICKNAME_EXPIRED_STRING,
                                     NICKNAME_NOT_YET_VALID_STRING);

    char *curName = NULL;

    if(nicknames)
    {
        int num = nicknames->numnicknames;

        for(int i = 0; i < num ; i ++)
        {
            curName = nicknames->nicknames[i];

            PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetCertKeyNicknames name %s \n",GetTStamp(tBuff,56),curName));

            string str = curName;
            aStrings.push_back (str);
        } 

        CERT_FreeNicknames(nicknames);
    }

    if(certs)
        CERT_DestroyCertList(certs);

    if(slot)
        PK11_FreeSlot(slot);
    
    return S_OK;
}

HRESULT NSSManager::GetKeyIssuer(const CoolKey *aKey, char *aBuf, int aBufLength)
{
    char tBuff[56];
    if(!aBuf)
        return E_FAIL;

    aBuf[0]=0;

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyIssuedTo \n",GetTStamp(tBuff,56)));

    if(!aKey )
        return E_FAIL;

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);

    if (!slot)
        return E_FAIL;

    CERTCertList *certs = PK11_ListCerts(PK11CertListAll,NULL);

    if (!certs)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%sNSSManager::GetKeyIssuer no certs found! \n",GetTStamp(tBuff,56)));
        PK11_FreeSlot(slot);
        return E_FAIL;
    }

    CERTCertListNode *node= NULL;

    char *orgID = NULL;

    for( node = CERT_LIST_HEAD(certs);
             ! CERT_LIST_END(node, certs);
             node = CERT_LIST_NEXT(node))
    {
        if(node->cert)
        {
            CERTCertificate *cert = node->cert;

            if(cert)
            {
                if(cert->slot == slot)
                {
                    if(IsCACert(cert))
                    {
                        continue;
                    }
                    orgID    = CERT_GetOrgName(&cert->subject);
                    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyIssuedTo ourSlot %p curSlot  %p org %s \n",GetTStamp(tBuff,56),slot,cert->slot,orgID));

                }

                if(orgID)
                    break;
            }
        }

    }

    if(orgID && ((int)strlen(orgID)  <  aBufLength))
    {
        strcpy(aBuf,orgID);
    }

    if(certs)
      CERT_DestroyCertList(certs);

    if(slot)
      PK11_FreeSlot(slot);

    if(orgID)
        PORT_Free(orgID);

    return S_OK;
}

HRESULT NSSManager::GetKeyIssuedTo(const CoolKey *aKey, char *aBuf, int aBufLength)
{
    char tBuff[56];
    if(!aBuf)
        return E_FAIL;

    aBuf[0]=0;

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyIssuedTo \n",GetTStamp(tBuff,56)));

    if(!aKey )
        return E_FAIL;

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);

    if (!slot)
        return E_FAIL;

    CERTCertList *certs = PK11_ListCerts(PK11CertListAll,NULL);

    if (!certs)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%sNSSManager::GetKeyIssuedTo no certs found! \n",GetTStamp(tBuff,56)));
        PK11_FreeSlot(slot);
        return E_FAIL;
    }

    CERTCertListNode *node= NULL;

    char *certID = NULL;

    for( node = CERT_LIST_HEAD(certs);
             ! CERT_LIST_END(node, certs);
             node = CERT_LIST_NEXT(node))     
    {     
        if(node->cert) 
        {
            CERTCertificate *cert = node->cert;

            if(cert)
            {
                if(cert->slot == slot)
                {
                    if(IsCACert(cert))
                    {
                        continue;
                    }

                    certID = CERT_GetCommonName(&cert->subject);

                    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyIssuedTo ourSlot %p curSlot  %p certID %s  \n",GetTStamp(tBuff,56),slot,cert->slot,certID));

                }

                if(certID)
                    break;
            }
        }

    }

    if(certID && ((int)strlen(certID)  <  aBufLength))
    {
        strcpy(aBuf,certID);
    }

    if(certs)
      CERT_DestroyCertList(certs);

    if(slot)
      PK11_FreeSlot(slot);

    if(certID)
        PORT_Free(certID);

    return S_OK;
}

HRESULT NSSManager::GetKeyCertInfo(const CoolKey *aKey, char *aCertNickname, string & aCertInfo)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo Nickname %s \n",GetTStamp(tBuff,56),aCertNickname));

    aCertInfo = "";

    if(!aKey )
    {
        return E_FAIL;
    }

    if(!aCertNickname)
    {
        return E_FAIL;
    }

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);

    if (!slot)
    {
        return E_FAIL;
    }

    CERTCertList *certs = PK11_ListCerts(PK11CertListAll,NULL);

    if (!certs)
    {
        if(slot)
            PK11_FreeSlot(slot);
        return E_FAIL;
    }

    PR_LOG(coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo About to get CertList for slot. \n",GetTStamp(tBuff,56)));

    CERTCertListNode *node= NULL;
    for( node = CERT_LIST_HEAD(certs);
             ! CERT_LIST_END(node, certs);
             node = CERT_LIST_NEXT(node))
    {
        if(node->cert)
        {
            CERTCertificate *cert = node->cert;
            if(cert)
            {
                if(cert->slot == slot)
                {
                    if(!strcmp(cert->nickname,aCertNickname))
                    {
                        PR_LOG(coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo We have a matching cert to our slot. nickname %s \n",GetTStamp(tBuff,56),cert->nickname));

                        char *issuerCN   = NULL;
                        char *issuedToCN = NULL;
                       
                        aCertInfo = (char *) "";
                        issuedToCN = cert->subjectName;
                        issuerCN   = cert->issuerName;
                        
                        string issuerCNStr =  "";
                        if(issuerCN)
                            issuerCNStr = issuerCN;

                        string issuedToCNStr = "" ;
                        if(issuedToCN)
                           issuedToCNStr = issuedToCN;

                        string notBeforeStr = "";
                        string notAfterStr  = "";

                        char *nBefore = (char *) DER_UTCTimeToAscii(&cert->validity.notBefore);
                        char  *nAfter  = (char *) DER_UTCTimeToAscii(&cert->validity.notAfter);
 
                        if(nBefore)
                            notBeforeStr = nBefore;
                        if(nAfter)
                            notAfterStr  = nAfter;

                        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo notBefore %s notAfter %s \n",GetTStamp(tBuff,56),nBefore, nAfter));

                        int serialNumber = DER_GetInteger(&cert->serialNumber);

                        std::ostringstream o;
                        string serialStr = "";
                        if (o << serialNumber)
                            serialStr = o.str();                        

                        aCertInfo = issuedToCNStr + "\n" + issuerCNStr + "\n"
                            + notBeforeStr + "\n" + notAfterStr + "\n" + serialStr ;
                        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo issuerCN %s issuedToCN %s \n",GetTStamp(tBuff,56),issuerCN, issuedToCN)); 

                        break;
                    }               
                }
            }
        }
    }

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyCertInfo info: %s \n",GetTStamp(tBuff,56),aCertInfo.c_str())); 

    if(certs)
        CERT_DestroyCertList(certs);

    if(slot)
        PK11_FreeSlot(slot);

    return S_OK;
}

HRESULT
NSSManager::GetKeyPolicy(const CoolKey *aKey, char *aBuf, int aBufLength)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::GetKeyPolicy \n",GetTStamp(tBuff,56)));
    aBuf[0] = '\0';
    char* carot = aBuf;

    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
 
    if (!slot)
        return E_FAIL;
 
    CERTCertListNode *node;
    CERTCertList *certs = PK11_ListCertsInSlot(slot);
    if (!certs)
        return E_FAIL;;

    for (node = CERT_LIST_HEAD(certs); !CERT_LIST_END(node,certs); node = CERT_LIST_NEXT(node)) {
        SECItem policyItem;
        policyItem.data = 0;
   
        SECStatus s = CERT_FindCertExtension(node->cert, SEC_OID_X509_CERTIFICATE_POLICIES, &policyItem);

        if (s != SECSuccess || !policyItem.data)
            continue;

        CERTCertificatePolicies *policies = CERT_DecodeCertificatePoliciesExtension(&policyItem);

        if (!policies) {
            PORT_Free(policyItem.data);
            continue;
        }

        CERTPolicyInfo **policyInfos = policies->policyInfos;

        while (*policyInfos) {
            char *policyID = CERT_GetOidString(&(*policyInfos)->policyID);
            int policyLen = (int)strlen(policyID);

      // check to see if there is space.  we substract an extra -1 for the comma
            if (aBufLength - policyLen - 1 >=0) {

        // if this policy ID isn't in the buffer, add it.

                if (!strstr(aBuf, policyID)) {

          // assuming that this isn't the start, add our delimiter
                    if (carot != aBuf)
                        strcat(carot++, ",");

          // Add the policy id
                    strcat(carot, policyID);
                    carot += policyLen;

          // decrement the length of the availability space in the |in| buffer.
                    aBufLength -= (policyLen + 1);
                }
            }
          policyInfos++;
          PR_smprintf_free(policyID);
        }

      PORT_Free(policyItem.data);
      CERT_DestroyCertificatePoliciesExtension(policies);
    }

    CERT_DestroyCertList(certs);
    PK11_FreeSlot(slot);
    return S_OK;
}

bool 
NSSManager::RequiresAuthentication(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::RequiresAuthentication \n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return false;
  
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
  
    if (!slot)
        return false;
  
    bool needsLogin = false;
  
    if (PK11_IsPresent(slot))
        needsLogin = PK11_NeedLogin(slot) ? true : false;
  
    PK11_FreeSlot(slot);
  
    return needsLogin;
}

bool 
NSSManager::IsAuthenticated(const CoolKey *aKey)
{
    char tBuff[56];
    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%s NSSManager::IsAuthenticated \n",GetTStamp(tBuff,56)));
    if (!aKey || !aKey->mKeyID)
        return false;
  
    PK11SlotInfo *slot = GetSlotForKeyID(aKey);
  
    if (!slot)
        return false;
  
    bool isAuthenticated = false;
  
    if (PK11_IsPresent(slot))
        isAuthenticated = PK11_IsLoggedIn(slot, NULL) ? true : false;
  
    PK11_FreeSlot(slot);
  
    return isAuthenticated;
}

bool 
NSSManager::IsCACert(CERTCertificate *cert)
{
    char tBuff[56];
    bool isCA = false;

    if(!cert)
        return isCA;

    SECItem basicItem;
    basicItem.data = 0;
    
    SECStatus s = CERT_FindCertExtension(cert, SEC_OID_X509_BASIC_CONSTRAINTS, &basicItem);

    if(s != SECSuccess || !basicItem.data)
        return isCA;

    CERTBasicConstraints basic_value;
   
    s = CERT_DecodeBasicConstraintValue(&basic_value,&basicItem); 

    if(s != SECSuccess)
       return isCA;

    PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%sNSSManager::GetKeyIssuedTo isCA %d  \n",GetTStamp(tBuff,56),basic_value.isCA));

    if(basic_value.isCA)
    {
        PR_LOG( coolKeyLogNSS, PR_LOG_DEBUG, ("%sNSSManager::GetKeyIssuedTo found a CA cert , skipping! \n",GetTStamp(tBuff,56)));
        isCA = true;
    }

    PORT_Free(basicItem.data);
    basicItem.data = NULL;

    return isCA;
}
