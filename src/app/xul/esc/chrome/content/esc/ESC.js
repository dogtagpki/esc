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

var gHidden = null;

var keyUITable = new Array();
var keyTypeTable = new Array();
var curChildWindow = null;

var gUsesTree = 0;

loadStringBundle();


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
          alert(getBundleString("errorJsNotifyInterface"));
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
     alert(getBundleString("errorUniversalXPConnect") + e);
  }

//
// unregister our notify event
//
function cleanup()
{

 TrayRemoveWindow();
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
      netkey.rhCoolKeyUnSetNotifyCallback(gNotify);
    } catch(e) {
     alert(getBundleString("errorUniversalXPConnect")  + e);
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
                alert(getBundleString("errorConfigValue")  + e);
            }

        if(tps_uri)
        {
            testTPSURI(tps_uri);

        }
    }

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
        alert(getBundleString("errorBlankTPSURI"));
        return;
    }

    if(!aURL)
    {
        alert(getBundleString("aboutToTestTPSURI") + uri);
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
                 alert(getBundleString("tpsURLContacted"));
            }
            else
            {
                if(!aURL)
                {
                    alert(getBundleString("errorContactTPSURL"));
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
        alert(getBundleString("errorBlankEnrollURI"));
        return;
    }

    alert(getBundleString("aboutToTestTPSURI"));

    req = new XMLHttpRequest();
    req.open('GET', uri, true);

    var response_text = null;

    var callback = function () {

        if (req.readyState == 4) {

            response_text = req.responseText;

            var index = response_text.indexOf(search);

            if(index != -1)
            {
                 alert(getBundleString("enrollURLContacted"));
            }
            else
            {
                alert(getBundleString("errorContactEnrollURL"));
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
            alert(getBundleString("tpsURIMustHaveValue"));
            return;
        }

             var res = 0;

             try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                res = netkey.SetCoolKeyConfigValue("esc.tps.url",tps_uri_value);


            } catch(e) {
                alert(getBundleString("errorSetConfigValue") + e);
                return;
       
            }


            try {
                    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

                res = netkey.SetCoolKeyConfigValue("esc.enroll.ui.url",esc_enroll_value);

                grantPrivilegesURL(esc_enroll_value);

            } catch(e) {
                alert(getBundleString("errorSetConfigValue") + e);
                return;
            }


            alert(getBundleString("configChangesSubmitted"));


    }

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
                alert(getBundleString("errorSetConfigValue") + e);
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
                alert(getBundleString("errorSetConfigValue") + e);
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
//    var test=getBundleString("testValue");
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
                alert(getBundleString("errorSetDataValue") + e);
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
                alert(getBundleString("errorSetDataValue") + e);
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
              alert(getBundleString("errorSetDataValue") + e);
          }

      }

}
 
function TestStatusMessages()
{

    for(i = 0 ; i < 48; i++)
    {
        alert(Status_Messages[i]);

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
  alert(msg + " " + e.description + "(" + e.number + ")");
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

    var issuer = "Red Hat";

     var keyStatus = GetStatusForKeyID(keyType, keyID);

     if(keyStatus != getBundleString("statusEnrolled"));
         issuer = getBundleString("unknownIssuer");

    return issuer;

}

function GetCoolKeyIssuedTo(keyType,keyID)
{

     var keyStatus = GetStatusForKeyID(keyType, keyID);

    var issuedTo = getBundleString("redHatUser");


    if(keyStatus != getBundleString("statusEnrolled"))
       issuedTo = getBundleString("statusUnknown");

    return issuedTo;

}


function DoShowKeyInfo()
{

  var doCertInfo = 1;
  var test = null;

  var dump = "<hr><center> <b>" + getBundleString("tokenInformation") + "</b></center>";

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
       alert(getBundleString("noCurrentlySelectedToken"));
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
    return ConvertVariantArrayToJScriptArray(netkey.ChallengeCoolKey(keyType, keyID, data));
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

//    table.deleteRow(row.rowIndex);
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


function UpdateInfoForKeyID(keyType, keyID, keyStatus, reqAuth, isAuthed)
{
  var row = GetRowForKey(keyType, keyID);

  if (!row)
    return;

  if(gUsesTree)
  {
      table = document.getElementById("BindingTree");
  }
  else
  {
      table = document.getElementById("BindingTable");
  }

  if (row && table)
  {
    if(gUsesTree)
    {
        RemoveTreeRow(table,row);
    }
    else
    {
        RemoveRow(table,row);
    }

    InsertCoolKeyIntoBindingTable(keyType, keyID);

    row = GetRowForKey(keyType,keyID);

    if(row)
    {
         SelectRow(row);
    }

  }
    
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

function InsertCoolKeyIntoBindingTree(keyType,keyID)
{
  var treechildren = document.getElementById("BindingTreeChildren");

  if(!treechildren)
      return;

  var treeitem = document.createElement("treeitem");

  var id =  KeyToRowID(keyType, keyID);

  if(!treeitem)
      return;

  treeitem.setAttribute("id",id);

  treechildren.appendChild(treeitem);
  
  var treerow = document.createElement("treerow");

  if(!treerow)
      return null;

  treeitem.appendChild(treerow);

  var treecell = document.createElement("treecell");
  var treecell1 = document.createElement("treecell");

  treerow.appendChild(treecell);
  treerow.appendChild(treecell1);

  if(treecell)
  {
      treecell.setAttribute("label", keyID);
  }

  if(treecell1)
  {
    // var progressMeter = document.createElement("progressmeter");
     //progressMeter.setAttribute("id", KeyToProgressBarID(keyType, keyID));
     //progressMeter.className = "ProgressMeter";
    // progressMeter.setAttribute("value", "0%");

     //progressMeter.setAttribute("mode","determined");

     treecell1.setAttribute("id",KeyToProgressBarID(keyType,keyID));
     treecell1.setAttribute("mode","normal");
     treecell1.setAttribute("value","0%"); 
  } 

  return treerow;

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

function ConvertVariantArrayToJScriptArray(varr)
{
  // C++ native methods, like netkey.GetAvailableCoolKeys(), can only
  // return variant SafeArrays, so to access the data inside, you must
  // first convert it to a VBArray, and then call toArray() to convert
  // it to a JScript array. Lame, but that's what it takes to
  // use an array returned from an ActiveX component.

  return new VBArray(varr).toArray();
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

function UpdateBindingTreeAvailability()
{
  gUsesTree = 1;

  var arr = GetAvailableCoolKeys();

  

  if (!arr || arr.length < 1)
    return; 
  var i;

  for (i=0; i < arr.length; i++)
  {

    InsertCoolKeyIntoBindingTree(arr[i][0],arr[i][1]);

    if (!gCurrentSelectedRow)
        SelectRowByKeyID(arr[i][0], arr[i][1]);
  }
}

function InitializeBindingTree()
{


    testTPSURISilent();

    UpdateBindingTreeAvailability();
    UpdateButtonStates();

    showOrHideEscOnLaunch();
}

function InitializeBindingTable()
{

  testTPSURISilent();
  UpdateBindingTableAvailability();
  UpdateButtonStates();
  showOrHideEscOnLaunch();


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
           format_item.setAttribute("disabled","true");
           info_item.setAttribute("disabled","true");
           reset_item.setAttribute("disabled","true");
       }
       else
       {
           format_item.setAttribute("disabled","false");
           info_item.setAttribute("disabled","false"); 
           reset_item.setAttribute("disabled","false");

       }

   }

}

function UpdateButtonStates()
{

  var enroll_btn =   document.getElementById("enrollbtn");
  var reset_btn =   document.getElementById("resetpinbtn");

  var format_btn =  document.getElementById("formatbtn");
  var blink_btn =   document.getElementById("blinkbtn");

  var info_btn = document.getElementById("keyinfobtn");
  var cancel_btn = document.getElementById("cancelbtn");


  if (gKeyEnrollmentType == "deviceKey")
  {
  }
  else
  {
  }

  if (gCurrentSelectedRow)
  {
    var keyInfo = RowIDToKeyInfo(gCurrentSelectedRow.getAttribute("id"));

    var keyType = keyInfo[0];
    var keyID = keyInfo[1];

    showHideNoKeysLabel(0);

    if(enroll_btn)
    {
         enroll_btn.disabled = false;

    }

    if(format_btn)
    {
        format_btn.disabled = false;

    }

     if(reset_btn)
     {
        reset_btn.disabled = false
     }

     if(info_btn)
     {
         info_btn.disabled = false;
     }
     
     if(cancel_btn)
     {
         cancel_btn.disabled = false;
     }

     UpdateMenuStates(false);
  }
  else
  {

      showHideNoKeysLabel(1);
      if(enroll_btn)
         enroll_btn.disabled = true; 
     if(format_btn)
        format_btn.disabled = true;

     if(reset_btn)
        reset_btn.disabled = true; 

     if(info_btn)
        info_btn.disabled = true;

     if(cancel_btn)
        cancel_btn.disabled = true;

     UpdateMenuStates(true);
  }

  //refresh();
}

function SetEnrollmentType(type)
{
  gKeyEnrollmentType = type;
  UpdateButtonStates();
}

function FindRow(node)
{
//  while (node && node.tagName != "TR")
 // {
  //  node = node.parentNode;
 // }

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
}

function SelectRowByKeyID(keyType, keyID)
{
  var row = GetRowForKey(keyType, keyID);

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

      //keyui.setAttribute("oncommand","DoSelectRow(this);");
      //keyui.setAttribute("class","rowLabelText");
      keyui.setAttribute("hidden","true");
      row.appendChild(keyui);

  }

  //var spacer1 = document.createElement("spacer");
 // spacer1.setAttribute("flex","1");

  //if(spacer1)
   //     row.appendChild(spacer1);



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

//  if (label == keyID)
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
  if (isSecured)
  {
  //  a.setAttribute("href", "javascript:UnbindCOOLKey(" + keyType + ", '" + keyID + "');");
  //  a.appendChild(document.createTextNode("Release"));
  }
  else
  {
  //  a.setAttribute("href", "javascript:BindCOOLKey(" + keyType + ", '" + keyID + "');");
 //   a.appendChild(document.createTextNode("Secure"));
  }
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

////////////////////////////////////////////////////////////////
//
// Functions that contact the server or talk directly to
// ASC native code.
//
// ASC Native Functions:
//
//     netkey.GetAvailableCoolKeys()
//
//       - Returns an ActiveX Variant SafeArray containing the ID for each key
//         that is currentlly plugged into the computer. Before accessing any
//         data in this array you must convert it to a JScript Array with a
//         call to ConvertVariantArrayToJScriptArray().
//
//     netkey.GetCoolKeyIsEnrolled(keyType, keyID)
//
//       - Returns true if a key has been initialized, false if it hasn't.
//         Initialized means the card has been formatted with certificates
//         for either an Cool HouseKey or NetKey.
//
//     netkey.EnrollCoolKey(keyType, keyID, enrollmentType, screenName, pin)
//
//       - Initiates an async connection to the RA to initialize a specific
//         key. If you want the key to be initialized as a HouseKey, you should
//         pass "houseKey" as the enrollmentType, and null values for both
//         screenName and pin. For a NetKey, use "netKey" as the enrollmentType,
//         and pass a valid screenName and pin.
//
//     netkey.ChallengeCoolKey(keyType, keyID, data)
//
//       - Signs some data with the specified key, and returns the results
//         in an AcviteX Variant SafeArray. Before accessing any data in
//         this array, you must convert it to a JScript Array with a
//         call to ConvertVariantArrayToJScriptArray(). The elements in the
//         array are as follows:
//
//             array[0] --> Length of the signed challenge data in binary form.
//             array[1] --> The signed challenge data as hex.
//             array[0] --> Length of the nonce data in binary form.
//             array[0] --> The nonce data as hex.
//
//     netkey.BlinkCoolKey(keyType, keyID, rate, duration)
//
//       - Make a specific key blink at a given rate for a given duration.
//         rate and duration are specified in milliseconds.
//
////////////////////////////////////////////////////////////////

function GetScreenNameValue()
{
  var sname = document.getElementById("snametf").value;

  if (! sname)
  {
    alert(getBundleString("errorProvideScreenName"));
    return null;
  }

  return sname;
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
    alert(getBundleString("errorProvideTokenPIN"));
    return null;
  }

  if ( pinVal != rpinVal && reenterpintf_obj)
  {
    alert(getBundleString("errorMatchPinValues"));
    return null;
  }

  return pinVal;
}

function GetScreenNamePwd()
{

  var pwd = document.getElementById("snamepwd").value;

   if(!pwd)
   {
       alert(getBundleString("errorValidUserPassword"));
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
    alert(getBundleString("errorSelectKey"));
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

  StartCylonAnimation("cylon1", "eye1");    

  SetOperationText(getBundleString("enrollingToken"));

  if (!EnrollCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode))
  {
    SetOperationText(null);
    StopCylonAnimation("cylon1", "eye1");
  }
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
  var pin = null;
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
      alert(getBundleString("errorEnrolledFirst"));
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

  SetOperationText(getBundleString("formattingToken"));
  StartCylonAnimation("cylon1", "eye1");

  if (!FormatCoolKey(keyType, keyID, type, screenname, pin,screennamepwd,tokencode))
  {
    SetOperationText(null);
    StopCylonAnimation("cylon1", "eye1");
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

function OnTreeSelected(tree)
{
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

  if (GetCoolKeyIsEnrolled(keyType, keyID))
  {
      openEnrolledTokenURLBrowser();
  }
  else
  {
       uninitialized = 1;
  }


  if(gUsesTree)
  {
      row =  InsertCoolKeyIntoBindingTree(keyType,keyID);
  }
  else
  {
     row = InsertCoolKeyIntoBindingTable(keyType, keyID);
  }

  if (!gCurrentSelectedRow)
    SelectRowByKeyID(keyType, keyID);

  ShowAllWindows();

  if(uninitialized)
  {

      window.setTimeout("loadESC()",2);
  }


  showHideNoKeysLabel(0);

  TraySendNotificationMessage(getBundleString("keyInserted"));

}

function OnCoolKeyRemoved(keyType, keyID)
{

  var table = null;

  var  row = GetRowForKey(keyType, keyID);

   TraySendNotificationMessage(getBundleString("keyRemoved"));
  if(gUsesTree)
  {
      table = document.getElementById("BindingTree");
  }
  else
  {
      table = document.getElementById("BindingTable");
  }

  if (row && table)
  {
    if(gUsesTree)
    {
        RemoveTreeRow(table,row);
    }
    else
    {
        RemoveRow(table,row);

    }

    if (row == gCurrentSelectedRow)
      gCurrentSelectedRow = null;
  }

  UpdateButtonStates();
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
  var keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);
  UpdateButtonStates();

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  alert(getBundleString("enrollmentFor") + " " + KeyToUIString(keyType, keyID) + " "  + getStringBundle("wasSucessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));
}

function OnCoolKeyPINResetComplete(keyType, keyID)
{
  var keyStatus = PolicyToKeyType(GetCoolKeyPolicy(keyType, keyID));
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);
  UpdateButtonStates();

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  alert(getBundleString("pinResetSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));
}

function OnCoolKeyFormatComplete(keyType, keyID)
{
  var keyStatus = GetStatusForKeyID(keyType, keyID);
  var keyReqAuth = BoolToYesNoStr(GetCoolKeyRequiresAuth(keyType, keyID));
  var keyIsAuthed = BoolToYesNoStr(GetCoolKeyIsAuthed(keyType, keyID));

  UpdateInfoForKeyID(keyType, keyID, keyStatus, keyReqAuth, keyIsAuthed);

  StopCylonAnimation("cylon1", "eye1");
  SetOperationText("");
  alert(getBundleString("formatOf") + " "  + KeyToUIString(keyType, keyID)+ " " + getBundleString("wasSuccessful"));
  ClearProgressBar(KeyToProgressBarID(keyType, keyID));
}

function OnCoolKeyStateError(keyType, keyID, keyState, errorCode)
{
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

  var typeStr = getBundleString("error") + " " + errorCode ;

  var  messageStr = getBundleString("serverResponse")  + MyGetErrorMessage(errorCode) ;

  var keyIDStr = KeyToUIString(keyType, keyID);

  if (keyState == 1004)
    typeStr = getBundleString("enrollmentOfKey") + " "   + keyIDStr + " " + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1016)
      typeStr = getBundleString("formatingtOfKey") + " "   + keyIDStr + " " + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1010)
    typeStr = getBundleString("pinResetOfKey") + " "   + keyIDStr + " " + getBundleString("failed") + " "  + typeStr + messageStr ;
  else if(keyState == 1020)
    typeStr = getBundleString("operationForKey") + " "   + keyIDStr + " " + getBundleString("cancelled") + " "  + typeStr + messageStr ;

  alert(typeStr);
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
      OnCoolKeyEnrollmentComplete(keyType, keyID);
      break;
    case 1004: // EnrollmentError
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

function loadESC()
{
    var esc_enroll_uri = null;

    esc_enroll_uri = getESCEnrollmentUI();

    if(esc_enroll_uri)
    {
        window.location = "chrome://esc/content/esc_browser.xul";
        return;
    }

    window.location = "chrome://esc/content/esc.xul";
}

function loadSETTINGS()
{
    window.location = "chrome://esc/content/settings.xul";
}

function loadExternalESC()
{
   window.location = "chrome://esc/content/esc_browser.xul";

}

function loadESCEnrollUI()
{
    var esc_enroll_uri = null;

    esc_enroll_uri = getESCEnrollmentUI();

    if(esc_enroll_uri)
    {
        var ui_id = document.getElementById("esc-ui");

        if(ui_id)
        {
            ui_id.setAttribute("src",esc_enroll_uri);
            return;
        }
    }

    window.location = "chrome://esc/content/esc.xul"; 

}


// test of nsIProcess

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
        alert(getBundleString("errorFindESCPlatform"));
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
                alert(getBundleString("errorConfigValue") + e);
               
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
                alert(getBundleString("errorConfigValue") + e);
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
                alert(getBundleString("errorSetConfigValue") + " "  + e);
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
          alert(getBundleString("errorConfigValue") + " "  + e);
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
