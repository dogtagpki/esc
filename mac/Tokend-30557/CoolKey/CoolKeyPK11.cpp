/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *  Portions Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 *  Contributor(s):
 *  Jack Magne,jmagne@redhat.com
 *  CoolKey CoolKeyPK11 interface.
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
 * CoolKeyPK11.cpp - CoolKey.tokend PKCS11 helper class 
 */
#include "CoolKeyPK11.h"
#include <security_utilities/logging.h>
#include <dlfcn.h>


CK_ATTRIBUTE obj_classTemplate[] = {
       {CKA_CLASS,NULL,0}
};

CK_ATTRIBUTE certTemplate[] = {
       {CKA_CERTIFICATE_TYPE,NULL,0},{CKA_LABEL,NULL,0},
       {CKA_SUBJECT,NULL,0},{CKA_ID,NULL,0},{CKA_ISSUER,NULL,0},
       {CKA_SERIAL_NUMBER,NULL,0}, {CKA_VALUE,NULL,0}
};

CK_ATTRIBUTE public_keyTemplate[] = {
       {CKA_KEY_TYPE,NULL,0},{CKA_ID,NULL,0},{CKA_LABEL,NULL,0},{CKA_MODULUS,NULL,0},
       {CKA_PUBLIC_EXPONENT,NULL,0},{CKA_START_DATE,NULL,0},
       {CKA_END_DATE,NULL,0},
       {CKA_ENCRYPT,NULL,0},{CKA_VERIFY,NULL,0},{CKA_VERIFY_RECOVER,NULL,0},
       {CKA_WRAP,NULL,0}
};

CK_ATTRIBUTE private_keyTemplate[] = {
       
       {CKA_KEY_TYPE,NULL,0},{CKA_ID,NULL,0},{CKA_MODULUS,NULL,0},
       {CKA_LABEL,NULL,0},{CKA_START_DATE,NULL,0},
       {CKA_END_DATE,NULL,0}, {CKA_SENSITIVE,NULL,0},
       {CKA_DECRYPT,NULL,0}, {CKA_SIGN,NULL,0}, {CKA_SIGN_RECOVER,NULL,0},
       {CKA_UNWRAP,NULL,0}, {CKA_EXTRACTABLE,NULL,0}, {CKA_ALWAYS_SENSITIVE,NULL,0},
       {CKA_NEVER_EXTRACTABLE,NULL,0}, 
       {CKA_UNWRAP_TEMPLATE,NULL,0},{CKA_ALWAYS_AUTHENTICATE}
};

CK_ATTRIBUTE secret_keyTemplate[] = {

        {CKA_KEY_TYPE,NULL,0},{CKA_ID,NULL,0},{CKA_LABEL,NULL,0},
        {CKA_SENSITIVE,NULL,0},{CKA_ENCRYPT,NULL,0},{CKA_DECRYPT,NULL,0},
        {CKA_SIGN,NULL,0},{CKA_VERIFY,NULL,0},{CKA_WRAP,NULL,0},{CKA_UNWRAP,NULL,0},
        {CKA_EXTRACTABLE,NULL,0},{CKA_ALWAYS_SENSITIVE,NULL,0},{CKA_NEVER_EXTRACTABLE,NULL,0},
        {CKA_CHECK_VALUE,NULL,0},{CKA_WRAP_WITH_TRUSTED,NULL,0},{CKA_TRUSTED,NULL,0},
        {CKA_WRAP_TEMPLATE,NULL,0}  
}; 

int CoolKeyPK11::loginToken(char *aPIN)
{
    if(!mInitialized)
        return 0;

    if(!aPIN)
       return 0;

    CK_RV ck_rv = mEpv->C_Login(mSessHandle,CKU_USER,(CK_UTF8CHAR_PTR ) aPIN,(CK_ULONG) strlen(aPIN));

    Syslog::notice("CoolKeyPK11::loginToken result %d aPin %s ",ck_rv,"****");

    if(ck_rv == CKR_OK && !mCachedPIN.size())
    {
        //Syslog::notice("CoolKeyPK11::loginToken setting cached PIN");
        mCachedPIN = aPIN;
    }

    return ck_rv;
}

void CoolKeyPK11::logoutToken()
{
    if(!mInitialized)
        return;

    CK_RV ck_rv = mEpv->C_Logout(mSessHandle);

    //clear cached pin
    mCachedPIN = "";
    Syslog::notice("CoolKeyPK11::logout result %d ",ck_rv);
}

int CoolKeyPK11::verifyCachedPIN(char *aPIN)
{
    if(!aPIN || !mInitialized)
        return CKR_PIN_INCORRECT; 

    if(!strcmp(aPIN,(char *) mCachedPIN.c_str()))
    {
        Syslog::notice("PIN OK!");
        return CKR_OK;
    }

   Syslog::notice("PIN not OK!");

   return CKR_PIN_INCORRECT;
}

int CoolKeyPK11::isTokenLoggedIn()
{
    CK_RV ck_rv;

    if(!mInitialized)
        return 0;

    CK_SESSION_INFO sinfo;


    CK_SESSION_HANDLE h = (CK_SESSION_HANDLE)0;

    ck_rv = mEpv->C_GetSessionInfo(mSessHandle, &sinfo);

    if( CKR_OK != ck_rv ) {
        Syslog::notice("isTokenLoggedIn C_GetSessionInfo(0x%08x) returned %lu ulDeviceError %lu", mSessHandle, ck_rv,sinfo.ulDeviceError);

        ck_rv = mEpv->C_OpenSession(mSlots[mOurSlotIndex], CKF_SERIAL_SESSION, (CK_VOID_PTR)CK_NULL_PTR, (CK_NOTIFY)CK_NULL_PTR, &h);
        if( CKR_OK != ck_rv ) {
            Syslog::error("C_OpenSession(%lu, CKF_SERIAL_SESSION, , ) returned 0x%08x", mSlots[mOurSlotIndex], ck_rv);
            return 0;
        }

        ck_rv = mEpv->C_GetSessionInfo(h, &sinfo);

        if( CKR_OK != ck_rv)  {
            Syslog::error("Failed second chance to get session info!!");
            return 0;
        }
        else
        {
            Syslog::notice("Created new session handle, old one is invalid! 0x%08x ",h);
            mSessHandle = h;
        }
    }

   int loggedIn = 0;

   Syslog::notice("isTokenLoggedIn token state %d", sinfo.state);

   if(sinfo.state == CKS_RO_USER_FUNCTIONS || sinfo.state == CKS_RW_USER_FUNCTIONS)
   {
       Syslog::notice("isTokenLoggedIn Token IS logged in");
       loggedIn = 1;
   }
   else
   {
       Syslog::notice("isTokenLoggedIn Token IS NOT logged in");
   }

   return loggedIn; 
}

int CoolKeyPK11::loadModule()
{

    CK_RV ck_rv;
    CK_FUNCTION_LIST_PTR epv = (CK_FUNCTION_LIST_PTR) NULL;

    CK_C_GetFunctionList gfl;

    CK_C_INITIALIZE_ARGS InitArgs;
    InitArgs.CreateMutex=0;              //CK_CREATEMUTEX
    InitArgs.DestroyMutex=0;             //CK_DESTROYMUTEX
    InitArgs.LockMutex=0;                //CK_LOCKMUTEX
    InitArgs.UnlockMutex=0;              //CK_UNLOCKMUTEX
    InitArgs.flags=0;    //CK_FLAGS
    InitArgs.pReserved=0;                //CK_C_INITIALIZE_ARGS

    mTokenUid[0] = 0;

    mPk11Driver = dlopen(PKCS11_PATH_NAME,RTLD_LAZY);

    if(!mPk11Driver)
    {
        Syslog::error("Can't load pkcs11 driver error: %d ",dlerror());
        return 0;
    }
    else
    {
        Syslog::debug("In CoolKeyToken::loadPKCS11() . past load lib %p ",mPk11Driver);
    }

    // Now try to load the functions

    gfl =(CK_C_GetFunctionList) dlsym(mPk11Driver,"C_GetFunctionList");

    if(gfl == NULL)
    {
        Syslog::error("In CoolKeyToken::loadPKCS11() Can't load symbol C_GetFunctionList error: $d ",dlerror());
        dlclose(mPk11Driver);
        mPk11Driver = NULL;
        return 0;
    }

    //Syslog::debug("Found C_GetFunctionList : %p " , gfl);

    ck_rv = (*gfl)(&epv);
    
    if(ck_rv != CKR_OK)
    {
        Syslog::error("Can't get actual function list "); 
        dlclose(mPk11Driver);
        mPk11Driver = NULL;
        return 0;
        }
    
   //Syslog::debug("Function list found %p ", epv);
    
    mEpv = epv;
            //Now try to actually initialize the module
        
    ck_rv =  mEpv->C_Initialize(&InitArgs);

    if(ck_rv != CKR_OK)
    {
        Syslog::error("Error initializing PKCS11 module result: %d ",ck_rv);

        dlclose(mPk11Driver);
        mPk11Driver = NULL;
        mEpv = NULL;
        return 0;
    }

    //Syslog::debug("Successfully Initialized PKCS11 module. ");

    mInitialized = 1;
    int res = loadSlotList();

    if(res)
    {
        mIsOurToken = 1;
    }
    else
        mInitialized = 0; 

    return res;    

}

int CoolKeyPK11::freeModule()
{
    if(!mInitialized)
    {
        return 1;
    }

    if(mEpv)
    {
        mEpv->C_Finalize(NULL);

        mInitialized = NULL;
    }

    return 1;

}

int CoolKeyPK11::freeObjects()
{
    return 1;
}

int CoolKeyPK11::loadObjects()
{
    CK_RV ck_rv;

    if(!mInitialized)
        return 0;

    CK_SESSION_HANDLE h = (CK_SESSION_HANDLE)0;
    CK_SESSION_INFO sinfo;
    CK_ULONG tnObjects = 0;

    mSessHandle = 0;

    ck_rv = mEpv->C_OpenSession(mSlots[mOurSlotIndex], CKF_SERIAL_SESSION, (CK_VOID_PTR)CK_NULL_PTR, (CK_NOTIFY)CK_NULL_PTR, &h);
    if( CKR_OK != ck_rv ) {
        Syslog::error("C_OpenSession(%lu, CKF_SERIAL_SESSION, , ) returned 0x%08x", mSlots[mOurSlotIndex], ck_rv);
        return 0;
    }

    Syslog::notice("    Opened a session: handle = 0x%08x", h);

    mSessHandle = h;

    ck_rv = mEpv->C_GetSessionInfo(h, &sinfo);
    if( CKR_OK != ck_rv ) {
      Syslog::notice("C_GetSessionInfo(%lu, ) returned 0x%08x", h, ck_rv);
      return 0;
    }

    Syslog::notice("    SESSION INFO:");
    Syslog::notice("        slotID = %lu", sinfo.slotID);
    Syslog::notice("        state = %lu", sinfo.state);
    Syslog::notice("        flags = 0x%08x", sinfo.flags);
    Syslog::notice("        ulDeviceError = %lu", sinfo.ulDeviceError);
   

    ck_rv = mEpv->C_FindObjectsInit(h, (CK_ATTRIBUTE_PTR)CK_NULL_PTR, 0);
    if( CKR_OK != ck_rv ) {
      Syslog::error("C_FindObjectsInit(%lu, NULL_PTR, 0) returned 0x%08x", h, ck_rv);
      return 0;
    }

    Syslog::notice("    All objects:");
    while(1) {
        CK_OBJECT_HANDLE o = (CK_OBJECT_HANDLE)0;
        CK_ULONG nObjects = 0;

        ck_rv = mEpv->C_FindObjects(h, &o, 1, &nObjects);

        if( CKR_OK != ck_rv ) {
            Syslog::notice("C_FindObjects(%lu, , 1, ) returned 0x%08x", h, ck_rv);
            return 0;
        }

        if( 0 == nObjects ) {
            break;
        }

        tnObjects++;

        Syslog::notice("        OBJECT HANDLE %lu", o);

        ck_rv = mEpv->C_GetAttributeValue(h, o, obj_classTemplate, 1);
        //Syslog::notice("ck_rv %d",ck_rv);
        switch( ck_rv ) {
            case CKR_OK:
            case CKR_ATTRIBUTE_SENSITIVE:
            case CKR_ATTRIBUTE_TYPE_INVALID:
            case CKR_BUFFER_TOO_SMALL:
            break;
            default:
                Syslog::notice("C_GetAtributeValue(%lu, %lu, {one attribute type}, %lu) returned 0x%08x",
                          h, o, 1, ck_rv);
                return 0;
        }

        if( 1 ) {
            if( -1 != (CK_LONG)obj_classTemplate[0].ulValueLen ) {
               
                obj_classTemplate[0].pValue = (CK_VOID_PTR) new char[obj_classTemplate[0].ulValueLen];

                if(!obj_classTemplate[0].pValue)
                {
                    Syslog::notice("Can't allocate memory for attribute of size %d",(int) obj_classTemplate[0].ulValueLen);
                    continue;
                } 

                ck_rv = mEpv->C_GetAttributeValue(h, o, obj_classTemplate, 1);
 
                if(ck_rv == CKR_OK)
                {

                      CK_LONG obj_class = (CK_LONG) *((CK_LONG *) obj_classTemplate[0].pValue); 

                      Syslog::notice("objclass: %lu",obj_class);
                      CoolKeyObject *newObject = NULL;

                      switch(obj_class)
                      {
                          case CKO_CERTIFICATE:
                              Syslog::notice("Found certificate:-----------------"); 

                               newObject = (CoolKeyObject *) new CoolKeyCertObject(o,h,obj_class,this);

                          break;

                          case CKO_PUBLIC_KEY:
                                  Syslog::notice("Found public key:----------------");
                                  newObject = (CoolKeyObject *) new CoolKeyKeyObject(o,h,obj_class,this); 
                              Syslog::notice("Found public key:");
                          break;
 
                          case CKO_PRIVATE_KEY:
                              Syslog::notice("Found private key:-------------------");
                              newObject = (CoolKeyObject *)  new CoolKeyKeyObject(o,h,obj_class,this);

                          break;
                
                          default:
                              Syslog::notice("Found something else:");
                          break;
                      };

                      if(newObject)
                      {
                          mObjects[o] = newObject;
                      } 
                      
                }
                    if(obj_classTemplate[0].pValue)
                        delete [] (char *) obj_classTemplate[0].pValue; 
                        obj_classTemplate[0].pValue = NULL;
                
              }
          }

    } /* while(1) */

    ck_rv = mEpv->C_FindObjectsFinal(h);
    if( CKR_OK != ck_rv ) {
        Syslog::notice("C_FindObjectsFinal(%lu) returned 0x%08x", h, ck_rv);
        return 0;
    }

    Syslog::notice("    (%lu objects total)", tnObjects);

    ck_rv = mEpv->C_CloseSession(h);
    if( CKR_OK != ck_rv ) {
        Syslog::notice( "C_CloseSession(%lu) returned 0x%08x", h, ck_rv);
        return 0;
    }

    return 1;
}

int CoolKeyPK11::loadSlotList()
{
    mTokenUid[0] = 0;
    int result = 0;

    if(!mInitialized)
    {
        return result;
    }

    CK_RV ck_rv = 0;

    CK_ULONG nSlots = 0;

    ck_rv = mEpv->C_GetSlotList(CK_FALSE,(CK_SLOT_ID) CK_NULL_PTR,&nSlots);

    if(ck_rv == CKR_OK)
    {
        Syslog::notice("In CoolKeyPK11:loadSlotList() GetSlotList found %d slot(s) ",nSlots);
     }     
     else
     {
         Syslog::notice("In CoolKeyToken::probe() GetSlotList error: %d ",ck_rv);
     } 

     if(nSlots > COOLKEY_MAX_SLOTS)
     {
         return result;
     }

     if(nSlots > 0)
     {
         ck_rv = mEpv->C_GetSlotList(CK_FALSE,mSlots, &nSlots);

         if(ck_rv != CKR_OK)
         {
             Syslog::debug("In CoolKeyToken::probe() GetSlotList error: %d ",ck_rv);
         }

         mOurSlotIndex = nSlots - 1;

         for(CK_ULONG i = 0; i < nSlots ; i++)
         {
             CK_SLOT_INFO sinfo;

             int j = 0;

             while( j++ < 5)
             {
             ck_rv = mEpv->C_GetSlotInfo(mSlots[i],&sinfo);


             if(ck_rv != CKR_OK)
             {
                 Syslog::error("In CoolKeyPK11::loadSlotListe() GetSlotInfo error: %d ",ck_rv);
                 break;
                 //continue;
             }
                    
             Syslog::notice("    Slot Info: Slot: %d" ,i);
             Syslog::notice("        slotDescription = \"%.64s\"", sinfo.slotDescription);
             Syslog::notice("        manufacturerID = \"%.32s\"", sinfo.manufacturerID);

             Syslog::notice("        flags = 0x%08lx", sinfo.flags);
             Syslog::notice("            -> TOKEN PRESENT = %s", 
                        sinfo.flags & CKF_TOKEN_PRESENT ? "TRUE" : "FALSE");
             Syslog::notice("            -> REMOVABLE DEVICE = %s",
                      sinfo.flags & CKF_REMOVABLE_DEVICE ? "TRUE" : "FALSE");
             Syslog::notice("            -> HW SLOT = %s", 
                               sinfo.flags & CKF_HW_SLOT ? "TRUE" : "FALSE");
             Syslog::notice("        hardwareVersion = %lu.%02lu", 
                           (uint32)sinfo.hardwareVersion.major, (uint32)sinfo.hardwareVersion.minor);
             Syslog::notice("        firmwareVersion = %lu.%02lu",
               (uint32)sinfo.firmwareVersion.major, (uint32)sinfo.firmwareVersion.minor); 

             Syslog::notice(" See if token is present in reader");


             if(!(sinfo.flags & CKF_TOKEN_PRESENT))
             {
                 Syslog::notice(" Failed to connect to the token try again.");
                 usleep(100000);
                 continue;
              }

              Syslog::notice(" Token is really present!");
              break;

             }

             if(sinfo.flags & CKF_TOKEN_PRESENT )
             {
                  CK_TOKEN_INFO tinfo;

                  (void)memset(&tinfo, 0, sizeof(CK_TOKEN_INFO));
                  ck_rv = mEpv->C_GetTokenInfo(mSlots[i], &tinfo);
                  if( CKR_OK != ck_rv ) {
                      Syslog::debug("C_GetTokenInfo(%lu, ) returned 0x%08x", mSlots[i], ck_rv);
                      return result;
                  }

                  Syslog::notice("    Token Info:");
                  Syslog::notice("        label = \"%.32s\"", tinfo.label);
                  Syslog::notice("        manufacturerID = \"%.32s\"", tinfo.manufacturerID);
                  Syslog::notice("        model = \"%.16s\"", tinfo.model);
                  Syslog::notice("        serialNumber = \"%.16s\"", tinfo.serialNumber);
                  Syslog::notice("        flags = 0x%08lx", tinfo.flags);

                  /*
                  Syslog::notice("            -> RNG = %s",
                        tinfo.flags & CKF_RNG ? "TRUE" : "FALSE");
                  Syslog::notice("            -> WRITE PROTECTED = %s",
                        tinfo.flags & CKF_WRITE_PROTECTED ? "TRUE" : "FALSE");
                  Syslog::notice("            -> LOGIN REQUIRED = %s",
                        tinfo.flags & CKF_LOGIN_REQUIRED ? "TRUE" : "FALSE");
                  Syslog::notice("            -> USER PIN INITIALIZED = %s",
                        tinfo.flags & CKF_USER_PIN_INITIALIZED ? "TRUE" : "FALSE");
                  Syslog::notice("            -> RESTORE KEY NOT NEEDED = %s",
                      tinfo.flags & CKF_RESTORE_KEY_NOT_NEEDED ? "TRUE" : "FALSE");
                  Syslog::debug("            -> CLOCK ON TOKEN = %s",
                             tinfo.flags & CKF_CLOCK_ON_TOKEN ? "TRUE" : "FALSE");
                  Syslog::notice( "        ulMaxSessionCount = %lu", tinfo.ulMaxSessionCount);
                  Syslog::notice( "        ulSessionCount = %lu", tinfo.ulSessionCount);
                  Syslog::notice( "        ulMaxRwSessionCount = %lu", tinfo.ulMaxRwSessionCount);
                  Syslog::notice("        ulRwSessionCount = %lu", tinfo.ulRwSessionCount);
                  Syslog::notice( "        ulMaxPinLen = %lu", tinfo.ulMaxPinLen);
                  Syslog::notice("        ulMinPinLen = %lu", tinfo.ulMinPinLen);
                  Syslog::notice("        ulTotalPublicMemory = %lu", tinfo.ulTotalPublicMemory);
                  Syslog::notice("        ulFreePublicMemory = %lu", tinfo.ulFreePublicMemory);
                  Syslog::notice("        ulTotalPrivateMemory = %lu", tinfo.ulTotalPrivateMemory);
                  Syslog::notice("        ulFreePrivateMemory = %lu", tinfo.ulFreePrivateMemory);
                  Syslog::notice("        hardwareVersion = %lu.%02lu", 
                      (uint32)tinfo.hardwareVersion.major, (uint32)tinfo.hardwareVersion.minor);
                       Syslog::notice("        firmwareVersion = %lu.%02lu",
                       (uint32)tinfo.firmwareVersion.major, (uint32)tinfo.firmwareVersion.minor);
                  Syslog::notice("        utcTime = \"%.16s\"", tinfo.utcTime);
                  */     

                  Syslog::notice(" Token is present uid %s",tinfo.label);
                  int label_size = 32;
                         
                  memcpy((void  *) mTokenUid, (void *) tinfo.label,label_size);
                  mTokenUid[label_size -1] = 0;
             }
             else
             {
                 Syslog::error(" Token not present in slot ");
                 return  result;
             }
             
         }
     }else
     {
         return result;
     }

     return 1;
}

//Actual crypto ops


int CoolKeyPK11::decryptData(CoolKeyObject *aObj,CK_BYTE *aEncData, CK_ULONG aEncDataLen, CK_BYTE *aData, CK_ULONG *aDataLen)
{
    int result = 0;

    if( !mEpv || !aObj ||  !aEncData || !aEncDataLen || !aData || !aDataLen
        || aDataLen <=0 || *aDataLen <= 0)
    {
        Syslog::error(" CoolKeyPK11::decryptData bad input data");
        return result;
    }

   CK_OBJECT_HANDLE objHandle = aObj->getHandle();

   CK_RV ck_rv = mEpv->C_DecryptInit(mSessHandle,NULL,objHandle);

   Syslog::notice("decryptData C_DecryptInit Init result %d", ck_rv);

   if(ck_rv != CKR_OK)
   {
       Syslog::notice("decryptData error calling C_DecryptInit");
       return result;
   }

   ck_rv = mEpv->C_Decrypt(mSessHandle,aEncData,aEncDataLen,aData,aDataLen);

   Syslog::notice("C_Decrypt result %d", ck_rv);

   if(ck_rv != CKR_OK)
   {
       Syslog::notice("C_Decrypt result in error");
       return 0;
   }

   Syslog::notice("decryptData return success");

   return 1;

}

int CoolKeyPK11::generateSignature(CoolKeyObject *aObj,CK_BYTE *aData, CK_ULONG aDataLen, CK_BYTE *aSignature, CK_ULONG *aSignatureLen)
{
    int result = 0;

    if( !mEpv || !aObj ||  !aData || !aDataLen || !aSignature || !aSignatureLen
        || aDataLen <=0 || *aSignatureLen <= 0)
    {
        Syslog::error(" CoolKeyPK11::generateSignature bad input data");
        return result;
    }

   CK_OBJECT_HANDLE objHandle = aObj->getHandle();

   CK_RV ck_rv = mEpv->C_SignInit(mSessHandle,NULL,objHandle);

   Syslog::notice("generateSignature C_SignInit result %d", ck_rv);

   if(ck_rv != CKR_OK)
   {
       Syslog::notice("generatSignature error calling C_SignInit");
       return result;
   }

   ck_rv = mEpv->C_Sign(mSessHandle,aData,aDataLen,aSignature,aSignatureLen);

   Syslog::notice("C_Sign result %d", ck_rv);

   if(ck_rv != CKR_OK)
   {
       Syslog::notice("C_Sign result in error");
       return 0;
   }
 
   Syslog::notice("generateSignature return success"); 
   return 1; 

}

void CoolKeyObject::dumpData(CK_CHAR *aData, CK_ULONG aDataLen)
{
    char line[256];

    Syslog::notice("dumping data %p len %lu",aData,aDataLen);

    CK_ULONG max = 8;
    for(CK_ULONG i = 0 ; i < aDataLen;  i ++)
    {

        if(i > max)
             break;

        sprintf(line," val[%lu]= %X",i,aData[i]);
        Syslog::notice(line);
    }
}

void CoolKeyObject::loadAttributes(CK_ATTRIBUTE *aTemplate,int aTemplateSize) 
{
    CK_RV ck_rv;

    Syslog::notice("CoolKeyObject::loadAttributes with args template size %d",aTemplateSize);

    if(!aTemplate || aTemplateSize <= 0 || mAttributesLoaded)
        return;

    CK_FUNCTION_LIST_PTR funcPtr = NULL;

    if(mParent && (funcPtr = mParent->getFunctionPointer()))
    {
         Syslog::notice("CoolKeyObject::loadAttributes got function pointer");
         ck_rv = funcPtr->C_GetAttributeValue(mSessHandle, mObjHandle, aTemplate, aTemplateSize);

         switch(ck_rv)
         {
             case CKR_OK:
             case CKR_ATTRIBUTE_SENSITIVE:
             case CKR_ATTRIBUTE_TYPE_INVALID:
             case CKR_BUFFER_TOO_SMALL:
             break;

             default:
                 Syslog::notice("CoolKeyObject::loadAttributes failed ck_rv %d",ck_rv);
                 return;
             break;
         };

         for(int i = 0 ; i < aTemplateSize ; i++)
         {
             Syslog::notice("Object attribute:  name % stype 0x%lx ,  size %d",
                 attributeName(aTemplate[i].type),aTemplate[i].type,
                 aTemplate[i].ulValueLen);             
         }

         //Do it again to get actual data

         for(int i = 0; i < aTemplateSize ; i++)
         {
             int size = (int)aTemplate[i].ulValueLen;

             if(size && size != -1)
             {
                 char *objData = new char [aTemplate[i].ulValueLen];

                 if(!objData)
                 {
                     continue;
                 }
         
                 aTemplate[i].pValue = objData;
             }
         }

         //Now have the data alocated go get it.

         ck_rv = funcPtr->C_GetAttributeValue(mSessHandle, mObjHandle, aTemplate, aTemplateSize);
          
         switch(ck_rv)
         {  
             case CKR_OK:
             case CKR_ATTRIBUTE_SENSITIVE:
             case CKR_ATTRIBUTE_TYPE_INVALID:
             case CKR_BUFFER_TOO_SMALL:
             break;

             default:
                 Syslog::notice("CoolKeyObject::loadAttributes failed ck_rv %d",ck_rv);
                 return;
             break;
         };

         //print out results of actually getting the data

         for(int i = 0 ; i < aTemplateSize ; i++)
         {
             int size = aTemplate[i].ulValueLen;
             char *data = (char *) aTemplate[i].pValue;

             if(size && size != -1 &&  data)
             {
                Syslog::notice("Legitimate Object attribute saving.... Name: %s : type 0x%lx ,  size %d",
                   attributeName(aTemplate[i].type),aTemplate[i].type,
                   aTemplate[i].ulValueLen);

                CK_ATTRIBUTE * newAttr = new CK_ATTRIBUTE ;

                //Check for CAC data, we want to let the Apple CAC TokenD take care of these.


                if(strstr(data,"CAC"))
                {
                     Syslog::notice("CAC related item found, exiting... \n");
                     exit(0);
                }

                if(!newAttr)
                {
                    Syslog::notice("Can't allocate memory for new attribute");
                    continue;
                }

                newAttr->ulValueLen = aTemplate[i].ulValueLen;
                newAttr->type = aTemplate[i].type;
                newAttr->pValue = aTemplate[i].pValue;

                //CoolKeyObject::dumpData((CK_BYTE *)newAttr->pValue,newAttr->ulValueLen);

                // put the attribute in our local map

                aTemplate[i].ulValueLen = 0;
                aTemplate[i].pValue = NULL;

                mAttributes[newAttr->type] = newAttr;
             } 
         }

    }    
}

void CoolKeyKeyObject::loadAttributes()
{
    Syslog::notice("CoolKeyKeyObject::loadAttributes no args");

    if(mAttributesLoaded)
        return;

    int templateSize = 0;

    if(mObjClass == CKO_PRIVATE_KEY)
    {
        templateSize = sizeof(private_keyTemplate)/sizeof(CK_ATTRIBUTE);

        CoolKeyObject::loadAttributes((CK_ATTRIBUTE *)private_keyTemplate,templateSize);
    }
    else
    {
        templateSize = sizeof(public_keyTemplate)/sizeof(CK_ATTRIBUTE);
        CoolKeyObject::loadAttributes((CK_ATTRIBUTE *)public_keyTemplate, templateSize);

    }

}

CK_BYTE CoolKeyKeyObject::getSensitive()
{
    CK_BYTE result = 0;

    result = getByteAttribute(CKA_SENSITIVE);

    Syslog::notice("In CoolKeyObject::getID type %c",result);
    return result;
}

CK_BYTE      CoolKeyKeyObject::getKeyEncrypt()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_ENCRYPT);

    Syslog::notice("In CoolKeyObject::getKeyEncrypt result %d",result);
    return result;


}

CK_BYTE      CoolKeyKeyObject::getKeyDecrypt()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_DECRYPT);

    Syslog::notice("In CoolKeyObject::getKeyDecrypt type %d",result);
    return result;

}

CK_BYTE      CoolKeyKeyObject::getKeySign()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_SIGN);

    Syslog::notice("In CoolKeyKeyObject::getKeySign type %d",result);
    return result;


}

CK_BYTE      CoolKeyKeyObject::getKeyWrap()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_WRAP);

    Syslog::notice("In CoolKeyKeyObject::getKeyWrap type %d",result);
    return result;


}

CK_BYTE      CoolKeyKeyObject::getKeyVerify()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_VERIFY);

    Syslog::notice("In CoolKeyKeyObject::getKeyVerify type %d",result);
    return result;


}

CK_BYTE      CoolKeyKeyObject::getKeyDerive()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_DERIVE);

    Syslog::notice("In CoolKeyKeyObject::getKeyDerive type %d",result);
    return result;


}

CK_BYTE      CoolKeyKeyObject::getKeyUnwrap()
{
             
    CK_BYTE result = 0;
         
    result = getByteAttribute(CKA_UNWRAP);

    Syslog::notice("In CoolKeyKeyObject::getKeyUnwrap type %d",result);
    return result;

         
}        

CK_BYTE      CoolKeyKeyObject::getKeySignRecover()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_SIGN_RECOVER);

    Syslog::notice("In CoolKeyKeyObject::getKeySignRecover type %d",result);
    return result;

}

CK_BYTE      CoolKeyKeyObject::getKeyVerifyRecover()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_VERIFY_RECOVER);

    Syslog::notice("In CoolKeyObject::getKeyKeyVerifyRecover type %d",result);
    return result;

}

CK_BYTE      CoolKeyKeyObject::getKeyExtractable()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_EXTRACTABLE);

    Syslog::notice("In CoolKeyKeyObject::getExtractable type %d",result);
    return result;

}

CK_BYTE      CoolKeyKeyObject::getKeyNeverExtractable()
{

    CK_BYTE result = 0;

    result = getByteAttribute(CKA_NEVER_EXTRACTABLE);

    Syslog::notice("In CoolKeyKeyObject::getNeverExtractable type %d",result);
    return result;

}

CK_BYTE CoolKeyKeyObject::getAlwaysSensitive()
{
    CK_BYTE result = 0;

    result = getByteAttribute(CKA_ALWAYS_SENSITIVE);

    Syslog::notice("In CoolKeyKeyObject::getAlwaysSensitive type %d",result);
    return result;
}

void CoolKeyKeyObject::getLabel(CK_BYTE *aData, CK_ULONG *aDataLen)
{

    if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_LABEL,aData,aDataLen);

   Syslog::notice("In CoolKeyKeyObject::getLabel %s",aData);
}

CK_BYTE CoolKeyObject::getID()
{
    CK_BYTE result = 0;

    result = getByteAttribute(CKA_ID);

    Syslog::notice("In CoolKeyObject::getID type %c",result);
    return result;
}

void CoolKeyObject::freeAttributes()
{

    Syslog::notice("CoolKeyObject::freeAttributes");

    map< CK_ATTRIBUTE_TYPE, CK_ATTRIBUTE  * >::iterator i;

    CK_ATTRIBUTE *cur = NULL;

    for(i = mAttributes.begin(); i!= mAttributes.end(); i++)
    {
         cur = (*i).second;

        if(cur)
        {
            if(cur->pValue)
                delete [] (char *) cur->pValue;
           
            delete cur;
        }

    }

}

void CoolKeyObject::loadAttributes() 
{
    //Syslog::notice("CoolKeyObject::loadAttributes no args");
}

void CoolKeyCertObject::loadAttributes()
{
    //Syslog::notice("In CoolKeyCertObject::loadAttributes no args");

    if(mAttributesLoaded)
        return;
    
    int templateSize = sizeof(certTemplate)/sizeof(CK_ATTRIBUTE);

    if(!templateSize)
        return;

    CoolKeyObject::loadAttributes(certTemplate,templateSize);
}

CoolKeyKeyObject::CoolKeyKeyObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent) : CoolKeyObject(aObjHandle,aSessHandle,aObjClass,aParent) 
{
    //Syslog::notice("CoolKeyKeyObject::CoolKeyKeyObject");

    loadAttributes();
}

CK_ULONG CoolKeyKeyObject::getKeySize()
{
     CK_ULONG size = 0;

     CK_ATTRIBUTE *theMod =  getAttribute(CKA_MODULUS);

     if(theMod)
     {    
         size =  (theMod->ulValueLen - 1 ) *  8;

     }

     return size;
}

CoolKeyCertObject::CoolKeyCertObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent) : CoolKeyObject(aObjHandle,aSessHandle,aObjClass,aParent)
{
    loadAttributes();
}

void CoolKeyCertObject::getIssuer(CK_BYTE *aData, CK_ULONG *aDataLen)
{
   if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_ISSUER,aData,aDataLen);

}

void CoolKeyCertObject::getSerialNo(CK_BYTE *aData,CK_ULONG *aDataLen)
{

    if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_SERIAL_NUMBER,aData,aDataLen);

}

void CoolKeyCertObject::getLabel(CK_BYTE *aData, CK_ULONG *aDataLen)
{

    if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_LABEL,aData,aDataLen);

}

void CoolKeyCertObject::getPublicKeyHash(CK_BYTE *aData,CK_ULONG *aDataLen)
{

    if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

}

void CoolKeyCertObject::getData(CK_BYTE *aData, CK_ULONG *aDataLen)
{

    if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_VALUE,aData,aDataLen);
}

void CoolKeyCertObject::getSubject(CK_BYTE *aData, CK_ULONG *aDataLen)
{

   if(!aData || !aDataLen || *aDataLen < 1)
       return;

   aData[0] = 0;

   getByteDataAttribute(CKA_SUBJECT,aData,aDataLen);

}

CK_ULONG CoolKeyCertObject::getType()
{

    CK_ULONG result = 0;

    result = getULongAttribute(CKA_CERTIFICATE_TYPE);

    Syslog::notice("In CoolKeyCertObject::getType type %lu",result);
    return result;

}

CoolKeyObject::CoolKeyObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent) : mObjHandle(aObjHandle),mSessHandle(aSessHandle),mAttributesLoaded(0),mObjClass(aObjClass),mParent(aParent)
{
    Syslog::notice("In CoolKeyObject::CoolKeyObject mObjClass %d",mObjClass);
}

CK_ATTRIBUTE * CoolKeyObject::getAttribute(CK_ATTRIBUTE_TYPE aAttr)
{
    CK_ATTRIBUTE *theAttr = mAttributes[aAttr];

    return theAttr;
}

CK_LONG CoolKeyObject::getLongAttribute(CK_ATTRIBUTE_TYPE aAttr)
{

    CK_ATTRIBUTE *theAttr = getAttribute(aAttr);

    if(!theAttr)
        return 0;


    CK_ULONG size = theAttr->ulValueLen   ;

    if(size != sizeof(CK_LONG))
        return 0;

    if(!theAttr->pValue)
        return 0;

    return (CK_LONG) *((CK_LONG *) theAttr->pValue);
}

CK_ULONG CoolKeyObject::getULongAttribute(CK_ATTRIBUTE_TYPE aAttr)
{

    CK_ATTRIBUTE *theAttr = getAttribute(aAttr);

    Syslog::notice("In CoolKeyObject::getULongAttr attr %p size %d  value %p",theAttr,theAttr->ulValueLen,theAttr->pValue);

    if(!theAttr)
        return 0;

    CK_ULONG size = theAttr->ulValueLen ;

    if(size != sizeof(CK_ULONG))
        return 0;

    if(!theAttr->pValue)
        return 0;

    return (CK_ULONG) *((CK_ULONG *) theAttr->pValue);
}

CK_BYTE CoolKeyObject::getByteAttribute(CK_ATTRIBUTE_TYPE aAttr)
{

    CK_ATTRIBUTE *theAttr = getAttribute(aAttr);

    if(!theAttr)
        return 0;

    CK_ULONG size = theAttr->ulValueLen ;

    if(size != sizeof(CK_BYTE))
        return 0;

    if(!theAttr->pValue)
        return 0;

    return (CK_BYTE) *((CK_BYTE *) theAttr->pValue);
}


void CoolKeyObject::getByteDataAttribute(CK_ATTRIBUTE_TYPE aAttr,CK_BYTE *aData, CK_ULONG *aDataLen)
{
   if(!aData || !aDataLen || *aDataLen <= 0 )
       return;

   CK_ATTRIBUTE *theAttr = getAttribute(aAttr);

   Syslog::notice("In CoolKeyObject::getByteData attr %p  attr size %d ",theAttr,theAttr->ulValueLen);
    if(!theAttr)
        return ;

    CK_ULONG size = theAttr->ulValueLen ;

    if(size < 1 || size >= *aDataLen)
        return;

    *aDataLen = 0;
    aData[0] = 0;

    if(!theAttr->pValue)
        return;

     memcpy( aData,  theAttr->pValue,size);

    *aDataLen = size;
}

char *CoolKeyObject::attributeName(uint32_t attributeId)
{
        static char buffer[20];

        switch (attributeId)
        {
        case CKA_CLASS: return "CLASS";
        case CKA_TOKEN: return "TOKEN";
        case CKA_PRIVATE: return "PRIVATE";
        case CKA_LABEL: return "LABEL";
        case CKA_APPLICATION: return "APPLICATION";
        case CKA_VALUE: return "VALUE";
        case CKA_OBJECT_ID: return "OBJECT_ID";
        case CKA_CERTIFICATE_TYPE: return "CERTIFICATE_TYPE";
        case CKA_ISSUER: return "ISSUER";
        case CKA_SERIAL_NUMBER: return "SERIAL_NUMBER";
        case CKA_AC_ISSUER: return "AC_ISSUER";
        case CKA_OWNER: return "OWNER";
        case CKA_ATTR_TYPES: return "ATTR_TYPES";
        case CKA_TRUSTED: return "TRUSTED";
        case CKA_KEY_TYPE: return "KEY_TYPE";
        case CKA_SUBJECT: return "SUBJECT";
        case CKA_ID: return "ID";
        case CKA_SENSITIVE: return "SENSITIVE";
        case CKA_ENCRYPT: return "ENCRYPT";
        case CKA_DECRYPT: return "DECRYPT";
        case CKA_WRAP: return "WRAP";
        case CKA_WRAP_TEMPLATE: return "WRAP_TEMPLATE";
        case CKA_UNWRAP: return "UNWRAP";
        case CKA_SIGN: return "SIGN";
        case CKA_SIGN_RECOVER: return "SIGN_RECOVER";
        case CKA_VERIFY: return "VERIFY";
        case CKA_VERIFY_RECOVER: return "VERIFY_RECOVER";
        case CKA_DERIVE: return "DERIVE";
        case CKA_START_DATE: return "START_DATE";
        case CKA_END_DATE: return "END_DATE";
        case CKA_MODULUS: return "MODULUS";
        case CKA_MODULUS_BITS: return "MODULUS_BITS";
        case CKA_PUBLIC_EXPONENT: return "PUBLIC_EXPONENT";
        case CKA_PRIVATE_EXPONENT: return "PRIVATE_EXPONENT";
        case CKA_PRIME_1: return "PRIME_1";
        case CKA_PRIME_2: return "PRIME_2";
        case CKA_EXPONENT_1: return "EXPONENT_1";
        case CKA_EXPONENT_2: return "EXPONENT_2";
        case CKA_COEFFICIENT: return "COEFFICIENT";
        case CKA_PRIME: return "PRIME";
        case CKA_SUBPRIME: return "SUBPRIME";
        case CKA_BASE: return "BASE";
        case CKA_PRIME_BITS: return "PRIME_BITS";
        case CKA_SUB_PRIME_BITS: return "SUB_PRIME_BITS";
        case CKA_VALUE_BITS: return "VALUE_BITS";
        case CKA_VALUE_LEN: return "VALUE_LEN";
        case CKA_EXTRACTABLE: return "EXTRACTABLE";
        case CKA_LOCAL: return "LOCAL";
        case CKA_NEVER_EXTRACTABLE: return "NEVER_EXTRACTABLE";
        case CKA_ALWAYS_SENSITIVE: return "ALWAYS_SENSITIVE";
        case CKA_KEY_GEN_MECHANISM: return "KEY_GEN_MECHANISM";
        case CKA_MODIFIABLE: return "MODIFIABLE";
        case CKA_EC_PARAMS: return "EC_PARAMS";
        case CKA_EC_POINT: return "EC_POINT";
        case CKA_SECONDARY_AUTH: return "SECONDARY_AUTH";
        case CKA_AUTH_PIN_FLAGS: return "AUTH_PIN_FLAGS";
        case CKA_HW_FEATURE_TYPE: return "HW_FEATURE_TYPE";
        case CKA_RESET_ON_INIT: return "RESET_ON_INIT";
        case CKA_HAS_RESET: return "HAS_RESET";
        case CKA_VENDOR_DEFINED: return "VENDOR_DEFINED";
        case CKA_ALWAYS_AUTHENTICATE: return "ALWAYS_AUTHENTICATE";
        case CKA_WRAP_WITH_TRUSTED: return "WRAP_WITH_TRUSTED";
        case CKA_UNWRAP_TEMPLATE: return "UNWRAP_TEMPLATE";
        case CKA_HASH_OF_SUBJECT_PUBLIC_KEY: return "HASH_OF_SUBJECT_PUBLIC_KEY";
        case CKA_HASH_OF_ISSUER_PUBLIC_KEY: return "HASH_OF_ISSUER_PUBLIC_KEY";
        default:
                snprintf(buffer, sizeof(buffer), "unknown(%0x08X)", attributeId);
                return buffer;
        }
}
