
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


var theForm = null;
var curKeyID = null;
var curKeyType = 0; 

var gTitle = null;
var gDescription = null;
var gStringBundle = null;

var gFormSubmitted = 0;


loadStringBundle();

function ConfirmPassword(password_element)
{

  if(!password_element)
      return 0;

  password_id = password_element.id;

  if(!password_id)
     return 0;

  confirm_id = "RE_" + password_element.id;

  var size = theForm.length;

  if(theForm)
  {
      for(i = 0; i < size ; i++)
      {
         var cur_element = theForm.elements[i];
  
         if(cur_element.id == confirm_id)
         {
             if(cur_element.value != password_element.value)
             {
                 alert(getBundleString("value") + " " + password_element.name + " " + getBundleString("mustMatch") + " " + cur_element.name);
                 return 0;

             }
             else
             {
                 return 1;
             } 

         } 

      }

  }

  return 1;
}

function Validate()
{
    if(theForm)
    {
         var rows = GetRowsNodeFromGrid(theForm);

         if(!rows)
             return;

         var rowsChildren = rows.childNodes;

         var size = rowsChildren.length;
 
         for( i = 0; i < size ; i++)
         {
             var element = rowsChildren[i];


             if(element)
             {
                 var rowkids = element.childNodes;
                 var len = rowkids.length;


                 var j = 0;
                 for(j = 0 ; j < len; j ++)
                 {
                     rowKid = rowkids[j];

                     if(rowKid)
                     {
                         if(rowKid.tagName == "textbox")
                         {
                             if(rowKid.value == "")
                             {
                                 alert(getBundleString("mustHaveValue") + " "  + rowKid.getAttribute("name"));
                                 return 0;
                             }

                         }
                         
                     }
                 }

             }
         } 
   
    }

    return 1;
}

function FormSubmit(noValidate)
{
    var result = true;

    if(!noValidate)
        Validate();

    var thisParent = window.opener;

    if(!thisParent)
    {
        alert(getBundleString("noParentWindow"));
        window.close(); 
        return; 
    }
  
    if(!result)
    {
        return;
    }

    if(theForm)
    {
        var rows = GetRowsNodeFromGrid(theForm);

        if(!rows)
            return;

        var rowsChildren = rows.childNodes;
        var size = rowsChildren.length;

        for( i = 0; i < size ; i++)
         {
             var element = rowsChildren[i];

             if(element)
             {
                 var rowkids = element.childNodes;
                 var len = rowkids.length;

                 var j = 0;
                 for(j = 0 ; j < len; j ++)
                 {
                     rowKid = rowkids[j];

                     if(rowKid)
                     {
                         if(rowKid.tagName == "textbox")
                         {

                              var id = rowKid.id;
                              var value = rowKid.value;

                              if(thisParent)
                              {
                                  thisParent.CoolKeySetDataValue(curKeyType,curKeyID,id,value);
                              }

                         }

                     }
                 }

             }
         }

    }
     gFormSubmitted = 1;
     window.close(); 
}

function GetUIObjectList(uiData)
{
    var  str = new String(uiData);
    var splits = str.split("&&");


    var params = new Array();
    var size = splits.length;

    for(i = 0 ; i <  size ; i++)
    {
       params[i] = splits[i].split("&");
    }

    size = params.length;
    var name_value_objects = new Array();

    for(i = 0 ; i < size; i++)
    {
         var name_values = new Array();

         pISize = params[i].length;
         for(j = 0 ;  j < pISize ; j ++)
         {
             var pair = params[i][j].split("=");

             if(pair[0] == "option")
             {
                 var options = pair[1].split(",");
             }

             name_values[pair[0]] = pair[1];
         }

         name_value_objects[i] = name_values;
    }

    return name_value_objects;
}

function SetNodeClass(theNode, theClass)
{
   if(!theNode || !theClass)
       return;

   theNode.setAttribute("class",theClass);
}

function AddSpacerToNode(theNode,theFlex,theStyle)
{

    if(!theNode)
        return;

    var spacer = document.createElement("spacer");

    spacer.setAttribute("flex",theFlex);
    spacer.setAttribute("style",theStyle);

    theNode.appendChild(spacer);
}

function AddBRToNode(theNode)
{

   if(!theNode)
       return;

   var br = document.createElement("br");

   theNode.appendChild(br);

}

function AddTextToLabel(theLabel,theText)
{

    if(!theLabel || !  theText)
        return;
   
    theLabel.setAttribute("value",theText);

}

function AddTextToNode(theNode,theText)
{
    if(!theNode || !theText)
        return;

    var text = document.createTextNode(theText);

    theNode.appendChild(text);
}

function AddTextToDocument(theText)
{
    if(!theText)
         return;

     var p = document.createElement("p");

     if(p)
     {
         p.appendChild(document.createTextNode(theText));
     } 

     document.body.appendChild(p);
}

function CreateForm()
{
    var form = document.createElement("form");
    document.body.appendChild(form);
    return form;
}

function AddRowToGrid(theGrid)
{
    if(!theGrid)
        return null;

    var childNodes = theGrid.childNodes;

    if(!childNodes)
        return null;

    var len = childNodes.length;

    var gridid = theGrid.id;

    var gridrowsid = gridid + "-rows";

    var rows = null;

    var curNode;
    for(i = 0 ; i < len ; i ++)
    {
        curNode = childNodes[i];

        if(!curNode)
            return null; 

        if(curNode.id == gridrowsid)
        {
            rows = curNode;
            break;
        }
    }

    var row = document.createElement("row");
    rows.appendChild(row);

    return row;
}

function CreateGrid(numCols,theId)
{
   if(numCols < 0 || numCols > 20)
       return null;

    var grid = document.createElement("grid");

    if(!grid)
        return null;

    grid.setAttribute("id",theId);

    var columns;

    columns = document.createElement("columns");

    if(!columns)
        return null;

    grid.appendChild(columns);

    var column;
    for(i = 0 ; i < numCols ; i++)
    {
       column = document.createElement("column"); 

       if(column)
           columns.appendChild(column);
    } 

    var rows;

    rows = document.createElement("rows");

    if(!rows)
       return null;

    rows.setAttribute("id",theId + "-rows");

    grid.appendChild(rows);

    return grid;
}

function CreateTable()
{
  var table = document.createElement("table");
  document.body.appendChild(table);
  tbody = document.createElement("tbody");
  table.appendChild(tbody);

  return table;
}

function AddRowToTable(table)
{
    if(!table)
        return null;

    var tr = document.createElement("tr");
    (table.tBodies[0]).appendChild(tr);

    return tr; 
}

function AddColumnToRow(row)
{
  if(!row)
     return null;

  var td = document.createElement("td");
  row.appendChild(td);

  return td;
}

function AddTextToColumn(column,text)
{
   if(!column || !text)
       return;

   var text_node = document.createTextNode(text);
   column.appendChild(text_node);

   return text_node;
}

function AddSpacer()
{
  var tSpacer = document.createElement("spacer");

  if(!tSpacer)
      return null;


  return tSpacer;

}

function AddButton(id,label)
{

  var tButton = document.createElement("button");

  if(!tButton)
      return null;

  tButton.setAttribute("id" , id);
  tButton.setAttribute("label", label);

  return tButton;
}
function AddTextBox(type,id,name,value)
{
    var tBox = document.createElement("textbox");

    if(!tBox)
       return null; 
    
    tBox.setAttribute("type",  type);
    tBox.setAttribute("id",id);
    tBox.setAttribute("name", name);

    tBox.setAttribute("value",  value); 

    return tBox;

}

function AddInputField(type,id, name,value)
{
    var    field = document.createElement("input");
   
    if(!field)
        return null;
  
    field.type =  type;   
    field.id =id;
    field.name =name;
    field.value =value;

    return field;
}

function ConstructUI(aKeyType,aKeyID,uiData)
{
    var name_value_objects = GetUIObjectList(uiData);
    var len = name_value_objects.length;

    gTitle = document.createElement("label");
    SetNodeClass(gTitle,"authTitleText");

    gDescription = document.createElement("label");

    SetNodeClass(gDescription,"authDescText");

    var box = document.getElementById("authbox");

    if(box)
    {
       box.appendChild(gTitle);
       AddSpacerToNode(box,"1","min-height: 20px");

       box.appendChild(gDescription);
    }

    curKeyID = aKeyID;
    curKeyType = aKeyType;

    var grid = CreateGrid(5,"authgrid");

    if(grid)
    {
       box.appendChild(grid);
       theForm = grid;
    }

    var i = 0;
    var first_box = 1;

    for(i = 0 ; i < len ; i ++)
    {
        curParameter = name_value_objects[i];

        if(curParameter)
        {
            var title = curParameter["title"];

            if(title)
            {
                AddTextToLabel(gTitle,title);
            }

            description =  curParameter["description"];

            if(description)
            {
               AddTextToLabel(gDescription,description);
            }

            id   = curParameter["id"];
            name = curParameter["name"];
            type = curParameter["type"];
            desc = curParameter["desc"]; 

            field = null;
            if(id)
            {
               if(grid)
               {
                   row = AddRowToGrid(grid);
               }

               if(name)
               {
                   var namelab = document.createElement("label");

                   if(namelab)
                   {
                       AddTextToNode(namelab,name);
                       row.appendChild(namelab);
                   }
               }

               if(type == "string" || type == "integer")
               {
                   field = AddTextBox("text",id,name,"");
               }

               re_field = null;
 
               if(type == "password")
               {
                   field = AddTextBox("password",id,name,"");
               }

             //  if(type == "hidden")
              // {
               //    field = AddInputField("hidden",id,name,"");
              // } 

               if(field)
               {
                    row.appendChild(field);
                    if(first_box)
                    {
                        field.focus();
                        first_box = 0;
                    }
               }

           }

        }

    }

    var ui_hbox = document.createElement("hbox");
    if(ui_hbox)
    {
        box.appendChild(ui_hbox);
        AddSpacerToNode(ui_hbox,"1","");
        var button = AddButton("",getBundleString("authSubmit")); 
        if(button)
        {
            button.setAttribute("oncommand" , "FormSubmit();");
            button.setAttribute("accesskey", getBundleString("authSubmitAccessKey"));
        }

        if(button)
            ui_hbox.appendChild(button);
    } 
}

function UiLoad()
{
   var thisParent = window.opener;
   if(!thisParent)
   {
        alert(getBundleString("authDialogNoParent"));
        return;
   }

   var keyID =  this.name;
   ui = thisParent.getUIForKey(keyID); 

   var type =   thisParent.getTypeForKey(keyID);

   if(ui)
   {
       ConstructUI(type,keyID,ui);
   }
}

function UiUnload()
{
    if(gFormSubmitted)
    {
        return;
    }

    var noValidate = 1;
 
    FormSubmit(noValidate);
}

function GetRowsNodeFromGrid(theGrid)
{
    if(!theGrid)
         return;

    var childNodes = theGrid.childNodes;

    if(!childNodes)
        return null;

    var len = childNodes.length;
    var gridid = theGrid.id;
    var gridrowsid = gridid + "-rows";

    var rows = null;

    var curNode;
    for(i = 0 ; i < len ; i ++)
    {
        curNode = childNodes[i];

        if(!curNode)
            return null;

        if(curNode.id == gridrowsid)
        {
            rows = curNode;
            break;
        }

    }

   return rows;

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

