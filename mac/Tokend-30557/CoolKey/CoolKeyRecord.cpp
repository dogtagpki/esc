/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Record implementation.
 * 
 *  @APPLE_LICENSE_HEADER_START@
 *  
 *  This file contains Original Code and/or Modifications of Original Code
 *  as defined in and that are subject to the Apple Public Source LicenseCoolKey
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
 *  CoolKeyRecord.cpp
 *  Tokend CoolKey 
 */

#include "CoolKeyPK11.h"
#include "CoolKeyRecord.h"
#include "CoolKeyError.h"
#include "CoolKeyToken.h"
//#include "Attribute.h"
#include "MetaAttribute.h"
#include "MetaRecord.h"
#include <security_cdsa_client/aclclient.h>
#include <Security/SecKey.h>
#include <security_utilities/logging.h>

//
// CoolKeyRecord
//
CoolKeyRecord::~CoolKeyRecord()
{
}

Tokend::Attribute *CoolKeyRecord::getDataAttribute(Tokend::TokenContext *tokenContext)
{
    Syslog::notice("CoolKeyRecord::getDataAttribute");

    CoolKeyObject *obj = (CoolKeyObject *) getCoolKeyObject();

    CK_OBJECT_CLASS theClass = 0;
  
    if(obj)
    { 
        theClass =   obj->getClass(); 
    }

    if(!obj)
        return NULL;

    CK_BYTE tData[2048];
    CK_ULONG dataLen = 2048;

   CoolKeyCertObject *theCert = NULL;
    switch(theClass)
    {
         case CKO_CERTIFICATE: 
             Syslog::notice("getDataAttribute: Found certificate:-----------------");

             theCert =  (CoolKeyCertObject *) obj;

             if(theCert)
             {
                  theCert->getData((CK_BYTE *)tData,&dataLen);
             }

             return new Tokend::Attribute((const void *)tData,dataLen);
         break;
       
         case CKO_PUBLIC_KEY:
              Syslog::notice("getDataAttribute:Found public key:----------------");
         break;
 
         case CKO_PRIVATE_KEY:
             Syslog::notice("getDataAttribute:Found private key:-------------------");
         break;
                
         default:
             Syslog::notice("getDataAttribute:Found something else:");
         break;
    };

    return NULL;
}

void CoolKeyRecord::getAcl(const char *tag, uint32 &count, AclEntryInfo *&acls)
{
    Syslog::notice("CoolKeyRecord::getAcl ----------------");
    if (!mAclEntries) {
            mAclEntries.allocator(Allocator::standard());
    // Anyone can read the DB record for this key (which is a reference
            // CSSM_KEY)
    mAclEntries.add(CssmClient::AclFactory::AnySubject(
        mAclEntries.allocator()),
        AclAuthorizationSet(CSSM_ACL_AUTHORIZATION_DB_READ, 0));
    }

    count = mAclEntries.size();
    acls = mAclEntries.entries();
}

/* arch-tag: 9703BFF8-0E73-11D9-ACDD-000A9595DEEE */
