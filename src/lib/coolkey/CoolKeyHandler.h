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

#ifndef CoolKeyHandler_h__
#define CoolKeyHandler_h__

//#include <XptlBase.h>
//#include <ISocket.h>
//#include <IProxyDescriptor.h>
//#include <IFlapStreamOwner.h>
//#include <IFlapStream.h>
#include "cky_card.h"
#include "SlotUtils.h"
#include "NssHttpClient.h"
#include "CoolKey_Message.h"

// List of SNACs for CoolKey protocol. These will go somewhere else eventually //
#define COOLKEY_BEGIN_OP              2
#define COOLKEY_LOGIN_REQUEST         3
#define COOLKEY_LOGIN_RESPONSE        4
#define COOLKEY_SECURID_REQUEST       5
#define COOLKEY_SECURID_RESPONSE      6
#define COOLKEY_ASQ_REQUEST           7
#define COOLKEY_ASQ_RESPONSE          8
#define COOLKEY_TOKEN_PDU_REQUEST     9
#define COOLKEY_TOKEN_PDU_RESPONSE    10
#define COOLKEY_NEW_PASSWORD_REQUEST  11
#define COOLKEY_NEW_PASSWORD_RESPONSE 12
#define COOLKEY_END_OP                13
#define COOLKEY_STATUS_UPDATE_REQUEST   14
#define COOLKEY_STATUS_UPDATE_RESPONSE  15

// SNAC Response Error Codes:
#define STATUS_NO_ERROR                      0 // Operation Success (CMS6.3) 
#define STATUS_ERROR_SNAC                    1 // Error in SNAC
                                               // encoding/decoding (CMS6.3) 
#define STATUS_ERROR_SEC_INIT_UPDATE         2 // Error in creating secure
                                               //  channel (CMS6.3) 
#define STATUS_ERROR_CREATE_CARDMGR          3 // Error in creating a card
                                               // manager context (CMS6.3) 
#define STATUS_ERROR_MAC_RESET_PIN_PDU       4 // Error in mac'ing the reset
                                               // pin PDU (CMS6.3) 
#define STATUS_ERROR_MAC_CERT_PDU            5 // Error in mac'ing the
                                               // certificate PDU (CMS6.3) 
#define STATUS_ERROR_MAC_LIFESTYLE_PDU       6 // Error in mac'ing the
                                               // lifestyle PDU (CMS6.3) 
#define STATUS_ERROR_MAC_ENROLL_PDU          7 // Error in enrollment (CMS6.3) 
#define STATUS_ERROR_READ_OBJECT_PDU         8 // Error in reading object from
                                               // the token (CMS6.3) 
#define STATUS_ERROR_BAD_STATUS              9 // Error in status (CMS6.3) 
#define STATUS_ERROR_CA_RESPONSE            10 // Error in CoolKey RA/CA
                                               // communication (CMS6.3) 
#define STATUS_ERROR_READ_BUFFER_OVERFLOW   11 // Error in overflowing the
                                               // internal read buffer (CMS6.3) 
#define STATUS_ERROR_TOKEN_RESET_PIN_FAILED 12 // Error in reset pin in the
                                               // token (CMS6.3) 
#define STATUS_ERROR_CONNECTION             13 // Error in the connection
                                               // between CoolKey RA and the
                                               // token (CMS6.3) 
#define STATUS_ERROR_LOGIN                  14 // Error in the
                                               // screenname/password
                                               // authentication (CMS6.3) 
#define STATUS_ERROR_DB                     15 // Error in token database
                                               // (CMS6.3) 
#define STATUS_ERROR_TOKEN_DISABLED         16 // Token disabled by operator
                                               // (CMS6.3) 
#define STATUS_ERROR_SECURE_CHANNEL         17 // Error in establishing secure
                                               // channel (CMS6.3) 
#define STATUS_ERROR_MISCONFIGURATION       18 // Error in configuration of the
                                               // CoolKey backend (CMS6.3) 
#define STATUS_ERROR_UPGRADE_APPLET         19 // Error in the applet upgrade
                                               // operation (CMS6.3) 
#define STATUS_ERROR_KEY_CHANGE_OVER        20 // Error in the key change over
                                               // operation (CMS6.3) 

enum {
  ENROLL = 1,
  UNBLOCK,
  RESET_PIN,
  RENEW,
  FORMAT
};

enum {
  CONFIG_ERROR = 44,
  CARD_CONTEXT_ERROR,
  PDU_WRITER_ERROR,
  HTTP_CLIENT_ERROR,
  CONN_READER_ERROR

};

class CoolKeyHandler 
{
 public:
  CoolKeyHandler();
  virtual ~CoolKeyHandler();
  HRESULT Init(const CoolKey *aKey, const char *aScreenName, const char *aPIN,const char *aScreenNamePwd, const char *aTokenCode,int op = 0);
  HRESULT Enroll(const char *aTokenType);
  HRESULT ResetPIN(void);
  HRESULT Renew(void);
  HRESULT Format(const char *aTokenType);
  HRESULT Disconnect();
  

  //Http Related


  NSS_HTTP_HANDLE getHttpHandle() { return mHttp_handle;}
  HRESULT ProcessMessageHttp(eCKMessage *msg);
eCKMessage *AllocateMessage(eCKMessage::sntype type,unsigned char *data, unsigned size);

  HRESULT OnConnectImpl();
  HRESULT OnDisConnectImpl();

  HRESULT CloseConnection();

  //End Http Related

  void setCancelled() { mCancelled = true;}
  bool isCancelled() { return mCancelled;}

  CKYCardContext* GetCardContext() { return mCardContext;}
  CKYCardConnection* GetCardConnection() { return  mCardConnection;}

  AutoCoolKey *GetAutoCoolKey() { return &mKey;}

  char* GetScreenName() { return mCharScreenName;}
  char* GetPIN() { return mCharPIN;}

  void AddRef();
  void Release();

  HRESULT SetAuthParameter(const char *param_id, const char *value);
  HRESULT CancelAuthParameters();
  HRESULT SetScreenName(const char *screenName);
  HRESULT SetPassword(const char *password);
  HRESULT SetTokenPin(const char *pin);


  nsNKeyREQUIRED_PARAMETERS_LIST *GetAuthParametersList() { return &mReqParamList;}


  PRLock    *mDataLock;
  PRCondVar *mDataCondVar;




 private:

  HRESULT SendSignOn();
  HRESULT BeginOpRequest(int action);
  
 
  //static void NotifyEndResult(CoolKeyHandler* context, int operation, int result, int description);
  
  HRESULT SendUsernameAndPW();
  HRESULT SendNewPassword();
  HRESULT SendSecurID(const char* securID);
 
  bool ConnectToReader(const char* readerName);
  void DisconnectFromReader();
 
  int m_dwRef;
  
  int mState;
  CKYCardContext* mCardContext;
  CKYCardConnection* mCardConnection;

  AutoCoolKey mKey;

 
  bool mReceivedEndOp;

  bool mHttpDisconnected;

  bool mCancelled;


  char *mAppDir;
 
  int mPort;
  class PDUWriterThread* mPDUWriter;


 //HTTP versions of string constants

	
 
  char* mCharScreenName;
  char* mCharPIN;
  char* mCharScreenNamePwd;  
  char* mCharHostName;
 
  char* mCharTokenType;
  char* mCharTokenCode;


  void CollectPreferences();    

  //String Constants end


  bool mStatusRequest;

  // Related to getting values from Scriptable Popups

  HRESULT GetAuthDataFromUser(const char* ui);

  //HRESULT GetUidPwordFromUser();
  //HRESULT GetTokenPinFromUser();

  //Http Related stuff

  int mHttpRequestTimeout;
  int mSSL;
  char *mRAUrl;
  NSS_HTTP_HANDLE mHttp_handle;

  // Auth info

  nsNKeyREQUIRED_PARAMETERS_LIST mReqParamList;  

  static bool HttpChunkedEntityCB(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status);


  bool HttpChunkedEntityCBImpl(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status);

  HRESULT HttpSendSecurID(eCKMessage_SECURID_REQUEST *req);
  HRESULT HttpSendNewPin(eCKMessage_NEWPIN_REQUEST * req);
  HRESULT HttpSendAuthResponse(CoolKeyHandler *context,eCKMessage_EXTENDED_LOGIN_REQUEST *req);
  HRESULT HttpBeginOpRequest();
  HRESULT HttpSendUsernameAndPW();
  HRESULT HttpProcessStatusUpdate(eCKMessage_STATUS_UPDATE_REQUEST * msg);
  HRESULT HttpDisconnect(int reason=0);

  static void HttpProcessTokenPDU(CoolKeyHandler *context,eCKMessage_TOKEN_PDU_REQUEST *req);
  static void HttpProcessEndOp(CoolKeyHandler* context, eCKMessage_END_OP *end);


};




void DisplayEndDialog(int operation, int result, int description);
CoolKeyInfo *CKHGetCoolKeyInfo(PK11SlotInfo *aSlot);
unsigned int CKHGetInfoFlags(PK11SlotInfo *aSlot);

#endif /* CoolKeyHandler.h__ */

