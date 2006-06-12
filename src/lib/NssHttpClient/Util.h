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

#ifndef RA_UTIL_H
#define RA_UTIL_H

#include "pk11func.h"
//#include "main/Buffer.h"

#ifdef XP_WIN32
#define NSAPI_PUBLIC __declspec(dllexport)
#else /* !XP_WIN32 */
#define NSAPI_PUBLIC
#endif /* !XP_WIN32 */

class Util
{
  public:
          Util();
          ~Util();
  public:
          NSAPI_PUBLIC static int ascii2numeric(char ch);
  /*        static char *Buffer2String (Buffer &data);
          static Buffer *Str2Buf (const char *s);
          static char *URLEncode (Buffer &data);
          static char *URLEncodeInHex (Buffer &data);
          NSAPI_PUBLIC static char *URLEncode (const char *data);
          static Buffer *URLDecode(const char *data);
          static char *SpecialURLEncode (Buffer &data);
          static Buffer *SpecialURLDecode(const char *data);
          static PRStatus GetRandomChallenge(Buffer &random);
          static PRStatus CreateKeySetData(
                             Buffer &key_set_version,
                             Buffer &old_kek_key,
                             Buffer &new_auth_key,
                             Buffer &new_mac_key,
                             Buffer &new_kek_key,
                             Buffer &output);
          static PRStatus ComputeCryptogram(PK11SymKey *key,
                          const Buffer &card_challenge,
                          const Buffer &host_challenge,
                          Buffer &output);
          static PRStatus ComputeMAC(PK11SymKey *key,
                          Buffer &input, const Buffer &icv,
                          Buffer &output);
          static PRStatus ComputeKeyCheck(
                          const Buffer& newKey, Buffer& output);
          static PK11SymKey *DeriveKey(const Buffer& permKey,
                        const Buffer& hostChallenge,
                        const Buffer& cardChallenge);
          static PRStatus EncryptData(PK11SymKey *encSessionKey,
                          Buffer &input, Buffer &output);
          static PRStatus EncryptData(Buffer &kek_key,
                          Buffer &input, Buffer &output);
          static PK11SymKey *DiversifyKey(PK11SymKey *master,
                          Buffer &data, PK11SlotInfo *slot);
          static PRStatus DecryptData(Buffer &kek_key,
                          Buffer &input, Buffer &output);
          static PRStatus DecryptData(PK11SymKey* enc_key,
                          Buffer &input, Buffer &output);
          static BYTE*    bool2byte(bool p);
          */
};

#endif /* RA_UTIL_H */
