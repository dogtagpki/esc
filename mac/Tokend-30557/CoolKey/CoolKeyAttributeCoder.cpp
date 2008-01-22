/*CoolKey
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey AttributeCoder implementation.
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
 *  CoolKeyAttributeCoder.cpp
 *  Tokend CoolKey
 */
#include "CoolKeyAttributeCoder.h"

#include "Adornment.h"
#include "MetaAttribute.h"
#include "MetaRecord.h"
#include "Attribute.h"
#include <security_utilities/logging.h>
#include "CoolKeyRecord.h"
#include "CoolKeyToken.h"
#include "CoolKeyPK11.h"
#include <Security/cssmtype.h>
#include <Security/SecKeychainItem.h>
#include <security_cdsa_utilities/cssmkey.h>
#include <Security/SecCertificate.h>
#include <Security/SecKey.h>

using namespace Tokend;


//
// CoolKeyDataAttributeCoder
//
CoolKeyDataAttributeCoder::~CoolKeyDataAttributeCoder()
{
}

void CoolKeyDataAttributeCoder::decode(TokenContext *tokenContext,
	const MetaAttribute &metaAttribute, Record &record)
{
   Syslog::notice("CoolKeyDataAttributeCoder::decode");
}

CoolKeyCertAttributeCoder:: ~CoolKeyCertAttributeCoder()
{
}


void CoolKeyCertAttributeCoder::decode(Tokend::TokenContext *tokenContext,
                const Tokend::MetaAttribute &metaAttribute, Tokend::Record &record)
{

    uint32 id = metaAttribute.attributeId();

    MetaAttribute::Format format = metaAttribute.attributeFormat();

    CoolKeyToken *token = (CoolKeyToken *) tokenContext;

    if(!token)
        return;

    CoolKeyRecord &coolRec = dynamic_cast<CoolKeyRecord &> (record);

    CoolKeyCertObject *cert = (CoolKeyCertObject *) coolRec.getCoolKeyObject();

    Syslog::notice("CertAttributeCoder::decode coder %p cert object %p id %lu format %lu record %p",this,cert,id,format,&record);

    if(!cert)
        return;


    CK_BYTE tData[2048];
    CK_ULONG dataLen = 2048;

    CK_ULONG type = 0;

    switch(id)
    {
        case kSecAlias:
            Syslog::notice("kSecAlias");

            cert->getLabel(tData,&dataLen);

            record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
        break;

        case kSecSubjectItemAttr:
           cert->getSubject(tData,&dataLen);

           Syslog::notice("kSecSubjectItemAttr retrieved data %p datalen %lu",tData,dataLen);

           record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
        break;
 
        case kSecIssuerItemAttr:
           cert->getIssuer(tData,&dataLen);

           Syslog::notice("kSecIssuertItemAttr retrieved data %p datalen %lu",tData,dataLen);

           record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
        break;

        case kSecSerialNumberItemAttr:
           cert->getSerialNo(tData,&dataLen);

           Syslog::notice("kSecSerialNumnberItemAttr retrieved data %p datalen %lu",tData,dataLen);

           record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
        break;

        case kSecPublicKeyHashItemAttr:
            Syslog::notice("kSecPublicKeyHashItemAttr");

            getCertAttributeFromData(cert,kSecPublicKeyHashItemAttr, tData, &dataLen);

            record.attributeAtIndex(metaAttribute.attributeIndex(),
                new Attribute((const void *)tData, dataLen));
        break;

        case kSecSubjectKeyIdentifierItemAttr:
            Syslog::notice("kSecSubjectKeyIdentifierItemAttr");
        break;

        case kSecCertTypeItemAttr:
            type = cert->getType();

            Syslog::notice("kSecCertTypeItemAttr type %lu",type);

            if(type == CKC_X_509)
               type  = CSSM_CERT_X_509v3;
            else
               if(type == CKC_X_509_ATTR_CERT)
                    type = CSSM_CERT_X_509_ATTRIBUTE;
            else
                type = CSSM_CERT_UNKNOWN;

            Syslog::notice("kSecCertTypeItemAttr final type %lu",type);
            record.attributeAtIndex(metaAttribute.attributeIndex(),new Attribute((uint32)type));
        break;

        case kSecCertEncodingItemAttr:
            Syslog::notice("kSecCertEncodingItemAttr");

            type =  CSSM_CERT_ENCODING_BER;

            record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)type));
        break;

        case kSecLabelItemAttr:
            cert->getLabel(tData,&dataLen);

            Syslog::notice("kSecLabelItemAttr retrieved data %p datalen %lu",tData,dataLen);

            record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
     break;

     default:
           Syslog::notice("missed one");
     break;
    };

}

void CoolKeyCertAttributeCoder::getCertAttributeFromData(CoolKeyCertObject *aCert,CK_ULONG aAttribute, CK_BYTE *aData, CK_ULONG *aDataLen)
{

    CSSM_DATA csData;

    CK_BYTE certData[2048];
    CK_ULONG certDataLen = 2048;

    OSStatus status = 0;

    if(!aCert || !aData || *aDataLen <= 0)
        return;

    CK_ULONG size_in = *aDataLen;

    *aDataLen = 0;

    Syslog::notice("CoolKeyCertAttributeCoder::getCertAttributeFromData");

     aCert->getData(certData,&certDataLen);

     SecCertificateRef theCertificate;

    csData.Data = certData;
    csData.Length = certDataLen;

     status = SecCertificateCreateFromData((CSSM_DATA * )&csData, CSSM_CERT_X_509v3,
                CSSM_CERT_ENCODING_BER, &theCertificate);
     
     if(status)
         return;

     Syslog::notice("CoolKeyCertAttributeCoder::getCertAttributeFromData done created cert");
     SecKeychainAttribute ska = { kSecPublicKeyHashItemAttr };

     SecKeychainItemRef tRef = (SecKeychainItemRef) theCertificate;
     SecKeychainAttributeList skal = { 1, &ska };
     status = SecKeychainItemCopyContent(tRef, NULL, &skal,
                NULL, NULL);

     Syslog::notice("CoolKeyCertAttributeCoder::getCertAttributeFromData done got attribute");

     if(!status)
         return;

     if(ska.length < size_in)
     {
         memcpy(aData,ska.data,ska.length);
         *aDataLen = ska.length;
     }

     SecKeychainItemFreeContent(&skal, NULL);
}


CoolKeyKeyAttributeCoder:: ~CoolKeyKeyAttributeCoder()
{
}


void CoolKeyKeyAttributeCoder::decode(Tokend::TokenContext *tokenContext,
                const Tokend::MetaAttribute &metaAttribute, Tokend::Record &record)
{

    Syslog::notice("CoolKeyKeyAttributeCoder::decode"); 

    uint32 id = metaAttribute.attributeId();

    MetaAttribute::Format format = metaAttribute.attributeFormat();

    CoolKeyRecord &coolRec = dynamic_cast<CoolKeyRecord &> (record);

    CoolKeyKeyObject *key = (CoolKeyKeyObject *) coolRec.getCoolKeyObject();

    if(!key)
        return;

    CK_BYTE tData[2048];   
    CK_ULONG dataLen = 2048; 
    CK_ULONG value = 0;

    CK_BYTE  attrib = 0;

    Syslog::notice("CoolKeyKeyAttributeCoder::decode coder %p id %d format %d record %p",this,id,format,&record);
    switch(id)
    {
         case  kSecKeyKeyClass:
             Syslog::notice("kSecKeyKeyClass");
         break;

         case  kSecKeyPrintName: 

            Syslog::notice("kSecKeyPrintName");

            key->getLabel(tData,&dataLen);

           record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
         break;

         case  kSecKeyAlias:
            Syslog::notice("kSecKeyAlias"); 
         break;

         case kSecKeyPermanent:
             Syslog::notice("kSecKeyPermanent"); 
         break;

         case kSecKeyPrivate:
             Syslog::notice("kSecKeyKeyPrivate");
             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)1));
         break;

         case  kSecKeyModifiable:
             Syslog::notice("kSecKeyKeyModifiable");
             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)0));
         break;

         case  kSecKeyApplicationTag:
            Syslog::notice("kSecKeyApplicationTag");
         break;

         case  kSecKeyKeyCreator:
            Syslog::notice("kSecKeyKeyCreator");
         break;

         case  kSecKeyKeyType:
             Syslog::notice("kSecKeyType");
             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)CSSM_ALGID_RSA));
         break;
            
         case  kSecKeyKeySizeInBits:
             Syslog::notice("kSecKeyKeySizeInBits");

             value = key->getKeySize();

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)value));

             Syslog::notice("kSecKeyKeySizeInBits %d",value);
         break;

         case  kSecKeyEffectiveKeySize:
            Syslog::notice("kSecKeyEffectiveKeySize");
 
             value =  key->getKeySize();;
 
             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)value));

             Syslog::notice("kSecKeyEffectiveKeySizeInBits %d",value);
         break;

         case  kSecKeyStartDate:
             Syslog::notice("kSecKeyKeyStartDate");
         break;

         case  kSecKeyEndDate:
             Syslog::notice("kSecKeyKeyEndDate");
         break;

         case  kSecKeySensitive:
             attrib = key->getSensitive();

             Syslog::notice("kSecKeySensitive %d",attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyAlwaysSensitive:
             attrib = key->getAlwaysSensitive();

             Syslog::notice("kSecKeyAlwaysSensitive %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyExtractable:
             Syslog::notice("kSecKeyExtractable");

             attrib = key->getKeyExtractable();

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case kSecKeyNeverExtractable:
             Syslog::notice("kSecKeyNeverExtractable");

             attrib = key->getKeyNeverExtractable();

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyEncrypt:
             Syslog::notice("kSecKeyKeyEncrypt");

             attrib = key->getKeyEncrypt();

             Syslog::notice("kSecKeyEncrypt value %d",attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyDecrypt:
             attrib = key->getKeyDecrypt();

             Syslog::notice("kSecKeyDecrypt value %d",attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyDerive:
             attrib = key->getKeyDerive();

             Syslog::notice("kSecKeyKeyDerive %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case kSecKeySign:
             attrib = key->getKeySign();

             Syslog::notice("kSecKeyKeySign value %d",attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case kSecKeyVerify:
             attrib = key->getKeyVerify();

             Syslog::notice("kSecKeyKeyVerify value %d",attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeySignRecover:
             attrib = key->getKeySignRecover();

             Syslog::notice("kSecKeyKeySignRecover %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyVerifyRecover:
             attrib = key->getKeyVerifyRecover();

             Syslog::notice("kSecKeyKeyVerifyRecover %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyWrap:
             attrib = key->getKeyWrap();

             Syslog::notice("kSecKeyKeyWrap %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case  kSecKeyUnwrap:
             attrib = key->getKeyUnwrap();

             Syslog::notice("kSecKeyKeyUnwrap %d", attrib);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((uint32)attrib));
         break;

         case kSecKeyLabel:
             Syslog::notice("kSecKeyLabel");

             key->getLabel(tData,&dataLen);

             record.attributeAtIndex(metaAttribute.attributeIndex(), new Attribute((const void *)tData,dataLen));
         break;

    };

}

/* arch-tag: 36510900-0DBC-11D9-9CFC-000A9595DEEE */
