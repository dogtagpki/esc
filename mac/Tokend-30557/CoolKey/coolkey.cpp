/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com 
 *  CoolKey main program implementation.
 *  @APPLE_LICENSE_HEADER_START@CoolKey
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
 * CoolKey.cpp - CoolKey.tokend main program
 */
#include "CoolKeyToken.h"
#include <security_utilities/logging.h>

int main(int argc, const char *argv[])
{
    secdebug("CoolKey.tokend", "main starting with %d arguments", argc);
    secdelay("/tmp/delay/CoolKey");

    Syslog::notice("argc %d",argc);
    for(int i = 0; i < argc ; i++)
    {
        Syslog::notice("coolkey arg[%d]: %s",i,argv[i]);
    }

    token = new CoolKeyToken();
    return SecTokendMain(argc, argv, token->callbacks(), token->support());

    Syslog::notice("CoolKey.tokend exiting.... ");
}

/* arch-tag: 372EB7FE-0DBC-11D9-9A28-000A9595DEEE */
