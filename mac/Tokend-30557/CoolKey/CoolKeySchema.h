/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Schema implementation.
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
 *  CoolKeySchema.h
 *  TokendMuscle
 */

#ifndef _COOLKEYCHEMA_H_
#define _COOLKEYSCHEMA_H_

#include "Schema.h"
#include "CoolKeyAttributeCoder.h"
#include "CoolKeyHandle.h"

namespace Tokend
{
	class Relation;
	class MetaRecord;
	class AttributeCoder;
}

class CoolKeySchema : public Tokend::Schema
{
	NOCOPY(CoolKeySchema)
public:
    CoolKeySchema();
    virtual ~CoolKeySchema();

	virtual void create();

protected:
	Tokend::Relation *createKeyRelation(CSSM_DB_RECORDTYPE keyType);
        Tokend::Relation *createCertRelation(CSSM_DB_RECORDTYPE certType);

private:
	// Coders we need.
	CoolKeyDataAttributeCoder mCoolKeyDataAttributeCoder;

        CoolKeyCertAttributeCoder mCoolKeyCertAttributeCoder;
        CoolKeyKeyAttributeCoder  mCoolKeyKeyAttributeCoder;

	CoolKeyKeyHandleFactory mCoolKeyKeyHandleFactory;
};

#endif /* !_CACSCHEMA_H_ */

/* arch-tag: 36DB400E-0DBC-11D9-A9F5-000A9595DEEE */
