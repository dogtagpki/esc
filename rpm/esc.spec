Name: esc 
Version: 1.0.0
Release: 16%{?dist} 
Summary: Enterprise Security Client Smart Card Client
License: GPL
URL: http://directory.fedora.redhat.com/wiki/CoolKey 
Group: Applications/Internet

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Patch1: firefox-1.5.0.1-dumpstack.patch
Patch2: xulrunner-1.8.0.1-coreconf.patch
Patch3: firefox-1.5-with-system-nss.patch
Patch4: firefox-1.1-nss-system-nspr.patch
Patch5: esc-1.0.0-xul-sys-nss-nspr.patch
Patch6: esc-1.0.0-ui-enhance.patch
Patch7: esc-1.0.0-notify-icon-fixes.patch
Patch8: esc-1.0.0-strings-fix.patch
Patch11: esc-1.0.0.strings-1-fix.patch
Patch12: esc-1.0.0-ui-enhance-1.patch
Patch13: esc-1.0.0-pw-reset-fix.patch
Patch14: esc-1.0.0-escd.patch
Patch15: esc-1.0.0-escd1.patch
Patch16: esc-1.0.0-escd2.patch
Patch17: esc-1.0.0-build-fix.patch
Patch18: esc-1.0.0-diag-fix.patch

BuildRequires: doxygen fontconfig-devel freetype-devel >= 2.1
BuildRequires: glib2-devel libIDL-devel atk-devel gtk2-devel libjpeg-devel
BuildRequires: pango-devel libpng-devel pkgconfig zlib-devel
BuildRequires: nspr-devel nss-devel
BuildRequires: autoconf213 libX11-devel libXt-devel

BuildRequires: pcsc-lite-devel coolkey-devel
BuildRequires: desktop-file-utils zip binutils libnotify-devel
BuildRequires: dbus-devel
Requires: pcsc-lite ifd-egate coolkey nss nspr
Requires: zip dbus >= 0.90 libnotify >= 0.4.2

# 390 does not have coolkey or smartCards
# Xulrunner won't compile on ppc64 right now.
ExcludeArch: s390 s390x ppc64

# We can't allow the internal xulrunner to leak out
AutoReqProv: 0

#%define __prelink_undo_cmd %{nil}
%define escname %{name}-%{version}
%define escdir %{_libdir}/%{escname}
%define escbindir %{_bindir}
%define esc_chromepath   chrome/content/esc
%define appdir applications
%define icondir %{_datadir}/icons/hicolor/48x48/apps
%define esc_vendor esc 
%define autostartdir %{_sysconfdir}/xdg/autostart
%define pixmapdir  %{_datadir}/pixmaps
%define docdir    %{_defaultdocdir}/%{escname}
%define escappdir src/app/xpcom


Source0: %{escname}.tar.bz2
Source1: esc
Source2: esc.desktop
Source3: xulrunner-1.8.0.1-source.tar.bz2


%description
Enterprise Security Client allows the user to enroll and manage their
cryptographic smartcards.

%prep

%setup -q -c -n %{escname}

#patch esc to use system nss and nspr.

%patch5 -p1 -b .fix5
%patch6 -p1 -b .fix6
%patch7 -p1 -b .fix7
%patch8 -p1 -b .fix8
%patch11 -p1 -b .fix11
%patch12 -p1 -b .fix12
%patch13 -p1 -b .fix13
%patch14 -p1 -b .fix14
%patch15 -p1 -b .fix15
%patch16 -p1 -b .fix16
%patch17 -p1 -b .fix17
%patch18 -p1 -b .fix18

#Unpack xulrunner where esc expects it to be.

%setup -T -D -a 3 -n %{escname}/esc/dist/src

#Perform the patching of xulrunner

cd mozilla 

%patch1 -p1 -b .fix1
%patch2 -p1 -b .fix2
%patch3 -p1 -b .fix3
%patch4 -p1 -b .fix4

%build

%ifarch x86_64 ppc64 ia64
USE_64=1
export USE_64
%endif

# last setup call moved  the current directory

cd ../..

make BUILD_OPT=1 HAVE_LIB_NOTIFY=1

%install

cd ../../src/app/xpcom 

mkdir -p $RPM_BUILD_ROOT/%{escbindir}
mkdir -p $RPM_BUILD_ROOT/%{icondir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/%{appdir}
mkdir -p $RPM_BUILD_ROOT/%{autostartdir}
mkdir -p $RPM_BUILD_ROOT/%{pixmapdir}
mkdir -p $RPM_BUILD_ROOT/%{docdir}


sed -e 's;\$LIBDIR;'%{_libdir}';g'  %{SOURCE1} > $RPM_BUILD_ROOT/%{escbindir}/%{name}


chmod 755 $RPM_BUILD_ROOT/%{escbindir}/esc

mkdir -p $RPM_BUILD_ROOT/%{escdir}

%ifarch x86_64 ppc64 ia64
USE_64=1
export USE_64
%endif


make BUILD_OPT=1 install DESTDIR=$RPM_BUILD_ROOT/%{escdir}

rm -rf $RPM_BUILD_ROOT/%{escdir}/usr

cd ../../../dist/*OPT*/esc_build/esc

cp %{esc_chromepath}/esc.png $RPM_BUILD_ROOT/%{icondir}
ln -s $RPMBUILD_ROOT%{icondir}/esc.png $RPM_BUILD_ROOT/%{pixmapdir}/esc.png

cp %{SOURCE2} $RPM_BUILD_ROOT/%{_datadir}/%{appdir}
cp %{SOURCE2} $RPM_BUILD_ROOT/%{autostartdir}

cd %{_builddir}
cp %{escname}/esc/LICENSE $RPM_BUILD_ROOT/%{docdir}

%clean

rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)

%{escdir}/esc
%{escdir}/escd
%{escbindir}/esc
%{escdir}/application.ini

%{escdir}/chrome/chrome.manifest

%{escdir}/chrome/content
%{escdir}/chrome/locale
%{escdir}/chrome/icons/default
%{escdir}/components

%{escdir}/defaults/preferences/esc-prefs.js   

%{escdir}/xulrunner
%{icondir}/esc.png
%{pixmapdir}/esc.png
%{autostartdir}/esc.desktop
%{_datadir}/%{appdir}/esc.desktop
%doc %{docdir}/LICENSE

%post
touch --no-create %{_datadir}/icons/hicolor || :
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
   %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi

%postun
touch --no-create %{_datadir}/icons/hicolor || :
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
   %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi

%changelog
* Fri Sep 22 2006 Jack Magne <jmagne@redhat.com>= 1.0.0-15
- Fix to the build version

* Fri Sep 22 2006 Jack Magne <jmagne@redhat.com>= 1.0.0-14
- Fix to compile error in daemon

* Fri Sep 22 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-13
- Fix to include the new esc daemon.  

* Sat Sep 16 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-12
- Fix for Password Reset and minor UI revision.

* Fri Sep 15 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-11
- Further UI enhancement bug fixes

* Thu Sep 7 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-10
- Further strings revisions.

* Wed Aug 30 2006 Jack Magne <jmagne@redhat.com>-  1.0.0-9
- Revision of the strings used in ESC.

* Sat Aug 27 2006 Jack Magne <jmagne@redhat.com>-  1.0.0-8
- Fixes to get libnotify working properly on FC6 systems.

* Tue Aug 22 2006 Jack Magne <jmagne@redhat.com> - 1.0.0-7
- Fix for bug #203211, use of system NSS and NSPR for
- Xulrunner ,addressing the problem running on 64 bit.
- Overwriting 5 and 6 due to important bug #203211.

* Fri Aug  18 2006 Jack Magne <jmagne@redhat.com> - 1.0.0-6
- Correct problem with Patch #6

* Tue Aug  17 2006 Jack Magne <jmagne@redhat.com> - 1.0.0-5
- Build ESC's xulrunner component using system nss and nspr
- Build process creates run script based on {_libdir} variable,
  accounting for differences on 64 bit machines.
- UI enhancements

* Tue Aug  1 2006 Matthias Clasen <mclasen@redhat.com> - 1.0.0-4
- Don't auto-generate requires either

* Mon Jul 31 2006 Matthias Clasen <mclasen@redhat.com> - 1.0.0-3
- Don't provide mozilla libraries

* Fri Jul 28 2006 Ray Strode <rstrode@redhat.com> - 1.0.0-2
- remove bogus gtk+ requires (and some others that will
  be automatic)

* Tue Jun 13 2006 Jack Magne <jmagne@redhat.com> - 1.0.0-1
- Initial revision for fedora

