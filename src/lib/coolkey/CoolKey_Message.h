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

#if !defined (_NETKEY_MESSAGE_H)
#define _NETKEY_MESSAGE_H  

//#define Boolean bool

extern "C" {
#include <prlog.h>
//#include <plstr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


}

#include <string>
#include <vector>
#include  <map>
using namespace std;

/* MESSAGE Base Types */

typedef unsigned char      MESSAGE_u08;
typedef unsigned char      MESSAGE_byte;
typedef unsigned short     MESSAGE_u16;
typedef int                MESSAGE_u32;

typedef char      MESSAGE_s08;
typedef short     MESSAGE_s16;

typedef int       MESSAGE_s32;

typedef unsigned char      Byte;

typedef enum {
    MESSAGE_SUCCESS=0,
    MESSAGE_ERROR_UNKNOWN_TYPE,
    MESSAGE_ERROR_MISMATCH_TYPE,
    MESSAGE_ERROR_UNKNOWN_FIELD,
    MESSAGE_ERROR_READONLY,
    MESSAGE_ERROR_BUFSZ,
    MESSAGE_ERROR_INCOMPLETE_OBJ,
    MESSAGE_ERROR_NOT_ARRAY,
    MESSAGE_ERROR_IS_ARRAY,
    MESSAGE_ERROR_NO_MORE_ELEMENTS,
    MESSAGE_ERROR_NOT_DATATYPE,
    MESSAGE_ERROR_NO_CALLBACK,
    MESSAGE_ERROR_NO_CUD,
    MESSAGE_ERROR_UNKNOWN_CONSTANT,
    MESSAGE_ERROR_CUD_UNLOADED,
    MESSAGE_ERROR_MISMATCH_FIELD,
    MESSAGE_ERROR_NO_ARRAY_FOUND,
    MESSAGE_ERROR_ARRAY_NOT_STARTED,
    MESSAGE_ERROR_DATASZ
} MESSAGE_Return_Code;

#define REQUIRED_PARAMETER "required_parameter"
#define MESSAGE_TYPE "msg_type"
#define MESSAGE_MAX_BUFF_SIZE 2000
#define MESSAGE_MAX_NAME_VAL_SIZE 2048
#define MESSAGE_STR_SIZE  200
#define MESSAGE_BYTE_ARRAY_SIZE 500
#define MESSAGE_SN_SIZE 20
#define MESSAGE_PW_SIZE 20
#define MESSAGE_SECID_SIZE 10
#define MESSAGE_PIN_SIZE   15
#define MESSAGE_MAX_MAX    20
#define MESSAGE_MIN_MIN     5
#define MESSAGE_MAX_PDU_SIZE 1024 
#define MESSAGE_MAX_TASK_SIZE 50
#define MESSAGE_MAX_NUM_OPS 50
#define TLV_ARRAY_MAX    200
#define EXTENSIONS_TAG   1
#define MESSAGE_MAX_ID_SIZE 50
#define MESSAGE_MAX_NAME_SIZE 50
#define MESSAGE_MAX_DESC_SIZE 50
#define MESSAGE_MAX_TYPE_SIZE 50
#define MESSAGE_MAX_VALUE_SIZE 256
#define MESSAGE_MAX_OPTION_SIZE 50
#define MESSAGE_MAX_NUM_OPTIONS 50 
#define MESSAGE_MAX_PARAMETERS 50
#define MESSAGE_MAX_ERROR_MSG 256 

class eCKMessage {

public:
    enum sntype {
		UNKNOWN_MESSAGE=0,
		ERROR_MESSAGE=1,
		BEGIN_OP=2,
		LOGIN_REQUEST=3,
		LOGIN_RESPONSE=4,
		SECURID_REQUEST=5,
		SECURID_RESPONSE=6,
		ASQ_REQUEST=7,
		ASQ_RESPONSE=8,
		TOKEN_PDU_REQUEST=9,
		TOKEN_PDU_RESPONSE=10,
		NEW_PIN_REQUEST=11,
		NEW_PIN_RESPONSE=12,
                END_OP=13,
                STATUS_UPDATE_REQUEST=14,
                STATUS_UPDATE_RESPONSE=15,
                EXTENDED_LOGIN_REQUEST=16,
                EXTENDED_LOGIN_RESPONSE=17
                 
    };
	
   sntype message_type;
   
   void  setIntValue(string &aKey,int aValue );
   void  setStringValue(string &aKey,string &aValue);
   void  setBinValue(string &aKey,unsigned char* aValue,int *aSize);


   int   getIntValue(string &aKey);             

   string &getStringValue(string &aKey);

   void   getBinValue(string &aKey,unsigned char *avalue,int * aSize);


   static sntype decodeMESSAGEType(const string aMessage);

   static string intToString(int aInt);

	
   sntype getMessageType() { return message_type;}
		

   eCKMessage() : msglength(0) {};

   virtual ~eCKMessage() { mTokens.clear(); mTokenMap.clear();};
   virtual void encode(string &aOutputVal) ;
   virtual void  decode(string &aInputVal) ;

   static void Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters = " ");

   static void CreateTokenMap(map<string,string> &aTokenMap, vector<string> &aTokens);

   void CreateTokenMap(vector<string> &aTokens); 
   void getNameValueValue(const string & aName,string & value);                
		
   const char *getMESSAGETypeAsString();
   const char *getMESSAGETypeAsString(sntype type);

   vector<string> &getTokens() { return mTokens;}
   
protected:

    vector<string> mTokens;
    map<string,string> mTokenMap;    
private:
       
    char *messagebuf;
    MESSAGE_u32 msglength;

};

class nsNKeyREQUIRED_PARAMETER
{

public:

    nsNKeyREQUIRED_PARAMETER() {
        num_options = 0;  setAttempted = 0; 
    }

    ~nsNKeyREQUIRED_PARAMETER();

    void setId(string &aId) { mId = aId; }
    void setName(string &aName) { mName = aName; }
    void setType (string &aType) { mType = aType; }
    void setDesc(string &aDesc) { mDesc = aDesc ;}
    void setValue(string &aValue) { setAttempted = 1;mValue = aValue; }

    int   hasValue() {
        if(mValue.size() == 0)
            return 0;
        else
            return 1;
    } 


    string &getId() { return mId; }
    string &getType() { return mType;}
    string &getDesc() { return mDesc;}
    string &getName() { return mName;}
    string &getValue() { return mValue;}
    
    int hasValueAttempted() {
        return setAttempted;
    }

    void AddOption(string &option) {  mOptions.push_back(option);}

    void CleanuUpOptions();

    string &GetOptionAt(int index) {
        return mOptions.at(index);
    }

    string mId;
    string mName;
    string mDesc;
    string mType;
    string mValue;


    string mRawText;


    vector<string> mOptions;
 
    int  num_options;

    int  setAttempted;

    void SetRawText(string &aText) { mRawText = aText; }
    string &GetRawText() { return mRawText; }

    char *raw_text;
};

class nsNKeyREQUIRED_PARAMETERS_LIST
{

public:

    nsNKeyREQUIRED_PARAMETERS_LIST();

    ~nsNKeyREQUIRED_PARAMETERS_LIST();

    nsNKeyREQUIRED_PARAMETER *Add();
    nsNKeyREQUIRED_PARAMETER *GetAt(int index);
    nsNKeyREQUIRED_PARAMETER *GetById(string &aId);

    int GetNumParameters() { return  mParameters.size();}

    void EmitToBuffer(string &aOutputBuff);
    void CleanUp();

    int AreAllParametersSet();
private:

    vector<nsNKeyREQUIRED_PARAMETER *> mParameters;

};


class eCKMessage_BEGIN_OP
: public eCKMessage
{
public:
	/* An enumeration of different kickoff classes
    - should really be gotten from CUD file
	*/
    enum eCKMessageBEGINEnum {
       ENROLL=1,
       UNBLOCK=2,
       RESET_PIN=3,
       RENEW=4,
       UPDATE=5
    };

    ~eCKMessage_BEGIN_OP(); 
    eCKMessage_BEGIN_OP();
    eCKMessage_BEGIN_OP(char *buffer,MESSAGE_u32 buflen);

   void  decode(string &aInputVal) ;
   void encode(string &aOutputVal) ; 

    void setOperation(MESSAGE_s32 theOp) { 
        string key = "operation";
        setIntValue(key,(int)theOp);
    }

    vector<string> mExtensions;


    void AddExtensionValue(string &aValue)
    {

        mExtensions.push_back(aValue);

    }

       
};


class eCKMessage_LOGIN_REQUEST
: public eCKMessage
{
    public:

    eCKMessage_LOGIN_REQUEST();
    ~eCKMessage_LOGIN_REQUEST();

    void decode(string &aInputVal) ;

    void encode(string &aOutputVal) ;
   
};


class eCKMessage_EXTENDED_LOGIN_REQUEST
: public eCKMessage
{
    public:

    eCKMessage_EXTENDED_LOGIN_REQUEST();

    ~eCKMessage_EXTENDED_LOGIN_REQUEST();

    void decode(string &aInputVal) ;

    string &getTitle()  {
        string sKey = "title";

        return getStringValue(sKey);

   }

   string &getDescription()  {
        string sKey = "description";

        return  getStringValue(sKey);

   }
    void encode(string &aOutputVal) ;

    void SetReqParametersList(nsNKeyREQUIRED_PARAMETERS_LIST *list)
    {
        paramList = list;
    }

    nsNKeyREQUIRED_PARAMETERS_LIST *GetReqParametersList() { 
       return paramList;
    }

    nsNKeyREQUIRED_PARAMETERS_LIST *paramList;
};

class eCKMessage_EXTENDED_LOGIN_RESPONSE
: public eCKMessage
{
public:

    eCKMessage_EXTENDED_LOGIN_RESPONSE();
    ~eCKMessage_EXTENDED_LOGIN_RESPONSE();

    void  decode(string &aInputVal) ;
    void  encode(string &aOutputVal) ;

    void SetReqParametersList(nsNKeyREQUIRED_PARAMETERS_LIST *list)
    {
        paramList = list;
    }

    nsNKeyREQUIRED_PARAMETERS_LIST *GetReqParametersList() {
       return paramList;
    }

    nsNKeyREQUIRED_PARAMETERS_LIST *paramList;

};

class eCKMessage_LOGIN_RESPONSE
: public eCKMessage
{
public:

    eCKMessage_LOGIN_RESPONSE() ; 
    ~eCKMessage_LOGIN_RESPONSE();
      
    void  decode(string &aInputVal) ; 

    void setScreenName(string &aScreenName)
    {
       string key = "screen_name";

       setStringValue(key,aScreenName);
    }

    void setPassWord(string &aPassword)
    {
       string key = "password";
       setStringValue(key,aPassword);

    }

    void encode(string &aOutputVal) ;

};

class eCKMessage_SECURID_REQUEST
: public eCKMessage
{

    public:

    eCKMessage_SECURID_REQUEST();
   
    ~eCKMessage_SECURID_REQUEST(); 
    
   
    void decode(string &aInputVal) ; 


    MESSAGE_s32   getPinRequired() { 
        string key = "pin_required";
        int result = getIntValue(key);
        return result;
    }


    MESSAGE_s32   getNextValue()   { 
        string key = "next_value";
        int result = getIntValue(key);
        return result;
    }

    void encode(string &aOutputVal) ;
          
};

class eCKMessage_SECURID_RESPONSE
: public eCKMessage
{
public:

    eCKMessage_SECURID_RESPONSE() ;

    ~eCKMessage_SECURID_RESPONSE();
       
    void decode(string &aInputVal) ; 

    void  setValue(string &aValue)
    {
       string key = "value";

       setStringValue(key,aValue);


    }

    void setPin(string &aPin)
    {

       string key = "pin";

       setStringValue(key,aPin);


    }

    void encode(string &aOutputVal) ;

};

class eCKMessage_NEWPIN_REQUEST
: public eCKMessage
{

    public:

    eCKMessage_NEWPIN_REQUEST();

    ~eCKMessage_NEWPIN_REQUEST();
   
    void decode(string &aInputVal) ; 

    void  encode(string &aOutputVal) ;
};

class eCKMessage_NEWPIN_RESPONSE
: public eCKMessage
{
public:

    eCKMessage_NEWPIN_RESPONSE();

    ~eCKMessage_NEWPIN_RESPONSE();
      
    void decode(string &aInputVal) ; 

    void setNewPin(string &aNewPin)
    {

        string key = "new_pin";
        setStringValue(key,aNewPin); 

    }

    void encode(string &aOutputVal) ;

};

class eCKMessage_TOKEN_PDU_REQUEST
	: public eCKMessage
{

    public:

    eCKMessage_TOKEN_PDU_REQUEST();

    ~eCKMessage_TOKEN_PDU_REQUEST();
    

    void decode(string &aInputVal) ;

    int                 getPduSize()
    {

        string key = "pdu_size";
        int result = getIntValue(key);
        return result;

    }



    void                 getPduData(unsigned char *avalue,int * aSize) {

        string key = "pdu_data";

        getBinValue(key,avalue,aSize);


    }

    void encode(string &aOutputVal) ;

};    

class eCKMessage_TOKEN_PDU_RESPONSE
: public eCKMessage
{
public:

    eCKMessage_TOKEN_PDU_RESPONSE();
    ~eCKMessage_TOKEN_PDU_RESPONSE();
     
      
    void decode(string &aInputVal) ; 


    void       setPduData(unsigned char *aData, int aSize)
    {

        string key = "pdu_data";

        int return_size = aSize ;
        setBinValue(key,aData,&return_size);

        string pduSizeKey = "pdu_size";

        setIntValue(pduSizeKey,return_size);

    }

     void                 getPduData(unsigned char *avalue,int * aSize) {

        string key = "pdu_data";

        getBinValue(key,avalue,aSize);


    }

    int                 getPduSize()
    {

        string key = "pdu_size";
        int result = getIntValue(key);
        return result;

    }

    void encode(string &aOutputVal) ;

};

class eCKMessage_END_OP
: public eCKMessage
{
public:

    eCKMessage_END_OP() ;
    ~eCKMessage_END_OP();
  
    void  decode(string &aInputVal) ; 
    void encode(string &aOutputVal) ;

    int getOperation() {
        string key = "operation";
        return getIntValue(key);
    }

    int getResult() { 
        string key = "result";
        return getIntValue(key);
    }

    int getMessage() { 
        string key = "message";
        return getIntValue(key);
    }
    
};


class eCKMessage_STATUS_UPDATE_REQUEST
: public eCKMessage
{

    public:

    ~eCKMessage_STATUS_UPDATE_REQUEST();
    eCKMessage_STATUS_UPDATE_REQUEST() ;
       
    int getCurrentState() {

            string key = "current_state";
            int result = getIntValue(key);
            return result;

    }

   string &getNextTaskName()  {
        string sKey = "description";

        return  getStringValue(sKey);

   }

    void decode(string &aInputVal) ;
    void encode(string &aOutputVal) ;
};


class eCKMessage_STATUS_UPDATE_RESPONSE
: public eCKMessage
{
public:
    eCKMessage_STATUS_UPDATE_RESPONSE();
        

    ~eCKMessage_STATUS_UPDATE_RESPONSE();

    void decode(string &aInputVal) ;
    void encode(string &aOutputVal) ;

    void setCurrentState(int newState)
    {

        string key = "current_state";

        setIntValue(key,newState);

    }
        
};
#endif


void URLDecode_str(const string &data,string &output);
void URLEncode_str(const string &data,string &output);


