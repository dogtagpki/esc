
var parentWindow = window.opener;

function doOperation()
{
  //alert("doOperation opener " + parentWindow + " nam " + window.name);

  var pin = GetLocalPINValue();

  if(!pin)
      return;

  parentWindow.SetPINValue(pin);

  var theOperation = window.name;

  if(theOperation == "resetpin") 
     parentWindow.DoResetSelectedCoolKeyPIN();

  if(theOperation == "enroll")
     parentWindow.DoEnrollCoolKey();

  window.close(); 
}


function GetLocalPINValue()
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
    MyAlert("You must provide a valid Token PIN!");
    return null;
  }

  if ( pinVal != rpinVal && reenterpintf_obj)
  {
    MyAlert("The PIN values you entered don't match!");
    return null;
  }

  return pinVal;
}

function PasswordLoad()
{
    window.sizeToContent();
}
