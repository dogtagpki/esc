/** BEGIN COPYRIGHT BLOCK
 * This Program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 of the License.
 *
 * This Program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/

#ifndef RH_COOLKEY_CSP_H
#define RH_COOLKEY_CSP_H
#include "rhICoolKey.h"
#include "CoolKey.h"
#include <wincrypt.h>

class CoolKeyCSPKeyListener: public rhIKeyNotify 
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_RHIKEYNOTIFY
  CoolKeyCSPKeyListener() ;
  ~CoolKeyCSPKeyListener() ;

   
    static HCRYPTPROV GetCryptHandle();   
    static HCRYPTPROV mCryptProv;

};

#endif
