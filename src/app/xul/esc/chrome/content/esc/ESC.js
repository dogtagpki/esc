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

// initialize coolkey globals

var gStringBundle = null;
var netkey;

var keyUITable = new Array();
var keyTypeTable = new Array();
var curChildWindow = null;

var gEnrollmentPage = 0;
var gAdminPage = 0;
var gFactoryMode = 0;
var gHiddenPage = 0;
var gExternalUI = 0;

loadStringBundle();

//ESC constants

const  KEY_ISSUER_URL    = "keyIssuerUrl";
const  KEY_ISSUER        = "keyIssuer";
const  TPS_URL           = "Operation";
const  TPS_UI            = "UI";
const  SERVICES_TAG      = "Services";
const  ISSUER_TAG        = "IssuerName"; 
const  SERVICE_INFO_TAG  = "ServiceInfo";
const  ENROLLED_TOKEN_BROWSER_URL = "EnrolledTokenBrowserURL";
const  ENROLLED_TOKEN_URL = "EnrolledTokenURL";
const  TOKEN_TYPE = "TokenType";
const  RESET_PHONE_HOME  = "ResetPhoneHome";

// Config params

const  ESC_IGNORE_TOKEN_BROWSER_URL = "esc.ignore.token.browser.url";
const  ESC_TOKEN_BROWSER_URL_ESTABLISHED = "esc.token.browser.established";
const  ESC_IGNORE_KEY_ISSUER_INFO = "esc.ignore.key.issuer.info";
const  ESC_FACE_TO_FACE_MODE = "esc.face.to.face.mode";
const  ESC_SECURITY_URL="esc.security.url";
const  ESC_SECURE_URL="esc.secure.url";
const  ESC_GLOBAL_PHONE_HOME_URL= "esc.global.phone.home.url";
const  ESC_HIDE_FORMAT="esc.hide.format";

const  CLEAN_TOKEN = "cleanToken";
const  UNINITIALIZED        = 1;
const  UNINITIALIZED_NOAPPLET = 2;
const  ESC_ENROLL_WIDTH  = 600;
const  ESC_ENROLL_HEIGHT = 570;

//Enrolled Token Browser constants

const MAC_PROG_OPEN = "/usr/bin/open";
const LINUX_PROG_OPEN = "/usr/bin/gnome-open";
const WIN_XP_PROG_OPEN = "C:\\Windows\\system32\\cmd.exe";
const WIN_2000_PROG_OPEN = "C:\\WINNT\\system32\\cmd.exe";

//Window names
const ENROLL_WINDOW      = "esc.xul";
const ADMIN_WINDOW       = "settings.xul";
const HIDDEN_WINDOW      = "hiddenWindow.xul";
const SECURITY_WINDOW    = "security.xul";


//Log level constants

const   PR_LOG_NONE = 0;
const   PR_LOG_ALWAYS = 1;
const   PR_LOG_ERROR = 2;
const   PR_LOG_WARNING = 3;
const   PR_LOG_DEBUG = 4;

const   PR_LOG_NOTICE = 4;
const   PR_LOG_WARN =   3;
const   PR_LOG_MIN =    4;
const   PR_LOG_MAX =    4; 

function getUIForKey(aKeyID)
{
    return keyUITable[aKeyID];
}

function getTypeForKey(aKeyID)
{
    return keyTypeTable[aKeyID];
}

//
// Notify callback for GECKO
//
function jsNotify()  {}

jsNotify.prototype = {

  rhNotifyKeyStateChange: function(aKeyType,aKeyID,aKeyState,aData,strData)
  {
    OnCoolKeyStateChange(aKeyType, aKeyID, aKeyState, aData,strData);
  },

  QueryInterface: function(iid)
  {
     //  alert("iid: " + iid); 
     if(!iid.equals(Components.interfaces.rhIKeyNotify) &&
         !iid.equals(Components.interfaces.nsISupports))
      {
          MyAlert(getBundleString("errorJsNotifyInterface"));
          throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
  }
};

//
// Attach to the object.
//
  var gNotify=null;

  try {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey = Components.classes["@redhat.com/rhCoolKey"].getService();
    netkey = netkey.QueryInterface(Components.interfaces.rhICoolKey);
    gNotify = new jsNotify;
    netkey.rhCoolKeySetNotifyCallback(gNotify);

    var logFileName = GetESCLogPathName("esc.log");
    netkey.CoolKeyInitializeLog(logFileName, 1000);
  } catch(e) {
     MyAlert(getBundleString("errorUniversalXPConnect") + e);
  }

//
// unregister our notify event
//
function cleanup()
{
    TrayRemoveWindow(null);
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      netkey.rhCoolKeyUnSetNotifyCallback(gNotify);
    } catch(e) {
     MyAlert(getBundleString("errorUniversalXPConnect")  + e);
    }

}

var gKeyEnrollmentType = "userKey";

var gCurrentSelectedRow = null;

var gCurKeyType = null;
var gCurKeyID = null;

////////////////////////////////////////////////////////////////
//
// Utility functions specific to this page.
//
////////////////////////////////////////////////////////////////

// List of Error Messages to be printed out

var Status_Messages = new Array(
    getBundleString("errorNone"),
    getBundleString("errorInternalServer"),
    getBundleString("errorInternalServer"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorProblemResetTokenPin"),
    getBundleString("errorInternalServer"),
    getBundleString("errorLifeCyclePDU"),
    getBundleString("errorTokenEnrollment"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorInternalServer"),
    getBundleString("errorInternalServer"),
    getBundleString("errorInternalServer"),
    getBundleString("errorInternalServer"),
    getBundleString("errorTermSecureConn"),
    getBundleString("errorAuthFailure"),
    getBundleString("errorInternalServer"),
    getBundleString("errorTokenDisabled"),
    getBundleString("errorSecureChannel"),
    getBundleString("errorServerMisconfig"),
    getBundleString("errorTokenUpgrade"),
    getBundleString("errorInternalServer"),
    getBundleString("errorExternalAuth"),
    getBundleString("errorInvalidTokenType"),
    getBundleString("errorInvalidTokenTypeParams"),
    getBundleString("errorCannotPublish"),
    getBundleString("errorCommTokenDB"),
    getBundleString("errorTokenSuspended"),
    getBundleString("errorPinResetable"),
    getBundleString("errorConnLost"),
    getBundleString("errorEntryTokenDB"),
    getBundleString("errorNoTokenState"),
    getBundleString("errorInvalidLostTokenReason"),
    getBundleString("errorTokenUnusable"),
    getBundleString("errorNoInactiveToken"),
    getBundleString("errorProcessMultiTokens"),
    getBundleString("errorTokenTerminated"),
    getBundleString("errorInternalServer"),
    getBundleString("errorKeyRecoveryFailed"),
    getBundleString("errorInternalServer"),
    getBundleString("errorKeyArchival"),
    getBundleString("errorConnTKS"),
    getBundleString("errorFailUpdateTokenDB"),
    getBundleString("errorCertRevocation"),
    getBundleString("errorNotOwnToken"),
    getBundleString("errorESCMisconfigured"),
    getBundleString("errorESCNoCommCardReader"),
    getBundleString("errorESCNoTokenSession"),
    getBundleString("errorESCNoTalkTPS"),
    getBundleString("errorESCNoTalkTokenReader")
);

// Main function that oversees obtaining Phone Home Info from the Server

function DoPhoneHome(keyType,keyID)
{
  CoolKeyLogMsg(PR_LOG_ALWAYS,"Attempting to phone home for Key " + keyID); 
  var callback = function (aResult) {

    recordMessage("In DoPhoneHome callback");

    var issuer = "";
    if(aResult == true)
    {
        issuer = GetCoolKeyIssuer(keyType,keyID);       
        if(!issuer)
            issuer = getBundleString("unknownIssuer");
        recordMessage("In DoPhoneHome callback success issuer " + issuer);
        TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
        LogKeyInfo(keyType,keyID,"Key Inserted ...");
        UpdateRowWithPhoneHomeData(keyType,keyID);
        recordMessage("cached issuer " + issuer);
        var browserURL =  GetCachedEnrolledTokenBrowserURL(keyID);
        recordMessage("Cached browserURL " + browserURL);

        if(browserURL)
        {
            DoCoolKeySetConfigValue(ESC_TOKEN_BROWSER_URL_ESTABLISHED,"yes");
            DoHandleEnrolledBrowserLaunch();
        }

    }
    else
    {
  
        issuer = GetCoolKeyIssuer(keyType,keyID);
        if(!issuer)
            issuer = getBundleString("unknownIssuer");
        recordMessage("Phone home callback failed , issuer " + issuer);
        TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
        LogKeyInfo(keyType,keyID,"Key Inserted ...");
    }
  }

  recordMessage("Attempting phone home...");

  var home = DoCoolKeyGetIssuerUrl(keyType,keyID);

  recordMessage("Returned IssuerURL " + home);

  if(IsPhoneHomeCached(keyID) && home)
  {
      var phoneHomeURI = GetCachedPhoneHomeURL(keyID);

      recordMessage("Phone home info cached...");
      issuer = GetCoolKeyIssuer(keyType,keyID);
      TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
      LogKeyInfo(keyType,keyID,"Key Inserted ...");

      var launchBrowserURL =  GetCachedEnrolledTokenBrowserURL(keyID);

      if(launchBrowserURL && GetCoolKeyIsEnrolled(keyType, keyID) )
      {
          recordMessage("About to attempt to launch Browser URL.");
          openEnrolledTokenURLBrowser(keyID); 
      }

      return true;
  }

  //Check for optional global phone home url.


  if(!home)   {
      home = GetGlobalPhoneHomeUrl(keyType,keyID);
  }

  var homeRes = false;

  if(home)
  {
      recordMessage("About to actually phone home for real...");
      homeRes =  phoneHome(home,keyID,callback);
  }

  // Launch the config dialog only if we can't
  // Phone Home and we are not in the special security mode

  if(!homeRes && !CheckForSecurityMode())
  {
      recordMessage("About to launch CONFIG , non secmode...");

      launchCONFIG(keyType,keyID);
  }

  return homeRes;
}

//Get global phone home url if pref is set 

function GetGlobalPhoneHomeUrl(keyType,keyID)
{

   var globalIssuerURL=null;

   globalIssuerURL = DoCoolKeyGetConfigValue(ESC_GLOBAL_PHONE_HOME_URL);

   return globalIssuerURL;

}

//Test Phone Home url in config UI

function DoPhoneHomeTest()
{
    // Test out user supplied phone home url 

    var name = this.name;

    var callback = function (aResult) {

        if(aResult == true)
        {
            MyAlert(getBundleString("tpsConfigSuccess"));
        }
        else
        {
            MyAlert(getBundleString("tpsConfigError"));
        }
    }

    var url = document.getElementById("phonehomeuri");

    if(!url)
    {
        MyAlert(getBundleString("noTpsConfigUrl"));
        return;
    }

    if(!url.value)
    {
        MyAlert(getBundleString("noTpsConfigUrl"));
        return;
    }

    MyAlert(getBundleString("tpsConfigTest") + " " + url.value);

    if(name)
    {
         phoneHome(url.value,name,callback);
    }
    else
    {
        MyAlert(getBundleString("tpsConfigError"));
    }
}

function DoPhoneHomeConfigClose()
{
    var name = this.name;

    recordMessage("DoPhoneHomeConfigClose()  name " + name + " opener " + window.opener);
    if(window.opener && name)
    {
        window.opener.UpdateRowWithPhoneHomeData(1,name);
    }

    window.close();
}

//Commit configuration values to preferences

function commitConfigValues()
{
    if(netkey)
    {
        var tps_uri_box = document.getElementById("tpsuri");
        var esc_ui_box = document.getElementById("tpsenrolluri");

        var  tps_uri_value = null;
        var  esc_enroll_value = null;

        if(tps_uri_box)
        {
             tps_uri_value = tps_uri_box.value;
        }

        if(esc_ui_box)
        {
            esc_enroll_value = esc_ui_box.value;
        }

        if(!tps_uri_value )
        {
            MyAlert(getBundleString("tpsURIMustHaveValue"));
            return;
        }
             var res = 0;

             try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                res = netkey.SetCoolKeyConfigValue("esc.tps.url",tps_uri_value);

            } catch(e) {
                MyAlert(getBundleString("errorSetConfigValue") + e);
                return;
            }

            try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                res = netkey.SetCoolKeyConfigValue("esc.enroll.ui.url",esc_enroll_value);
                grantPrivilegesURL(esc_enroll_value);

            } catch(e) {
                MyAlert(getBundleString("errorSetConfigValue") + e);
                return;
            }

            MyAlert(getBundleString("configChangesSubmitted"));
    }
}

function InitializePhoneHomeConfigUI()
{
    var uri_box = document.getElementById("phonehomeuri");
    
    if(uri_box)
        uri_box.focus();

    window.sizeToContent();
}

//Populate existing values for the configuration UI
function InitializeConfigUI()
{
   var tps_uri = null;

   if(netkey)
   {
             try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                tps_uri = netkey.GetCoolKeyConfigValue("esc.tps.url");

            } catch(e) {
                MyAlert(getBundleString("errorSetConfigValue") + e);
            }

        if(tps_uri)
        {
            var tps_uri_box = document.getElementById("tpsuri");

            if(tps_uri_box)
            {
                tps_uri_box.setAttribute("value",tps_uri);
            }
        }

        var esc_enroll_uri = null;

        try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    
                esc_enroll_uri = netkey.GetCoolKeyConfigValue("esc.enroll.ui.url");
    
            } catch(e) {
                MyAlert(getBundleString("errorSetConfigValue") + e);
       }

       if(esc_enroll_uri)
       {
           var esc_enroll_box = document.getElementById("tpsenrolluri");

           if(esc_enroll_box)
           {
               esc_enroll_box.setAttribute("value",esc_enroll_uri);

           }

       }

   }
}

//String bundling related functions

function loadStringBundle()
{
    gStringBundle = document.getElementById("esc_strings");
}

function getBundleString(string_id)
{
    var str = null;

    if(!string_id || !gStringBundle)
       return null;

    str = gStringBundle.getString(string_id);

    return str;
}

function GetAuthDataFromPopUp(aKeyType,aKeyID,aUiData)
{
   if(!gAdminPage)
      return;

   keyUITable[aKeyID] = aUiData;
   keyTypeTable[aKeyID] = aKeyType;

   var child =  window.open("chrome://esc/content/GenericAuth.xul", aKeyID, "chrome,centerscreen,width=400,height=250");
 
   curChildWindow = child; 
}

function CoolKeySetDataValue(aKeyType,aKeyID,name,value)
{
        if(netkey)
        {
             try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                netkey.SetCoolKeyDataValue(aKeyType,aKeyID,name,value);


            } catch(e) {
                //MyAlert(getBundleString("errorSetDataValue") + e);
            }
        }

}
 
function CoolKeySetTokenPin(pin)
{
        if(netkey)
        {
             try { 
                netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                netkey.SetCoolKeyDataValue(gCurKeyType,gCurKeyID,"TokenPin",pin);

            } catch(e) {
                MyAlert(getBundleString("errorSetDataValue") + e);
            }
        }
}

function CoolKeySetUidPassword(uid,pwd)
{
      if(netkey)
      {
          try {
              netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
              netkey.SetCoolKeyDataValue(gCurKeyType,gCurKeyID,"UserId",uid);
              netkey.SetCoolKeyDataValue(gCurKeyType,gCurKeyID,"Password",pwd);

          } catch(e) {
              MyAlert(getBundleString("errorSetDataValue") + e);
          }

      }

}
 
function TestStatusMessages()
{
    for(i = 0 ; i < 49; i++)
    {
        MyAlert( i + " " + Status_Messages[i]);
    }
}
 
function MyGetErrorMessage(status_code)
{
   var result =  getBundleString("errorInternalServer");

   if(status_code < 0 && status_code >= Status_Messages.length)
   {
       return result;
   }   
      
   return Status_Messages[status_code];
}   

function KeyToRowID(keyType, keyID)
{
  return keyType + "--" + keyID;
}

function CellIDToKeyInfo(cellID)
{
  return cellID.split("--");
}

function KeyToCellID(keyType,keyID,cellID)
{
  return keyType + "--" + keyID + "--" + cellID;
}

function RowIDToKeyInfo(rowID)
{
  return rowID.split("--");
}

function GetRowForKey(keyType, keyID)
{
  return document.getElementById(KeyToRowID(keyType, keyID));
}

function ReportException(msg, e)
{
  MyAlert(msg + " " + e.description + "(" + e.number + ")");
}

function GetCoolKeyStatus(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.GetCoolKeyStatus(keyType, keyID);
  } catch (e) {
    ReportException(getBundleString("errorCoolKeyGetStatus"), e);
    return 0;
  }
}

function ConfigValueWithKeyID(keyID,configValue)
{
    if(!configValue || !keyID)
        return null;

    return configValue + "-" + keyID;
}
 
function GetCoolKeyPolicy(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.GetCoolKeyPolicy(keyType, keyID);
  } catch (e) {
    //ReportException(getBundleString("errorCoolKeyGetPolicy"), e);
    return "";
  }
}

function GetCoolKeyRequiresAuth(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.GetCoolKeyRequiresAuthentication(keyType, keyID);
  } catch(e) {
    ReportException(getBundleString("errorCoolKeyRequiresAuth"), e);
    return false;
  }
}

function AuthenticateCoolKey(keyType, keyID,aPIN)
{   
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.AuthenticateCoolKey(keyType, keyID,"netscape");
  } catch(e) {
    ReportException(getBundleString("errorAuthCoolKey"), e);
    return false;
  }
}

function GetCoolKeyIsAuthed(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.GetCoolKeyIsAuthenticated(keyType, keyID);
  } catch(e) {
    ReportException(getBundleString("errorCoolKeyIsAuth"), e);
    return false;
  }
}

function GetCoolKeyIssuer(keyType,keyID)
{
    var issuer = null;

     issuer = GetCachedIssuer(keyID);

     // Now try to read off the certs if applicable

    if(!issuer && (GetStatusForKeyID(keyType, keyID) == getBundleString("statusEnrolled")))
    {
        try {
            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
            issuer = netkey.GetCoolKeyIssuer(keyType,keyID);

            var issuer_url = ConfigValueWithKeyID(keyID,KEY_ISSUER_URL);
            var issuer_url_value = null;

            if(issuer_url)
            {
                 issuer_url_value = DoCoolKeyGetConfigValue(issuer_url);
            }

            if(issuer && !issuer_url_value)
            {
                 var issuer_config_value = ConfigValueWithKeyID(keyID,KEY_ISSUER);
                 if(issuer_config_value)
                 {
                     DoCoolKeySetConfigValue(issuer_config_value,issuer);
                 }
            }
        } catch (e)
        {
            issuer = null;
        }
    }


    if(!issuer)
        issuer = getBundleString("unknownIssuer");

    return issuer;
}

function GetCoolKeyIssuedTo(keyType,keyID)
{
    var keyStatus = GetStatusForKeyID(keyType, keyID);

    var defaultIssuedTo = getBundleString("statusUnknown");
    var issuedTo = null;

    try {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        issuedTo = netkey.GetCoolKeyIssuedTo(keyType,keyID);

    } catch (e)
    {
    }

    if(!issuedTo || issuedTo.length <= 3 || issuedTo.indexOf("CoolKey") != -1)
        issuedTo = defaultIssuedTo;    

    return issuedTo;
}

function DoShowAdvancedInfo()
{
    var arr = GetAvailableCoolKeys();
    var coolkeyVersion = GetCoolKeyVersion();

    var textDump="";
    textDump +=  getBundleString("diagnosticsReport") + "\n\n";
    textDump += "***" + getBundleString("diagnosticsSystemInfo") + "***" + "\n\n";
    var agent = getBundleString("diagnosticsSoftVersioInfo") + " " +  navigator.userAgent.toLowerCase() + "\n";

    textDump +=  " " + getBundleString("coolkeyComponentVersion");
    textDump += " " + coolkeyVersion + "\n";
    textDump += " " + agent + "\n";
    textDump += "***" + getBundleString("diagnosticsDetails") + "***" + "\n\n";
    textDump += "  " +  getBundleString("coolkeyDetectedNumberKeys") + " ";
    textDump +=   arr.length + "\n\n" ;

    var i = 0;
    for(i = 0 ; i < arr.length ; i++)
    {
       keyID = arr[i][1];
       keyType = arr[i][0];

       var appletVerMaj = DoGetCoolKeyGetAppletVer(keyType, keyID , true);
       var appletVerMin = DoGetCoolKeyGetAppletVer(keyType, keyID, false);

       var issuer = GetCoolKeyIssuer(keyType,keyID);
       if(!issuer)
           issuer = getBundleString("unknownIssuer");

       var cardName = DoCoolKeyGetTokenName(keyType,keyID);

       if(!cardName)
           cardName = i;

       textDump += getBundleString("smartCardU") + "  " + cardName + ":"  + "\n\n";

       textDump += "  " + getBundleString("appletVersion") + " " + appletVerMaj + "." + appletVerMin + "\n";

       var status =  GetStatusForKeyID(keyType, keyID);
       var atr =     DoCoolKeyGetATR(keyType,keyID);

       textDump += "  " + getBundleString("keyID") + " " + " " +  keyID  + "\n";
       textDump += "  " + getBundleString("status") + " " + " " + status + "\n";
       textDump += "  " + getBundleString("issuer") + " " + " " + issuer + "\n";
       textDump += "  " + getBundleString("atr") + " " + " " + atr + "\n";

       var tpsURI = GetCachedTPSURL(keyID);
       var tpsUI  = GetCachedTPSUI(keyID);
       var phoneHomeURI = GetCachedPhoneHomeURL(keyID);

       if(!tpsURI)
           tpsURI="";
      
       if(!tpsUI)
           tpsUI = "";

       if(!phoneHomeURI)
           phoneHomeURI = ""; 

       textDump += "  " + getBundleString("tpsPhoneHomeURL") + " " + " " + phoneHomeURI + "\n";
       textDump += "  " + getBundleString("tpsURI") + " " + " " + tpsURI + "\n";
       textDump += "  " +getBundleString("tpsUI") + " " + " " + tpsUI + "\n";
       textDump += "\n";

       var nicknames  = GetCoolKeyCertNicknames(keyType,keyID);
       if(nicknames && nicknames.length)
       {
        textDump += "  " + getBundleString("certsOnToken")  + " \n\n";
       }

       if(nicknames)
       {
           var cert_info = null;
           for (j = 0; j < nicknames.length ; j ++)
           {
                textDump += "    " + getBundleString("certificateNickname") + " " + nicknames[j] + " \n\n";

                cert_info = GetCoolKeyCertInfo(keyType,keyID,nicknames[j]);
                var cert_split = cert_info.split("\n");

                if(cert_split.length)
                {
                    textDump += "      " + getBundleString("certIssuedTo") + " " + cert_split[0] + "\n";
                    textDump += "      " + getBundleString("certIssuedBy") + " " + cert_split[1] + "\n";
                    textDump += "      " + getBundleString("certValidityFrom") + " " + cert_split[2] + "\n";
                    textDump += "      " + getBundleString("certValidityTo") + " " + cert_split[3] + "\n";
                    textDump += "      " + getBundleString("certSerialNumber") + " " + cert_split[4] + "\n";
                    textDump += "\n";
                }
           }
       }

    }

    if(i <= 0)
    {
        textDump += "\n";
    }

    var lines = null;
    var lines = ReadESCLog();

    textDump += "***" + getBundleString("escLogEntries") + "***"  + "\n";

    if(lines)
    {
        for(i = 0 ; i < lines.length ; i++)
        {
             textDump += lines[i] + "\n";
        }
    }
    else
    {
         textDump += getBundleString("noLogFileOrData");
    }

    var wnd = window.openDialog("chrome://esc/content/advancedinfo.xul","Info","chrome,centerscreen,width=600,height=500,modal=yes",textDump);

}

function GetCoolKeyCertNicknames(aKeyType,aKeyId)
{
  try {
    var nameArr;
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var inArray = netkey.GetCoolKeyCertNicknames(aKeyType,aKeyId, {} );
      nameArr = new Array(inArray.length);
      var i;

      for (i=0; i < nameArr.length; i++) {
          nameArr[i] = new Array(  inArray[i]);
      }
    return nameArr;
  } catch(e) {
    ReportException(getBundleString("errorCoolKeyCertNicknames"), e);
    return [];
  }
}

function GetCoolKeyVersion()
{
    var result = null;

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      result = netkey.GetCoolKeyVersion();
    return result;
  } catch(e) {

    return result;
  }
}

function GetCoolKeyCertInfo(aKeyType,aKeyId,aCertNickname)
{
  try {
    var info = null;
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      info = netkey.GetCoolKeyCertInfo(aKeyType,aKeyId, aCertNickname );
      return info;
  } catch(e) {
    ReportException(getBundleString("errorCoolKeyCertInfo"), e);
    return "";
  }
}

function GetAvailableCoolKeys()
{
  try {
    var keyArr;

      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var inArray = netkey.GetAvailableCoolKeys( {} );
      keyArr = new Array(inArray.length);
      var i;

      for (i=0; i < keyArr.length; i++) {
	keyArr[i] = new Array( "1", inArray[i]);
      }
    return keyArr;
  } catch(e) {
    ReportException(getBundleString("errorGetAvailCoolKeys"), e);
    return [];
  }
}

function CoolKeyLogMsg(aLogLevel,aMessage)
{
    if(!aMessage)
        return;

    try { 
         netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
         netkey.CoolKeyLogMsg(aLogLevel,aMessage);
         } catch(e) {
         }
}

function ChallengeCoolKey(keyType, keyID, data)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.ChallengeCoolKey(keyType, keyID, data);
  } catch(e) {
    return [];
  }
}

function EnrollCoolKey(keyType, keyID, enrollmentType, screenname, pin,screennamepwd,tokencode)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey.EnrollCoolKey(keyType, keyID, enrollmentType, screenname, pin,screennamepwd,tokencode);
  } catch(e) {
    ReportException(getBundleString("errorEnollCoolKey"), e);
    return false;
  }

  return true;
}

function GetCoolKeyIsEnrolled(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    return netkey.GetCoolKeyIsEnrolled(keyType, keyID);
  } catch(e) {
    ReportException(getBundleString("errorCoolKeyIsEnrolled"), e);
    return false;
  }
}

function ResetCoolKeyPIN(keyType, keyID, screenname, pin,screennamepwd)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey.ResetCoolKeyPIN(keyType, keyID, screenname, pin,screennamepwd);
  } catch(e) {
    ReportException(getBundleString("errorResetCoolKeyPIN"), e);
    return false;
  }
  return true;
}
function FormatCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey.FormatCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode);
  } catch(e) {
    ReportException(getBundleString("errorFormatCoolKey"), e);
    return false;
  }
  return true;
}

function CancelCoolKeyOperation(keyType, keyID)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey.CancelCoolKeyOperation(keyType, keyID);
  } catch(e) {
    ReportException(getBundleString("errorCancelCoolKey"), e);
    return false;
  }
  return true;
}

//
// /GECKO DOM functions.
//

function RemoveAdminRow(row)
{
   var listbox = document.getElementById("AdminBindingList");

   if(listbox)
       listbox.removeChild(row);
}

function GetCell(row, index)
{
   var cell;

   cell = row.cells[index];
   return cell;
}

function GetNode(parent, index)
{
   var node;
   node = parent.childNodes[index];
   return node;
}

function CreateListBoxRow()
{
    var listitem = document.createElement("listitem");
    return listitem;
}

function InsertListBoxRow(listbox)
{
    if(!listbox)
        return null;

    var listitem = document.createElement("listitem");

    if(!listitem)
        return null;

    listbox.appendChild(listitem);

    return listitem;
}

function SelectImageForKeyStatus(keyStatus,observeBusy,doubleSize)
{
  var image_src = "";

  if(observeBusy && (keyStatus == getBundleString("statusBusy")))
  {
      return "throbber-anim5.gif";
  }
  if(keyStatus == getBundleString("statusUnavailable"))
  {
      return "";
  }
  if(keyStatus == getBundleString("statusEnrolled"))
  {
      image_src = "enrolled-key";
  }
  else
  {
          if(keyStatus == getBundleString("statusUninitialized"))
              image_src = "initializecard";
           else
               if(keyStatus == getBundleString("statusNoApplet"))
                   image_src = "blank-card";
  }

  if(image_src == "")
    return image_src;

  var suffix = "";
  if(doubleSize)
     suffix = "x2";

  return image_src + suffix + ".png";
}

function CreateImage(aImageName)
{
    if(!aImageName)
        return null;

    var theImage= document.createElement("image");

    if(theImage)
    {
        theImage.setAttribute("src",aImageName);
        theImage.setAttribute("autostretch","never");
    }

    return theImage;
}

function InsertRow(table)
{
    var row;

    row = table.insertRow(table.rows.length);
    return row;
}

function InsertCell(row)
{
    var cell;
    cell = row.insertCell(row.cells.length);
    return cell;
}

function InsertListCell(listboxrow)
{
    if(!listboxrow)
        return null;

    var listcell = document.createElement("listcell");

    if(!listcell)
        return null;

    listboxrow.appendChild(listcell);

    return listcell;
}

function RemoveAllChildNodes(parent)
{
   var numChildren = parent.childNodes.length;
   var i;

   i = numChildren;
   while (numChildren)
   {
      parent.removeChild(GetNode(parent,0));
      numChildren--;
   }
}

function ShowEnrollmentAnimation(keyType,keyID,starting)
{
    if(!keyType || !keyID)
        return;

    var cylon = document.getElementById("cylonImage");
    var progmeterId =  KeyToProgressBarID(keyType, keyID) ;

    if(!progmeterId)
        return;

    var progressMeter = document.getElementById(progmeterId);

    if(starting)
    {
        if(progressMeter)
            progressMeter.setAttribute("hidden","false");
    }
    else
    {
        if(progressMeter)
            progressMeter.setAttribute("hidden","true");
        if(cylon)
           cylon.setAttribute("hidden","true");
    }
}

function UpdateSecurityPage()
{
     var securityURL =   DoCoolKeyGetConfigValue(ESC_SECURITY_URL);

     if(securityURL)
     {
          var ui_id = document.getElementById("security-ui");
          if(ui_id)
          {
              grantPrivilegesURL(securityURL);
              ui_id.setAttribute("src",securityURL); 
          }
     }

     window.setTimeout('GrantSecurityPagesPrivileges()',5000);
}

function GrantSecurityPagesPrivileges()
{
    var curSecUrl = null;
    var i = 1;

    var capability = "capability.principal.codebase";
    var uni_connect = "UniversalXPConnect";
    var granted     = "granted";
    var id          = "id";

    var base_iter = 2;

    while(1)
    {
        curSecUrl = DoCoolKeyGetConfigValue(ESC_SECURE_URL + "." + i); 
       
        if(curSecUrl)
        {
            DoCoolKeySetConfigValue(capability + ".p" + base_iter + "." + granted,uni_connect);

            DoCoolKeySetConfigValue(capability + ".p" + base_iter + "." + id,curSecUrl);
        }

        if(!curSecUrl)
            break; 

        i++;
        base_iter++;
    }
}

function DoShowFullEnrollmentUI()
{
   if (!gCurrentSelectedRow)
   {
       SetCurrentSelectedRowForEnrollment();

       if(!gCurrentSelectedRow)
       {
           MyAlert(getBundleString("errorSelectKey"));
           return;
       }
   }

   var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
   var keyType = keyInfo[0];
   var keyID = keyInfo[1];
   var keyInserted = 1;
   var showFullUI  = 1;
   var externalUI = GetCachedTPSUI(keyID);

   if(externalUI)
   {
       loadExternalESCUI();
   }
   else
   {
       UpdateEnrollmentArea(keyType,keyID,keyInserted,showFullUI);
   }
}

function UpdateEnrollmentArea(keyType,keyID,inserted,showFullUI,showExternalUI)
{

     if(!gEnrollmentPage)
         return;

     var alreadyEnrolled = false;
     var keyStatus = GetStatusForKeyID(keyType, keyID);

     if(inserted && keyStatus == getBundleString("statusEnrolled"))
     {
         alreadyEnrolled = true;
     } 

      var numUnenrolledKeys = DoGetNumUnenrolledCoolKeys();

      //alert("inserted " + inserted + " showFulUI " + showFullUI + " showExternalUI " + showExternalUI + " already enrolled " + alreadyEnrolled + " numUnenrolledKeys " + numUnenrolledKeys);


     var ui_id = document.getElementById("esc-ui");

     if(ui_id)
     {
         var enrollment_ui = ui_id.getAttribute("src");
         if(enrollment_ui && !showExternalUI && inserted)
             return;

         if(!inserted)
         {
             if(!numUnenrolledKeys)
             {
               ui_id.setAttribute("src",null);
             }
             else
             {
                return;
             }

         }
     }

     gExternalUI = false;

     var enroll_area = document.getElementById("key_enrollment_area");
     if(!enroll_area)
         return;
     var no_key_area = document.getElementById("no_key_box");
     if(!no_key_area)
        return;
     var yes_key_area = document.getElementById("yes_key_box");
     if(!yes_key_area)
         return;
     var enrollBtn = document.getElementById("enrollbtn");
     if(!enrollBtn)
         return;
     var detected_key_message = document.getElementById("detected-key-message");    
     var enroll_key_message = document.getElementById("enroll-key-message");
     if(!enroll_key_message)
         return;

     var unenrolled_key_heading = document.getElementById("unenrolled-key-heading");
     if(!unenrolled_key_heading)
        return;

     var enroll_proceed_message = document.getElementById("enroll-proceed-message");
     if(!enroll_proceed_message)
        return;
     if(alreadyEnrolled)
     {
         unenrolled_key_heading.setAttribute("value",getBundleString("enrolledDetected"));
         HideItem(enroll_proceed_message);
     }
     else
     {
         ChangeDescription(enroll_proceed_message,getBundleString("readyToProceed"));
     }

     var no_key_heading = document.getElementById("no-key-heading");

     if(!no_key_heading)
         return;
     var enrolling_key_heading = document.getElementById("enrolling-key-heading");
     if(!enrolling_key_heading)
        return;
     var ui_id = document.getElementById("esc-ui");

     if(!ui_id)
         return;

     HideItem(unenrolled_key_heading);
     HideItem(enrolling_key_heading);
     HideItem(no_key_heading);

     if(!inserted)
     {
          HideItem(ui_id);
          HideItem(enroll_area);
          ShowItem(no_key_area);
          HideItem(yes_key_area);
          HideItem(enroll_key_message);
          HideItem(detected_key_message);
          ShowItem(no_key_heading);
          HideItem(enrollBtn);

          if(gCurrentSelectedRow)
          {
              var progId = KeyToProgressBarID(keyType, keyID) ;
              var meter = document.getElementById(progId);

              if(meter)
                 meter.setAttribute("id","progress-id");

              gCurrentSelectedRow.setAttribute("id","key_enrollment_row");
              gCurrentSelectedRow= null;
          }
     }
     else
     {
         if(showFullUI)
         {
             if(!showExternalUI)
             {
                 HideItem(ui_id);
                 HideItem(detected_key_message);
                 ShowItem(enrolling_key_heading);
                 ShowItem(enroll_area);
                 HideItem(yes_key_area);
                 ShowItem(enroll_key_message);
                 enrollBtn.setAttribute("onclick","DoEnrollCoolKey();");
                 ShowItem(enrollBtn);
             }
             else
             {
                 gExternalUI = true;
                 ShowItem(ui_id); 
                 HideItem(detected_key_message);
                 ShowItem(enrolling_key_heading);
                 HideItem(enroll_area);
                 HideItem(yes_key_area);
                 HideItem(enroll_key_message);
                 HideItem(enrollBtn);
                 UpdateESCSize(ESC_ENROLL_WIDTH,ESC_ENROLL_HEIGHT);
             }
         }
         else
         {
             ShowItem(yes_key_area);
             ShowItem(unenrolled_key_heading);
             HideItem(enroll_key_message);

             if(alreadyEnrolled)
             {
                 ChangeDescription(detected_key_message,getBundleString("enrolledDetectedMessage"));
             }
             else
             {
                ChangeDescription(detected_key_message,getBundleString("unenrolledDetectedMessage"));

             } 

             ShowItem(detected_key_message);
             enrollBtn.setAttribute("onclick","DoShowFullEnrollmentUI();");
             ShowItem(enrollBtn);
         }

         HideItem(no_key_area);
     }

    if(!alreadyEnrolled  && inserted && showExternalUI)
     {
         UpdateESCSize(ESC_ENROLL_WIDTH,ESC_ENROLL_HEIGHT);
         return;
     }

     if(!showExternalUI)
         UpdateESCSize();
}

//Evaulate Password Quality

function EvaluatePasswordQuality()
{
   var qualityImage = document.getElementById("password-image");
   var pw = document.getElementById("pintf").value;
   var pwlength = 0;

   var qualityMeter = document.getElementById("pass-progress-id");


   if(pw)
       pwlength = pw.length;

   if (pwlength>5)
    pwlength=5;

//use of numbers in the password
   var numnumeric = pw.replace (/[0-9]/g, "");
   var numeric=(pw.length - numnumeric.length);
   if (numeric>3)
     numeric=3;

//use of symbols in the password
   var symbols = pw.replace (/\W/g, "");
   var numsymbols=(pw.length - symbols.length);
   if (numsymbols>3)
     numsymbols=3;

//use of uppercase in the password
   var numupper = pw.replace (/[A-Z]/g, "");
   var upper=(pw.length - numupper.length);
   if (upper>3)
     upper=3;

   var pwstrength=((pwlength*10)-20) + (numeric*10) + (numsymbols*15) + (upper*10);

// make sure we're give a value between 0 and 100
  if ( pwstrength < 0 ) {
    pwstrength = 0;
  }
  if ( pwstrength > 100 ) {
    pwstrength = 100;
  }
   if(qualityMeter)
   {
       qualityMeter.setAttribute("value",  pwstrength);

   }
   if(qualityImage)
   {
        if(pwlength==0)
        {
           qualityImage.setAttribute("src","1-none.png");
           return;
        }
        if(pwstrength < 40)
        {
            qualityImage.setAttribute("src", "2-vweak.png");
            return;
        }
        if(pwstrength >= 40 && pwstrength < 50)
        {
            qualityImage.setAttribute("src","3-weak.png");
            return;
        }
        if(pwstrength >=50 && pwstrength < 60)
        {
            qualityImage.setAttribute("src","4-fair.png");
            return;
        }
        if(pwstrength >= 60 && pwstrength < 80)
        {
           qualityImage.setAttribute("src","5-good.png");
           return;
         }
        if(pwstrength >= 80)
           qualityImage.setAttribute("src","6-strong.png");
   }
}

function UpdateInfoForKeyID(keyType, keyID, keyStatus, reqAuth, isAuthed)
{
  var row = GetRowForKey(keyType, keyID);

  if (!row)
    return;

  if(gAdminPage)
  {
      if(row)
      {
          SelectRow(row);
      }
  }
}

function GetOperationInProgressForKeyID(keyType,keyID)
{
   var status;
   var result = null;

   try {
       status = GetCoolKeyStatus(keyType, keyID);
   } catch(e) {
       status = 0;
   }

   switch (status) {
       case 7: // PINResetInProgress
           result = getBundleString("operationPINReset");
       break;
       case 5: // EnrollmentInProgress
            result = getBundleString("operationEnrollment");
       break;
       case 9: // FormatInProgress
           result = getBundleString("operationFormat");
       break;
   }

   return result;
}

function GetStatusForKeyID(keyType, keyID)
{
  var keyStatus = "BLANK";
  var status;

  try {
    status = GetCoolKeyStatus(keyType, keyID);
  } catch(e) {
    status = 0;
  }

  switch (status) {
    case 0: // Unavailable
      keyStatus = getBundleString("statusUnavailable");
      break;
    case 1: // AppletNotFound
      keyStatus = getBundleString("statusNoApplet");
      break;
    case 2: // Uninitialized
      keyStatus = getBundleString("statusUninitialized");
      break;
    case 3: // Unknown
      keyStatus = getBundleString("statusUnknown");
      break;
    case 4: // Available
    case 6: // UnblockInProgress
    case 8: // RenewInProgress
      keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
      break;
    case 7: // PINResetInProgress
      keyStatus = getBundleString("statusBusy");
      break;
    case 5: // EnrollmentInProgress
      keyStatus = getBundleString("statusBusy");
      break;
    case 9: // FormatInProgress
      keyStatus = getBundleString("statusBusy");
      break;
  }

  return keyStatus;
}

function GetKeyStatusForKeyID(keyType, keyID)
{
  var row = GetRowForKey(keyType, keyID);

  if (!row)
    return getBundleString("statusUnknown");

   var status = row.childNodes[2];

   if(!row)
       return getBundleString("statusUnknown");

   return status.value;
}

function InsertCoolKeyIntoEnrollmentPage(keyType,keyID)
{
   var row = GetRowForKey(keyType, keyID);

   //MyAlert("Insert CoolKeyInto Enrollment Page " + " type " + keyType + " id " +  keyID + " row " + row);

  if (!row)
  {
    var issuer = GetCoolKeyIssuer(keyType,keyID);
    var issuedTo = GetCoolKeyIssuedTo(keyType,keyID);

    var keyStatus = GetStatusForKeyID(keyType, keyID);
    var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
    var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

    row = CreateEnrollPageKeyRow(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed,issuer,issuedTo);

    if (!row)
      return null;
  }
  return row;
}

function InsertCoolKeyIntoAdminBindingList(keyType,keyID)
{
    var row = GetRowForKey(keyType, keyID);

    if(!row)
    {
        var listbox = document.getElementById("AdminBindingList");
        var issuer = GetCoolKeyIssuer(keyType,keyID);
        var issuedTo = GetCoolKeyIssuedTo(keyType,keyID);

        if (listbox)
        {
            var keyStatus = GetStatusForKeyID(keyType, keyID);
            var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
            var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));
            row = CreateAdminListRow(listbox, keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed,issuer,issuedTo);
        }

        if (!row)
            return null;
    }

  return row;
}

function UpdateCoolKeyAvailabilityForEnrollment()
{
  //Here we only allow ONE key
  //Take the first unenrolled one that shows up.

  var arr = GetAvailableCoolKeys();

  if (!arr || arr.length < 1)
  {
    UpdateESCSize();
    return;
  }

  var i=0;

  for (i=0; i < arr.length; i++)
  {
      var status =  GetStatusForKeyID(arr[i][0],arr[i][1]);

      if(status  != getBundleString("statusEnrolled"))
      {
          var row = InsertCoolKeyIntoEnrollmentPage(arr[i][0],arr[i][1]);

          if(row)
            gCurrentSelectedRow = row;

          var keyInserted = 1;
          var showFullUI = 0;
    
          UpdateEnrollmentArea(arr[i][0],arr[i][1],keyInserted,showFullUI);

          break;
      }
  }

  UpdateESCSize();
}

function UpdateAdminBindingListAvailability()
{
    var arr = GetAvailableCoolKeys();

    if (!arr || arr.length < 1)
    {
        UpdateAdminKeyDetailsArea(null,null);
        UpdateESCSize();
        return;
    }

    var i;

    for (i=0; i < arr.length; i++)
    {
        InsertCoolKeyIntoAdminBindingList(arr[i][0], arr[i][1]);

        if (!gCurrentSelectedRow)
        {
            SelectRowByKeyID(arr[i][0], arr[i][1]);
            UpdateAdminKeyDetailsArea(arr[i][0],arr[i][1]);
        }
    }

    if(i > 0)
    {
        UpdateESCSize();
    }

    window.focus();
}

function SetCurrentSelectedRowForEnrollment()
{
  var arr = GetAvailableCoolKeys();

  if (!arr || arr.length < 1)
    return;

  var last = arr.length - 1;

  if( last < 0)
     return;

    if (!gCurrentSelectedRow)
    {
      var row = GetRowForKey(arr[last][0],arr[last][1]);

      if(row)
          gCurrentSelectedRow = row;
    }
}

function InitializeSecurityEnrollment()
{
  UpdateSecurityPage();
}

function InitializeEnrollment()
{
  gEnrollmentPage = 1;
  UpdateCoolKeyAvailabilityForEnrollment();
}

function InitializeAdminBindingList()
{

 gAdminPage = 1;

 UpdateAdminBindingListAvailability();

 DoSetEnrolledBrowserLaunchState(); 
 DoHandleEnrolledBrowserLaunch();

 window.setTimeout('ShowWindow()',250);

}

//Window related functions

function hiddenWindowStartup()
{
  gHiddenPage = 1;

  // We don't want the hidden window to be shown ever.

  // We do want notify events though
  var doPreserveNotify = true;

  
  SetMenuItemsText(); 
  HideWindow();
  TrayRemoveWindow(doPreserveNotify);
}

function IdentifyWindow()
{
     var locName = window.location.toString();
     return locName;
}

function IsPageWindowPresent(aPageID)
{
    var pageWindow = null;

    if(!aPageID)
        return null;

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);

    var count = 0;

    var enumerator = wm.getEnumerator("");
    while(enumerator.hasMoreElements()) {
        var win = enumerator.getNext();
        if(win)
        {
            var locName = win.location.toString();
            if(locName.indexOf(aPageID) != -1)
            {
               pageWindow = win;
            }
        }
    }

    return pageWindow;
}

function CloseAllNonHiddenWindows()
{

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);

    var enumerator = wm.getEnumerator("");
    while(enumerator.hasMoreElements()) {
        var win = enumerator.getNext();
        if(win)
        {
            var locName = win.location.toString();
            if(locName.indexOf(HIDDEN_WINDOW) != -1)
            {
               continue;
            }
            else
            {
                win.close();
            }
        }
    }

}

function GetESCHiddenWindow()
{
    const hiddenName = HIDDEN_WINDOW;
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);

    var count = 0;
    var hidden = null;

    var enumerator = wm.getEnumerator("");
    while(enumerator.hasMoreElements()) {
        var win = enumerator.getNext();
        if(win)
        {
            var locName = win.location.toString();
            if(locName.indexOf(hiddenName) != -1)
            {
               hidden = win;
            }
        }
    }

    return hidden;
}

function CountESCWindows()
{
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);

    var count = 0;
    var enumerator = wm.getEnumerator("");
    while(enumerator.hasMoreElements()) {
        var win = enumerator.getNext();
        if(win)
        {
            count++;
        }
    }

    return count;
}

function IsPrimaryESCPage()
{
    var result = false;

    if(gAdminPage || gEnrollmentPage  || gHiddenPage)
        result = true;

    return result;
}

function SelectESCPageCMDLine()
{
    var securityURL =   DoCoolKeyGetConfigValue(ESC_SECURITY_URL);

    if(securityURL)
    {
        launchESCSecMode();
        return;
    }

    launchSETTINGS();

}

function SelectESCPage(keyType,keyID,phoneHomeFailed)
{

     if(!gHiddenPage)
       return;

     recordMessage("Inside SelectESCPage");
     var keyUninitialized = 0;
     var keyStatus = GetCoolKeyStatus(keyType,keyID);

     switch (keyStatus) {
       case 1: //no applet
           keyUninitialized = UNINITIALIZED_NOAPPLET; 
       break;
       case 2: // uninitialized 
           keyUninitialized = UNINITIALIZED;
       break;
       case 4: // Enrolled 
           keyUninitialized = 0;
       break;
   }

   //alert("SelectESCPage  initialized " + keyUninitialized + " gEnrollmentPage " + gEnrollmentPage + " gFactoryMode " + gFactoryMode + " gHiddenPage " + gHiddenPage);

   //Get the primary page windows if present

   var enrollWnd = IsPageWindowPresent(ENROLL_WINDOW);
   var adminWnd  = IsPageWindowPresent(ADMIN_WINDOW);

   if(keyUninitialized == UNINITIALIZED && !phoneHomeFailed )  //formatted uninitialized card
   {
       if(enrollWnd)   //Enrollment window is  already up
       {
          enrollWnd.focus();
          enrollWnd.ShowWindow();
       }
       else
       {
          launchESC();
       }
   }
   else
   {
       //Launch admin page if factory mode is enabled

       if(gFactoryMode || phoneHomeFailed || keyUninitialized == UNINITIALIZED_NOAPPLET )  //no applet
       { 

           if(adminWnd)    // Handle case where admin page is already up
           {
               adminWnd.focus();
           }
           else
           {
               launchSETTINGS();
           }
       } 
       else
       {
           // No factory mode do nothing 
       }

   }
 
}

function KeyIsPresent(keyType, keyID)
{
  row = document.all.item(keyType, keyID);

  if (!row)
    return false;

  return true;
}

function SetStatusMessage(str)
{
  var cell = document.getElementById("statusMsg");

  if (!cell)
    return;
  RemoveAllChildNodes(cell);
  cell.appendChild(document.createTextNode(str));
}

function SelectRow(row)
{

  if(!row)
      return;

  var theID = row.getAttribute("id");
  if (!row || gCurrentSelectedRow == row)
    return;

  if (gCurrentSelectedRow)
    gCurrentSelectedRow.setAttribute("class","UnSelectedRow");

  gCurrentSelectedRow = row;

  gCurrentSelectedRow.setAttribute("class","SelectedRow");

  var keyInfo = RowIDToKeyInfo(theID);

  if(gAdminPage)
  {
      UpdateAdminKeyDetailsArea(keyInfo[0],keyInfo[1]);
      UpdateAdminListRow(keyInfo[0],keyInfo[1]);
  }

}

function SelectRowByKeyID(keyType, keyID)
{
  var row = GetRowForKey(keyType, keyID);

  SelectRow(row);
}

function DoSelectAdminListRow(event)
{
    var id = event.getAttribute("id");
    row = document.getElementById(id);

    if(row)
        SelectRow(row);
}

function KeyToUIString(keyType, keyID)
{
  // If it's an CoolKey, format the keyID string.

  if (keyType == 1 && keyID.length == 20)
  {
    var re = /([0-9a-f]{4})([0-9a-f]{4})([0-9a-f]{4})([0-9a-f]{4})([0-9a-f]{4})/i;
    keyID = keyID.replace(re, "$1-$2-$3-$4-$5").toLowerCase();
  }

  return keyID;
}

function CreateEnrollPageKeyRow(keyType,keyID,keyStatus,reqAuth,isAuthed,keyIssuer,keyIssuedTo)
{
    if(!gEnrollmentPage)
        return null;

    var row = document.getElementById("key_enrollment_row");

    if(!row)
        return null;

    var progressMeter = document.getElementById("progress-id");

    if(progressMeter)
    {
        progressMeter.setAttribute("id",KeyToProgressBarID(keyType, keyID));
        progressMeter.setAttribute("mode","determined");
        progressMeter.setAttribute("value","0%");
    }
   row.setAttribute("id", KeyToRowID(keyType, keyID));
   var status = document.getElementById("status-id");

   if(status)
   {
       if(keyStatus)
           status.setAttribute("value",keyStatus);
   }

   var issuer = document.getElementById("issuer-id");

   if(issuer && keyIssuer)
   {
       issuer.setAttribute("value",keyIssuer); 
   }

    return row;
}

function UpdateAdminKeyAreaImageToBusy(keyType, keyID)
{
    var image_src="throbber-anim5.gif";

    var detailsImage = document.getElementById("admin-details-image");

    if(!detailsImage)
        return;

    detailsImage.setAttribute("src",image_src);
}

function UpdateAdminKeyAreaDetailsLabel(label)
{
    if(!label)
        return;

    var detailsKeyLabel = document.getElementById("admin-key-details-label");

    if(detailsKeyLabel)
    {
        detailsKeyLabel.setAttribute("value",label);
    }
}

function UpdateAdminKeyDetailsArea(keyType,keyID)
{
    if(!gAdminPage)
        return;

    var isCool = null;

    //alert("blub " + " keyType " + keyType + " keyID " + keyID);

    isCool = DoGetCoolKeyIsReallyCoolKey(keyType, keyID);

    var hideFormatConfig = DoCoolKeyGetConfigValue(ESC_HIDE_FORMAT);
    var hideFormat = false;

    if(hideFormatConfig == "yes")
        hideFormat = true;

    var noKey = 0;

    if(!keyType && !keyID)
    {
        noKey = 1;
    }

    var keyStatus = 0;

    if(!noKey)
        keyStatus = GetStatusForKeyID(keyType, keyID);

    recordMessage("No Key: " + noKey + " status " + keyStatus);

    var passwordArea = document.getElementById("password-area-id");

    if(passwordArea)
        HideItem(passwordArea);

    var detailsImage = document.getElementById("admin-details-image");

    if(!detailsImage)
        return;

    var detailsMessage = document.getElementById("admin-details-message");

    if(!detailsMessage)
        return;

    var detailsKeyLabel = document.getElementById("admin-key-details-label");

    if(!detailsKeyLabel)
        return;

    var formatbtn = document.getElementById("formatbtn");

    if(!formatbtn)
        return;

    var resetpinbtn = document.getElementById("resetpinbtn");

    if(!resetpinbtn)
        return;

    var enrollbtn = document.getElementById("enrollbtn");

    if(!enrollbtn)
        return;

    var advancedbtn = document.getElementById("advancedbtn");

    if(!advancedbtn)
        return;

    var viewcertsbtn = document.getElementById("viewcertsbtn");

    if(!viewcertsbtn)
        return;

    var image_src = SelectImageForKeyStatus(keyStatus,1,1);

    recordMessage("image_src " + image_src);
    if(!image_src)
        HideItem(detailsImage);
    else
    {
        ShowItem(detailsImage);
        detailsImage.setAttribute("src", image_src);
    }

    // Now take care of the right click context menu that is
    // Invisible at this point

   var adminkeymenu = document.getElementById("adminkeymenu");
   var menu_format = null;
   var menu_enroll = null;
   var menu_resetpassword = null;

   if(adminkeymenu)
   {
       menu_format = document.getElementById("menu-format");
       menu_enroll = document.getElementById("menu-enroll");
       menu_resetpassword = document.getElementById("menu-resetpassword");
       
       if(!menu_format || !menu_enroll || !menu_resetpassword)
       {
           menu_format = null;
           menu_enroll = null;
           menu_resetpassword = null;
           adminkeymenu = null;
       }
   }

   recordMessage("Obtained admin popup menu object.");
    ShowItem(advancedbtn);
    EnableItem(advancedbtn);

    var isBusy =  0;
    var operationLabel = null;

    if(keyStatus == getBundleString("statusBusy") || keyStatus == getBundleString("statusUnavailable"))
        isBusy = 1;

    if(isBusy)
    {
        operationLabel = GetOperationInProgressForKeyID(keyType,keyID);
    }

   if(!keyStatus)
   {
      EnableItem(viewcertsbtn);
      DisableItem(enrollbtn);
      if(adminkeymenu)
      {
         DisableItem(menu_enroll);
         DisableItem(menu_resetpassword);
         DisableItem(menu_format);
      }

      DisableItem(resetpinbtn);
      DisableItem(formatbtn);

      detailsKeyLabel.setAttribute("value",getBundleString("noKeysPresent"));
      HideItem(detailsImage);

      if(hideFormat) {
          HideItem(formatbtn);
          if(adminkeymenu)
              HideItem(menu_format);
      }

      return;
   }

   if(keyStatus == getBundleString("statusEnrolled"))
   {
       var isLoginKey = IsKeyLoginKey(keyType,keyID);
       EnableItem(viewcertsbtn);

      EnableItem(enrollbtn);
      if(adminkeymenu)
          EnableItem(menu_enroll);

       if(isCool)
       {
           if(adminkeymenu)
               EnableItem(menu_resetpassword);

           EnableItem(resetpinbtn);

           if(!isLoginKey)
           {
               EnableItem(formatbtn);

               if(adminkeymenu)
                   EnableItem(menu_format);
           }
           else
           {
               DisableItem(formatbtn);
               if(adminkeymenu)
                   DisableItem(menu_format);
           }
       }
       else
       {
           DisableItem(resetpinbtn);
           DisableItem(formatbtn);
           if(adminkeymenu)
           {
               DisableItem(menu_format);
               DisableItem(menu_resetpassword);
           }
       }

       if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("enrolledKey"));

       if(hideFormat) {
           HideItem(formatbtn);
           if(adminkeymenu)
               HideItem(menu_format);
       }

       return;
   }

   if(keyStatus == getBundleString("statusUninitialized"))
   {
         EnableItem(viewcertsbtn);

         if(isCool)
         {
              EnableItem(enrollbtn);
              if(adminkeymenu)
                  EnableItem(menu_enroll);
         }
         else
         {
              if(adminkeymenu)
                  DisableItem(menu_enroll);

              DisableItem(enrollbtn);
         }

         DisableItem(resetpinbtn);
         if(adminkeymenu)
             DisableItem(menu_resetpassword);

         if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("statusUninitialized"));

         if(isCool)
         {
             EnableItem(formatbtn);
             if(adminkeymenu)
                 EnableItem(menu_format);
         }
         else
         {
             if(adminkeymenu) 
                 DisableItem(menu_format);

             DisableItem(formatbtn);
         }

       if(hideFormat) {
           HideItem(formatbtn);
           if(adminkeymenu)
               HideItem(menu_format);
       }
      
       return;
   }

   if(keyStatus == getBundleString("statusNoApplet"))
   {
       EnableItem(viewcertsbtn);
       DisableItem(enrollbtn);
       DisableItem(resetpinbtn);

       if(adminkeymenu)
       {
           DisableItem(menu_enroll);
           DisableItem(menu_resetpassword);
       }

       if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("statusNoApplet"));

       if(isCool)
       {
           if(adminkeymenu)
               EnableItem(menu_format);

           EnableItem(formatbtn);
       }
       else
       {
           if(adminkeymenu)
               DisableItem(menu_format);

           DisableItem(formatbtn);

       }

       if(hideFormat) {
          HideItem(formatbtn);
          if(adminkeymenu)
             HideItem(adminkeymenu);
       }

       return;
   }

  if(isBusy)
   {
      DisableItem(viewcertsbtn);
      DisableItem(enrollbtn);
      DisableItem(resetpinbtn);
      DisableItem(formatbtn);

       if(operationLabel)
            detailsKeyLabel.setAttribute("value",operationLabel);

       if(adminkeymenu)
       {
           DisableItem(menu_enroll);
           DisableItem(menu_format);
           DisableItem(menu_resetpassword);
       }

       if(hideFormat) {
           HideItem(formatbtn);
           if(adminkeymenu)
               HideItem(menu_format);
       }
   }
}

function HideEnrollmentPage()
{
    window.close();
}

function OnSecurityPageHidden()
{
    // Security URL no longer applies
    DoCoolKeySetConfigValue(ESC_SECURITY_URL,"");
}

function HideAdminPage()
{
    window.close();
}
function UpdateAdminListRow( keyType, keyID)
{
    if(!gAdminPage)
        return;

    var row = GetRowForKey(keyType, keyID);

    if(!row)
        return;

    var listbox = document.getElementById("AdminBindingList");
    if(!listbox)
       return;

    var isLoginKey = IsKeyLoginKey(keyType,keyID);

    var issuer = GetCoolKeyIssuer(keyType,keyID);
    var issuedTo = GetCoolKeyIssuedTo(keyType,keyID);
    var keyStatus = GetStatusForKeyID(keyType, keyID);

    var imageCell    = document.getElementById(KeyToCellID(keyType,keyID,"image"));
    var issuerCell   = document.getElementById(KeyToCellID(keyType,keyID,"issuer"));
    var issuedToCell = document.getElementById(KeyToCellID(keyType,keyID,"issued-to"));
    var statusCell   = document.getElementById(KeyToCellID(keyType,keyID,"status"));

    if(issuerCell)
        issuerCell.setAttribute("label",issuer);

    if(issuedToCell)
        issuedToCell.setAttribute("label",issuedTo);

    if(statusCell)
    {
        if(!isLoginKey)
            statusCell.setAttribute("label",keyStatus);
        else
            statusCell.setAttribute("label",getBundleString("statusLoggedIn"));

    }

    if(imageCell)
        imageCell.setAttribute("image",SelectImageForKeyStatus(keyStatus,1,0));
}

function CreateAdminListRow(adminListBox,keyType,keyID,keyStatus,reqAuth,isAuthed,keyIssuer,keyIssuedTo)
{

  if(!gAdminPage)
      return null;

  var listrow = CreateListBoxRow(adminListBox);

  if(!listrow)
      return null;

  listrow.setAttribute("flex","1");
  listrow.setAttribute("id",KeyToRowID(keyType,keyID));

  var imageCell = InsertListCell(listrow);

  if(!imageCell)
       return null;

  var image_src = SelectImageForKeyStatus(keyStatus);

  imageCell.setAttribute("class","listcell-iconic");
  imageCell.setAttribute("image",image_src);
  imageCell.setAttribute("id",KeyToCellID(keyType,keyID,"image"));

  var issuer = InsertListCell(listrow);

  if(!issuer)
    return null;

  issuer.setAttribute("label",keyIssuer);
  issuer.setAttribute("id",KeyToCellID(keyType,keyID,"issuer"));
  issuer.setAttribute("class","rowLabelText");

  var issuedTo = InsertListCell(listrow);

  if(!issuedTo)
      return null;

  issuedTo.setAttribute("label",keyIssuedTo);
  issuedTo.setAttribute("id",KeyToCellID(keyType,keyID,"issued-to"));


  var status = InsertListCell(listrow);
  if(!status)
      return null;

  status.setAttribute("class","rowLabelText");

  var isLoginKey = IsKeyLoginKey(keyType,keyID);
  if(!isLoginKey)
      status.setAttribute("label",keyStatus);
  else
      status.setAttribute("label",getBundleString("statusLoggedIn"));

  status.setAttribute("id",KeyToCellID(keyType,keyID,"status"));

  var progressCell = InsertListCell(listrow);

  if(!progressCell)
      return null;

  progressCell.setAttribute("flex","1");
  var progressMeter = document.createElement("progressmeter");

  if(progressMeter)
  {
      progressCell.appendChild(progressMeter);
      progressMeter.setAttribute("id", KeyToProgressBarID(keyType, keyID));
      progressMeter.className = "ProgressMeter";
      progressMeter.setAttribute("value", "0%");

      progressMeter.setAttribute("class","progressMeter");
      HideItem(progressMeter);
  }

  listrow.setAttribute("onclick","DoSelectAdminListRow(this);");
  listrow.setAttribute("ondblclick","launchCertViewerIfCerts();");
  listrow.setAttribute("context","adminkeymenu");

  adminListBox.appendChild(listrow);
  return listrow;
}

function GetProgressMeterValue(progMeterID)
{
  var progMeter = document.getElementById(progMeterID);

  if (!progMeter)
    return -1;

  return parseInt(progMeter.getAttribute("value"));
}

function SetProgressMeterValue(progMeterID, value)
{
  var progMeter = document.getElementById(progMeterID);

  if (!progMeter || value < 0)
    return;

  if (value > 100)
    value = 100;

  var newWidth = parseInt(progMeter.style.width) * value / 100 - 2;

  progMeter.setAttribute("value", value);
}

function SetProgressMeterStatus(progMeterID, statusMsg)
{
  var progMeter = document.getElementById(progMeterID);

  if (!progMeter)
    return;
}

function ClearProgressBar(progMeterID)
{
  SetProgressMeterValue(progMeterID, 0);
  SetProgressMeterStatus(progMeterID, "");

  var meter = document.getElementById(progMeterID);
  if(meter)
      HideItem(meter);

  
}

function KeyToProgressBarID(keyType, keyID)
{
  return "PM" + keyType + "-" + keyID;
}

function GetScreenNameValue()
{
  var sname = document.getElementById("snametf").value;

  if (! sname)
  {
    MyAlert(getBundleString("errorProvideScreenName"));
    return null;
  }

  return sname;
}

function SetPINValue(thePin)
{

 if(!thePin)
     return;

 var pintf_obj = document.getElementById("pintf");
 var reenterpintf_obj = document.getElementById("reenterpintf");



 if(pintf_obj)
      pintf_obj.value = thePin;

 if(reenterpintf_obj)
      reenterpintf_obj.value = thePin;


}


function GetPINValue()
{
  var pintf_obj = document.getElementById("pintf");
  var reenterpintf_obj = document.getElementById("reenterpintf");

  var pinVal = null;
  var rpinVal = null;

  if(pintf_obj)
       pinVal =  pintf_obj.value;

  if(reenterpintf_obj)
      rpinVal =  reenterpintf_obj.value;

  if (! pinVal && pintf_obj)
  {
    MyAlert(getBundleString("errorProvideTokenPIN"));
    return null;
  }

  if ( pinVal != rpinVal && reenterpintf_obj)
  {
    MyAlert(getBundleString("errorMatchPinValues"));
    return null;
  }

  return pinVal;
}

function GetScreenNamePwd()
{
  var pwd = document.getElementById("snamepwd").value;

   if(!pwd)
   {
       MyAlert(getBundleString("errorValidUserPassword"));
       return null;
   }
   return pwd;
}

function GetTokenCode()
{
  return null;
}
function DoEnrollCoolKey()
{

  if (!gCurrentSelectedRow)
  {
    MyAlert(getBundleString("errorSelectKey"));
    return;
  }

 if(!Validate())
     return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  var type = gKeyEnrollmentType;
  var screenname = null;
  var pin = null;

  var screennamepwd = null;
  var tokencode = null;
  
  var failed = 0;

  if (type == "userKey")
  {
    screenname =  null; //GetScreenNameValue();

    pin =  GetPINValue();

    screennamepwd = null; // GetScreenNamePwd();

    tokencode = GetTokenCode();
  }

   if(gAdminPage)
       UpdateAdminKeyAreaImageToBusy(keyType, keyID);

  if(gEnrollmentPage)
  {
      ShowEnrollmentAnimation(keyType,keyID,1);
  }

  if (!EnrollCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode))
  {
    failed = 1;
    recordMessage("EnrollCoolKey failed.");
  }

  if(gAdminPage)
  {
     UpdateAdminListRow(keyType,keyID);
     UpdateAdminKeyDetailsArea(keyType,keyID);
     if(!failed)
     {
          AdminToggleStatusProgress(1,keyType,keyID);
          UpdateAdminKeyAreaDetailsLabel(getBundleString("enrollingToken"));
     }
  }
}

function DoCollectPassword(operation)
{
       var child =  window.openDialog("chrome://esc/content/password.xul", operation, "chrome,centerscreen,modal=yes");
}

function DoResetSelectedCoolKeyPIN()
{
  if (!gCurrentSelectedRow)
    return;

  if(!Validate())
     return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  var screenname = null;
  var pin =  GetPINValue();
  var screennamepwd = null;

  var failed = 0;

  if (GetCoolKeyIsEnrolled(keyType, keyID))
  {

    if (!ResetCoolKeyPIN(keyType, keyID, screenname, pin,screennamepwd))
    {
      failed = 1;
      recordMessage("ResetCoolKeyPIN failed.");
    }
  }
  else
  {
      failed = 1;
      MyAlert(getBundleString("errorEnrolledFirst"));
  }

  if(gAdminPage)
  {
     UpdateAdminListRow(keyType,keyID);
     UpdateAdminKeyDetailsArea(keyType,keyID);

      if(!failed)
      {
          AdminToggleStatusProgress(1,keyType,keyID);
          UpdateAdminKeyAreaDetailsLabel(getBundleString("resettingTokenPIN"));
      }
  }
}

function DoFormatCoolKey(type)
{
  if (!gCurrentSelectedRow)
    return;

  var lType = null;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  var failed = 0;
  var globalType = GetCachedTokenType(keyID);

  if(!type)
      lType = gKeyEnrollmentType;
  else
      lType = type;

  if(globalType && type != CLEAN_TOKEN)
      lType = globalType;

  var screenname = null;
  var pin = null;

  var screennamepwd = null;
  var tokencode = null;


  if (!FormatCoolKey(keyType, keyID, lType, screenname, pin,screennamepwd,tokencode))
  {
    failed = 1;
    recordMessage("FormatCoolKey failed.");
  }

  if(gAdminPage)
  {
      UpdateAdminListRow(keyType,keyID);
      UpdateAdminKeyDetailsArea(keyType,keyID);
      if(!failed)
      {
          AdminToggleStatusProgress(1,keyType,keyID);
          UpdateAdminKeyAreaDetailsLabel(getBundleString("formatingToken"));
      }
  }
}
function DoCancelOperation()
{
  if (!gCurrentSelectedRow)
    return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  CancelCoolKeyOperation(keyType, keyID);

}

////////////////////////////////////////////////////////////////
//
// Functions called directly from ESC native code.
//
////////////////////////////////////////////////////////////////

function OnCoolKeyInserted(keyType, keyID)
{
  var row = null;

  var uninitialized = 0;

  recordMessage("Key inserted!" + "Window " + IdentifyWindow());

  if(gHiddenPage)
  {
      TrayShowNotificationIcon();
  }

  if (!GetCoolKeyIsEnrolled(keyType, keyID) )
  {
       uninitialized = 1;
  }

   if(gEnrollmentPage)
   { 
        row = InsertCoolKeyIntoEnrollmentPage(keyType,keyID);

        gCurrentSelectedRow = row;
        UpdateEnrollmentArea(keyType,keyID,1);
   }

   if(gAdminPage)
   {
       row = InsertCoolKeyIntoAdminBindingList(keyType,keyID);

       if (!gCurrentSelectedRow)
          SelectRowByKeyID(keyType, keyID);

   }

  if(gHiddenPage)
  {
      var phoneHomeSuccess = 1;
      if(DoGetCoolKeyIsReallyCoolKey(keyType, keyID))
      {
          phoneHomeSuccess = DoPhoneHome(keyType,keyID);
      }
      else
      {
          var issuer = GetCoolKeyIssuer(keyType,keyID);
          if(!issuer )
              issuer = getBundleString("unknownIssuer");

          TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));

      }

      ShowAllWindows();
      if(!CheckForSecurityMode())
      {
          SelectESCPage(keyType,keyID,1 - phoneHomeSuccess);
      }

  }

}

function OnCoolKeyRemoved(keyType, keyID)
{
  var table = null;

  var  row = GetRowForKey(keyType, keyID);

  if(gHiddenPage)
  {
      if(curChildWindow)
      {
          curChildWindow.close();
          curChildWindow = null;
      }
      var issuer = GetCoolKeyIssuer(keyType,keyID);
      if(!issuer)
          issuer = getBundleString("unknownIssuer");
      TraySendNotificationMessage(getBundleString("keyRemoved"),"\"" + issuer + "\"" + " " + getBundleString("keyRemovedComputer"),1,4000,GetESCNotifyIconPath(keyType,keyID));
       LogKeyInfo(keyType,keyID, "Key Removed ...");

  }

  if(gEnrollmentPage)
  {
      UpdateEnrollmentArea(keyType,keyID,0);
  }
  if(gAdminPage)
  {
      RemoveAdminRow(row);
      if (row == gCurrentSelectedRow)
      {
          gCurrentSelectedRow = null;
          UpdateAdminBindingListAvailability();
      }
      else
      {
          if(DoGetNumCoolKeys() == 0)
          {
              UpdateAdminKeyDetailsArea(null,null);
          }
      }
  }

}

function PolicyToKeyType(policy)
{
   return getBundleString("statusEnrolled");
}

function BoolToYesNoStr(b)
{
  if (b)
    return "YES";
  return "NO";
}

function OnCoolKeyEnrollmentComplete(keyType, keyID)
{
  if(gHiddenPage)
      return;

  var keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);

  if(!CheckForSecurityMode())
      MyAlert(getBundleString("enrollmentFor") + " "  + getBundleString("smartCard") + " " + getBundleString("wasSuccessful"));


  if(gEnrollmentPage)
  {
      ShowEnrollmentAnimation(keyType,keyID,0);
  }
  if(gAdminPage)
  {
      UpdateAdminKeyDetailsArea(keyType,keyID);
      UpdateAdminListRow(keyType,keyID);
      AdminToggleStatusProgress(0,keyType,keyID);
  }

  ClearProgressBar(KeyToProgressBarID(keyType, keyID));
}

function OnCoolKeyPINResetComplete(keyType, keyID)
{
  if(gHiddenPage)
      return;

  var keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);

  if(!CheckForSecurityMode())
      MyAlert(getBundleString("pinResetSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));

  if(gAdminPage)
   {
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminListRow(keyType,keyID);
     AdminToggleStatusProgress(0,keyType,keyID);
   }

}

function OnCoolKeyFormatComplete(keyType, keyID)
{
  if(gHiddenPage)
      return;

  var keyStatus = GetStatusForKeyID(keyType, keyID);
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);

  if(!CheckForSecurityMode())
      MyAlert(getBundleString("formatOf") + " " + getBundleString("smartCard") + " "  + getBundleString("wasSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));

   if(gAdminPage)
   {
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminListRow(keyType,keyID);
     AdminToggleStatusProgress(0,keyType,keyID);
   }

}

function OnCoolKeyStateError(keyType, keyID, keyState, errorCode)
{
  if(gHiddenPage)
      return;

  var keyStatus = GetStatusForKeyID(keyType, keyID);
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  if(curChildWindow)
  {
      curChildWindow.close();
      curChildWindow = null;
  }

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);


  var typeStr = getBundleString("error") + ": " ;

  var  messageStr = " "  + MyGetErrorMessage(errorCode) ;

  var keyIDStr = KeyToUIString(keyType, keyID);

  if (keyState == 1004)
    typeStr = getBundleString("enrollmentOfKey") + " "  + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1016)
      typeStr = getBundleString("formatingOfKey") + " " + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1010)
    typeStr = getBundleString("pinResetOfKey") + " "   + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1020)
    typeStr = getBundleString("operationForKey") + " "  + getBundleString("cancelled") + " "  + typeStr + messageStr ;


  if(gEnrollmentPage)
  {
    ShowEnrollmentAnimation(keyType,keyID,0);
  }
   if(gAdminPage)
   {
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminListRow(keyType,keyID);
     AdminToggleStatusProgress(0,keyType,keyID);
   }

  if(!CheckForSecurityMode())
      MyAlert(typeStr);
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));
}

function OnCoolKeyStatusUpdate(progMeterID, statusUpdate)
{
  SetProgressMeterValue(progMeterID, statusUpdate);
}

function Validate()
{
  var type = gKeyEnrollmentType;
  var screenname = null;
  var pin = null;

  var screennamepwd = null;
  var tokencode = null;

  if (type == "userKey")
  {
    pin =  GetPINValue();

    if (! pin)
      return 0;
   }

   return 1;
}

function OnCoolKeyStateChange(keyType, keyID, keyState, data,strData)
{
  // alert("KeyID:    " + keyID + "\n" +
  //       "KeyState: " + keyState + "\n" +
  //       "Data:     " + data);
  //alert("State Change ="+keyState);


  switch(keyState)
  {
    case 1000: // KeyInserted
      OnCoolKeyInserted(keyType, keyID);
      break;
    case 1001: // KeyRemoved
      OnCoolKeyRemoved(keyType, keyID);
      break;
    case 1002: // EnrollmentStart
      // OnCoolKeyEnrollmentStart(keyType, keyID);
      break;
    case 1003: // EnrollmentComplete
      if(gExternalUI)
          break;
      OnCoolKeyEnrollmentComplete(keyType, keyID);
      break;
    case 1004: // EnrollmentError
       if(gExternalUI)
          break;
      OnCoolKeyStateError(keyType, keyID, keyState, data);
      break;
    case 1008: // PINResetStart
      // OnCoolKeyPINResetStart(keyType, keyID);
      break;
    case 1009: // PINResetComplete
      OnCoolKeyPINResetComplete(keyType, keyID);
      break;
    case 1010: // PINResetError
      OnCoolKeyStateError(keyType, keyID, keyState, data);
      break;
    case 1014: // FormatStart
      // OnCoolKeyFormatStart(keyType, keyID);
      break;
    case 1015: // FormatComplete
      OnCoolKeyFormatComplete(keyType, keyID);
      break;
    case 1016: // FormatError
      OnCoolKeyStateError(keyType, keyID, keyState, data);
      break;
    case 1017: // BlinkStatus Update?
      break;
    case 1018: 
      break;
    case 1020: // OperationCancelled
      OnCoolKeyStateError(keyType, keyID, keyState, data);
      break;
    case 1021: // OperationStatusUpdate
      OnCoolKeyStatusUpdate(KeyToProgressBarID(keyType, keyID), data);
      break;

     case 1022: //Need Auth 

       gCurKeyID = keyID;
       gCurKeyType = keyType;

       GetAuthDataFromPopUp(keyType,keyID,strData);

       break;
  }
}

function refresh()
{
  window.resizeBy(0,1);
  window.resizeBy(0,-1);
}

function loadExternalESCUI()
{
   var esc_enroll_uri = null;

    var keyType= null;
    var keyID = null;
    var keyType = null;

    var inserted = true;
    var showFullUI = true;
    var showExternalUI = true;

    //get latest inserted key

    var arr = GetAvailableCoolKeys();

    if (arr && arr.length > 0)
    {
       var pos = arr.length - 1;
       keyID = arr[pos][1];
       keyType = arr[pos][0];
    }

    var esc_enrolled_token_url = null;
    var esc_enroll_uri = null;

    if(keyID)
    {
         esc_enroll_uri = GetCachedTPSUI(keyID);

         esc_enrolled_token_url = GetCachedEnrolledTokenURL(keyID);

         if(esc_enrolled_token_url )
         {
               var alreadyEnrolled = false;

               var keyStatus = GetStatusForKeyID(keyType, keyID);

              if(inserted && keyStatus == getBundleString("statusEnrolled"))
              {
                  alreadyEnrolled = true;
              }

              if(alreadyEnrolled)
              {
                  esc_enroll_uri = esc_enrolled_token_url;

              }

         }
    }

    if(esc_enroll_uri)
    {
        //Deal with Moz external UI privileges

        grantPrivilegesURL(esc_enroll_uri);

        var ui_id = document.getElementById("esc-ui");

        if(ui_id)
        {
            ui_id.setAttribute("src",esc_enroll_uri);
            UpdateEnrollmentArea(keyType,keyID,inserted,showFullUI,showExternalUI)
        }

    }

}

// Special feature to open a default browser to
// a configurable URL. 


function DoSetEnrolledBrowserLaunchState()
{
    var launch_id = document.getElementById("enrolled_key_browser");

    if(launch_id)
    {
        var doIgnoreBrowserUrl = DoCoolKeyGetConfigValue(ESC_IGNORE_TOKEN_BROWSER_URL);
        recordMessage("DoSetEnrolledBrowserLaunchState: doIgnore: " + doIgnoreBrowserUrl);
        var checked= "false";

        if(doIgnoreBrowserUrl == "yes")
        {
            checked = "true";
        }
        else
        {
            checked = "false";
        }
     
        launch_id.setAttribute("checked",checked); 
    }

}

function DoHandleEnrolledBrowserLaunch()
{

    var launch_id = document.getElementById("enrolled_key_browser");

    var doShow = DoCoolKeyGetConfigValue(ESC_TOKEN_BROWSER_URL_ESTABLISHED);
    
    if(launch_id)
    {

      if(doShow == "yes")
      {
          ShowItem(launch_id);
      }
      else
      {
          HideItem(launch_id);
      }

      var checked = launch_id.getAttribute("checked");

      recordMessage("DoHandleEnrolledBrowserLaunch checked: " + checked );
      if(checked == "true")
      {
          recordMessage("DoHandleEnrolledBrowserLaunch Setting ESC_IGNORE to yes");
          DoCoolKeySetConfigValue(ESC_IGNORE_TOKEN_BROWSER_URL,"yes");
      }
      else
      {
          recordMessage("DoHandleEnrolledBrowserLaunch Setting ESC_IGNORE to no");
          DoCoolKeySetConfigValue(ESC_IGNORE_TOKEN_BROWSER_URL,"no");
      }

    }
}
//Implement special feature to open browser to configured URL
function openEnrolledTokenURLBrowser(aKeyID)
{

    if(!gHiddenPage)
        return;

    var agent = navigator.userAgent.toLowerCase();

    var doWindows = 0;

    var platform = null;
    var executable = null;

    //Check to see if we should ignore this

     var doIgnoreBrowserUrl = DoCoolKeyGetConfigValue(ESC_IGNORE_TOKEN_BROWSER_URL);

     if(doIgnoreBrowserUrl == "yes")
     {
         recordMessage("openEnrolledTokenURLBrowser don't open browser because config param is set to ignore!");

         return;
     }

    if(agent && agent.indexOf("mac") != -1)
    {
        platform = "mac";
        executable = MAC_PROG_OPEN ;
    }

    if(agent && agent.indexOf("linux") != -1)
    {
       platform = "linux";
       executable = LINUX_PROG_OPEN ;
    }

    if(agent && agent.indexOf("nt 5.0") != -1)
    {
        platform = "windows";
        executable = WIN_2000_PROG_OPEN ;
        doWindows = 1;
    }

    if(agent && agent.indexOf("nt 5.1") != -1)
    {
        platform = "windows";
        executable = WIN_XP_PROG_OPEN ;
        doWindows = 1;
    }

    recordMessage("openEnrolledTokenURLBrowser platform: " + platform);
    if(!platform)
    {
        return;
    }

    //Now get enrolled token URL

    var enrolled_token_uri = null;

    if(netkey)
    {
        enrolled_token_uri = GetCachedEnrolledTokenBrowserURL(aKeyID); 

        recordMessage("openEnrolledTokenURLBrowser uri: " + enrolled_token_uri);

        if(!enrolled_token_uri)
        {
            return; 
        }
    }

    // create an nsILocalFile for the executable
    var file = Components.classes["@mozilla.org/file/local;1"]
                     .createInstance(Components.interfaces.nsILocalFile);

    file.initWithPath(executable);

   // create an nsIProcess
    var process = Components.classes["@mozilla.org/process/util;1"]
                        .createInstance(Components.interfaces.nsIProcess);

    process.init(file);

    // Run the process.
    // If first param is true, calling process will be blocked until
    // called process terminates. 
    // Second and third params are used to pass command-line arguments
    // to the process.

    var args;

    if(doWindows)
    {
        args = ["/c","start",enrolled_token_uri];
    }
    else
    {
        args = [enrolled_token_uri];
    } 

    process.run(false, args, args.length);

}

function getESCEnrollmentUI()
{
    var esc_enroll_uri = null;

   if(netkey)
   {
        try {
                netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
            esc_enroll_uri = netkey.GetCoolKeyConfigValue("esc.enroll.ui.url");

            } catch(e) {
                MyAlert(getBundleString("errorConfigValue") + e);
            }
 
    }

    return esc_enroll_uri;
}

function grantPrivilegesURL(aURL)
{
    const NETWORK_STD_CID="@mozilla.org/network/standard-url;1";

    if(!netkey)
    {
        return;
    }

    if(!aURL)
    {
        return;
    }

    var theURL = Components.classes[NETWORK_STD_CID].createInstance();
    theURL = theURL.QueryInterface(Components.interfaces.nsIURI);

    if(theURL)
    {
        theURL.spec = aURL;
    }

    var prePath =theURL.prePath;

    if(prePath)
    {
        try {
                netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            res = netkey.SetCoolKeyConfigValue("capability.principal.codebase.p0.id",prePath);

            } catch(e) {
                MyAlert(getBundleString("errorSetConfigValue") + " "  + e);
                return;
            }

    }

}

//Utility function to set a CoolKey Configuration Value
function DoCoolKeySetConfigValue(configValue,newValue)
{
   if(!configValue || !newValue)
      return null;

   var result = null;

   if(netkey)
   {
      try {
            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            result = netkey.SetCoolKeyConfigValue(configValue,newValue);

            } catch(e) {
                MyAlert(getBundleString("errorConfigValue") + " " + e);
      }

    }

    return result;
}
//Utility function to get a CoolKey Configuration Value
function DoCoolKeyGetConfigValue(configValue)
{
   var result = null;

   if(!configValue)
       return null;

   if(netkey)
   {
      try {
            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            result = netkey.GetCoolKeyConfigValue(configValue);

            } catch(e) {
                MyAlert(getBundleString("errorConfigValue") + " " + e);
      }

    }

    return result;
}
//Return how many cards are plugged in.
function DoGetNumCoolKeys()
{
    var num = 0;
    var arr = GetAvailableCoolKeys();
    if (arr && arr.length )
        num = arr.length;

    return num;
}

//Return how many unenrolled cards are plugged in.
function DoGetNumUnenrolledCoolKeys()
{
    var num = 0;
    var arr = GetAvailableCoolKeys();
    if (arr && arr.length )
    {
        for (i=0; i < arr.length; i++)
        {
            var status =  GetStatusForKeyID(arr[i][0],arr[i][1]);

            if(status  != getBundleString("statusEnrolled"))
                num++;

        }
    }

    return num;

}


//Is this really a CoolKey and not a CAC card?
function DoGetCoolKeyIsReallyCoolKey(keyType,keyID)
{
    if(!keyType && !keyID)
        return 0;

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var isCool =  netkey.GetCoolKeyIsReallyCoolKey(keyType, keyID);

      return isCool;
    } catch (e) {

        return 0;
    }
}
//Get burned in card issuer url
function DoCoolKeyGetIssuerUrl(keyType,keyID)
{
    var url = null;
    var isMac = 0;

    var agent = navigator.userAgent.toLowerCase();
    //Back door for testing, ignore the value if so configured

    if(agent && agent.indexOf("mac") != -1)  {
        isMac = 1;
    }

    var ignoreIssuer =  DoCoolKeyGetConfigValue(ESC_IGNORE_KEY_ISSUER_INFO);

    recordMessage("DoCoolKeyGetIssuerUrl agent " + agent);

    if(ignoreIssuer == "yes")
    {
        recordMessage("Ignoring issuer url returning null!");
        return url;
    }

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

      var tries = 0;
      while(tries < 3 )  
      { 
          url =  netkey.GetCoolKeyIssuerInfo(keyType, keyID);

          if(!isMac)  {  
              break;
          }

          if(!url ||  url.length < 10)   // Check for bogus junk
          {
              recordMessage("Bogus url found .... " + url);
              url = null;
              Sleep(250);
              recordMessage("Going to try again... ");
          }
          else
              break;

          tries ++;
      }

      if(url)
      {
          var issuer_config_value = ConfigValueWithKeyID(keyID,KEY_ISSUER_URL);
          var result = DoCoolKeySetConfigValue(issuer_config_value,url);
      }
    } catch (e) {
      recordMessage("Exception attempting to get token issuer info.");

      var tries = 0;
      while(tries < 3)
      {
          url =  netkey.GetCoolKeyIssuerInfo(keyType, keyID);

          if(!isMac)  {  
              break;
          }
          if(!url || url.length < 10)   // Check for bogus junk
          {
              recordMessage("Bogus url found from exception....");
              url = null;
              sleep(250);
              recordMessage("From exception.  Going to try again... ");
          }
          else
              break;

          tries ++;
      }
      if(url)
      {
          var issuer_config_value_exp = ConfigValueWithKeyID(keyID,KEY_ISSUER_URL);
          var result_exp = DoCoolKeySetConfigValue(issuer_config_value_exp,url);
      }

      if(isMac && !url) {
          var phoneHomeUrl = GetCachedPhoneHomeURL(keyID);
          if(phoneHomeUrl)  {
             url = phoneHomeUrl;
          }

      }
      recordMessage("From exception returning " + url);
      return url;
  }

  if(isMac && !url) {
      var phoneHomeUrl = GetCachedPhoneHomeURL(keyID);
      if(phoneHomeUrl)  {
         url = phoneHomeUrl;
      }
  }

  return url;
}

//Get ATR value of card
function DoCoolKeyGetATR(keyType,keyID)
{
    var atr = null;
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      atr =  netkey.GetCoolKeyATR(keyType, keyID);
    } catch (e) {
      return atr;
  }

  return atr;
}

//Get Token Name of card
function DoCoolKeyGetTokenName(keyType,keyID)
{
    if(!keyType && !keyID)
        return null;

    var name = null;
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      name =  netkey.GetCoolKeyTokenName(keyType, keyID);
    } catch (e) {
      return name;
    }

    return name;
}

//Get applet version of card
function DoGetCoolKeyGetAppletVer(keyType, keyID , isMajor)
{
    var ver = -1;

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var ver  =  netkey.GetCoolKeyGetAppletVer(keyType, keyID,isMajor);
    } catch (e) {
      ver = -1;
  }
  return ver;
}

//Is factory mode configured?
function CheckForFactoryMode()
{
  var factory= DoCoolKeyGetConfigValue("esc.factory.mode");

  if(factory == "yes")
  {
      gFactoryMode = 1;
  }

}

function ShowUsage()
{
    var usageStr = getBundleString("escUsage1")  + "\n";
    usageStr += getBundleString("escUsage2") + "\n";
    usageStr += getBundleString("escUsage3") + "\n";
    usageStr += getBundleString("escUsage4") + "\n";
    usageStr += getBundleString("escUsage5"); 

    MyAlert(usageStr);
}

function ShowVersion()
{
    var verStr = getBundleString("coolkeyComponentVersion")  + "\n\n";

    MyAlert(verStr + " " + GetCoolKeyVersion());

}

//Is the security mode up?
function CheckForSecurityMode()
{
    var securityWnd = IsPageWindowPresent(SECURITY_WINDOW);
    var faceToFaceMode = 0;

    recordMessage("CheckForSecurityMode: " + securityWnd);

    if(securityWnd) {
        faceToFaceMode = 1;
        return faceToFaceMode;
    }

    var securityURL =   DoCoolKeyGetConfigValue(ESC_SECURITY_URL);

    if(securityURL)
    {
        faceToFaceMode = 1;
    }

    return faceToFaceMode;
}

//Launch Phone Home bootstrap dialog as last resort
function launchCONFIG(keyType,keyID)
{
    var agent = navigator.userAgent.toLowerCase();

    var platform = "";

    if(agent && agent.indexOf("mac") != -1)
    {
        platform = "mac";
    }

    var wind = null;

    if(platform == "mac")
    {
        wind = window.openDialog("chrome://esc/content/config.xul",keyID,"chrome,centerscreen,resizable,modal=no");
    }
    else
    {
        wind = window.openDialog("chrome://esc/content/config.xul",keyID,"chrome,centerscreen,resizable,modal=yes");

    }
}

//Launch cert viewer if key has certs

function launchCertViewerIfCerts()
{
  var row = null;

  if(gCurrentSelectedRow)
     row = gCurrentSelectedRow;

  if(!row)
      return;

  var theID = row.getAttribute("id");

  if (!theID)
    return;

  var keyInfo = RowIDToKeyInfo(theID);

  var status = GetStatusForKeyID(keyInfo[0],keyInfo[1]);

  if(status == getBundleString("statusEnrolled"))
  {
      launchCertViewer();
  }
}

//Launch page to view card's certificates
function launchCertViewer()
{
   var wind = window.openDialog("chrome://esc/content/certManager.xul", "","chrome,centerscreen,modal=yes");
}

//Launch ESC admin page window
function launchSETTINGS()
{

    var adminWnd  = IsPageWindowPresent(ADMIN_WINDOW);

    if(!adminWnd)
    {
        var wind = window.open("chrome://esc/content/settings.xul","","chrome,resizable,centerscreen,dialog");
    } else
    {
        adminWnd.focus();
    }
}

//Launch ESC enrollment window
function launchESC()
{
    var enrollWnd = IsPageWindowPresent(ENROLL_WINDOW);

    if(!enrollWnd)
    {
        var wind = window.open("chrome://esc/content/esc.xul","","chrome,resizable,centerscreen,dialog");

    }
    else
    {
        enrollWnd.focus();
    }

}


//Launch security mode window

function launchESCSecMode()
{
    recordMessage("In launchESCSecMode");

    var secWnd = IsPageWindowPresent(SECURITY_WINDOW);
    if(!secWnd)
    {
        recordMessage("About to launch security window.");
        var wind = window.open("chrome://esc/content/security.xul","","chrome,resizable,centerscreen,dialog");
         wind.focus();
    }
    else
    {
        secWnd.focus();
    }
}

function UpdateESCSize(newWidth,newHeight)
{
    if(!gEnrollmentPage && !gAdminPage)
        return;

    if(newWidth && newHeight)
    {
        window.resizeTo(newWidth,newHeight);
    }
    else
    {
        window.sizeToContent();
    }
}

//Receive Tray Notifications

function NotifyESCOfTrayEvent(aEvent,aEventData,aKeyData,aData1,aData2)
{
   //alert("NotifyESCOfTrayEvent window" + window.location + "gHiddenPage " + gHiddenPage );

    if(!gHiddenPage)
        return;

   //Get the primary page windows if present

   var enrollWnd = IsPageWindowPresent(ENROLL_WINDOW);
   var adminWnd  = IsPageWindowPresent(ADMIN_WINDOW);

   var securityURL =   DoCoolKeyGetConfigValue(ESC_SECURITY_URL);

   if(securityURL)
   {
       launchESCSecMode();
       return;

   }

   if(!adminWnd)
   {
        launchSETTINGS();
   }
   else
   {
       adminWnd.focus();
   }

}

//Utility DOM functions

//Change a static text item on page in code

function ChangeDescription(theDesc,theNewText)
{
    if(!theDesc || !theNewText)
        return;

   RemoveAllChildNodes(theDesc);
   var theLabel = document.createTextNode(theNewText);

   theDesc.appendChild(theLabel);
   
   UpdateESCSize(); 

}

function HideItem(theItem)
{
    if(theItem)
       theItem.setAttribute("hidden","true");
}

function ShowItem(theItem)
{
    if(theItem)
       theItem.setAttribute("hidden","false");
}

function EnableItem(theItem)
{
    if(theItem)
        theItem.setAttribute("disabled","false");
}

function DisableItem(theItem)
{
    if(theItem)
        theItem.setAttribute("disabled","true");
}


// Phone Home Related Functions

function IsPhoneHomeCached(aKeyID)
{
    if(!aKeyID)
        return false;

    var phoneHomeUrl = null;
    var phoneHomeIssuer = null;
    var tpsURL = null;
    var tpsUI = null;

    phoneHomeUrl = GetCachedPhoneHomeURL(aKeyID);

    if(!phoneHomeUrl)
    {
        CoolKeyLogMsg(PR_LOG_ALWAYS,"IsPhoneHomeCached keyID:  " + aKeyID + " IsCached: false " );
        return false;
    }

    phoneHomeIssuer = GetCachedIssuer(aKeyID);

    if(!phoneHomeIssuer)
    {
       CoolKeyLogMsg(PR_LOG_ALWAYS,"IsPhoneHomeCached keyID:  " + aKeyID + " IsCached: false " );
       return false;
    }

    tpsURL = GetCachedTPSURL(aKeyID);

    if(!tpsURL)
    {
        CoolKeyLogMsg(PR_LOG_ALWAYS,"IsPhoneHomeCached keyID:  " + aKeyID + " IsCached: false " );
        return false;
    }

    tpsUI = GetCachedTPSUI(aKeyID);

    CoolKeyLogMsg(PR_LOG_ALWAYS,"IsPhoneHomeCached keyID:  " + aKeyID + " IsCached: true " );
    return true;
}

function GetCachedPhoneHomeValue(aKeyID,aValue)
{
     var retValue = null;

     if(!aKeyID || ! aValue)
         return null;

     var theValue = ConfigValueWithKeyID(aKeyID,aValue);

     if(!theValue)
         return null;

     retValue = DoCoolKeyGetConfigValue(theValue);

     return retValue;
}

function GetCachedTokenType(aKeyID)
{
    return GetCachedPhoneHomeValue(aKeyID,TOKEN_TYPE);
}

function GetCachedEnrolledTokenURL(aKeyID)
{
    return GetCachedPhoneHomeValue(aKeyID,ENROLLED_TOKEN_URL);
}
function GetCachedEnrolledTokenBrowserURL(aKeyID)
{
     return GetCachedPhoneHomeValue(aKeyID,ENROLLED_TOKEN_BROWSER_URL);
}

function GetCachedPhoneHomeURL(aKeyID)
{
     var url = null;

     if(!aKeyID)
         return null;

     var urlValue = ConfigValueWithKeyID(aKeyID,KEY_ISSUER_URL);

     if(!urlValue)
         return null;

     url = DoCoolKeyGetConfigValue(urlValue);

     return url;
}

function GetCachedTPSUI(aKeyID)
{
     var ui = null;

     if(!aKeyID)
         return null;

     var uiValue = ConfigValueWithKeyID(aKeyID,TPS_UI);

     if(!uiValue)
         return null;

     ui = DoCoolKeyGetConfigValue(uiValue);

     return ui;
}

function GetCachedIssuer(aKeyID)
{
     var issuer = null;

     if(!aKeyID)
         return null;

     var issuerValue = ConfigValueWithKeyID(aKeyID,KEY_ISSUER);

     if(!issuerValue)
         return null;

     issuer = DoCoolKeyGetConfigValue(issuerValue);
     return issuer;
}

function GetCachedTPSURL(aKeyID)
{
     var url = null;

     if(!aKeyID)
         return null;

     var urlValue = ConfigValueWithKeyID(aKeyID,TPS_URL);

     if(!urlValue)
         return null;

     url = DoCoolKeyGetConfigValue(urlValue);

     return url;
}

//Log information about the key

function LogKeyInfo(aKeyType,aKeyID,aMessage)
{
    var issuer = GetCoolKeyIssuer(aKeyType,aKeyID);
    var status =  GetStatusForKeyID(aKeyType, aKeyID);
    var atr =     DoCoolKeyGetATR(aKeyType,aKeyID);
    var tpsURI = GetCachedTPSURL(aKeyID);
    var tpsUI  = GetCachedTPSUI(aKeyID);
    var phoneHomeURI = GetCachedPhoneHomeURL(aKeyID);

    if(aMessage)
        CoolKeyLogMsg(PR_LOG_ALWAYS,aMessage);

    CoolKeyLogMsg(PR_LOG_ALWAYS,"CoolKey ID: " + aKeyID);

    if(issuer)
        CoolKeyLogMsg(PR_LOG_ALWAYS,"CoolKey Issuer: " + issuer);

    if(status)
        CoolKeyLogMsg(PR_LOG_ALWAYS,"CoolKey Status: " + status);

    if(atr)
        CoolKeyLogMsg(PR_LOG_ALWAYS,"CoolKey Atr: " + atr);

    if(phoneHomeURI)
        CoolKeyLogMsg(PR_LOG_ALWAYS,"CoolKey Phone Home URI: " + phoneHomeURI);
}


//Update display when phone home data comes in
function UpdateRowWithPhoneHomeData(keyType,keyID)
{
    if(gEnrollmentPage)
    {
        UpdateEnrollmentArea(keyType,keyID,1);
    }


    var adminWnd  = IsPageWindowPresent(ADMIN_WINDOW);

    if(adminWnd)
    {
          adminWnd.SelectRowByKeyID(keyType, keyID);
          adminWnd.UpdateAdminListRow(keyType,keyID);
          adminWnd.UpdateAdminKeyDetailsArea(keyType,keyID);
    }
}

//Phone home and actually get data values
function phoneHome(theUrl,aKeyID,resultCB)
{

   CoolKeyLogMsg(PR_LOG_ALWAYS,"Actually phoning Home for Key: " + aKeyID + " URI: " + theUrl); 
   var url = null ;

   if(!theUrl || !aKeyID)
       return false;

    if(theUrl != null) {
        url = theUrl + "?cuid=" + aKeyID;
    }

    var req = new XMLHttpRequest();
    req.overrideMimeType('text/xml');
   
    var async = true;

    req.open('GET', url, true);

    var callback = function () {

        var result = false;
        if (req.readyState == 4) {
            var response = req.responseXML;

            if(!response)
            {
                if(resultCB)
                   resultCB(false);
                return false;
            }
            var root = response.getElementsByTagName("ServiceInfo");

            if(!root)
            {
               if(resultCB)
                   resultCB(false);
               return false;
            }

            var issuer = response.getElementsByTagName("IssuerName").item(0);

            if(!issuer)
            {
               if(resultCB)
                   resultCB(false);
               return false;
            }

            var theIssuer = issuer.lastChild.data;

            if(theIssuer)
            {
                 var issuer_config_value = ConfigValueWithKeyID(aKeyID,KEY_ISSUER);
                 if(issuer_config_value)
                 {
                     DoCoolKeySetConfigValue(issuer_config_value,theIssuer);

                 }
            }

            var services = response.getElementsByTagName("Services");

            if(!services)
            {
                if(resultCB)
                    resultCB(false);
                return false;
            }

            var servicesNodes = services[0].childNodes;
            var len = servicesNodes.length;

            for (var i = 0; i < len; i++)
            {
                var oChild = servicesNodes.item(i);
                
                var name=null;
                var value = null;
                if(oChild)
                {
                   name = oChild.nodeName;

                   if(oChild.firstChild)
                       value = oChild.firstChild.data;

                }

                if(name && value)
                {
                    var cValue = ConfigValueWithKeyID(aKeyID,name);

                    if(cValue)
                    {
                        recordMessage("Writing out config : " +cValue + " value: " + value);
                        DoCoolKeySetConfigValue(cValue,value);
                        CoolKeyLogMsg(PR_LOG_ALWAYS,"Phone Home config value for Key: " + aKeyID + " ConfigKey: " + cValue + " ConfigValue: " + value);

                    }
                }
               
            }

            recordMessage("Done writing out phone home config cache.");
            var browserURL =  GetCachedEnrolledTokenBrowserURL(aKeyID);
            recordMessage("Cached browserURL " + browserURL);

            if(browserURL)
            {
                DoCoolKeySetConfigValue(ESC_TOKEN_BROWSER_URL_ESTABLISHED,"yes");
                DoHandleEnrolledBrowserLaunch();

            }

            if(resultCB)
            {
                recordMessage("About to write out KEY_ISSUER_URL value.");
               //Manually write out entry for phone home url

                var issuer_config_value = ConfigValueWithKeyID(aKeyID,KEY_ISSUER_URL);
                var result = DoCoolKeySetConfigValue(issuer_config_value,url);
                CoolKeyLogMsg(PR_LOG_ALWAYS,"Phone Home config value for Key: " + aKeyID + " ConfigKey: " + issuer_config_value + " ConfigValue: " + url);
                resultCB(true);
            }
            return true;
        }
     }

     req.onreadystatechange = callback;
     req.send(null);

     return true;
}

function ReadESCLog()
{
    const logFileName = "esc.log";

    // Get executable directory

    var file = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties)
                     .get("ProfD", Components.interfaces.nsIFile);

    file = file.parent;
    file.append(logFileName);
    var istream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                        .createInstance(Components.interfaces.nsIFileInputStream);

    try {
        istream.init(file, 0x01, 0444, 0);
        istream.QueryInterface(Components.interfaces.nsILineInputStream);

    } catch (e){

        return null;
    }
   // read lines into array
    var line = {}, lines = [], hasmore;
    do {
        hasmore = istream.readLine(line);

        var value = line.value;

        //var colonIndex = value.indexOf(":");

        //value = value.substring(colonIndex + 1);

        lines.push(value); 
    } while(hasmore);

    istream.close();

    return lines;

}

// Get the url form of the balloon tooltip icon
// Only useful on Linux

function GetESCNotifyIconPath(keyType,keyID)
{
    var path = null;
    var def_image = "/components/icon.png";

    var chrome_path = "/chrome/content/esc/";

    var image = null;

     var status;

     try {
         status = GetCoolKeyStatus(keyType, keyID);
     } catch(e) {
        status = 0;
     }

     if(status == 1) // no applet
     {
         image = chrome_path + "blank-card.png";
     }
     if(status == 2) // uninitialized
     {
         image = chrome_path + "initializecard.png";
     }
     if(status == 4) // enrolled
     {
         image = chrome_path + "enrolled-key.png";
     }

     if(!image)
         image = def_image;

     var file = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties)
                     .get("CurProcD", Components.interfaces.nsIFile);

    if(file)
      file = file.parent;

    if(file.path)
        path = "file://" + file.path + image;

    return path;
}

// New Alert Functions based on nsIPromptService

function MyAlert(message)
{
    if(message)
        DoMyAlert(message,getBundleString("escTitle")); 
}

function DoMyAlert(message,title)
{
   if(!message || !title)
       return;     try {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
       var prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService); 

       prompts.alert(window,title,message);

   } catch(e) {

   }
}

//Utility function to sleep for a short time

function Sleep(milliSeconds)
{
    var then = new Date(new Date().getTime() + milliSeconds ); while (new Date() < then) {}
}

function DoCopyAdvancedInfoToClipBoard()
{
    var textinfo = window.document.getElementById("advanced-info");

    CopyDataToClipboard(gDiagnosticsDataText);
}

function CopyDataToClipboard(aDataText)
{
   if(!aDataText)
       return false;

    var str = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString); 

    if (!str) 
       return false; 

    str.data = aDataText; 

    var trans = Components.classes["@mozilla.org/widget/transferable;1"]. createInstance(Components.interfaces.nsITransferable); 

    if (!trans) 
        return false; 

    trans.setTransferData("text/unicode",str,str.data.length *2); 
    var clipid = Components.interfaces.nsIClipboard; 

    var clip = Components.classes["@mozilla.org/widget/clipboard;1"].getService(clipid); 

    if (!clip) 
        return false; 

    clip.setData(trans,null,clipid.kGlobalClipboard);
    MyAlert(getBundleString("dataCopiedToClipboard") );

    return true;
}

//Write message to Javascript console if open

function recordMessage( message ) {

  var consoleService = Components
  .classes['@mozilla.org/consoleservice;1']
  .getService( Components.interfaces.nsIConsoleService );

  if(consoleService)
      consoleService.logStringMessage("esc: " + message  + "\n");
}

function GetEnvironmentVar(aVar)
{
    if(!aVar)
        return null;

    var environ =     Components.classes["@mozilla.org/process/environment;1"]
      .getService(Components.interfaces.nsIEnvironment);


    var retVar = null;

    if(environ)
        retVar =   environ.get(aVar); 

    //alert("var: " + aVar + " value: " + retVar);

   return retVar;
}

function SetEnvironmentVar(aVar,aValue)
{
    if(!aVar || !aValue)
        return ;

    var environ =     Components.classes["@mozilla.org/process/environment;1"]
      .getService(Components.interfaces.nsIEnvironment);

    if(environ)
        retVar =   environ.set(aVar,aValue);
}

function IsKeyLoginKey(keyType,keyID)
{
    var result = 0;

    var token_name = DoCoolKeyGetTokenName(keyType,keyID);
    var login_token_name = GetEnvironmentVar("PKCS11_LOGIN_TOKEN_NAME");

    if(token_name == login_token_name)
    {
        result = 1;
    }

    return result;
}

function AdminToggleStatusProgress(aOn,keyType,keyID)
{
    if(!gAdminPage)
        return;

    var statusCell   = document.getElementById(KeyToCellID(keyType,keyID,"status"));

    if(!statusCell)
        return;

    var progMeterID = KeyToProgressBarID(keyType, keyID);
    if(!progMeterID)
        return;

    var meter = document.getElementById(progMeterID);

    if(!meter)
        return;

    if(aOn)
    {
        HideItem(statusCell);
        ShowItem(meter);
    }
    else
    {
        HideItem(meter);
        ShowItem(statusCell);

        var adminList = document.getElementById("AdminBindingList");
        if(adminList)
            adminList.focus();
    }
}

function GetESCLogPathName(aName)
{

    if(!aName)
        return null;

    const logFileName = aName;

    // Get executable directory

    var file = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties)
                     .get("ProfD", Components.interfaces.nsIFile);

    file = file.parent;
    file.append(logFileName);

   
    //alert("LogPathName " + file.path); 


    return file.path;


}
