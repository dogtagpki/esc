; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

; BEGIN COPYRIGHT BLOCK
; This Program is free software; you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free Software
; Foundation; version 2 of the License.
;
; This Program is distributed in the hope that it will be useful, but WITHOUT
; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along with
; this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
; Place, Suite 330, Boston, MA 02111-1307 USA.
;
; Copyright (C) 2005 Red Hat, Inc.
; All rights reserved.
; END COPYRIGHT BLOCK

[Setup]
AppName=Smart Card Manager
AppMutex=ESCMutex
AppVerName=Smart Card Manager 1.0.1-6
AppPublisher=Fedora
CreateAppDir=true
Compression=lzma
SolidCompression=true
MinVersion=0,5.0.2195
ShowLanguageDialog=yes
OutputBaseFilename=SmartCardManagerSetup-1.0.1-6.win32.i386
DefaultDirName={pf}\Fedora\ESC
DisableProgramGroupPage=false
DefaultGroupName=Fedora
SetupIconFile=BUILD\ESC\components\esc.ico
UninstallDisplayIcon={app}\components\esc.ico
;WizardImageFile=BUILD\ESC\chrome\content\esc\esc-image-large.bmp
WizardImageFile=esc-image-large.bmp
WizardSmallImageFile=BUILD\ESC\components\esc.bmp
AllowNoIcons=yes
LicenseFile=esc-license.txt
InfoBeforeFile=info-before.txt
InfoAfterFile=info-after.txt
PrivilegesRequired=admin
VersionInfoVersion=1.0.1.4


[Files]
Source: BUILD\egate\slbmgpg.dll; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egate.cat; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egate.inf; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egate.sys; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egate_License.txt; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egatebus.cat; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egatebus.inf; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egatebus.sys; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egaterdr.cat; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egaterdr.inf; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egaterdr.sys; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\egdrvins1.dll; DestDir: {win}\egate2.4; Flags: uninsneveruninstall
Source: BUILD\egate\eginstall.exe; DestDir: {win}\egate2.4; Flags: ignoreversion
Source: BUILD\pk11install.exe; DestDir: {app}\PKCS11

;Files related to CSP, comment out if not available
Source: BUILD\clkcsp.dll; DestDir: {sys}; Flags: regserver restartreplace
Source: BUILD\cspres.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\clkcsp.sig; DestDir: {sys}

Source: BUILD\atl71.dll; DestDir: {sys}; Flags: uninsneveruninstall onlyifdoesntexist

Source: BUILD\msvcr71.dll; DestDir: {sys}; Flags: uninsneveruninstall onlyifdoesntexist

Source: BUILD\msvcp71.dll; DestDir: {sys}; Flags: uninsneveruninstall onlyifdoesntexist

Source: BUILD\mfc71.dll; DestDir: {sys}; Flags: uninsneveruninstall onlyifdoesntexist

; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: BUILD\ESC\components\rhTray.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhCoolKey.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhICoolKey.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhIKeyNotify.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhITray.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhITrayWindNotify.xpt; DestDir: {app}\components
Source: BUILD\ESC\components\rhTray.dll; DestDir: {app}\components
Source: BUILD\ESC\components\esc.ico; DestDir: {app}\components
Source: BUILD\ESC\components\rhCoolKey.dll; DestDir: {app}\components
Source: BUILD\coolkeypk11.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\libckyapplet-1.dll; DestDir: {sys}; Flags: restartreplace
Source: BUILD\zlib1.dll;DestDir: {sys}; Flags: restartreplace
;Source: BUILD\coolkeypk11.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\esc.exe; DestDir: {app}
;Source: BUILD\ESC\esc.bat; Destdir: {app}
Source: BUILD\ESC\application.ini; DestDir: {app}
Source: BUILD\ESC\chrome\chrome.manifest; DestDir: {app}\chrome
Source: BUILD\ESC\chrome\content\esc\TRAY.js; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\CertInfo.js; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\certinfo.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\config.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\contents.rdf; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\EnrollPopupInclude.html; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\esc.css; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\ESC.js; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\esc.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\security.xul; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\esc_browser.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\GenericAuth.js; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\GenericAuth.xul; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\GenericAuthInclude.html; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\logo.gif; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\settings.xul; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\style.css; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\bg.jpg; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\xulrunner\chrome\toolkit.manifest; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\classic.jar; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\classic.manifest; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\comm.jar; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\comm.manifest; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\en-US.jar; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\en-US.manifest; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\pippki.jar; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\pippki.manifest; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\toolkit.jar; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\chrome\chromelist.txt; DestDir: {app}\xulrunner\chrome
Source: BUILD\ESC\xulrunner\xulrunner-stub.exe; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\dependentlibs.list; DestDir: {app}\xulrunner
;Source: BUILD\ESC\xulrunner\GenerateJavaInterfaces.exe; DestDir: {app}\xulrunner
;Source: BUILD\ESC\xulrunner\javaxpcom.jar; DestDir: {app}\xulrunner
;Source: BUILD\ESC\xulrunner\javaxpcomglue.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\js3250.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\LICENSE; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\mozctl.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\mozctlx.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\nspr4.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\nspr4.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\xulrunner\nss3.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\nss3.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\xulrunner\nssckbi.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\plc4.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\plc4.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\xulrunner\plds4.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\plds4.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\xulrunner\README.txt; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\smime3.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\softokn3.chk; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\softokn3.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\softokn3.dll; DestDir: {app}\PKCS11
Source: BUILD\ESC\xulrunner\ssl3.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\updater.exe; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\xpcom.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\xpicleanup.exe; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\xul.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\xulrunner.exe; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\AccessibleMarshal.dll; DestDir: {app}\xulrunner
Source: BUILD\ESC\xulrunner\components\xulutil.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\accessibility-msaa.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\alerts.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\appshell.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\appstartup.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\auth.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\autocomplete.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\autoconfig.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\autoconfig.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\caps.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\chardet.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\chrome.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\commandhandler.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\commandlines.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\composer.xpt; DestDir: {app}\xulrunner\components
;Source: BUILD\ESC\xulrunner\components\compreg.dat; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_html.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_htmldoc.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_xmldoc.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_xslt.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\content_xtf.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\directory.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\docshell_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_canvas.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_core.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_css.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_events.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_html.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_loadsave.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_range.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_stylesheets.xpt; DestDir: {app}\xulrunner\components
;Source: BUILD\ESC\xulrunner\components\dom_svg.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_traversal.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_views.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_xbl.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_xpath.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\dom_xul.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\downloads.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\editor.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\embed_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\extensions.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\exthandler.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\fastfind.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\find.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\gfx.xpt; DestDir: {app}\xulrunner\components
;Source: BUILD\ESC\xulrunner\components\gksvgrenderer.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\history.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\htmlparser.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\imgicon.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\imglib2.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\intl.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\intlcmpt.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\jar.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\jsconsole.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\jsconsole-clhandler.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\jsdservice.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\layout_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\layout_printing.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\layout_xul.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\layout_xul_tree.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\locale.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\lwbrk.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\mimetype.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\mozbrwsr.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\mozfind.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_about.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_cache.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_cookie.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_data.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_dns.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_file.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_ftp.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_http.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_res.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_socket.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_strconv.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\necko_viewsource.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsCloseAllWindows.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsDefaultCLH.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsDictionary.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsExtensionManager.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsHelperAppDlg.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsInterfaceInfoToIDL.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsKillAll.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsPostUpdateWin.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsProgressDialog.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsProxyAutoConfig.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsResetPref.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsUpdateService.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsXmlRpcClient.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\nsXULAppInstall.js; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\oji.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\passwordmgr.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pipboot.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pipboot.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pipnss.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pipnss.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pippki.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pippki.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\plugin.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\pref.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\prefetch.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\profile.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\progressDlg.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\proxyObject.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\rdf.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\satchel.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\shistory.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\toolkitprofile.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\transformiix.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\txmgr.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\txtsvc.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\uconv.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\unicharutil.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\universalchardet.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\update.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\uriloader.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\webBrowser_core.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\webbrowserpersist.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\webshell_idls.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\websrvcs.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\websrvcs.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\widget.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\windowds.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\windowwatcher.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xmlextras.dll; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xmlextras.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xml-rpc.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_base.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_components.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_ds.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_io.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_obsolete.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_thread.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpcom_xpti.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpconnect.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xpinstall.xpt; DestDir: {app}\xulrunner\components
;Source: BUILD\ESC\xulrunner\components\xpti.dat; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xulapp.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xulapp_setup.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xuldoc.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xulrunner.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\xultmpl.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\components\accessibility.xpt; DestDir: {app}\xulrunner\components
Source: BUILD\ESC\xulrunner\defaults\autoconfig\prefcalls.js; DestDir: {app}\xulrunner\defaults\autoconfig
Source: BUILD\ESC\xulrunner\defaults\autoconfig\platform.js; DestDir: {app}\xulrunner\defaults\autoconfig
Source: BUILD\ESC\xulrunner\defaults\pref\xulrunner.js; DestDir: {app}\xulrunner\defaults\pref
Source: BUILD\ESC\xulrunner\defaults\profile\chrome\userContent-example.css; DestDir: {app}\xulrunner\defaults\profile\chrome
Source: BUILD\ESC\xulrunner\defaults\profile\chrome\userChrome-example.css; DestDir: {app}\xulrunner\defaults\profile\chrome
Source: BUILD\ESC\xulrunner\defaults\profile\extensions\installed-extensions.txt; DestDir: {app}\xulrunner\defaults\profile\extensions
Source: BUILD\ESC\xulrunner\defaults\profile\extensions\Extensions.rdf; DestDir: {app}\xulrunner\defaults\profile\extensions
Source: BUILD\ESC\xulrunner\defaults\profile\US\localstore.rdf; DestDir: {app}\xulrunner\defaults\profile\US
Source: BUILD\ESC\xulrunner\defaults\profile\US\chrome\userContent-example.css; DestDir: {app}\xulrunner\defaults\profile\US\chrome
Source: BUILD\ESC\xulrunner\defaults\profile\US\chrome\userChrome-example.css; DestDir: {app}\xulrunner\defaults\profile\US\chrome
Source: BUILD\ESC\xulrunner\greprefs\xpinstall.js; DestDir: {app}\xulrunner\greprefs
Source: BUILD\ESC\xulrunner\greprefs\security-prefs.js; DestDir: {app}\xulrunner\greprefs
Source: BUILD\ESC\xulrunner\greprefs\all.js; DestDir: {app}\xulrunner\greprefs
Source: BUILD\ESC\xulrunner\plugins\npnul32.dll; DestDir: {app}\xulrunner\plugins
Source: BUILD\ESC\xulrunner\res\dtd\xhtml11.dtd; DestDir: {app}\xulrunner\res\dtd
Source: BUILD\ESC\xulrunner\res\dtd\mathml.dtd; DestDir: {app}\xulrunner\res\dtd
Source: BUILD\ESC\xulrunner\res\entityTables\transliterate.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\entityTables\html40Special.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\entityTables\html40Symbols.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\entityTables\htmlEntityVersions.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\entityTables\mathml20.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\entityTables\html40Latin1.properties; DestDir: {app}\xulrunner\res\entityTables
Source: BUILD\ESC\xulrunner\res\fonts\mathfontSymbol.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\fontNameMap.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfont.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontCMEX10.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontCMSY10.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontMath1.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontMath2.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontMath4.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontMTExtra.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\mathfontPUA.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\fonts\fontEncoding.properties; DestDir: {app}\xulrunner\res\fonts
Source: BUILD\ESC\xulrunner\res\html\gopher-unknown.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-binary.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-find.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-image.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-menu.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-movie.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-sound.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-telnet.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-text.gif; DestDir: {app}\xulrunner\res\html
Source: BUILD\ESC\xulrunner\res\html\gopher-audio.gif; DestDir: {app}\xulrunner\res\html
;Source: BUILD\ESC\xulrunner\sdk\lib\MozillaInterfaces-src.jar; DestDir: {app}\xulrunner\sdk\lib
;Source: BUILD\ESC\xulrunner\sdk\lib\MozillaInterfaces.jar; DestDir: {app}\xulrunner\sdk\lib
Source: BUILD\ESC\defaults\preferences\esc-prefs.js; DestDir: {app}\defaults\preferences
Source: BUILD\ESC\xulrunner\res\wincharset.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\arrowd.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\bloatcycle.html; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\broken-image.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\charsetalias.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\charsetData.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\cmessage.txt; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\EditorOverride.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\forms.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\grabber.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\hiddenWindow.html; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\html.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\langGroups.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\language.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\loading-image.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\mathml.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\quirk.css; DestDir: {app}\xulrunner\res
;Source: BUILD\ESC\xulrunner\res\svg.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-after.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-after-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-after-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-before.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-before-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-column-before-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-after.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-after-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-after-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-before.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-before-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-add-row-before-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-column.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-column-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-column-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-row.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-row-active.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\table-remove-row-hover.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\ua.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\viewer.properties; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\viewsource.css; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\xulrunner\res\arrow.gif; DestDir: {app}\xulrunner\res
Source: BUILD\ESC\chrome\icons\default\esc-window.ico; DestDir: {app}\chrome\icons\default
Source: BUILD\ESC\chrome\icons\default\settings-window.ico; DestDir: {app}\chrome\icons\default
Source: BUILD\ESC\chrome\locale\en-US\esc.properties; DestDir: {app}\chrome\locale\en-US
Source: BUILD\ESC\chrome\locale\en-US\esc.dtd; DestDir: {app}\chrome\locale\en-US
Source: BUILD\ESC\chrome\content\esc\throbber-anim5.gif; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\2-vweak.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\3-weak.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\4-fair.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\5-good.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\6-strong.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\blank-card.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\enrolled-key.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\initializecard.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\progress.7.gif; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\1-none.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\components\escCLH.js; DestDir: {app}\components
;Source: BUILD\ESC\chrome\content\esc\hiddenWindow.html; DestDir: {app}\chrome\content\esc
;Source: BUILD\ESC\chrome\content\esc\aol.gif; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\AdvancedInfo.js; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\advancedinfo.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\blank-cardx2.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\password.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\enroll.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\enrolled-keyx2.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\enrollx2.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\esc-client-24.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\formatcard.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\formatcardx2.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\hiddenWindow.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\initializecardx2.png; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\MineOverlay.xul; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\password.js; DestDir: {app}\chrome\content\esc
Source: BUILD\ESC\chrome\content\esc\certManager.xul; DestDir: {app}\chrome\content\esc

[Run]

Filename: {win}\egate2.4\eginstall.exe; Parameters: -f -l eginstall.txt; WorkingDir: {win}\egate2.4; Flags: waituntilterminated; StatusMsg: Installing smart card drivers....
Filename: {app}\PKCS11\pk11install.exe; Parameters: "-v ""name='CoolKey Module' library=coolkeypk11.dll NSS=""slotParams={{0x1=[slotFlags=PublicCerts]}"""""; WorkingDir: {sys}; StatusMsg: Configuring System for smart cards...
Filename: {app}\esc.exe; WorkingDir: {app}; StatusMsg: {code:GetEscStatusMsg} Flags: nowait

[UninstallDelete]
Type: filesandordirs; Name: {app}
[_ISTool]
OutputExeFilename=BUILD\coolkey-setup.exe
UseAbsolutePaths=false
LogFile=inst.log
LogFileAppend=false
[Registry]
;The following lines register the CSP. Comment out if not available
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Defaults\Provider\CoolKey PKCS#11 CSP; ValueType: string; ValueName: PKCS11Module; ValueData: coolkeypk11.dll; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: binary; ValueName: ATRMask; ValueData: ff ff ff ff ff ff ff ff 00 00; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: string; ValueName: Crypto Provider; ValueData: CoolKey PKCS#11 CSP
Root: HKLM; Subkey: Software\Microsoft\Cryptography\Calais\SmartCards\Axalto Developer; ValueType: binary; ValueName: ATR; ValueData: 3b 75 94 00 00 62 02 02 00 00
;End CSP registration
Root: HKLM; Subkey: Software\
; Turn off the "pick a cert" dialog box
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\3; ValueType: dword; ValueName: 1A04; ValueData: 0
; Enable TLS 1.0
Root: HKCU; Subkey: Software\Microsoft\Windows\CurrentVersion\Internet Settings; ValueType: dword; ValueName: SecureProtocols; ValueData: 168
[Icons]
Name: {userdesktop}\Smart Card Manager; Filename: {app}\esc.exe; WorkingDir: {app}; IconFileName : {app}\components\esc.ico
Name: {group}\ESC\Smart Card Manager; Filename: {app}\esc.exe; WorkingDir: {app}; IconFileName : {app}\components\esc.ico
Name: {commonstartup}\Smart Card Manager; Filename: {app}\esc.exe; WorkingDir: {app}; IconFileName : {app}\components\esc.ico

[Dirs]
Name: {app}\PKCS11
Name: {app}\components; Flags: deleteafterinstall uninsalwaysuninstall
Name: {app}\chrome
Name: {app}\defaults
Name: {app}\xulrunner
Name: {app}\chrome\content
Name: {app}\chrome\icons
Name: {app}\chrome\content\esc
Name: {app}\chrome\locale
Name: {app}\chrome\locale\en-US
Name: {app}\chrome\icons\default
Name: {app}\xulrunner\chrome
Name: {app}\xulrunner\components
Name: {app}\xulrunner\defaults
Name: {app}\xulrunner\greprefs
Name: {app}\xulrunner\plugins
Name: {app}\xulrunner\res
Name: {app}\xulrunner\sdk
Name: {app}\xulrunner\defaults\autoconfig
Name: {app}\xulrunner\defaults\pref
Name: {app}\xulrunner\defaults\profile
Name: {app}\xulrunner\defaults\profile\chrome
Name: {app}\xulrunner\defaults\profile\extensions
Name: {app}\xulrunner\defaults\profile\US
Name: {app}\xulrunner\defaults\profile\US\chrome
Name: {app}\xulrunner\res\dtd
Name: {app}\xulrunner\res\entityTables
Name: {app}\xulrunner\res\fonts
Name: {app}\xulrunner\res\html
Name: {app}\xulrunner\sdk\lib
Name: {app}\xulrunner\estensions
Name: {app}\xulrunner\updates
Name: {app}\xulrunner\updates\0
Name: {app}\defaults\preferences

[Code]

function GetEscStatusMsg(Param: String) :string;
var MyMillis: LongInt;
begin

MyMillis := 5000;

Sleep(MyMillis);

Result := 'Starting Smart Card Manager.....';
end;
