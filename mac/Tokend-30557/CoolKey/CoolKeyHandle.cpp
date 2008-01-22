/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey KeyHandle implementation.
 * 
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
 *  CoolKeyKeyHandle.cpp
 *  Tokend CoolKey
 */

#include "CoolKeyHandle.h"

#include "CoolKeyRecord.h"
#include "CoolKeyToken.h"

#include <security_utilities/debugging.h>
#include <security_utilities/utilities.h>
#include <security_cdsa_utilities/cssmerrors.h>
#include <security_cdsa_client/aclclient.h>
#include <Security/cssmerr.h>
#include <security_utilities/logging.h>

static const unsigned char sha1sigheader[] =
{
        0x30, // SEQUENCE
        0x21, // LENGTH
        0x30, // SEQUENCE
        0x09, // LENGTH
        0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1a, // SHA1 OID (1 4 14 3 2 26)
        0x05, 0x00, // OPTIONAL ANY algorithm params (NULL)
        0x04, 0x14 // OCTECT STRING (20 bytes)
};

static const unsigned char md5sigheader[] =
{
        0x30, // SEQUENCE
        0x20, // LENGTH
        0x30, // SEQUENCE
        0x0C, // LENGTH
                // MD5 OID (1 2 840 113549 2 5)
        0x06, 0x08, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05,
        0x05, 0x00, // OPTIONAL ANY algorithm params (NULL)
        0x04, 0x10 // OCTECT STRING (16 bytes)
};

using CssmClient::AclFactory;
//
// CoolKeyKeyHandle
//
CoolKeyKeyHandle::CoolKeyKeyHandle(CoolKeyToken &coolToken,
	const Tokend::MetaRecord &metaRecord, CoolKeyRecord &coolKey) :
	Tokend::KeyHandle(metaRecord, &coolKey),
        mToken(coolToken),mRecord(coolKey)
{
}

CoolKeyKeyHandle::~CoolKeyKeyHandle()
{
}

void CoolKeyKeyHandle::getKeySize(CSSM_KEY_SIZE &keySize)
{
    Syslog::notice("CoolKeyHandle::getKeySize");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

uint32 CoolKeyKeyHandle::getOutputSize(const Context &context, uint32 inputSize,
	bool encrypting)
{
    Syslog::notice("CoolKeyHandle::getOutputSize");

    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);

    return 0;
}

void CoolKeyKeyHandle::generateSignature(const Context &context,
	CSSM_ALGORITHMS signOnly, const CssmData &input, CssmData &signature)
{
    Syslog::notice("CoolKeyHandle::generateSignature Input length %d context.type %d context.alg %d",input.length(),context.type(),context.algorithm());
    CoolKeyObject * coolObj = mRecord.getCoolKeyObject();

    if(!coolObj  || coolObj->getClass() != CKO_PRIVATE_KEY)
    {
        Syslog::notice("Can't find object for record %p or incorrect object type", &mRecord);
        CssmError::throwMe(CSSM_ERRCODE_INVALID_DATA);
    }

    CoolKeyKeyObject * keyObj = (CoolKeyKeyObject *) coolObj;

    CK_ULONG keyLength = keyObj->getKeySize() / 8;

    Syslog::notice("keyLength %d",keyLength);

    if (context.type() != CSSM_ALGCLASS_SIGNATURE)
        CssmError::throwMe(CSSMERR_CSP_INVALID_CONTEXT);

    if (context.algorithm() != CSSM_ALGID_RSA)
        CssmError::throwMe(CSSMERR_CSP_INVALID_ALGORITHM);


    // Find out if we are doing a SHA1 or MD5 signature and setup header to
    // point to the right asn1 blob.

    const unsigned char *header = NULL;
    size_t headerLength = 0;
    if (signOnly == CSSM_ALGID_SHA1)
    {
        Syslog::notice("Asking for SHA1");
        header = sha1sigheader;
        headerLength = sizeof(sha1sigheader);

        Syslog::notice("header is sha1sigheader, len %d", headerLength);
        if (input.Length != 20)
            CssmError::throwMe(CSSMERR_CSP_BLOCK_SIZE_MISMATCH);
    }
    else if (signOnly == CSSM_ALGID_MD5)
    {
        Syslog::notice("Asking for MD5");
        header = md5sigheader;
        headerLength = sizeof(md5sigheader);

        Syslog::notice("header is md5sigheader, len %d", headerLength);
        if (input.Length != 16)
            CssmError::throwMe(CSSMERR_CSP_BLOCK_SIZE_MISMATCH);
    }
    else if (signOnly == CSSM_ALGID_NONE)
    {
        header = NULL;
        headerLength = 0;

        Syslog::notice("Asking for CSSM_ALGID_NONE");
        // Special case used by SSL it's an RSA signature, without the ASN1 stuff
    }
    else
        CssmError::throwMe(CSSMERR_CSP_INVALID_DIGEST_ALGORITHM);

    CoolKeyPK11 pk11Manager =  mToken.getPK11Manager();

    int loggedIn = pk11Manager.isTokenLoggedIn();

    if(!loggedIn)
    {
        Syslog::error("Can't sign , not logged in.");
        CssmError::throwMe(CSSM_ERRCODE_OPERATION_AUTH_DENIED);
    }

    signature.Data =( uint8*) malloc(keyLength);

    // Create an input buffer in which we construct the data we will send to
    // the token.
    size_t inputDataSize = headerLength + input.Length;

    Syslog::notice("inputDataSize %d", inputDataSize);

    auto_array<unsigned char> inputData(keyLength);
    unsigned char *to = inputData.get();

     // Get padding, but default to pkcs1 style padding
    uint32 padding = CSSM_PADDING_NONE;
    context.getInt(CSSM_ATTRIBUTE_PADDING, padding);

    Syslog::notice("padding value %d",padding);

    if (padding == CSSM_PADDING_PKCS1)
    {
         Syslog::notice("CSSM_PADDING_PKCS1.");
         // Add PKCS1 style padding
         *(to++) = 0;
         *(to++) = 1; /* Private Key Block Type. */
         size_t padLength = keyLength - 3 - inputDataSize;

         Syslog::notice("padlength %d",padLength);

         memset(to, 0xff, padLength);
         to += padLength;
            *(to++) = 0;
            inputDataSize = keyLength;
    }
    else if (padding == CSSM_PADDING_NONE)
    {
        Syslog::notice("CSSM_PADDING_NONE");

        // Token will fail if the input data isn't exactly keysize / 8 octects
        // long
    }
    else
         CssmError::throwMe(CSSMERR_CSP_INVALID_ATTR_PADDING);

    if (headerLength)
    {
        memcpy(to, header, headerLength);
        to += headerLength;
    }

    // Finally copy the passed in data to the input buffer.
    memcpy(to, input.Data, input.Length);

    if(!signature.Data)
    {
        Syslog::error("Can't allocate memory for signature operation.");
        CssmError::throwMe(CSSM_ERRCODE_INVALID_DATA);
    } 

    signature.Length = (size_t) keyLength;

    int result = pk11Manager.generateSignature(coolObj,inputData.get(),inputDataSize,signature.Data,&signature.Length);

    if(!result)
    {
        Syslog::notice("Problem generating signature");
        if(signature.Data)
            free(signature.Data);
        CssmError::throwMe(CSSMERR_CSP_FUNCTION_FAILED);
    }

    Syslog::notice("generateSignature returned %d data lenght %d", result, signature.Length);    
}

void CoolKeyKeyHandle::verifySignature(const Context &context,
	CSSM_ALGORITHMS signOnly, const CssmData &input, const CssmData &signature)
{
    Syslog::notice("CoolKeyHandle::verifySignature");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

void CoolKeyKeyHandle::generateMac(const Context &context,
	const CssmData &input, CssmData &output)
{
    Syslog::notice("CoolKeyHandle::generateMac");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

void CoolKeyKeyHandle::verifyMac(const Context &context,
	const CssmData &input, const CssmData &compare)
{
    Syslog::notice("CoolKeyHandle::verifyMac");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

void CoolKeyKeyHandle::encrypt(const Context &context,
	const CssmData &clear, CssmData &cipher)
{
    Syslog::notice("CoolKeyHandle::encrypt");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

void CoolKeyKeyHandle::decrypt(const Context &context,
	const CssmData &cipher, CssmData &clear)
{

    Syslog::notice("CoolKeyHandle::decrypt type %d alg %d length %d",context.type(), context.algorithm(),cipher.length());

    CoolKeyObject * coolObj = mRecord.getCoolKeyObject();

    if(!coolObj || coolObj->getClass() != CKO_PRIVATE_KEY )
    {
        Syslog::notice("Can't find object for record or incorrect object %p", &mRecord);
        CssmError::throwMe(CSSM_ERRCODE_INVALID_DATA);
    }

    CoolKeyKeyObject * keyObj = (CoolKeyKeyObject *) coolObj;

    CK_ULONG keyLength = keyObj->getKeySize() / 8;


    Syslog::notice("keyLength %d",keyLength);

    if (context.type() != CSSM_ALGCLASS_ASYMMETRIC)
    {
        Syslog::error("In decrypt wrong key type, not asymmetric");
        CssmError::throwMe(CSSMERR_CSP_INVALID_CONTEXT);

    }

    if (context.algorithm() != CSSM_ALGID_RSA)
    {
        Syslog::error("In decrypt wrong algorithm, not RSA");
        CssmError::throwMe(CSSMERR_CSP_INVALID_ALGORITHM);
    }

    CoolKeyPK11 pk11Manager =  mToken.getPK11Manager();

    int loggedIn = pk11Manager.isTokenLoggedIn();

    if(!loggedIn)
    {
        Syslog::error("Can't decrypt , not logged in.");
        CssmError::throwMe(CSSM_ERRCODE_OPERATION_AUTH_DENIED);
    }

    clear.Data = (uint8 *) malloc((size_t) keyLength);
    clear.Length = keyLength;

    if(!clear.Data)
    {
        Syslog::error("Can't allocate data for decrype operation.");
        CssmError::throwMe(CSSM_ERRCODE_INVALID_DATA);
    }

    int result = pk11Manager.decryptData(coolObj,cipher.Data,cipher.Length,clear.Data,&clear.Length);

    if(!result)
    {
        Syslog::error("Problem with decrypt operation");

        if(clear.Data)
        {
            free(clear.Data);
        }

        CssmError::throwMe(CSSMERR_CSP_FUNCTION_FAILED);
    }
    Syslog::notice("decryptData returned %d data lenght %d", result, clear.Length);

}

void CoolKeyKeyHandle::exportKey(const Context &context,
	const AccessCredentials *cred, CssmKey &wrappedKey)
{
    Syslog::notice("CoolKeyHandle::exportKey");
    CssmError::throwMe(CSSM_ERRCODE_FUNCTION_NOT_IMPLEMENTED);
}

void CoolKeyKeyHandle::getOwner(AclOwnerPrototype &owner)
{
    Syslog::notice("CoolKeyKeyHandle::getOwner");
    if (!mAclOwner) {
        Allocator &alloc = Allocator::standard();

        mAclOwner.allocator(alloc);

        mAclOwner = AclFactory::AnySubject(alloc);
    }
    owner = mAclOwner;

}

void CoolKeyKeyHandle::getAcl(const char *tag, uint32 &count, AclEntryInfo *&aclList)
{
    Syslog::notice("CoolKeyKeyHandle::getAcl tag %s",tag);

    // we don't (yet) support queries by tag
    if (tag)
        CssmError::throwMe(CSSM_ERRCODE_INVALID_ACL_ENTRY_TAG);

    if(!mAclEntries) {
        mAclEntries.allocator(Allocator::standard());
        // Anyone can read the DB record for this key (which is a reference
                // CSSM_KEY)
        mAclEntries.add(CssmClient::AclFactory::AnySubject(
                        mAclEntries.allocator()),
                        AclAuthorizationSet(CSSM_ACL_AUTHORIZATION_DB_READ, 0));
                
       CssmData prompt;

       CoolKeyPK11 pk11Manager =  mToken.getPK11Manager();

       int loggedIn = pk11Manager.isTokenLoggedIn();

       if(!loggedIn)
       {
           Syslog::notice("CoolKeyKeyHandle:getAcl token NOT logged in already");

           mAclEntries.add(CssmClient::AclFactory::PromptPWSubject(
                        mAclEntries.allocator(), prompt),
                        AclAuthorizationSet(
                                 CSSM_ACL_AUTHORIZATION_SIGN
                                , CSSM_ACL_AUTHORIZATION_DECRYPT,CSSM_ACL_AUTHORIZATION_ENCRYPT, 0));
           }
           else
           {
               Syslog::notice("CoolKeyKeyHandle:getAcl token logged in already"); 
               mAclEntries.add(CssmClient::AclFactory::AnySubject(
               mAclEntries.allocator()), 
               AclAuthorizationSet(
                   CSSM_ACL_AUTHORIZATION_SIGN
                   , CSSM_ACL_AUTHORIZATION_DECRYPT,CSSM_ACL_AUTHORIZATION_ENCRYPT, 0));
                   
           }
    }

    count = mAclEntries.size();
    aclList = mAclEntries.entries();
}



//
// CoolKeyKeyHandleFactory
//
CoolKeyKeyHandleFactory::~CoolKeyKeyHandleFactory()
{
}


Tokend::KeyHandle *CoolKeyKeyHandleFactory::keyHandle(
	Tokend::TokenContext *tokenContext, const Tokend::MetaRecord &metaRecord,
	Tokend::Record &record) const
{
    Syslog::notice("CoolKeyKeyHandleFactory::keyHandle record %p ",&record);

    CoolKeyToken &theToken = static_cast<CoolKeyToken& >(*tokenContext);
    CoolKeyRecord &keyRecord = dynamic_cast<CoolKeyRecord &>(record);

    return new CoolKeyKeyHandle(theToken, metaRecord, keyRecord);
}


/* arch-tag: 3685A262-0DBC-11D9-BC66-000A9595DEEE */
