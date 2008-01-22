/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Record.
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
 *  CoolKeyRecord.h
 *  TokendMuscle
 */

#ifndef _COOLKEYRECORD_H_
#define _COOLKEYRECORD_H_

#include "Record.h"
#include "CoolKeyPK11.h"

//class CoolKeyObject;

class CoolKeyRecord : public Tokend::Record
{
	NOCOPY(CoolKeyRecord)
public:
	CoolKeyRecord(CoolKeyObject *aObject) :
		mObject(aObject) {}

	virtual ~CoolKeyRecord();

         CoolKeyObject *getCoolKeyObject() { return mObject; }

         virtual Tokend::Attribute *getDataAttribute(Tokend::TokenContext *tokenContext);

         virtual void getAcl(const char *tag, uint32 &count,
                AclEntryInfo *&aclList);
private:
        AutoAclEntryInfoList mAclEntries;

protected:

        CoolKeyObject  *mObject;
};



#endif /* !_CACRECORD_H_ */

/* arch-tag: 96BC854C-0E73-11D9-B9B1-000A9595DEEE */

