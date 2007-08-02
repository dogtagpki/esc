 # BEGIN COPYRIGHT BLOCK
 # Copyright (C) 2005 Red Hat, Inc.
 # All rights reserved.
 #
 # This library is free software; you can redistribute it and/or
 # modify it under the terms of the GNU Lesser General Public
 # License as published by the Free Software Foundation version
 # 2.1 of the License.
 #
 # This library is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 # Lesser General Public License for more details.
 #
 # You should have received a copy of the GNU Lesser General Public
 # License along with this library; if not, write to the Free Software
 # Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 # END COPYRIGHT BLOCK

Name: esc 
Version: 1.0.1
Release: 5%{?dist} 
Summary: Enterprise Security Client Smart Card Client
License: GPL
URL: http://directory.fedora.redhat.com/wiki/CoolKey 
Group: Applications/Internet

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Patch1: firefox-1.5-with-system-nss.patch
Patch2: firefox-1.1-nss-system-nspr.patch
Patch3: esc-1.0.1-admin-row-update.patch
Patch4: esc-1.0.1-ui-fixes-1.patch 
Patch5: esc-1.0.1-log-fixes.patch
Patch6: esc-1.0.1-log-fixes-1.patch

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
%define escxuldir src/app/xul/esc


Source0: %{escname}.tar.bz2
Source1: esc
Source2: esc.desktop
Source3: xulrunner-1.8.0.4-source.tar.bz2
Source4: esc.png


%description
Enterprise Security Client allows the user to enroll and manage their
cryptographic smartcards.

%prep

%setup -q -c -n %{escname}

#Perform esc patching

%patch3 -p1 -b .fix3
%patch4 -p1 -b .fix4
%patch5 -p1 -b .fix5
%patch6 -p1 -b .fix6


#Unpack xulrunner where esc expects it to be.

%setup -T -D -a 3 -n %{escname}/esc/dist/src

#Perform the patching of xulrunner

cd mozilla 

%patch1 -p1 -b .fix1
%patch2 -p1 -b .fix2

%build

%ifarch x86_64 ppc64 ia64
USE_64=1
export USE_64
%endif

# last setup call moved  the current directory

cd ../..

cp %{SOURCE4} %{escxuldir}/%{esc_chromepath}

make BUILD_OPT=1 HAVE_LIB_NOTIFY=1 ESC_VERSION=%{version}-%{release}

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

%preun

killall --exact -q escd
exit 0

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
* Tue Jul 17 2007 Jack Magne <jmagne@redhat.com>- 1.0.1-5
- Further fixes to the diagnostics logging.
* Wed Jun 20 2007 Jack Magne <jmagne@redhat.com>- 1.0.1-4
- Fixes to the diagnostics log files and esc  error messages.
* Thu Apr 26 2007 Jack Magne <jmagne@redhat.com>- 1.0.1-3
- Many UI usability fixes.
* Tue Apr 03 2007 Jack Magne <jmagne@redhat.com>- 1.0.1-2
* Mon Mar 05 2007 Jack Magne <jmagne@redhat.com>- 1.0.1-1
- Stability fixes
* Fri Oct 27 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-19
- More mac and win fixes.
* Tue Oct 24 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-18
-rebuilt on RHEL-5 branch
* Sun Oct 4  2006 Jack Magne <jmagne@redhat.com>- 1.0.0-17
- Diagnostics display fixes, Mac and Window fixes.

* Sun Oct 01 2006 Jesse Keating <jkeating@redhat.com> - 1.0.0-16
- rebuilt for unwind info generation, broken in gcc-4.1.1-21

* Fri Sep 22 2006 Jack Magne <jmagne@redhat.com>- 1.0.0-15
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

