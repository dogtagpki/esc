/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey Error implementation.

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
 *  CoolKeyError.cpp
 *  TokendMuscle
 */


#include "CoolKeyError.h"
#include <Security/cssmerr.h>

//
// CoolKeyError exceptions
//
CoolKeyError::CoolKeyError(uint16_t sw) : SCardError(sw)
{
	IFDEBUG(debugDiagnose(this));
}

const char *CoolKeyError::what() const throw ()
{ return "CoolKey Error"; }

OSStatus CoolKeyError::osStatus() const
{
    switch (statusWord)
    {
	case COOLKEY_AUTHENTICATION_FAILED_0:
	case COOLKEY_AUTHENTICATION_FAILED_1:
	case COOLKEY_AUTHENTICATION_FAILED_2:
	case COOLKEY_AUTHENTICATION_FAILED_3:
        return CSSM_ERRCODE_OPERATION_AUTH_DENIED;
    default:
        return SCardError::osStatus();
    }
}

void CoolKeyError::throwMe(uint16_t sw)
{ throw CoolKeyError(sw); }

#if !defined(NDEBUG)

void CoolKeyError::debugDiagnose(const void *id) const
{
    secdebug("exception", "%p CoolKeyError %s (%04hX)",
             id, errorstr(statusWord), statusWord);
}

const char *CoolKeyError::errorstr(uint16_t sw) const
{
	switch (sw)
	{
	case COOLKEY_AUTHENTICATION_FAILED_0:
		return "Authentication failed, 0 retries left.";
	case COOLKEY_AUTHENTICATION_FAILED_1:
		return "Authentication failed, 1 retry left.";
	case COOLKEY_AUTHENTICATION_FAILED_2:
		return "Authentication failed, 2 retries left.";
	case COOLKEY_AUTHENTICATION_FAILED_3:
		return "Authentication failed, 3 retries left.";
	default:
		return SCardError::errorstr(sw);
	}
}

#endif //NDEBUG


/* arch-tag: 0D984528-10D9-11D9-84A3-000393D5F80A */
