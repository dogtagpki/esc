/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Schema implementation.
 * 
 *  @APPLE_LICENSE_HEADER_START@
 *  CoolKey
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
 *  CoolKeySchema.cpp
 *  Tokend CoolKey
 */

#include "CoolKeySchema.h"

#include "MetaAttribute.h"
#include "MetaRecord.h"

#include <Security/SecCertificate.h>
#include <Security/SecKeychainItem.h>
#include <Security/SecKey.h>
#include <Security/SecKeychainItemPriv.h>
#include <security_utilities/logging.h>

#include <Security/cssmapple.h>

using namespace Tokend;

CoolKeySchema::CoolKeySchema() 
{
}

CoolKeySchema::~CoolKeySchema()
{
}

Tokend::Relation *CoolKeySchema::createKeyRelation(CSSM_DB_RECORDTYPE keyType)
{
    Relation *rn = createStandardRelation(keyType);

    Syslog::info("createKeyRelation coder %p",&mCoolKeyKeyAttributeCoder);
    // Set up coders for key records.
    MetaRecord &mr = rn->metaRecord();
    mr.keyHandleFactory(&mCoolKeyKeyHandleFactory);

    mr.attributeCoder(kSecKeyPrintName, &mCoolKeyKeyAttributeCoder);

    // Other key valuess
    mr.attributeCoder(kSecKeyKeyType, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyKeySizeInBits, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyEffectiveKeySize, &mCoolKeyKeyAttributeCoder);

    // Key attributes
    mr.attributeCoder(kSecKeyExtractable, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeySensitive, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyModifiable, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyPrivate, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyNeverExtractable, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyAlwaysSensitive, &mCoolKeyKeyAttributeCoder);

    // Key usage
    mr.attributeCoder(kSecKeyEncrypt, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyDecrypt, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyWrap, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyUnwrap,&mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyVerify, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyDerive, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeySignRecover, &mCoolKeyKeyAttributeCoder);
    mr.attributeCoder(kSecKeyVerifyRecover, &mCoolKeyKeyAttributeCoder);

    mr.attributeCoder(kSecKeyLabel, &mPublicKeyHashCoder);
    mr.attributeCoder(kSecKeySign, &mCoolKeyKeyAttributeCoder);

    return rn;
}

Tokend::Relation *CoolKeySchema::createCertRelation(CSSM_DB_RECORDTYPE certType)
{
    Relation *rn = createStandardRelation(certType);

    // Set up coders for key records.
    MetaRecord &mr = rn->metaRecord();

    Syslog::info("createCertRelation coder %p",&mCoolKeyCertAttributeCoder);
    // cert attributes

    mr.attributeCoder(kSecAlias,&mCoolKeyCertAttributeCoder);

   mr.attributeCoder(kSecSubjectItemAttr, &mCoolKeyCertAttributeCoder);

   mr.attributeCoder(kSecLabelItemAttr,&mCoolKeyCertAttributeCoder);

   mr.attributeCoder(kSecIssuerItemAttr, &mCoolKeyCertAttributeCoder);
   mr.attributeCoder(kSecSerialNumberItemAttr, &mCoolKeyCertAttributeCoder);
   mr.attributeCoder(kSecPublicKeyHashItemAttr, &mCoolKeyCertAttributeCoder);

   mr.attributeCoder(kSecSubjectKeyIdentifierItemAttr, &mCoolKeyCertAttributeCoder);
   mr.attributeCoder(kSecCertTypeItemAttr, &mCoolKeyCertAttributeCoder);
   mr.attributeCoder(kSecCertEncodingItemAttr, &mCoolKeyCertAttributeCoder);

   return rn;
}

void CoolKeySchema::create()
{
    Schema::create();

    createStandardRelation(CSSM_DL_DB_RECORD_X509_CERTIFICATE);
    createKeyRelation(CSSM_DL_DB_RECORD_PRIVATE_KEY);
    Relation *rn_publ = createKeyRelation(CSSM_DL_DB_RECORD_PUBLIC_KEY);
    // @@@ We need a coder that calculates the public key hash of a public key
    rn_publ->metaRecord().attributeCoder(kSecKeyLabel, &mPublicKeyHashCoder);
}

/* arch-tag: 36BF1864-0DBC-11D9-8518-000A9595DEEE */
