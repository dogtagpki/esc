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

#ifndef __SLOTUTILS_H__
#define __SLOTUTILS_H__

//#define DUMP_CHALLENGE_RESPONSE 1

#include "nss.h"
#include "secmod.h"
#include "pk11func.h"
#include "ssl.h"
#include "p12plcy.h"
#include "secmod.h"
#include "secerr.h"
#include "certdb.h"
#include "secmodt.h"
#include "keythi.h"
#include "keyhi.h"
#include "CoolKey.h"

// CoolKey info flags/masks used by CoolKeyGetInfo():

#define COOLKEY_INFO_HAS_ATR_MASK          (1 << 0)
#define COOLKEY_INFO_HAS_APPLET_MASK       (1 << 1)
#define COOLKEY_INFO_IS_PERSONALIZED_MASK  (1 << 2) 
#define COOLKEY_INFO_IS_REALLY_A_COOLKEY_MASK   (1 << 3)

#define HAS_ATR(info)         ((info)&COOLKEY_INFO_HAS_ATR_MASK)
#define HAS_APPLET(info)      ((info)&COOLKEY_INFO_HAS_APPLET_MASK)
#define IS_PERSONALIZED(info) ((info)&COOLKEY_INFO_IS_PERSONALIZED_MASK)
#define IS_REALLY_A_COOLKEY(info) ((info)&COOLKEY_INFO_IS_REALLY_A_COOLKEY_MASK)

struct CoolKeyInfo
{
  CoolKeyInfo() : mReaderName(NULL),
                 mATR(NULL),
                 mCUID(NULL),
                 mMSN(NULL),
                 mInfoFlags(0),
		 mSeries(0) {}

  ~CoolKeyInfo()
  {
    if (mReaderName)
      free(mReaderName);
    if (mATR)
      free(mATR);
    if (mCUID)
      free(mCUID);
    if (mMSN)
      free(mMSN);
    if (mSlot)
      PK11_FreeSlot(mSlot);
  }

  char *mReaderName;        // CoolKey Card Reader Name
  char *mATR;               // Token ATR
  char *mCUID;              // Token CUID
  char *mMSN;               // Token MSN
  PK11SlotInfo *mSlot;      // Token MSN
  unsigned int mInfoFlags;  // Bit flags: HAS_ATR, HAS_APPLET, IS_PERSONALIZED
  unsigned int mSeries;     // Tell if a token has changes in the slot
};

void InitCoolKeyList(void);
void DestroyCoolKeyList(void);
void LockCoolKeyList(void);
void UnlockCoolKeyList(void);
CoolKeyInfo *GetCoolKeyInfoByReaderName(const char *aReaderName);
CoolKeyInfo *GetCoolKeyInfoBySlotName(const char *aSlotName);
CoolKeyInfo *GetCoolKeyInfoBySlot(PK11SlotInfo *);
CoolKeyInfo *GetCoolKeyInfoByKeyID(const CoolKey *aKey);
CoolKeyInfo *GetCoolKeyInfoByTokenName(const char *aTokenName);
const char *GetReaderNameForKeyID(const CoolKey *aKey);
const char *GetSlotNameForKeyID(const CoolKey *aKey);
const char *GetATRForKeyID(const CoolKey *aKey);
const char *GetCUIDForKeyID(const CoolKey *aKey);
const char *GetMSNForKeyID(const CoolKey *aKey);
unsigned int GetInfoFlagsForKeyID(const CoolKey *aKey);
int RefreshInfoFlagsForKeyID(const CoolKey *aKey);
int InsertCoolKeyInfoIntoCoolKeyList(CoolKeyInfo *aInfo);
int RemoveCoolKeyInfoFromCoolKeyList(CoolKeyInfo *aInfo);
int ClearCoolKeyList(void);


PK11SlotInfo *GetSlotForKeyID(const CoolKey *aKey);
SECKEYPrivateKey* GetAuthenticationPrivateKey(PK11SlotInfo* slot);
SECItem* GetAuthenticationPublicKey(PK11SlotInfo* slot);
//long GetPublicKeyCRC(PK11SlotInfo *slot);
#endif // __SLOTUTILS_H__
