/*
 *  CoolKeyPK11.h
 *  Tokend CoolKey
 */

#ifndef _COOLKEYPK11_H_
#define _COOLKEYPK11_H_

#include "mypkcs11.h"
//#include <Security/SecKey.h>
#include <map>
#include <string>
#include <Token.h>

#define COOLKEY_MAX_SLOTS 20
#define PKCS11_PATH_NAME "/Library/Application Support/CoolKey/PKCS11/libcoolkeypk11.dylib"

class CoolKeyPK11;

class CoolKeyObject
{
public:
 
    CoolKeyObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent); 
    virtual ~CoolKeyObject() {  freeAttributes();};

    virtual void loadAttributes() ;

    char *CoolKeyObject::attributeName(uint32_t attributeId);

    CK_ATTRIBUTE * getAttribute(CK_ATTRIBUTE_TYPE aAttr);
    CK_LONG      getLongAttribute(CK_ATTRIBUTE_TYPE aAttr);
    CK_ULONG     getULongAttribute(CK_ATTRIBUTE_TYPE aAttr);
    CK_BYTE      getByteAttribute(CK_ATTRIBUTE_TYPE aAttr);    
    CK_BYTE      getID();

    void         getByteDataAttribute(CK_ATTRIBUTE_TYPE aAttr,CK_BYTE *aData, CK_ULONG *aDataLen);

    CK_OBJECT_CLASS   getClass() {return mObjClass;}    

    static void dumpData(CK_BYTE *aData, CK_ULONG aDataLen);

    CK_OBJECT_HANDLE getHandle() { return mObjHandle;}
    CK_SESSION_HANDLE getSessHandle() { return mSessHandle; } 

protected:

    void loadAttributes(CK_ATTRIBUTE *aTemplate,int aTemplateSize);
    void freeAttributes();

    CK_OBJECT_HANDLE mObjHandle;
    CK_SESSION_HANDLE mSessHandle;
    int mAttributesLoaded;
    CK_LONG mObjClass; 

    std::map< CK_ATTRIBUTE_TYPE, CK_ATTRIBUTE* > mAttributes;

private:

    CoolKeyPK11 *mParent;

public:


};

class CoolKeyKeyObject : public CoolKeyObject
{

public:

    CoolKeyKeyObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessionHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent) ;

    CK_ULONG getKeySize();
    void getLabel(CK_BYTE *aData, CK_ULONG *aDataLen);
    CK_BYTE getSensitive();
    CK_BYTE getAlwaysSensitive();
    CK_BYTE      getKeyEncrypt();
    CK_BYTE      getKeyDecrypt();
    CK_BYTE      getKeySign();
    CK_BYTE      getKeyWrap();
    CK_BYTE      getKeyExtractable();
    CK_BYTE      getKeyNeverExtractable();
    CK_BYTE      getKeyVerify();
    CK_BYTE      getKeyDerive();
    CK_BYTE      getKeyUnwrap();
    CK_BYTE      getKeySignRecover();
    CK_BYTE      getKeyVerifyRecover();

     void loadAttributes();

    ~CoolKeyKeyObject() {};

};

class CoolKeyCertObject : public CoolKeyObject
{

public:

    CoolKeyCertObject(CK_OBJECT_HANDLE aObjHandle, CK_SESSION_HANDLE aSessionHandle,CK_LONG aObjClass,CoolKeyPK11 *aParent) ;

    void loadAttributes();

    void getSubject(CK_BYTE *aData, CK_ULONG *aDataLen);
    void getIssuer(CK_BYTE *aData, CK_ULONG *aDataLen);
    void getSerialNo(CK_BYTE *aData,CK_ULONG *aDataLen);
    void getLabel(CK_BYTE *aData, CK_ULONG *aDataLen);
    void getData(CK_BYTE *aData, CK_ULONG *aDataLen);


    void getPublicKeyHash(CK_BYTE *aData,CK_ULONG *aDataLen);

    CK_ULONG getType();

    ~CoolKeyCertObject() {};

};

class CoolKeyPK11
{
public:

    typedef std::map< CK_OBJECT_HANDLE, CoolKeyObject  * >::iterator ObjIterator;

    CoolKeyPK11(): mPk11Driver(NULL),mEpv(NULL),mInitialized(0),mOurSlotIndex(0),mIsOurToken(0),mCachedPIN("") {} ;
    virtual ~CoolKeyPK11() {};

    int loadModule(const SCARD_READERSTATE &readerInfo);
    int freeModule();

    int loginToken(char *aPIN);
    void logoutToken();

    int isTokenLoggedIn();

    int loadObjects();
    int freeObjects();

    int getIsOurToken() { return mIsOurToken;}
    char *getTokenId() {  
           if(mTokenUid[0] != 0) 
               return mTokenUid;
                   else 
               return NULL;
    }; 

    CK_FUNCTION_LIST_PTR getFunctionPointer() { return mEpv;}
    int                  getInitialized()     { return mInitialized; }

    int verifyCachedPIN(char *aPIN);


//Actual cryto operations

    int generateSignature(CoolKeyObject *aObj,CK_BYTE *aData, CK_ULONG aDataLen, CK_BYTE *aSignature, CK_ULONG *aSignatureLen);

    int decryptData(CoolKeyObject *aObj,CK_BYTE *aEncData, CK_ULONG aEncDataLen, CK_BYTE *aData, CK_ULONG *aDataLen);


protected:

private:

    int loadSlotList(const SCARD_READERSTATE &readerInfo);

    void * mPk11Driver;
    CK_FUNCTION_LIST_PTR mEpv;

    int mInitialized;

    CK_SLOT_ID  mSlots[COOLKEY_MAX_SLOTS];
    int mOurSlotIndex;
    CK_SLOT_INFO mOurSlotInfo;

    char mTokenUid[32];
    int mIsOurToken;

    CK_SESSION_HANDLE  mSessHandle;

    std::map<CK_OBJECT_HANDLE   , CoolKeyObject * > mObjects;

    std::string mCachedPIN;

public:

    ObjIterator begin() { return ObjIterator(mObjects.begin()); }
    ObjIterator end() { return ObjIterator(mObjects.end()); }
    
};

#endif /* !_COOLKEYPK11_H_ */

