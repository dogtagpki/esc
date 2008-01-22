/*
 *  Copyright (c) 2004 Apple Computer, ICoolKeync. All Rights Reserved.
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
 *  CoolKeyAttributeCoder.h
 *  Tokend CoolKey
 */

#ifndef _COOLKEY_ATTRIBUTECODER_H_
#define _COOLKEY_ATTRIBUTECODER_H_

#include "AttributeCoder.h"
#include <string>

#include <PCSC/musclecard.h>
#include "CoolKeyPK11.h"

//
// A coder that reads the data of an object
//
class CoolKeyDataAttributeCoder : public Tokend::AttributeCoder
{
	NOCOPY(CoolKeyDataAttributeCoder)
public:

	CoolKeyDataAttributeCoder() {}
	virtual ~CoolKeyDataAttributeCoder();

	virtual void decode(Tokend::TokenContext *tokenContext,
		const Tokend::MetaAttribute &metaAttribute, Tokend::Record &record);
};

class CoolKeyCertAttributeCoder : public Tokend::AttributeCoder
{
        NOCOPY(CoolKeyCertAttributeCoder)
public:

        CoolKeyCertAttributeCoder() {}
        virtual ~CoolKeyCertAttributeCoder();

        virtual void decode(Tokend::TokenContext *tokenContext,
                const Tokend::MetaAttribute &metaAttribute, Tokend::Record &record);

protected:  
        void getCertAttributeFromData(CoolKeyCertObject *aCert,CK_ULONG aAttribute, CK_BYTE *aData, CK_ULONG *aDataLen);
};

class CoolKeyKeyAttributeCoder : public Tokend::AttributeCoder
{
        NOCOPY(CoolKeyKeyAttributeCoder)
public:

        CoolKeyKeyAttributeCoder() {}
        virtual ~CoolKeyKeyAttributeCoder();

        virtual void decode(Tokend::TokenContext *tokenContext,
                const Tokend::MetaAttribute &metaAttribute, Tokend::Record &record);
};


#endif /* !_CoolKeyATTRIBUTECODER_H_ */

/* arch-tag: 366E16D4-0DBC-11D9-B030-000A9595DEEE */
