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
 *  Version 2.0 (the 'License'). You may not use this filCoolKeye except in
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
 *  CoolKeyToken.h
 *  Tokend CoolKey
 */

#ifndef _COOLKEY_TOKEN_H_
#define _COOLKEY_TOKEN_H_

#include "mypkcs11.h"
#include <Token.h>
#include "TokenContext.h"
#include "CoolKeyPK11.h"

#include <security_utilities/pcsc++.h>

class CoolKeySchema;

#define PKCS11_PATH_NAME "/Library/Application Support/CoolKey/PKCS11/libcoolkeypk11.dylib"
#define COOLKEY_PRESENT_SCORE 10000000
//
// "The" token
//
class CoolKeyToken : public Tokend::ISO7816Token
{
	NOCOPY(CoolKeyToken)
public:
	CoolKeyToken() ;
	~CoolKeyToken();

	virtual void didDisconnect();
	virtual void didEnd();

        virtual void initial();
        virtual uint32 probe(SecTokendProbeFlags flags,
		char tokenUid[TOKEND_MAX_UID]);
	virtual void establish(const CSSM_GUID *guid, uint32 subserviceId,
		SecTokendEstablishFlags flags, const char *cacheDirectory,
		const char *workDirectory, char mdsDirectory[PATH_MAX],
		char printName[PATH_MAX]);
	virtual void getOwner(AclOwnerPrototype &owner);
	virtual void getAcl(const char *tag, uint32 &count, AclEntryInfo *&acls);

	virtual void changePIN(int pinNum,
		const unsigned char *oldPin, size_t oldPinLength,
		const unsigned char *newPin, size_t newPinLength);
	virtual uint32_t pinStatus(int pinNum);
	virtual void verifyPIN(int pinNum, const unsigned char *pin, size_t pinLength);
	virtual void unverifyPIN(int pinNum);
        virtual void authenticate(CSSM_DB_ACCESS_TYPE mode, const AccessCredentials *cred);
        virtual bool isLocked();

	bool identify();
	void select(const unsigned char *applet);
	uint32_t exchangeAPDU(const unsigned char *apdu, size_t apduLength,
                          unsigned char *result, size_t &resultLength);

	uint32_t getData(unsigned char *result, size_t &resultLength);

        CoolKeyPK11 &getPK11Manager() { return  mCoolKey; }

protected:
	void populate();

        CoolKeyPK11 mCoolKey; 
public:
	const unsigned char *mCurrentApplet;
	uint32_t mPinStatus;

	// temporary ACL cache hack - to be removed
	AutoAclOwnerPrototype mAclOwner;
	AutoAclEntryInfoList mAclEntries;

private:

        static void cleanup(int aSig);
};


#endif /* !_CACTOKEN_H_ */

/* arch-tag: 3714259E-0DBC-11D9-8D58-000A9595DEEE */
