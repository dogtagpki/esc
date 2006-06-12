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

#include "nspr.h"
#include "CoolKey_Message.h"
#include "math.h"
#include <iostream>

PRLogModuleInfo *nkeyLogMS = PR_NewLogModule("netkey");

void URLEncode(unsigned char *buf,char *ret, int *ret_len,int buff_len);
void URLDecode(char *buf,unsigned char *ret, int *ret_len,int buff_len);

void eCKMessage::CreateTokenMap(vector<string> &aTokens)
{
  CreateTokenMap(mTokenMap,aTokens);
}

void eCKMessage::CreateTokenMap(map<string,string> &aTokenMap, vector<string> &aTokens)
{
    vector<string>::iterator i;

    for(i = aTokens.begin(); i!= aTokens.end(); i++)
    {
        //cout << (*i) << endl;

        string curValue = "";
        string curKey = "";

        string::size_type pos = (*i).find_first_of('=', 0);

        if(pos == string::npos)
            continue;

        curKey = (*i).substr(0,pos);
        curValue = (*i).substr(pos + 1);
             
        aTokenMap[curKey] = curValue;
  
        //cout << "key = " << curKey << " value = " << curValue << " \n" << endl;

    }

}

void eCKMessage::Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters )
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void eCKMessage::encode(string &aOutputVal) 
{

    string delim = "&";

    int message_size = aOutputVal.size();

    string len_str   = intToString(message_size);

    string temp = "s=" + len_str + delim +  aOutputVal;
    aOutputVal  = temp ; 

}

void eCKMessage::decode(string &aInputVal) 
{
    string delim = "&";
    
    Tokenize(aInputVal, mTokens,delim);

    CreateTokenMap(mTokens);

}

void eCKMessage::getNameValueValue(const string & aName,string & aValue)
{
    aValue = "";
    aValue = mTokenMap[aName];
  
}

eCKMessage::sntype eCKMessage::decodeMESSAGEType(const string aMessage)
{
    sntype type = eCKMessage::UNKNOWN_MESSAGE;

    int code = 0;

    string name = MESSAGE_TYPE;

    string delim = "&";

    vector<string> tokens;

    Tokenize(aMessage, tokens,delim);

    vector<string>::iterator i;

    for(i = tokens.begin(); i!= tokens.end(); i++)
    {
        //cout << (*i) << endl;

        string::size_type position = (*i).find(name);

        if(position != string::npos)
        {
            //cout << (*i)  << "bingo \n" << endl;

             string hit = "";
             string::size_type pos = (*i).find_first_of('=', 0);

             if(pos == string::npos)
                 return type;

             hit = (*i).substr(pos + 1);

             //cout << "hit = " << hit << " \n" << endl;
           
             const char *hit_str = hit.c_str();

             code = atoi((char *)hit_str);

             //cout << "type number " << code << "\n" << endl;

             type =   (eCKMessage::sntype) code; 
            return type;
        }
    }

    return type;
}

string eCKMessage::intToString(int aInt)
{
    string result = "";

    int buff_size = 0;

    int magnitude = abs(aInt);

    if(aInt == 0)
    {
        buff_size = 3;
    }
    else
        buff_size =((int) log10((float)magnitude)) + 3;

    char *temp = new char[buff_size];

    sprintf(temp,"%d",aInt);

    result = temp;
    delete temp;

    return result;
}

void  eCKMessage::setIntValue(string &aKey,int aValue )
{

    if(!aKey.length())
        return;

    string new_value = intToString(aValue);


    mTokenMap[aKey] = new_value;

}

void  eCKMessage::setStringValue(string &aKey,string &aValue)
{
   if(!aKey.length())
       return;

   mTokenMap[aKey] = aValue;

}

void  eCKMessage::setBinValue(string &aKey,unsigned char*aValue,int *aSize)
{
    if(!aKey.length())
        return;

    if(aSize <=0 || !aValue)
        return;

    string data = "";

    unsigned char *raw_data = aValue;

    int encode_output_size = 4 *  (*aSize) + 1; 
    char *encode_output = new char[encode_output_size];

    if(!encode_output)
    {
         *aSize = 0;
         return;

    }

    int input_size = *aSize;

    URLEncode(raw_data,encode_output, &input_size, encode_output_size);

    *aSize = input_size;

    //printf("output %s \n",encode_output);

    data = encode_output;

    mTokenMap[aKey] = data;

    delete encode_output;

}

int   eCKMessage::getIntValue(string &aKey)
{
    string  value =  mTokenMap[aKey];

    return atoi(value.c_str());

}

string &eCKMessage::getStringValue(string &aKey)
{
    return mTokenMap[aKey];
}

void   eCKMessage::getBinValue(string &aKey,unsigned char *avalue,int * aSize)
{
    if(!aKey.length())
        return;

   string value = mTokenMap[aKey];

   char *raw_data =(char *) value.c_str();

   int size = (int) value.size();

   char unsigned  *decode_output = avalue;

   int decode_output_size =  size  + 1 ;

   if(*aSize <= decode_output_size)
   {
       *aSize = 0;
        return;
   }

   int output_size = 0;

   URLDecode(raw_data,decode_output, &output_size, *aSize);

   *aSize = output_size;

   //printf("output %s \n",decode_output);

}

const char *eCKMessage::getMESSAGETypeAsString()
{
	return getMESSAGETypeAsString(message_type);
}

const char *eCKMessage::getMESSAGETypeAsString(sntype type)
{
    const char *st=NULL;

    switch (type) {
    case eCKMessage::UNKNOWN_MESSAGE:       st = "UNKNOWN_MESSAGE"; break;
    case eCKMessage::ERROR_MESSAGE:         st = "ERROR_MESSAGE"; break;
    case eCKMessage::BEGIN_OP:           st = "BEGIN_OP"; break;
    case eCKMessage::LOGIN_REQUEST:      st = "LOGIN_REQUEST"; break;
    case eCKMessage::LOGIN_RESPONSE:     st = "LOGIN_RESPONSE"; break;
    case eCKMessage::SECURID_REQUEST:    st = "SECURID_REQUEST"; break;
    case eCKMessage::SECURID_RESPONSE:   st = "SECURID_RESPONSE"; break;
    case eCKMessage::ASQ_REQUEST:        st = "ASQ_REQUEST"; break;
    case eCKMessage::ASQ_RESPONSE:       st = "ASQ_RESPONSE";   break;
    case eCKMessage::TOKEN_PDU_REQUEST:  st = "TOKEN_PDU_REQUEST"; break;
    case eCKMessage::TOKEN_PDU_RESPONSE: st = "TOKEN_PDU_RESPONSE"; break;
    case eCKMessage::NEW_PIN_REQUEST:    st = "NEW_PIN_REQUEST"; break;
    case eCKMessage::NEW_PIN_RESPONSE:   st = "NEW_PIN_RESPONSE"; break;
    case eCKMessage::END_OP:             st = "END_OP"; break;
    case eCKMessage::STATUS_UPDATE_REQUEST: st = "STATUS_UPDATE_REQUEST"; break;
    case eCKMessage::STATUS_UPDATE_RESPONSE: st = "STATUS_UPDATE_RESPONSE"; break;
    default:  st = "unknown";
    }

	return st;
}

eCKMessage_EXTENDED_LOGIN_REQUEST::eCKMessage_EXTENDED_LOGIN_REQUEST()
{           paramList = NULL; message_type = EXTENDED_LOGIN_REQUEST;
}

eCKMessage_EXTENDED_LOGIN_REQUEST::~eCKMessage_EXTENDED_LOGIN_REQUEST()
{

    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_EXTENDED_LOGIN_REQUEST::~eCKMessage_EXTENDED_LOGIN_REQUEST \n"));
}

void eCKMessage_EXTENDED_LOGIN_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_EXTENDED_LOGIN_REQUEST::decode(string &aInputVal) 
{
    eCKMessage::decode(aInputVal);

    string decoded_value = "";
    map<string,string>::iterator i;

    for(i = mTokenMap.begin(); i!= mTokenMap.end(); i++)
    {
        URLDecode_str((*i).second, decoded_value);

        if( ((*i).first).find("required_parameter") != string::npos)
        {
              vector<string> tTokens;
              map<string,string> tTokenMap;

              string delim = "&";
              if(paramList)
              {
                  Tokenize(decoded_value,tTokens,delim);

                  CreateTokenMap(tTokenMap,tTokens);

                  nsNKeyREQUIRED_PARAMETER  *tParam =  paramList->Add();

                  string id   = tTokenMap["id"];
                  string desc = tTokenMap["desc"]; 
                  string name = tTokenMap["name"];
                  string type = tTokenMap["type"];

                  //cout << " id = " << id << " desc = " << desc << " name = " << name << " type = " << type << endl;

                  if(tParam)
                  {
                     string raw =  decoded_value;
                     tParam->SetRawText(raw);
                     tParam->setId(id);
                     tParam->setDesc(desc);
                     tParam->setName(name);
                     tParam->setType(type);
                  }
              }
        
        }

    } 
}

nsNKeyREQUIRED_PARAMETERS_LIST::nsNKeyREQUIRED_PARAMETERS_LIST()
{
}

    

nsNKeyREQUIRED_PARAMETERS_LIST::~nsNKeyREQUIRED_PARAMETERS_LIST()
{
    CleanUp();
}

nsNKeyREQUIRED_PARAMETER *nsNKeyREQUIRED_PARAMETERS_LIST::Add()
{
    nsNKeyREQUIRED_PARAMETER *new_parameter = new nsNKeyREQUIRED_PARAMETER();

    if(new_parameter)
    {
       mParameters.push_back(new_parameter);

    }
    return new_parameter;
}

nsNKeyREQUIRED_PARAMETER *nsNKeyREQUIRED_PARAMETERS_LIST::GetAt(int index)
{
    int size = mParameters.size();

    if(index >= size || index < 0)
        return NULL;

    return mParameters.at(index);

}
nsNKeyREQUIRED_PARAMETER *nsNKeyREQUIRED_PARAMETERS_LIST::GetById(string &aId)
{
    int size = mParameters.size();

    nsNKeyREQUIRED_PARAMETER *curParam =  NULL;

    for(int i = 0; i < size ; i++)
    {
        curParam = mParameters[i];

        if(curParam)
        {
            string tId = "";

            tId = curParam->getId();

            if(tId == aId)
                return curParam;

        }        

    }
    return NULL;
}

void nsNKeyREQUIRED_PARAMETERS_LIST::CleanUp()
{

    vector<nsNKeyREQUIRED_PARAMETER *>::iterator i;

    nsNKeyREQUIRED_PARAMETER * curParam = NULL;


    for(i = mParameters.begin(); i!= mParameters.end(); i++)
    {

        curParam = (*i);

        if(curParam)
            delete curParam;

        curParam = NULL;

    }


    mParameters.clear();

}

int nsNKeyREQUIRED_PARAMETERS_LIST::AreAllParametersSet()
{

    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("nsNKeyREQUIRED_PARAMETERS_LIST::AreAllParametersSet:\n"));

    int done = 0;

    int num_params = GetNumParameters();

    for(int i = 0; i < num_params ; i++)
    {
        nsNKeyREQUIRED_PARAMETER *curParam = GetAt(i);

        if(curParam)
        {
            if( !curParam->hasValueAttempted())
            {
                PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("nsNKeyREQUIRED_PARAMETERS_LIST::AreAllParametersSet found parameter not set: index %d\n",i));

                return done;
            } 
        }
    }

    done = 1;
 
    return done;
}

void nsNKeyREQUIRED_PARAMETERS_LIST::EmitToBuffer(string &aOutputBuff)
{

    aOutputBuff = "";

    string separater = "&&";

    int num_params = GetNumParameters();

    for(int i = 0 ; i < num_params ; i++)
    {
         nsNKeyREQUIRED_PARAMETER *curParam = GetAt(i);

         if(curParam)
         {
             string raw = curParam->GetRawText();
             aOutputBuff += raw + separater;

         }

    }

    int size = aOutputBuff.size();

    if(aOutputBuff[size - 1] == '&' && aOutputBuff[size - 2] == '&')
    {
        aOutputBuff.erase(size - 1);
        aOutputBuff.erase(size - 2);
    }

}
eCKMessage_LOGIN_REQUEST::eCKMessage_LOGIN_REQUEST() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_LOGIN_REQUEST::eCKMessage_LOGIN_REQUEST:\n"));
    message_type = LOGIN_REQUEST; 
}

eCKMessage_LOGIN_REQUEST:: ~eCKMessage_LOGIN_REQUEST()
{
   PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_LOGIN_REQUEST::~eCKMessage_LOGIN_REQUEST:\n"));
}

void eCKMessage_LOGIN_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_LOGIN_REQUEST::decode(string &aInputVal) 
{
  eCKMessage::decode(aInputVal);
}

eCKMessage_LOGIN_RESPONSE::eCKMessage_LOGIN_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_LOGIN_RESPONSE::eCKMessage_LOGIN_RESPONSE:\n"));

    message_type = LOGIN_RESPONSE;
}

eCKMessage_LOGIN_RESPONSE:: ~eCKMessage_LOGIN_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_LOGIN_RESPONSE::~eCKMessage_LOGIN_RESPONSE:\n"));
}

void eCKMessage_LOGIN_RESPONSE::decode(string &aInputVal) 
{
}

void eCKMessage_LOGIN_RESPONSE::encode(string &aOutputVal)
{
    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;

    string sKey = "screen_name";

    string lScreenName = getStringValue(sKey);

    string pKey = "password";

    string lPassword = getStringValue(pKey);

    aOutputVal += sKey + delim1 + lScreenName + delim + pKey + delim1 + lPassword;

    eCKMessage::encode(aOutputVal);

}

eCKMessage_EXTENDED_LOGIN_RESPONSE::eCKMessage_EXTENDED_LOGIN_RESPONSE()
{
  PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_EXTENDED_LOGIN_RESPONSE::eCKMessage_EXTENDED_LOGIN_RESPONSE:\n"));

   message_type = EXTENDED_LOGIN_RESPONSE;
   paramList = NULL;
}

eCKMessage_EXTENDED_LOGIN_RESPONSE:: ~eCKMessage_EXTENDED_LOGIN_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_EXTENDED_LOGIN_RESPONSE::~eCKMessage_EXTENDED_LOGIN_RESPONSE:\n"));

}

void eCKMessage_EXTENDED_LOGIN_RESPONSE::decode(string &aInputVal) 
{
}

void eCKMessage_EXTENDED_LOGIN_RESPONSE::encode(string &aOutputVal)
{
    aOutputVal = "";

    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;
    if(paramList)
    {
        int num_parameters = paramList->GetNumParameters();

        for(int i = 0 ; i < num_parameters ; i++)
        {
            nsNKeyREQUIRED_PARAMETER *param = paramList->GetAt(i);

            if(!param)
               break;

            string id = param->getId();
            string value = param->getValue();

            string id_encoded = "";
            string value_encoded = "";

            URLEncode_str(id,id_encoded);
            URLEncode_str(value,value_encoded);

            aOutputVal += id_encoded + delim1 + value_encoded ;

            if(i < (num_parameters - 1))
                aOutputVal += delim;
 
        } 

    }

    //cout << "nsNKeyMESSGE_EXTENDED_LOGIN_RESPONSE::encode " << aOutputVal + "\n" << endl;

    eCKMessage::encode(aOutputVal);
}

eCKMessage_SECURID_REQUEST::eCKMessage_SECURID_REQUEST() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_SECURID_REQUEST::eCKMessage_SECURID_REQUEST:\n"));

    message_type = SECURID_REQUEST;
}

eCKMessage_SECURID_REQUEST::~eCKMessage_SECURID_REQUEST()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_SECURID_REQUEST::~eCKMessage_SECURID_REQUEST:\n"));

}

void eCKMessage_SECURID_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_SECURID_REQUEST::decode(string &aInputVal) 
{
      eCKMessage::decode(aInputVal);

}
eCKMessage_SECURID_RESPONSE::eCKMessage_SECURID_RESPONSE() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_SECURID_RESPONSE::eCKMessage_SECURID_RESPONSE:\n"));

    message_type = SECURID_RESPONSE;
}

eCKMessage_SECURID_RESPONSE::~eCKMessage_SECURID_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_SECURID_RESPONSE::~eCKMessage_SECURID_RESPONSE:\n"));

}

void eCKMessage_SECURID_RESPONSE::decode(string &aInputVal)
{
    aInputVal = "";
}
void eCKMessage_SECURID_RESPONSE::encode(string &aOutputVal)
{
    aOutputVal = "";

    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;

    string keyPin = "pin";
    string keyValue = "value";

    string lPin = getStringValue(keyPin);
    string lValue = getStringValue(keyValue);

    aOutputVal += keyPin + delim1 + lPin + delim + keyValue + delim1 + lValue ;

    eCKMessage::encode(aOutputVal);

}

eCKMessage_NEWPIN_REQUEST::eCKMessage_NEWPIN_REQUEST()
 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_NEWPIN_REQUEST::eCKMessage_NEWPIN_REQUEST:\n"));

    message_type = NEW_PIN_REQUEST;
}

eCKMessage_NEWPIN_REQUEST::~eCKMessage_NEWPIN_REQUEST()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_NEWPIN_REQUEST::~eCKMessage_NEWPIN_REQUEST:\n"));
}

void eCKMessage_NEWPIN_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_NEWPIN_REQUEST::decode(string &aInputVal)
{
    eCKMessage::decode(aInputVal);
}
eCKMessage_NEWPIN_RESPONSE::eCKMessage_NEWPIN_RESPONSE() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_NEWPIN_RESPONSE::eCKMessage_NEWPIN_RESPONSE:\n"));

    message_type = NEW_PIN_RESPONSE;
}

eCKMessage_NEWPIN_RESPONSE::~eCKMessage_NEWPIN_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_NEWPIN_RESPONSE::~eCKMessage_NEWPIN_RESPONSE:\n"));
}

void eCKMessage_NEWPIN_RESPONSE::decode(string &aInputVal)
{
}

void eCKMessage_NEWPIN_RESPONSE::encode(string &aOutputVal)
{
    aOutputVal = "";

    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;

    string pinKey = "new_pin";

    string lNewPin = getStringValue(pinKey);

    aOutputVal += pinKey + delim1 + lNewPin;

    eCKMessage::encode(aOutputVal);
}

eCKMessage_TOKEN_PDU_REQUEST::eCKMessage_TOKEN_PDU_REQUEST() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_TOKEN_PDU_REQUEST::eCKMessage_TOKEN_PDU_REQUEST:\n"));

    message_type = TOKEN_PDU_REQUEST;
}

eCKMessage_TOKEN_PDU_REQUEST::~eCKMessage_TOKEN_PDU_REQUEST()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_TOKEN_PDU_REQUEST::~eCKMessage_TOKEN_PDU_REQUEST:\n"));

}

void eCKMessage_TOKEN_PDU_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_TOKEN_PDU_REQUEST::decode(string &aInputVal)
{
    eCKMessage::decode(aInputVal);
}
 
eCKMessage_TOKEN_PDU_RESPONSE::eCKMessage_TOKEN_PDU_RESPONSE() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_TOKEN_PDU_RESPONSE::eCKMessage_TOKEN_PDU_RESPONSE:\n"));

    message_type = TOKEN_PDU_RESPONSE;
}

eCKMessage_TOKEN_PDU_RESPONSE::~eCKMessage_TOKEN_PDU_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_TOKEN_PDU_RESPONSE::~eCKMessage_TOKEN_PDU_RESPONSE:\n"));
}

void eCKMessage_TOKEN_PDU_RESPONSE::decode(string &aInputVal)
{
    aInputVal = "";
}

void eCKMessage_TOKEN_PDU_RESPONSE::encode(string &aOutputVal)
{
    aOutputVal = "";

    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;

    string pduKey = "pdu_data";
    string pduSizeKey = "pdu_size";


    string pduSize = getStringValue(pduSizeKey);
    string pduData = getStringValue(pduKey);

    aOutputVal += pduSizeKey + delim1 + pduSize + delim + pduKey + delim1 + pduData;

    eCKMessage::encode(aOutputVal);
}

eCKMessage_STATUS_UPDATE_REQUEST:: ~eCKMessage_STATUS_UPDATE_REQUEST()
{

    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_STATUS_UPDATE_REQUEST::~eCKMessage_STATUS_UPDATE_REQUEST:\n"));

}

eCKMessage_STATUS_UPDATE_REQUEST::eCKMessage_STATUS_UPDATE_REQUEST() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_STATUS_UPDATE_REQUEST::eCKMessage_STATUS_UPDATE_REQUEST:\n"));

    message_type = STATUS_UPDATE_REQUEST;
}

void eCKMessage_STATUS_UPDATE_REQUEST::encode(string &aOutputVal)
{
    aOutputVal = "";
}

void eCKMessage_STATUS_UPDATE_REQUEST::decode(string &aInputVal)
{
     eCKMessage::decode(aInputVal);
}
eCKMessage_STATUS_UPDATE_RESPONSE:: ~eCKMessage_STATUS_UPDATE_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_STATUS_UPDATE_RESPONSE::~eCKMessage_STATUS_UPDATE_RESPONSE:\n"));

}

eCKMessage_STATUS_UPDATE_RESPONSE::eCKMessage_STATUS_UPDATE_RESPONSE()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_STATUS_UPDATE_RESPONSE::eCKMessage_STATUS_UPDATE_RESPONSE:\n"));

    message_type = STATUS_UPDATE_RESPONSE;
}

void eCKMessage_STATUS_UPDATE_RESPONSE::decode(string &aInputVal)
{

}

void eCKMessage_STATUS_UPDATE_RESPONSE::encode(string &aOutputVal)
{
    aOutputVal = "";

    string delim = "&";
    string delim1 = "=";

    aOutputVal += "msg_type" + delim1 +  intToString(message_type) + delim;

    string key = "current_state";

    int currentState = getIntValue(key);

    aOutputVal += delim1 +  intToString(currentState);

    eCKMessage::encode(aOutputVal);

}

eCKMessage_END_OP::eCKMessage_END_OP() 
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_END_OP::eCKMessage_END_OP:\n"));

    message_type = END_OP;
}
eCKMessage_END_OP::~eCKMessage_END_OP()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_END_OP::~eCKMessage_END_OP:\n"));
}

void eCKMessage_END_OP::encode(string &aOutputVal)
{
}

void  eCKMessage_END_OP::decode(string &aInputVal)
{
   eCKMessage::decode(aInputVal);
}

eCKMessage_BEGIN_OP::eCKMessage_BEGIN_OP()
{
  PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_BEGIN_OP::eCKMessage_BEGIN_OP:\n"));

    message_type = BEGIN_OP;
}

eCKMessage_BEGIN_OP::~eCKMessage_BEGIN_OP()
{
    PR_LOG( nkeyLogMS, PR_LOG_DEBUG, ("eCKMessage_BEGIN_OP::~eCKMessage_BEGIN_OP:\n"));

}

void eCKMessage_BEGIN_OP::decode(string &aInputVal)
{
}

void eCKMessage_BEGIN_OP::encode(string &aOutputVal) 
{
    string extensions = "";
    string encoded = "";
    string delim = "=";
    string delim1 = "&";
    aOutputVal = "";

    string opcode = "";
    string key = "operation";

    opcode = getStringValue(key);

    aOutputVal += "msg_type" + delim +  intToString(message_type)+ delim1 + key + delim + opcode + delim1;

    aOutputVal += "extensions" + delim;

    vector<string>::iterator i;

    for(i = mExtensions.begin(); i!= mExtensions.end(); i++)
    {
        extensions += (*i) +  delim1;
    }  


    int size = extensions.length();

    if(aOutputVal[size  -1 ] == '&') 
    {
        extensions.erase(size -1);
    }

    URLEncode_str(extensions,encoded);
   
    aOutputVal += encoded;

    eCKMessage::encode(aOutputVal);
}

static int isAlphaNumeric (char ch)
{
    return ((ch >='a') && (ch <= 'z') ||   /* logical AND &&, OR || */
            (ch >='A') && (ch <= 'Z') ||
            (ch >='0') && (ch <= '9') );
}

static char bin2hex (unsigned char ch)
{
    ch = ch & 0x0f;
    ch += '0';
    if (ch > '9')
            ch += 7;
    return (ch);
}

static unsigned char hex2bin (unsigned char ch)
{
      if (ch > '9')
            ch = ch - 'A' + 10;
      else
            ch = ch - '0';
      return (ch);
}

void URLDecode_str(const string &data,string &output)
{
    output = "";

    const char *raw_data = data.c_str();

    //printf("raw_data %s \n",raw_data);

    string::size_type data_size = data.length();

    int size = (int) data_size;

    //printf("size of encode buffer will be %d \n",size);
    int decode_output_size = 4 * size + 1 ;

    unsigned char *decode_output = new unsigned char[decode_output_size];    

    int output_size = 0;

    URLDecode((char *)raw_data,decode_output, &output_size, decode_output_size);

    //printf("output %s \n",decode_output);
    output = (char *) decode_output;

    delete decode_output;
}
void URLDecode(char *data, unsigned char *buf, int *ret_len, int buff_len)
{
        int i;
        int len = (int )strlen(data);
        unsigned char *tmp;
        int sum = 0;

        int limit = buff_len - 1;

        if (len == 0)
            return ;
        tmp = (unsigned char *)buf;
        for (i = 0; i < len; i++) {

                if(sum == limit)
                {
                    tmp[sum] = '\0';
                    return;
                }

                if (data[i] == '+') {

                        tmp[sum++] = ' ';
                } else if (data[i] == '%') {
                        tmp[sum++] = (hex2bin(data[i+1]) << 4) + hex2bin(data[i+2]);
                        i+=2;
                } else {
                        tmp[sum++] = (unsigned char)data[i];
                }
        }

        tmp[sum] = '\0';

        *ret_len = sum;
}

void URLEncode_str(const string &data,string &output)
{

    output = "";

    unsigned char *raw_data =(unsigned char *) data.c_str();

    //printf("raw_data %s \n",raw_data);

    string::size_type data_size = data.length();

    int size = (int) data_size;

     //printf("size of encode buffer will be %d \n",size);
    int encode_output_size = 4 * size + 1;

    char *encode_output = new char[encode_output_size];

    int input_size = size;

    URLEncode(raw_data,encode_output, &input_size, encode_output_size);

    output = encode_output;

    //printf("output %s \n",encode_output);
    delete encode_output;
}


void URLEncode (unsigned char *data,char *buff, int *len,int buff_len)
{
        int i;
        unsigned char *buf = data;
        char *ret = buff;
        char *cur = ret;
        char *limit = NULL;

        limit = buff + buff_len - 1;

        for (i = 0; i < (int)*len; i ++) {

                if(cur + 3 >= limit)
                {
                   if(cur <= limit)
                   {
                       *cur = '\0';
                   }
                   return;

                }

                if (buf[i] == ' ') {
                        *cur++ = '+';
                } else if (isAlphaNumeric(buf[i])) {
                        *cur++ = buf[i];
                } else {
                        *cur++ = '%';
                        *cur++ = bin2hex(buf[i] >> 4);
                        *cur++ = bin2hex(buf[i]);
                }
        }
        *cur = '\0'; // null-terminated
}


nsNKeyREQUIRED_PARAMETER::~nsNKeyREQUIRED_PARAMETER()
{
}

void nsNKeyREQUIRED_PARAMETER::CleanuUpOptions()
{
}
