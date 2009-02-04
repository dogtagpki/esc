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

#ifndef _OPENKEY_H
#define _OPENKEY_H 1

#define OPENKEY_CARD_NAME "CoolKey Card Type"
#define OPENKEY_CARD_NAME_MS "CoolKey Card Type\0"  /* a multi-string */

//#define OPENKEY_PROV   "Identity Alliance CSP"
//#define OPENKEY_PROV_W L"Identity Alliance CSP"

#define OPENKEY_PROV   "CoolKey PKCS #11 CSP"
#define OPENKEY_PROV_W L"CoolKey PKCS #11 CSP"

#define OPENKEY_NAME_W L"CoolKey"

/*
 * User-defined certificate properties must faill in the range
 * between CERT_FIRST_USER_PROP_ID (0x00008000) and
 * CERT_LAST_USER_PROP_ID (0x0000FFFF).  We choose an offset
 * that is unlikely to collide with other people's user-defined
 * certificate properties.
 */

/* DEAD, BEEF, CAFE, F00D, C0DE, anything else? */

#define OPENKEY_CERT_FIRST_PROP_ID 0x0000FACE
#define OPENKEY_CERT_KEY_ID_PROP_ID 0x0000FACE

#endif /* _OPENKEY_H */
