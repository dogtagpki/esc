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
var gUsesListBox = 0;
var gFactoryMode = 0;
var gHiddenPage = 0;
var gHiddenPageDone = 0;
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

const  UNINITIALIZED        = 1;
const  UNINITIALIZED_NOAPPLET = 2;
const  ESC_ENROLL_WIDTH  = 600;
const  ESC_ENROLL_HEIGHT = 570;

//Window names

const ENROLL_WINDOW      = "esc.xul";
const ADMIN_WINDOW       = "settings.xul";
const HIDDEN_WINDOW      = "hiddenWindow.xul";


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
    <!--  alert("iid: " + iid); -->
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
  try {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey = Components.classes["@redhat.com/rhCoolKey"].getService();
    netkey = netkey.QueryInterface(Components.interfaces.rhICoolKey);
    gNotify = new jsNotify;
    netkey.rhCoolKeySetNotifyCallback(gNotify);
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

var gScreenName = "";
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
    getBundleString("serverError"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorProblemResetTokenPin"),
    getBundleString("errorInternalServer"),
    getBundleString("errorInternalServer"),
    getBundleString("errorTokenEnrollment"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorInternalServer"),
    getBundleString("errorCommCA"),
    getBundleString("errorInternalServer"),
    getBundleString("errorResetPin"),
    getBundleString("errorInternalServer"),
    getBundleString("errorAuthFailure"),
    getBundleString("errorInternalServer"),
    getBundleString("errorTokenDisabled"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorInternalServer"),
    getBundleString("errorTokenUpgrade"),
    getBundleString("errorInternalServer"),
    getBundleString("errorProblemCommToken"),
    getBundleString("errorInvalidTokenType"),
    getBundleString("errorInvalidTokenType"),
    getBundleString("errorCannotPublish"),
    getBundleString("errorCommTokenDB"),
    getBundleString("errorTokenDisabled"),
    getBundleString("errorPinReset"),
    getBundleString("errorConnLost"),
    getBundleString("errorEntryTokenDB"),
    getBundleString("errorNoTokenState"),
    getBundleString("errorInvalidLostTokenReason"),
    getBundleString("errorTokenUnusable"),
    getBundleString("errorNoInactiveToken"),
    getBundleString("errorProcessMultiTokens"),
    getBundleString("errorInternalServer"),
    getBundleString("errorKeyRecoveryProcessed"),
    getBundleString("errorKeyRecoveryFailed"),
    getBundleString("errorNoOperateLostToken"),
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

//silent set of the tps uri connection on startup

function testTPSURISilent()

{

   var tps_uri = null;

   if(netkey)
   {
      try {
            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            tps_uri = netkey.GetCoolKeyConfigValue("esc.tps.url");


            } catch(e) {
                MyAlert(getBundleString("errorConfigValue")  + e);
            }

        if(tps_uri)
        {
            testTPSURI(tps_uri);

        }
    }

}

// Main function that oversees obtaining Phone Home Info from the Server

function DoPhoneHome(keyType,keyID)
{
  var callback = function (aResult) {

    var issuer = "";
    if(aResult == true)
    {
        issuer = GetCachedIssuer(keyID);       
        if(!issuer)
            issuer = getBundleString("unknownIssuer");
        TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
        UpdateRowWithPhoneHomeData(keyType,keyID);
    }
    else
    {
        issuer = getBundleString("unknownIssuer");
        TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
        //launchCONFIG(keyType,keyID);
    }
  }

  if(IsPhoneHomeCached(keyID))
  {
      issuer = GetCachedIssuer(keyID);
      TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
      return true;
  }

  var home = DoCoolKeyGetIssuerUrl(keyType,keyID);

  var homeRes = false;

  if(home)
  {
      homeRes =  phoneHome(home,keyID,callback);
  }

  if(!home)
  {
      launchCONFIG(keyType,keyID);
  }

  return homeRes;
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

    if(window.opener && name)
    {
        window.opener.UpdateRowWithPhoneHomeData(1,name);
    }

    window.close();
}

//Test the tps uri connection

function testTPSURI(aURL)
{

   var search = "<HTML>Registration Authority</HTML>";

    var tps_uri_box = document.getElementById("tpsuri");

    var uri = null;


    if(tps_uri_box )
    {
        uri = tps_uri_box.value;
    } else
    {
        if(aURL )
        {
            uri = aURL;
        }
    }

    if(!uri)
    {
        MyAlert(getBundleString("errorBlankTPSURI"));
        return;
    }

    if(!aURL)
    {
        MyAlert(getBundleString("aboutToTestTPSURI") + uri);
    }

    req = new XMLHttpRequest();
    req.open('GET', uri, true);


    var response_text = null;

    var callback = function () {

        if (req.readyState == 4) {

            response_text = req.responseText;

            var index = response_text.indexOf(search);

            if(index == 0 && !aURL)
            {
                 MyAlert(getBundleString("tpsURLContacted"));
            }
            else
            {
                if(!aURL)
                {
                    MyAlert(getBundleString("errorContactTPSURL"));
                }
            }
         }
    }

     req.onreadystatechange = callback;
     req.send(null);  

}

function testEnrollURI()
{

    var search = "Enterprise";

    var tps_enroll_box = document.getElementById("tpsenrolluri");

    var uri = null;
    var uri = tps_enroll_box.value;

    if(!uri)
    {
        MyAlert(getBundleString("errorBlankEnrollURI"));
        return;
    }

    MyAlert(getBundleString("aboutToTestTPSURI"));

    req = new XMLHttpRequest();
    req.open('GET', uri, true);

    var response_text = null;

    var callback = function () {

        if (req.readyState == 4) {

            response_text = req.responseText;

            var index = response_text.indexOf(search);

            if(index != -1)
            {
                 MyAlert(getBundleString("enrollURLContacted"));
            }
            else
            {
                MyAlert(getBundleString("errorContactEnrollURL"));
            }
         }
    }

     req.onreadystatechange = callback;
     req.send(null);

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

   keyUITable[aKeyID] = aUiData;
   keyTypeTable[aKeyID] = aKeyType;

   var child =  window.open("chrome://esc/content/GenericAuth.xul", aKeyID, "chrome,width=400,height=250");
 
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
                MyAlert(getBundleString("errorSetDataValue") + e);
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

    for(i = 0 ; i < 48; i++)
    {
        MyAlert(Status_Messages[i]);

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
    ReportException(getBundleString("errorCoolKeyGetPolicy"), e);
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

    for(i = 0 ; i < arr.length ; i++)
    {
       keyID = arr[i][1];
       keyType = arr[i][0];

       var appletVerMaj = DoGetCoolKeyGetAppletVer(keyType, keyID , true);
       var appletVerMin = DoGetCoolKeyGetAppletVer(keyType, keyID, false);

       var issuer = GetCachedIssuer(keyID);
       if(!issuer)
           issuer = getBundleString("unknownIssuer");

       textDump += "***" + getBundleString("smartCardU") + " " + i + ":" + "***" + "\n\n";

       textDump += "  " + getBundleString("appletVersion") + " " + appletVerMaj + "." + appletVerMin + "\n";


       var status =  GetStatusForKeyID(keyType, keyID);

       textDump += "  " + getBundleString("keyID") + " " + " " +  keyID  + "\n";
       textDump += "  " + getBundleString("status") + " " + " " + status + "\n";
       textDump += "  " + getBundleString("issuer") + " " + " " + issuer + "\n";

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
           for (i = 0; i < nicknames.length ; i ++)
           {
                textDump += "    " + getBundleString("certificateNickname") + " " + nicknames[i] + " \n\n";

                cert_info = GetCoolKeyCertInfo(keyType,keyID,nicknames[i]);

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

function DoShowKeyInfo()
{
  var doCertInfo = 1;
  var test = null;

  if (gCurrentSelectedRow)
  {
    var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));

    var keyType = keyInfo[0];
    var keyID = keyInfo[1];


    dump += "<b> <center>" + getBundleString("keyID") + "</center></b><center> " +  keyID + "</center>  ";
    var status =  GetStatusForKeyID(keyType, keyID); 

    dump += "<b><center>" + getBundleString("status") + "</b> </center><center>" + status + " </center><br><hr>";

    var nicknames  = GetCoolKeyCertNicknames(keyType,keyID);

    if(nicknames && nicknames.length)
    {
        dump += "<center><b>" + getBundleString("certsOnToken") + "</b><center> ";
    } 

    if(nicknames)
    {
        var cert_info = null;
        for (i = 0; i < nicknames.length ; i ++)
        {
            cert_info = GetCoolKeyCertInfo(keyType,keyID,nicknames[i]);
            dump += "<b>" + getBundleString("certificateNickname") +"</b><br> " + nicknames[i] + " <br><br>";

            if(doCertInfo)
            {
                dump +=  cert_info + "  <br><hr>";
            }
        }
    } 

  var wnd = window.openDialog("chrome://esc/content/certinfo.xul","Info","chrome,centerscreen,width=350,height=425",dump);

   }
   else
   {
       MyAlert(getBundleString("noCurrentlySelectedToken"));
   }
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

function BlinkCoolKey(keyType, keyID, rate, duration)
{
  try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    netkey.BlinkCoolKey(keyType, keyID, rate, duration);
  } catch(e) {
    ReportException(getBundleString("errorBlinkCoolKey"), e);
    return false;
  }
  return true;
}

function RemoveTreeRow(table, row)
{

     var rows = document.getElementById("BindingTreeChildren");

    if(rows)
        rows.removeChild(row);

}

//
// /GECKO DOM functions.
//
function RemoveRow(table, row)
{
     var rows = document.getElementById("BindingTableRows");

    if(rows)
        rows.removeChild(row);
}

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

function InsertGridRow(grid)
{
  var rows = document.getElementById("BindingTableRows");

  if(!rows)
      return null;

  var row =  document.createElement("row");
  var newnode = rows.appendChild(row);

  row.setAttribute("pack","center");
  return row;
}

function SelectImageForKeyStatus(keyStatus,observeBusy,doubleSize)
{

  var image_src = "";

  if(observeBusy && (keyStatus == "BUSY" || keyStatus == "UNAVAILABLE"))
  {
      return "throbber-anim5.gif";
  }
  if(keyStatus == "ENROLLED")
  {
      image_src = "enrolled-key";
  }
  else
  {
          if(keyStatus == "UNINITIALIZED")
              image_src = "initializecard";
           else
               if(keyStatus == "NO APPLET")
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

function DoShowFullEnrollmentUI()
{
   if (!gCurrentSelectedRow)
   {
       MyAlert(getBundleString("errorSelectKey"));
       return;
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
         ChangeDescription(enroll_proceed_message,getBundleString("enrollAnyway"));
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

         UpdateButtonStates();
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

  table = document.getElementById("BindingTable");

  if (row && table)
  {
    RemoveRow(table,row);
    InsertCoolKeyIntoBindingTable(keyType, keyID);
    row = GetRowForKey(keyType,keyID);

    if(row)
    {
         SelectRow(row);
    }

  }

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
    case 7: // PINResetInProgress
    case 8: // RenewInProgress
      keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
      break;
    case 5: // EnrollmentInProgress
      keyStatus = getBundleString("statusBusy");
      break;
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

function InsertCoolKeyIntoBindingTable(keyType, keyID)
{
  var row = GetRowForKey(keyType, keyID);

  if (!row)
  {
    var table = document.getElementById("BindingTable");
    var issuer = GetCoolKeyIssuer(keyType,keyID);
    var issuedTo = GetCoolKeyIssuedTo(keyType,keyID);

    if (table)
    {
      var keyStatus = GetStatusForKeyID(keyType, keyID);
      var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
      var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

      row = CreateTableRow(table, keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed,issuer,issuedTo);
    }

    if (!row)
      return null;
  }

  return row;
}

function UpdateCoolKeyAvailabilityForEnrollment()
{
  //Here we only allow ONE key
  //Take the first one that shows up.

  var arr = GetAvailableCoolKeys();

  if (!arr || arr.length < 1)
  {
    UpdateESCSize();
    return;
  }

  var i;

  for (i=0; i < 1; i++)
  {
      var row = InsertCoolKeyIntoEnrollmentPage(arr[i][0],arr[i][1]);

      if(row)
        gCurrentSelectedRow = row;

       var keyInserted = 1;
       var showFullUI = 0;
       UpdateEnrollmentArea(arr[i][0],arr[i][1],keyInserted,showFullUI);

  }

  UpdateButtonStates();
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
}

function UpdateBindingTableAvailability()
{
  var arr = GetAvailableCoolKeys();

  if (!arr || arr.length < 1)
    return;

  var i;

  for (i=0; i < arr.length; i++)
  {
    InsertCoolKeyIntoBindingTable(arr[i][0], arr[i][1]);

    if (!gCurrentSelectedRow)
      SelectRowByKeyID(arr[i][0], arr[i][1]);
  }
}

function InitializeEnrollment()
{
  gEnrollmentPage = 1;
  UpdateCoolKeyAvailabilityForEnrollment();
  UpdateButtonStates();
  //showOrHideEscOnLaunch();
  window.setTimeout("showOrHideTabsUI()",2);
}

function InitializeBindingTable()
{
  UpdateBindingTableAvailability();
  UpdateButtonStates();
  //showOrHideEscOnLaunch();
}

function InitializeAdminBindingList()
{

 gAdminPage = 1;

 UpdateAdminBindingListAvailability();
 UpdateButtonStates();
 //showOrHideEscOnLaunch();
 //showOrHideTabsUI();
}

//Window related functions

function hiddenWindowStartup()
{
  gHiddenPage = 1;

  // We don't want the hidden window to be shown ever.

  // We do want notify events though
  var doPreserveNotify = true;

  SetMenuItemsText(); 
  TrayRemoveWindow(doPreserveNotify);
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
    var pageWindow = null;

    if(!aPageID)
        return null;

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

function SelectESCPage(keyType,keyID,phoneHomeFailed)
{

     if(!gHiddenPage)
       return;

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

   if(keyUninitialized == UNINITIALIZED && !phoneHomeFailed)  //formatted uninitialized card
   {

       if(!TrayLoadedOK()) // We have no tray icon, launch both
       {

             if(!adminWnd)
             {
   //               launchSETTINGS();
             }

       }

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

       if(gFactoryMode || phoneHomeFailed || keyUninitialized == UNINITIALIZED_NOAPPLET)  //no applet
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

function UpdateMenuStates(disable)
{
   var menu = document.getElementById("op-menu");

   if(!menu)
       return;

   var  format_item = document.getElementById("menu-format-item");
   var  info_item   = document.getElementById("menu-info-item");
   var  reset_item  = document.getElementById("menu-reset-item"); 

   if(format_item && info_item && reset_item)
   {
       if(disable)
       {
           DisableItem(format_item);
           DisableItem(info_item);
           DisableItem(reset_item);
       }
       else
       {
           EnableItem(format_item);
           EnableItem(info_item); 
           EnableItem(reset_item);

       }

   }

}

function UpdateButtonStates()
{

return;

  var enroll_btn =   document.getElementById("enrollbtn");
  var reset_btn =   document.getElementById("resetpinbtn");

  var format_btn =  document.getElementById("formatbtn");
  var blink_btn =   document.getElementById("blinkbtn");

  var info_btn = document.getElementById("keyinfobtn");
  var cancel_btn = document.getElementById("cancelbtn");

  if (gCurrentSelectedRow)
  {
    var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));

    var keyType = keyInfo[0];
    var keyID = keyInfo[1];

    showHideNoKeysLabel(0);

    if(enroll_btn)
    {
         EnableItem(enroll_btn);
    }

    if(format_btn)
    {
        EnableItem(format_btn);
    }

     if(reset_btn)
     {
        EnableItem(reset_btn);
     }

     if(info_btn)
     {
         EnableItem(info_btn);
     }
     
     if(cancel_btn)
     {
         EnableItem(cancel_btn);
     }

     UpdateMenuStates(false);
  }
  else
  {
      showHideNoKeysLabel(1);
      if(enroll_btn)
         DisableItem(enroll_btn); 
     if(format_btn)
        DisableItem(format_btn);

     if(reset_btn)
        DisableItem(reset_btn); 

     if(info_btn)
        DisableItem(info_btn);

     if(cancel_btn)
        DisableItem(cancel_btn);

     UpdateMenuStates(true);
  }
}

function FindRow(node)
{
  return node;
}

function SelectRow(row)
{
  var theID = row.getAttribute("id");
  if (!row || gCurrentSelectedRow == row)
    return;

  if (gCurrentSelectedRow)
    gCurrentSelectedRow.setAttribute("class","UnSelectedRow");

  gCurrentSelectedRow = row;

  gCurrentSelectedRow.setAttribute("class","SelectedRow");
  UpdateButtonStates();

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

function DoSelectRow(event)
{
  var row;
  row = FindRow(event.parentNode);
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

    var isCool =  DoGetCoolKeyIsReallyCoolKey(keyType, keyID);

    var noKey = 0;

    if(!keyType || !keyID)
    {
        noKey = 1;
    }

    var keyStatus = 0;

    if(!noKey)
        keyStatus = GetStatusForKeyID(keyType, keyID);


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

    if(!image_src)
        HideItem(detailsImage);
    else
    {
        ShowItem(detailsImage);
        detailsImage.setAttribute("src", image_src);
    }
    ShowItem(advancedbtn);
    EnableItem(advancedbtn);

    var isBusy =  0;
    var operationLabel = null;

    if(keyStatus == "BUSY" || keyStatus == "UNAVAILABLE")
        isBusy = 1;

    if(isBusy)
    {
        operationLabel = GetOperationInProgressForKeyID(keyType,keyID);
    }

   if(!keyStatus)
   {
      DisableItem(viewcertsbtn);
      DisableItem(enrollbtn);
      DisableItem(resetpinbtn);
      DisableItem(formatbtn);
      detailsKeyLabel.setAttribute("value",getBundleString("noKeysPresent"));
       return;
   }

   if(keyStatus == "ENROLLED")
   {
       EnableItem(viewcertsbtn);

       DisableItem(enrollbtn);

       if(isCool)
       {
           EnableItem(resetpinbtn);
           EnableItem(formatbtn);
       }
       else
       {
           DisableItem(resetpinbtn);
           DisableItem(formatbtn);
       }

       if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("enrolledKey"));

       return;
   }

   if(keyStatus == "UNINITIALIZED")
   {
         DisableItem(viewcertsbtn);

         if(isCool)
         {
              EnableItem(enrollbtn);
         }
         else
         {
              DisableItem(enrollbtn);
         }

         DisableItem(resetpinbtn);
         if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("uninitializedKey"));

         if(isCool)
             EnableItem(formatbtn);
         else
             DisableItem(formatbtn);

       return;
   }

   if(keyStatus == "NO APPLET")
   {
       DisableItem(viewcertsbtn);
       DisableItem(enrollbtn);
       DisableItem(resetpinbtn);

       if(!isBusy)
           detailsKeyLabel.setAttribute("value",getBundleString("blankKey"));

       if(isCool)
           EnableItem(formatbtn);
       else
           DisableItem(formatbtn);

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

   }
}

function HideEnrollmentPage()
{
    window.close();

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
        statusCell.setAttribute("label",keyStatus);

    if(imageCell)
        imageCell.setAttribute("image",SelectImageForKeyStatus(keyStatus,1,0));
}

function CreateAdminListRow(adminListBox,keyType,keyID,keyStatus,reqAuth,isAuthed,keyIssuer,keyIssuedTo)
{
   //alert("CreateAdminListRow keyType " + keyType + " keyID " + keyID + " keyStatus " + keyStatus);

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
  status.setAttribute("label",keyStatus);
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
  }

  listrow.setAttribute("onclick","DoSelectAdminListRow(this);");

  adminListBox.appendChild(listrow);
  return listrow;
}

function CreateTableRow(table, keyType, keyID, keyStatus, reqAuth, isAuthed,keyIssuer,keyIssuedTo)
{

  var row = InsertGridRow(table);

  if (!row)
    return null;

  row.setAttribute("id", KeyToRowID(keyType, keyID));

  var keyui = document.createElement("label");
  if(keyui)
  {
      keyui.setAttribute ("value", KeyToUIString(keyType,keyID));

      keyui.setAttribute("hidden","true");
      row.appendChild(keyui);

  }

  var issuer = document.createElement("label");

  if(issuer)
  {
      issuer.setAttribute("value",keyIssuer);
      issuer.setAttribute("class","rowLabelText");
      row.appendChild(issuer);

      issuer.setAttribute("onclick","DoSelectRow(this);");
  }

  var spacer2 = document.createElement("spacer");
  spacer2.setAttribute("flex","1");  

  if(spacer2)
      row.appendChild(spacer2);

   var issuedTo = document.createElement("label");

 if(issuedTo)
 {
     issuedTo.setAttribute("value",keyIssuedTo);
     issuedTo.setAttribute("class","rowLabelText");
     row.appendChild(issuedTo);
 }
 var spacer3 = document.createElement("spacer");
 spacer3.setAttribute("flex","1");

 if(spacer3)
     row.appendChild(spacer3);

  var status = document.createElement("label");
  if(status)
  {
      status.setAttribute("value",  keyStatus);

      if(keyStatus == getBundleString("statusUninitialized"))
         status.setAttribute("class","rowLabelTextUninit");
      else
         status.setAttribute("class","rowLabelText");
      row.appendChild(status);
  }

  var spacer4 = document.createElement("spacer");
  spacer4.setAttribute("flex","1");

  if(spacer4)
        row.appendChild(spacer4);

  var progressMeter = document.createElement("progressmeter");
  progressMeter.setAttribute("id", KeyToProgressBarID(keyType, keyID));
  progressMeter.className = "ProgressMeter";
  progressMeter.setAttribute("value", "0%");

  progressMeter.setAttribute("mode","determined");

  progressMeter.setAttribute("class","progressMeter");

  row.appendChild(progressMeter);

  return row;
}

function CreateKeyTableRow(table, keyType, keyID, isAvailable, label, isSecured,keyIssuer,keyIssuedTo)
{

  var row = InsertRow(table);
  if (!row)
    return null;

  row.setAttribute("id", KeyToRowID(keyType, keyID));

  var keyIDStr = KeyToUIString(keyType, keyID);

    label = keyIDStr;

  // Add a tooltip to the row so that it displays more info.
  var title = keyType + " - " + keyIDStr;
  if (isSecured && label)
    title += " - " + label;
  row.setAttribute("title", title);

  // Create the isAvailable cell:
  cell = InsertCell(row);
  cell.setAttribute("align", "center");
  var a = document.createElement("a");
  a.setAttribute("href", "javascript:DoBlinkCoolKey(" + keyType + ", '" + keyID + "');");
  if (! isAvailable)
    a.style.visibility = "hidden";
  var img = document.createElement("img");
  img.setAttribute("src", "../images/NetKey-Small.gif");
  a.appendChild(img);
  cell.appendChild(a);

  // Create the label cell. Make sure we truncate long
  // labels so that they fit nicely into the window.
  cell = InsertCell(row);
  if (label.length > 24)
    label = label.substr(0, 24) + "...";
  cell.appendChild(document.createTextNode(label));

  // Create the action cell:
  cell = InsertCell(row);
  a = document.createElement("a");

  cell.appendChild(a);

  // Create the secured cell:
  cell = InsertCell(row);
  cell.setAttribute("align", "center");
  img = document.createElement("img");
  img.setAttribute("src", "../images/PadLock.gif");
  if (!isSecured)
  img.style.visibility = "hidden";
  cell.appendChild(img);

  return row;
}

gAnimationMSecs = 10000/30;

function SetCylonTimer(cylonID, cylonEyeID)
{
  setTimeout("AnimateCylonStatusBar(\"" + cylonID +
             "\", \"" + cylonEyeID + "\");", gAnimationMSecs);
}

function AnimateCylonStatusBar(cylonID, cylonEyeID)
{
  var cylon = document.getElementById(cylonID);

  if (!cylon)
    return;

  var eye = document.getElementById(cylonEyeID);

  if (!eye)
    return;

 var curValue = eye.value;

 if(curValue == "....")
   curValue = "..." ;
  else
    if(curValue == "...")
       curValue = "..";
     else
       if(curValue == "..")
        curValue = ".";
          else
               curValue = "....";

  eye.setAttribute("value",curValue);

  SetCylonTimer(cylonID, cylonEyeID);
}

function StartCylonAnimation(cylonID, cylonEyeID)
{
  var cylon = document.getElementById(cylonID)

  if (!cylon)
    return;

    var eye = document.getElementById(cylonEyeID);
    if (eye)
    {
      eye.setAttribute("hidden","false"); 
      eye.setAttribute("value", "....");
    }

    SetCylonTimer(cylonID, cylonEyeID);
}

function StopCylonAnimation(cylonID, cylonEyeID)
{
  var cylon = document.getElementById(cylonID)
  var eye = document.getElementById(cylonEyeID);

  if (eye)
    eye.setAttribute("hidden","true"); 
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

  if (type == "userKey")
  {
    screenname =  null; //GetScreenNameValue();

    pin =  GetPINValue();

    screennamepwd = null; // GetScreenNamePwd();

    tokencode = GetTokenCode();
  }

   if(gAdminPage)
       UpdateAdminKeyAreaImageToBusy(keyType, keyID);

  if(!gEnrollmentPage)
  {
      StartCylonAnimation("cylon1", "eye1");
      SetOperationText("Enrolling ");
  }
  else
  {
      ShowEnrollmentAnimation(keyType,keyID,1);
  }

  if (!EnrollCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode))
  {
    SetOperationText(null);
    StopCylonAnimation("cylon1", "eye1");
  }
  if(gAdminPage)
  {
     UpdateAdminListRow(keyType,keyID);
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminKeyAreaDetailsLabel(getBundleString("enrollingToken"));
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

  if (GetCoolKeyIsEnrolled(keyType, keyID))
  {
    SetOperationText(getBundleString("resettingTokenPIN")); 
    StartCylonAnimation("cylon1", "eye1");

    if (!ResetCoolKeyPIN(keyType, keyID, screenname, pin,screennamepwd))
    {
      SetOperationText(null);
      StopCylonAnimation("cylon1", "eye1");
    }
  }
  else
  {
      MyAlert(getBundleString("errorEnrolledFirst"));
  }

  if(gAdminPage)
  {
     UpdateAdminListRow(keyType,keyID);
     UpdateAdminKeyDetailsArea(keyType,keyID);
      UpdateAdminKeyAreaDetailsLabel(getBundleString("resettingTokenPIN"));
  }
}

function DoFormatCoolKey()
{
  if (!gCurrentSelectedRow)
    return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  var type = gKeyEnrollmentType;
  var screenname = null;
  var pin = null;

  var screennamepwd = null;
  var tokencode = null;

  SetOperationText(getBundleString("formatingToken"));
  StartCylonAnimation("cylon1", "eye1");

  if (!FormatCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode))
  {
    SetOperationText(null);
    StopCylonAnimation("cylon1", "eye1");
  }

  if(gAdminPage)
  {
      UpdateAdminListRow(keyType,keyID);
      UpdateAdminKeyDetailsArea(keyType,keyID);
      UpdateAdminKeyAreaDetailsLabel(getBundleString("formatingToken"));
  }
}
function DoCancelOperation()
{
  if (!gCurrentSelectedRow)
    return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  SetOperationText(getBundleString("cancellingOperation"));

  StartCylonAnimation("cylon1", "eye1");

  CancelCoolKeyOperation(keyType, keyID);

  SetOperationText("");
  StopCylonAnimation("cylon1", "eye1");
}

function DoBlinkCoolKey()
{
  if (!gCurrentSelectedRow)
    return;

  var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));
  var keyType = keyInfo[0];
  var keyID = keyInfo[1];

  if (!keyID)
    return;

  SetOperationText(getBundleString("blinkingToken"));
  StartCylonAnimation("cylon1", "eye1");

  BlinkCoolKey(keyType, keyID, 400, 5000);

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
}

function OnCoolKeyBlinkComplete(keyType,keyID)
{
   StopCylonAnimation("cylon1", "eye1");
   SetStatusMessage(" ");
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

  if(gHiddenPage)
  {
      TrayShowNotificationIcon();
  }

  if (GetCoolKeyIsEnrolled(keyType, keyID))
  {
      openEnrolledTokenURLBrowser();
  }
  else
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

  var phoneHomeSuccess = 1;

  if(DoGetCoolKeyIsReallyCoolKey(keyType, keyID))
      phoneHomeSuccess = DoPhoneHome(keyType,keyID);

  ShowAllWindows();

  SelectESCPage(keyType,keyID,1 - phoneHomeSuccess);

  UpdateESCSize();

  if(gHiddenPage)
  {
      var issuer = GetCachedIssuer(keyID);
      if(!issuer )
      {

             issuer = getBundleString("unknownIssuer");

      }
      //TraySendNotificationMessage(getBundleString("keyInserted"),"\"" + issuer +"\"" + " " + getBundleString("keyInsertedComputer"),3,4000,GetESCNotifyIconPath(keyType,keyID));
  }

}

function OnCoolKeyRemoved(keyType, keyID)
{
  var table = null;

  var  row = GetRowForKey(keyType, keyID);

  if(gHiddenPage)
  {
      var issuer = GetCachedIssuer(keyID);
      if(!issuer)
          issuer = getBundleString("unknownIssuer");
      TraySendNotificationMessage(getBundleString("keyRemoved"),"\"" + issuer + "\"" + " " + getBundleString("keyRemovedComputer"),1,4000,GetESCNotifyIconPath(keyType,keyID));

  }

   table = document.getElementById("BindingTable");

   if(gEnrollmentPage)
  {
      UpdateEnrollmentArea(keyType,keyID,0);
  }

  if(gAdminPage)
  {
      RemoveAdminRow(row);

      if (row == gCurrentSelectedRow)
          gCurrentSelectedRow = null;

      UpdateAdminKeyDetailsArea(null,null);
  }

  if (row && table)
  {
    RemoveRow(table,row);

    if (row == gCurrentSelectedRow)
      gCurrentSelectedRow = null;
  }

  UpdateButtonStates();
  UpdateESCSize();

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
  UpdateButtonStates();

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  MyAlert(getBundleString("enrollmentFor") + " "  + getBundleString("smartCard") + " " + getBundleString("wasSuccessful"));


  if(gEnrollmentPage)
  {
      ShowEnrollmentAnimation(keyType,keyID,0);
  }
  if(gAdminPage)
  {
      UpdateAdminKeyDetailsArea(keyType,keyID);
      UpdateAdminListRow(keyType,keyID);
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
  UpdateButtonStates();

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  MyAlert(getBundleString("pinResetSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));

  if(gAdminPage)
   {
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminListRow(keyType,keyID);
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

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  MyAlert(getBundleString("formatOf") + " " + getBundleString("smartCard") + " "  + getBundleString("wasSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));

   if(gAdminPage)
   {
     UpdateAdminKeyDetailsArea(keyType,keyID);
     UpdateAdminListRow(keyType,keyID);
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

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");

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
   }

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
 //    screenname = GetScreenNameValue();
 //    if (! screenname)
 //     return 0;

    pin =  GetPINValue();

    if (! pin)
      return 0;

 //    screennamepwd = GetScreenNamePwd();
 //    if(! screennamepwd)
 //       return 0;

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
      //OnCoolKeyStateError(keyType, keyID, keyState, data);
      break;
    case 1018: 
      OnCoolKeyBlinkComplete(keyType, keyID);
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

function SetOperationText(aText)
{
    var text = document.getElementById("operationtext");
    
    if(text)
    {
        text.setAttribute("value",aText);
    }
}
function refresh()
{
  window.resizeBy(0,1);
  window.resizeBy(0,-1);
}

function loadCONFIG()
{
    window.location = "chrome://esc/content/config.xul";
}

function loadSETTINGS()
{
    window.location = "chrome://esc/content/settings.xul";
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

    //get first key

    var arr = GetAvailableCoolKeys();

    if (arr && arr.length > 0)
    {
       keyID = arr[0][1];
       keyType = arr[0][0];
    }

    if(keyID)
    {
         esc_enroll_uri = GetCachedTPSUI(keyID);
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

function openEnrolledTokenURLBrowser()
{
    var agent = navigator.userAgent.toLowerCase();

    var doWindows = 0;

    var platform = null;
    var executable = null;

    if(agent && agent.indexOf("mac") != -1)
    {
        platform = "mac";
        executable = "/usr/bin/open" ;
    }

    if(agent && agent.indexOf("linux") != -1)
    {
       platform = "linux";
       executable = "/usr/bin/firefox";
    }

    if(agent && agent.indexOf("nt 5.0") != -1)
    {
        platform = "windows";
        executable = "C:\\WINNT\\system32\\cmd.exe" ;
        doWindows = 1;
    }

    if(agent && agent.indexOf("nt 5.1") != -1)
    {
        platform = "windows";
        executable = "C:\\Windows\\system32\\cmd.exe" ;
        doWindows = 1;
    }

    if(!platform)
    {
        MyAlert(getBundleString("errorFindESCPlatform"));
        return;
    }

    //Now get enrolled token URL

    var enrolled_token_uri = null;

   if(netkey)
   {
             try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                enrolled_token_uri = netkey.GetCoolKeyConfigValue("esc.enrolled.token.url");

            } catch(e) {
                MyAlert(getBundleString("errorConfigValue") + e);
               
            }

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


function showOrHideEscOnLaunch()
{
    if(GetTrayIsInitialized())
        return;

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      var hide = netkey.GetCoolKeyConfigValue("esc.hide.on.startup");

      if(hide == "yes")
      {
           window.setTimeout("HideAllWindows()",0);          
      }

      } catch(e) {
          MyAlert(getBundleString("errorConfigValue") + " "  + e);
      }

}

function showOrHideTabsUI()
{
  var tabs = document.getElementById("tablist");

  if(!tabs)
      return;

  var show = DoCoolKeyGetConfigValue("esc.show.tabs.ui");

  if(show == "yes")
  {
      tabs.setAttribute("hidden","false");
  }
}

function showHideNoKeysLabel(aShow)
{
    var noKeys = document.getElementById("NoKeysLabel");

    if(noKeys)
    {
        if(aShow)
        {
            noKeys.setAttribute("hidden","false");
        }
        else
        {
            noKeys.setAttribute("hidden","true");
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

function DoGetCoolKeyIsReallyCoolKey(keyType,keyID)
{

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      isCool =  netkey.GetCoolKeyIsReallyCoolKey(keyType, keyID);

      //alert("isCool " + isCool);

      return isCool;
    } catch (e) {

        return 0;
    }

}

function DoCoolKeyGetIssuerUrl(keyType,keyID)
{
    var url = null;

    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      url =  netkey.GetCoolKeyIssuerInfo(keyType, keyID);


      if(url.length < 10)   // Check for bogus junk
          url = null;

      if(url)
      {
          var issuer_config_value = ConfigValueWithKeyID(keyID,KEY_ISSUER_URL);
          var result = DoCoolKeySetConfigValue(issuer_config_value,url);
      }
    } catch (e) {
      ReportException(getBundleString("errorIssuerInfo") + " " , e);
      return url;
  }

  return url;
}

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

function CheckForFactoryMode()
{
  var factory= DoCoolKeyGetConfigValue("esc.factory.mode");

  if(factory == "yes")
  {
      gFactoryMode = 1;

  }

}

function launchCONFIG(keyType,keyID)
{
    var wind = window.openDialog("chrome://esc/content/config.xul",keyID,"chrome,centerscreen,resizable,modal=yes");
}

function launchCertViewer()
{
   var wind = window.openDialog("chrome://esc/content/certManager.xul", "","chrome,centerscreen,modal=yes");
//   var wind =  window.openDialog("chrome://pippki/content/certManager.xul", "","chrome,centerscreen,modal=yes");
}

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
   //alert("NotifyESCOfTrayEvent window" + window.location + "gHiddenPage " + gHiddenPage + " gHiddenPageDone " + gHiddenPageDone);

    if(!gHiddenPage)
        return;

   //Get the primary page windows if present

   var enrollWnd = IsPageWindowPresent(ENROLL_WINDOW);
   var adminWnd  = IsPageWindowPresent(ADMIN_WINDOW);


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
        return false;

    phoneHomeIssuer = GetCachedIssuer(aKeyID);


    if(!phoneHomeIssuer)
       return false;

    tpsURL = GetCachedTPSURL(aKeyID);


    if(!tpsURL)
        return false;
    tpsUI = GetCachedTPSUI(aKeyID);


    return true;
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

function UpdateRowWithPhoneHomeData(keyType,keyID)
{

    if(gEnrollmentPage)
    {
        UpdateEnrollmentArea(keyType,keyID,1);
    }

    if(gAdminPage)
    {
          SelectRowByKeyID(keyType, keyID);
          UpdateAdminListRow(keyType,keyID);
    }

}

//Phone home and get values
function phoneHome(theUrl,aKeyID,resultCB)
{

   var url = null ;

   if(!theUrl || !aKeyID)
       return false;

    if(theUrl != null)
        url = theUrl;

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
                var name = oChild.nodeName;
                var value = oChild.firstChild.data;

                if(name && value)
                {
                    var cValue = ConfigValueWithKeyID(aKeyID,name);

                    if(cValue)
                    {
                        DoCoolKeySetConfigValue(cValue,value);
                    }
                }
               
            }

            if(resultCB)
            {
               //Manually write out entry for phone home url

                var issuer_config_value = ConfigValueWithKeyID(aKeyID,KEY_ISSUER_URL);
                var result = DoCoolKeySetConfigValue(issuer_config_value,url);
                resultCB(true);
            }
            return;
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

        var colonIndex = value.indexOf(":");

        value = value.substring(colonIndex + 1);

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


       alert("Problem with nsIPromptService " + e);
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
       return;


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

}
