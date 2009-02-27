/*
* -myapp commandline handler; starts up My App.
*/

 const nsIAppShellService    = Components.interfaces.nsIAppShellService;
 const nsISupports           = Components.interfaces.nsISupports;
 const nsICategoryManager    = Components.interfaces.nsICategoryManager;
 const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
 const nsICommandLine        = Components.interfaces.nsICommandLine;
 const nsICommandLineHandler = Components.interfaces.nsICommandLineHandler;
 const nsIFactory            = Components.interfaces.nsIFactory;
 const nsIModule             = Components.interfaces.nsIModule;
 const nsIWindowWatcher      = Components.interfaces.nsIWindowWatcher;

 var consoleService = Components
  .classes['@mozilla.org/consoleservice;1']
  .getService( Components.interfaces.nsIConsoleService );

 function recordMessage( message ) {
  consoleService.logStringMessage("esc: " + message  + "\n");
 }

/*
* Classes
*/
 
 const escCLH = {
   /* nsISupports */
   QueryInterface : function clh_QI(iid) {
       if (iid.equals(nsICommandLineHandler) ||
           iid.equals(nsIFactory) ||
           iid.equals(nsISupports))
           return this;

       throw Components.results.NS_ERROR_NO_INTERFACE;
   },

   /* nsICommandLineHandler */

   handle : function clh_handle(cmdLine) {
       var args = new Object();
       args.urlopt = false;

       recordMessage("handle command line");

       try {

           var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
           var win = wm.getMostRecentWindow(null);

           var showUsage = cmdLine.handleFlag("usage",false);

           recordMessage("ShowUsage flag: " + showUsage);
           if(showUsage) {
               cmdLine.preventDefault = true;
           }

           if(win)
           {
               recordMessage("Subsequent command invocation. Launch appropriate  page.");
               if(showUsage) 
               {
                   recordMessage("About to show usage.");
                   win.ShowUsage();
                   return;
               }
               var locName = win.location.toString();

               recordMessage("Base window . " + locName);

               win.SelectESCPageCMDLine();

               recordMessage("Done command line handling...");
               return;
           }
           
           var chromeURI = "chrome://esc/content/hiddenWindow.xul";

           recordMessage(chromeURI);

           var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                         .getService(nsIWindowWatcher);
           win = wwatch.openWindow(null, chromeURI, "_blank",
                     "chrome,dialog,height=-1,width=-1,popup=yes", cmdLine);


       }
       catch(e) {}

   },

   helpInfo : "  -myapp  Open the My App.\n",

   /* nsIFactory */

   createInstance : function clh_CI(outer, iid) {
       if (outer != null)
           throw Components.results.NS_ERROR_NO_AGGREGATION;

       return this.QueryInterface(iid);
   },

   lockFactory : function clh_lock(lock) {
       /* no-op */
   }
 };

 const clh_contractID = "@redhat.com/esc-clh;1";
 const clh_CID = Components.ID("{36c65861-52a8-4ce9-aa3b-235b88216ed4}");
 const clh_category = "c-esc";

 const escCLHModule = {
   /* nsISupports */

   QueryInterface : function mod_QI(iid) {
       if (iid.equals(nsIModule) ||
           iid.equals(nsISupports))
           return this;

       throw Components.results.NS_ERROR_NO_INTERFACE;
   },

   /* nsIModule */
   getClassObject : function mod_gch(compMgr, cid, iid) {
       if (cid.equals(clh_CID))
           return escCLH.QueryInterface(iid);

       throw Components.results.NS_ERROR_NOT_REGISTERED;
   },

   registerSelf : function mod_regself(compMgr, fileSpec, location, type) {
       compMgr.QueryInterface(nsIComponentRegistrar);

       compMgr.registerFactoryLocation(clh_CID,
                                       "escCLH",
                                       clh_contractID,
                                       fileSpec,
                                       location,
                                       type);

       var catMan = Components.classes["@mozilla.org/categorymanager;1"]
                              .getService(nsICategoryManager);
       catMan.addCategoryEntry("command-line-handler",
                               clh_category,
                               clh_contractID, true, true);
   },

   unregisterSelf : function mod_unreg(compMgr, location, type) {
       compMgr.QueryInterface(nsIComponentRegistrar);

       compMgr.unregisterFactoryLocation(clh_CID, location);

       var catMan = Components.classes["@mozilla.org/categorymanager;1"]
                              .getService(nsICategoryManager);
       catMan.deleteCategoryEntry("command-line-handler", clh_category);
   },

   canUnload : function (compMgr) {
       return true;
   }
 };

 /* Module initialisation */
 function NSGetModule(comMgr, fileSpec) {
   return escCLHModule;
 }
