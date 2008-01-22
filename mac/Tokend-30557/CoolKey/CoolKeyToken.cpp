/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Token implementation.
 * CoolKey
 *  @APPLE_LICENSE_HEADER_START@
 *  
 *  This file contains Original Code and/or Modifications of Original Code
 *  as defined in and that are subject to the Apple Public Source License
 *  Version 2.0 (the 'License'). You may not use this file except in
 *  compliance with the License. Please obtain a copy of the License at
 *  http://www.opensource.apple.com/apsl/ and read it before using this
 *  file.
 *  
 *  The Original Code and all software distributed under the License are
 *  distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 *  EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 *  INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *  Please see the License for the specific language governing rights and
 *  limitations under the License.
 *  
 *  @APPLE_LICENSE_HEADER_END@
 */
/*
 *  CoolKeyToken.cpp
 *  Tokend CoolKey
 */

#include "CoolKeyToken.h"

#include "Adornment.h"
#include "AttributeCoder.h"
#include "CoolKeyError.h"
#include "CoolKeyRecord.h"
#include "CoolKeySchema.h"
#include <security_cdsa_client/aclclient.h>
#include <security_cdsa_utilities/cssmerrors.h>
#include <security_utilities/logging.h>
#include <security_utilities/refcount.h>
#include <map>
#include <vector>
#include <dlfcn.h>
#include <csignal>

using CssmClient::AclFactory;

static CoolKeyPK11 *coolKeyModule = NULL;

CoolKeyToken::CoolKeyToken() :
	mCurrentApplet(NULL),
	mPinStatus(1)
{
    mTokenContext = this;
}

CoolKeyToken::~CoolKeyToken()
{
    Syslog::notice("CoolKeyToken::~CoolKeyToken");
    delete mSchema;
}

// Here is where we initialize our PKCS11 module
void CoolKeyToken::initial()
{
    Syslog::notice("In CoolKeyToken::initial() . " );
}


bool CoolKeyToken::identify()
{
    Syslog::notice("In CoolKeyToken::identify");

    return true;
}

void CoolKeyToken::select(const unsigned char *applet)
{
    Syslog::debug("In CoolKeyToken::select");
}

uint32_t CoolKeyToken::exchangeAPDU(const unsigned char *apdu, size_t apduLength,
	unsigned char *result, size_t &resultLength)
{
    return 0;
}

void CoolKeyToken::didDisconnect()
{
    Syslog::debug("In CoolKeyToken::didDisconnect");
}

void CoolKeyToken::didEnd()
{
    mCurrentApplet = NULL;
}

void CoolKeyToken::changePIN(int pinNum,
	const unsigned char *oldPin, size_t oldPinLength,
	const unsigned char *newPin, size_t newPinLength)
{
    Syslog::debug("In CoolKeyToken::changePIN");
}

uint32_t CoolKeyToken::pinStatus(int pinNum)
{
    Syslog::notice("In CoolKeyToken::pinStatus num %d",pinNum);

    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

bool CoolKeyToken::isLocked()
{
    int result = mCoolKey.isTokenLoggedIn();
    mPinStatus =  1 - result;
    Syslog::notice("In CoolKeyToken::isLocked mPinStatus %d",mPinStatus);
    return mPinStatus;
}


void CoolKeyToken::verifyPIN(int pinNum,
	const unsigned char *pin, size_t pinLength)
{
    Syslog::notice("In CoolKeyToken::verifyPIN");

    int result = 0;

    if(mCoolKey.isTokenLoggedIn())
    {
        result = mCoolKey.verifyCachedPIN((char *) pin);
    }
    else
    {
        result = mCoolKey.loginToken((char *) pin);
    }

    Syslog::notice("Result of loginToken %d",result);
   
    if(result == CKR_OK)
        return; 

    //logout because we failed

    mCoolKey.logoutToken();

    //any other error complain

    CssmError::throwMe(CSSM_ERRCODE_OPERATION_AUTH_DENIED);

}

void CoolKeyToken::unverifyPIN(int pinNum)
{
    Syslog::notice("In CoolKeyToken::unverifyPIN");
    mPinStatus = 1;
}

uint32_t CoolKeyToken::getData(unsigned char *result, size_t &resultLength)
{
    Syslog::notice("In CoolKeyToken::getData");
    return NULL;
}

uint32 CoolKeyToken::probe(SecTokendProbeFlags flags,
	char tokenUid[TOKEND_MAX_UID])
{
    uint32 score =  0; //Tokend::ISO7816Token::probe(flags, tokenUid);

    uint32 max_uid = TOKEND_MAX_UID;

    const SCARD_READERSTATE &readerState = *(*startupReaderInfo)();
      
   Syslog::notice("TOKEND_MAX_UID %d",max_uid); 
   Syslog::notice("READER_STATE -> szReader %s", (char *) readerState.szReader);
   Syslog::notice ("READER_STATE -> dwCurrentState %u",readerState.dwCurrentState);
   Syslog::notice ("READER_STATE -> dwEventState %u",readerState.dwEventState);
   Syslog::notice ("READER_STATE -> cbAtr %u",readerState.cbAtr);
   Syslog::notice("READER_STATE -> rgbAtr %32x",(char *) readerState.rgbAtr);

    int res = mCoolKey.loadModule();
     
    /* if(res)
         res = mCoolKey.loadObjects();
     */


    if(!res || ! mCoolKey.getIsOurToken())
    {
        Syslog::error(" Can't load CoolKey pkcs11 module. ");
        return score;
    }

    if(coolKeyModule == NULL)
       coolKeyModule = &mCoolKey;

    char *tUid = mCoolKey.getTokenId();

    if(tUid)
    {
        sprintf((char *) tokenUid,"%s",(char *)tUid);
        Syslog::notice("tokenUid %s",(char *) tokenUid);
    }

    score = 199;

    signal(SIGTERM, cleanup); // register a SIGTERM handler

    return score;
}

void CoolKeyToken::establish(const CSSM_GUID *guid, uint32 subserviceId,
	SecTokendEstablishFlags flags, const char *cacheDirectory,
	const char *workDirectory, char mdsDirectory[PATH_MAX],
	char printName[PATH_MAX])
{

    char *mCuid = mCoolKey.getTokenId();
      
    int pathSize = PATH_MAX;
 
    if(mCuid)
    {
        Syslog::notice("printName size %d", pathSize);

        int predictedSize = strlen(mCuid);

        if(predictedSize < pathSize)
        {
             sprintf((char *) printName, (char *) "%s",mCuid);
        }
    }

    Syslog::notice("In CoolKeyToken::establish setting printName to: %s subserviceId: %d",printName,subserviceId);

    int res = mCoolKey.loadObjects();

    if(!res)
        return;
    
    mSchema = new CoolKeySchema();
    mSchema->create();

    populate();
}

//
// Authenticate to the token
//

void CoolKeyToken::authenticate(CSSM_DB_ACCESS_TYPE mode, const AccessCredentials *cred)
{
    Syslog::notice("In CoolKeyToken::authenticate cred %p tag %s size %d",cred,cred->tag(),cred->size());

    if (cred) {   

        if(mode == CSSM_DB_ACCESS_RESET)
        {
            Syslog::notice("authenticate CSSM_DB_ACCESS_RESET");
            return;
        }
        const TypedList &sample = (*cred)[0];
        switch (sample.type()) {
            case CSSM_SAMPLE_TYPE_PASSWORD:
            case CSSM_SAMPLE_TYPE_PROMPTED_PASSWORD:
            case CSSM_SAMPLE_TYPE_PROTECTED_PASSWORD:
            {
                 Syslog::notice("sample type %d",sample.type());
                 CssmData &pin = sample[1].data();
                                        
                verifyPIN(1, pin.Data,pin.Length);
            }
            break;
           default:
           Syslog::notice("sample type %ld not supported", sample.type());
           CssmError::throwMe(CSSM_ERRCODE_ACL_SUBJECT_TYPE_NOT_SUPPORTED);
       }
   } else
      Syslog::notice("authenticate without credentials ignored");
}

//
// Database-level ACLs
//
void CoolKeyToken::getOwner(AclOwnerPrototype &owner)
{
        Syslog::notice("In CoolKeyToken::getOwner");
	// we don't really know (right now), so claim we're owned by PIN #0
	if (!mAclOwner)
	{
		mAclOwner.allocator(Allocator::standard());
		mAclOwner = AclFactory::PinSubject(Allocator::standard(), 0);
	}
	owner = mAclOwner;
}


void CoolKeyToken::getAcl(const char *tag, uint32 &count, AclEntryInfo *&acls)
{
    Syslog::notice("In CoolKeyToken::getAcl.");

    Allocator &alloc = Allocator::standard();

    // mAclEntries sets the handle of each AclEntryInfo to the
    // offset in the array.

    if (!mAclEntries) {
        mAclEntries.allocator(alloc);
        // Anyone can read the attributes and data of any record on this token
        // (it's further limited by the object itself).
		mAclEntries.add(CssmClient::AclFactory::AnySubject(
			mAclEntries.allocator()),
			AclAuthorizationSet(CSSM_ACL_AUTHORIZATION_DB_READ, 0));
        // We support PIN1 with either a passed in password
        // subject or a prompted password subject.


          mAclEntries.addPin(AclFactory::PWSubject(alloc),1);
          mAclEntries.addPin(AclFactory::PromptPWSubject(alloc,CssmData()), CssmData());
              
	}

	count = mAclEntries.size();
	acls = mAclEntries.entries();
}


#pragma mark ---------------- CoolKey Specific --------------

void CoolKeyToken::populate()
{
    Syslog::notice("In CoolKeyToken::populate");

    Tokend::Relation &certRelation = mSchema->findRelation(CSSM_DL_DB_RECORD_X509_CERTIFICATE);
    Tokend::Relation &privateKeyRelation = mSchema->findRelation(CSSM_DL_DB_RECORD_PRIVATE_KEY);
    Tokend::Relation &publicKeyRelation = mSchema->findRelation(CSSM_DL_DB_RECORD_PUBLIC_KEY);

    std::map<int, CoolKeyObject *> certs;
    std::map< CoolKeyObject *, RefPointer<CoolKeyRecord> > keys;
    std::map< CoolKeyObject *, RefPointer<CoolKeyRecord> >  certRecs;

    for(CoolKeyPK11::ObjIterator i = mCoolKey.begin(); i != mCoolKey.end() ; i++)
    {
        CoolKeyObject *obj =(*i).second;
        CK_OBJECT_CLASS oClass;
        if(obj)
        {
            CK_BYTE id = obj->getID();
            oClass = obj->getClass();
            Syslog::notice("Retrieved object %p class %lu id %d",obj,oClass,id); 
 
            CoolKeyRecord *newRecord = new CoolKeyRecord(obj); 

            RefPointer<CoolKeyRecord>    theRecord( newRecord);
            if(!theRecord)
                continue;

            switch(oClass)
            {
                case CKO_PRIVATE_KEY:
                    privateKeyRelation.insertRecord(theRecord);
                    Syslog::notice("Inserting private key record %p",newRecord);
                    keys[obj] = theRecord;
                break;

                case CKO_PUBLIC_KEY:
                Syslog::notice("Inserting public key record %p theRefRecord %p",newRecord,theRecord.get());
                             publicKeyRelation.insertRecord(theRecord);
                             keys[obj] = theRecord;
                break;

                case CKO_CERTIFICATE:
                    certs[id] = obj;
                    certRecs[obj] = theRecord;
                    Syslog::notice("Inserting cert record %p",newRecord);
                    certRelation.insertRecord(theRecord);
                break; 

                default:
                break;

             };
         }

    }

    for(CoolKeyPK11::ObjIterator i = mCoolKey.begin(); i != mCoolKey.end() ; i++)
    {
        CoolKeyObject *obj =(*i).second;
        CoolKeyObject *cert = NULL;

        CK_OBJECT_CLASS oClass;
        if(obj)
        {
            CK_BYTE id = obj->getID();
            oClass = obj->getClass();

            switch(oClass)
            {
                case CKO_PRIVATE_KEY:
                case CKO_PUBLIC_KEY:
                    cert = certs[id];
                    if(cert)
                    {
                        RefPointer<CoolKeyRecord>  coolKeyRecRef = keys[obj];
                        CoolKeyRecord *  coolKeyRec = coolKeyRecRef.get();  

                        Syslog::notice("Key %p  linked to cert %p",obj,cert);

                        if(coolKeyRec)
                        {
                            Syslog::notice("Found record to create adornment record: %p",coolKeyRec);
                            if(certRecs[cert])
                            {
                                Tokend::LinkedRecordAdornment * lra = new Tokend::LinkedRecordAdornment(certRecs[cert]);
                                 Syslog::notice("lra %p",lra);

                                 if(lra)
                                 { 
                                     coolKeyRec->setAdornment(mSchema->publicKeyHashCoder().certificateKey(),
                                             lra);
                                     Syslog::notice("certificateKey %p certRecs[cert] %p",mSchema->publicKeyHashCoder().certificateKey(),certRecs[cert].get());
                                 }
                             }
                         }
                     }
                     else
                         Syslog::notice("Key %p not linked to found cert");
                  break;

                  default:
                  break;

             };

         }

    }     

}

void CoolKeyToken::cleanup(int aSig)
{
    Syslog::notice("We are going away!");

    if(coolKeyModule)
    {
        coolKeyModule->logoutToken();
        coolKeyModule->freeModule();
    }

}

/* arch-tag: 36F733B4-0DBC-11D9-914C-000A9595DEEE */
