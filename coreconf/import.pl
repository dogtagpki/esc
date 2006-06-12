#! /usr/local/bin/perl
#
# BEGIN COPYRIGHT BLOCK
# This Program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 of the License.
#
# This Program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA.
#
# Copyright (C) 2005 Red Hat, Inc.
# All rights reserved.
# END COPYRIGHT BLOCK

print STDERR "import.pl\n";

require('coreconf.pl');

use File::Basename ;

sub fetch {
    ($jartarget , $distpathname) = @_;
    print STDERR "\nFetching jarfile from: $jartarget\n";
    $disttarget = "$distpathname/download";
    unlink ("$disttarget/$jarfile");
    system("$var{MKDIR} -p $disttarget");
    system("$var{WGET} -P $disttarget $jartarget");
    return "$disttarget/$jarfile";
}

$returncode =0;


#######-- read in variables on command line into %var

$var{ZIP} = "zip";
$var{UNZIP} = "unzip -o";
$var{MKDIR} = "mkdir";
$var{WGET} = "wget";
$var{TAR} = "tar";

&parse_argv;

if (! ($var{IMPORTS} =~ /\w/)) {
    print STDERR "nothing to import\n";
}

######-- Do the import!

foreach $import (split(/ /,$var{IMPORTS}) ) {

    # skip the blank lines
    if ($import =~ m/^[ \t]*$/) {
	next;
    }

    print STDERR "\n\nIMPORTING .... $import\n-----------------------------\n";


# if a specific version specified in IMPORT variable
# (if $import has a slash in it)

    if ($import =~ /\//) {
        # $component=everything before the first slash of $import

	$import =~ m|^([^/]*)/|; 
	$component = $1;

	$import =~ m|^(.*)/([^/]*)$|;

	# $path=everything before the last slash of $import
	$path = $1;

	# $version=everything after the last slash of $import
	$version = $2;

	if ($var{VERSION} ne "current") {
	    $version = $var{VERSION};
	}
    }
    else {
	$component = $import;
	$path = $import;
	$version = $var{VERSION};
    }

    $releasejardir = "$var{RELEASE_TREE}/$path";
    $netfetch = 0;
#is the path a URL. we tell this by matching any alpha string followed by a
#://
    if ($releasejardir =~ m|^[a-zA-Z]*://| ) {
	$netfetch = 1;
    } elsif ($version eq "current") {
	print STDERR "Current version specified. Reading 'current' file ... \n";
	
	open(CURRENT,"$releasejardir/current") || die "NO CURRENT FILE\n";
	$version = <CURRENT>;
	$version =~ s/(\r?\n)*$//; # remove any trailing [CR/]LF's
	close(CURRENT);
	print STDERR "Using version $version\n";
	if ( $version eq "") {
	    die "Current version file empty. Stopping\n";
	}
    }

my %ext2type = (
   ".tar.gz", "$var{TAR} -xvzf ",
   ".tgz", "$var{TAR} -xzvf ",
   ".tar.bz2", "$var{TAR} -xjvf ",
   ".tar.Z", "$var{TAR} -xZvf ",
   ".zip", "$var{UNZIP} ",
   ".tar", "$var{TAR} -xvf " );

    $tarball = 0;
    foreach ( keys %ext2type ) {
	if ( $version =~ m|$_$| ) {
	    print STDERR "Tar Type = '$_' => '$ext2type{$_}'\n ";
	    $tarType = $_;
	    $tarball = 1;
	    next;
	}
    }
    
    $releasejardir = "$releasejardir/$version";
    if ( $tarball == 1) {
	$jartarget = $releasejardir;
	$distdir = "$var{SOURCE_XP_DIR}/$component";

	if ($netfetch == 1) {

            ($base,$path,$type) = fileparse($jartarget);

            $jarfile = $base . $type;
	    $jartarget = fetch($jartarget, $distdir);
	}
	if ( ! -e $jartarget )  {
	    die "$jartarget doesn't exist.\n";
	}
	if ( $tarType eq ".zip" ) {
	    $location="-d $distdir";
	} else {
	    $location="-C$distdir";
	}
	if (! -d $distdir) {
	   print "Making $distdir\n";
	   system("$var{MKDIR} -p $distdir");
	   &rec_mkdir("$distdist");
	}
	print STDERR "$ext2type{$tarType}$jartarget $location\n";
	system("$ext2type{$tarType}$jartarget $location");
	unlink ($jartarget);
	next;
    }
    if ($netfetch != 1 && ! -d $releasejardir) {
	die "$releasejardir doesn't exist (Invalid Version?)\n";
    }
    foreach $jarfile (split(/ /,$var{FILES})) {

	($relpath,$distpath,$options) = split(/\|/, $var{$jarfile});

	if ($var{'OVERRIDE_IMPORT_CHECK'} eq 'YES') {
	    $options =~ s/v//g;
	}

	if ( $relpath ne "") { $releasejarpathname = "$releasejardir/$relpath";}
	else { $releasejarpathname = $releasejardir; }

	if ($distpath =~ m|/$|) {
	    $distpathname = "$distpath$component";
	} else {
	    $distpathname = "$distpath"; 
	}


# If a component doesn't have IDG versions, import the DBG ones
# there are no IDG's on the net
	if ( $netfetch == 1 ||  ! -e "$releasejarpathname/$jarfile" ) {
            if( $relpath =~ /IDG\.OBJ$/ ) {
        	$relpath =~ s/IDG.OBJ/DBG.OBJ/;
        	$releasejarpathname = "$releasejardir/$relpath";
            } elsif ( $relpath =~ /IDG\.OBJD$/ ) {
        	$relpath =~ s/IDG.OBJD/DBG.OBJD/;
        	$releasejarpathname = "$releasejardir/$relpath";
            }
	}

	$jartarget="$releasejarpathname/$jarfile";
        if ( $netfetch == 1 ) {
	    $jartarget = fetch($jartarget, $distpathname)
	}

	if (  -e "$jartarget") {
	    print STDERR "\nWorking on jarfile: $jarfile\n";
	    
	  
	  
#the block below is used to determine whether or not the xp headers have
#already been imported for this component

	  $doimport = 1;
	  if ($options =~ /v/) {   # if we should check the imported version
	      print STDERR "Checking if version file exists $distpathname/version\n";
	      if (-e "$distpathname/version") {
		  open( VFILE, "<$distpathname/version") ||
		      die "Cannot open $distpathname/version for reading. Permissions?\n";
		  $importversion = <VFILE>;
		  close (VFILE);
		  $importversion =~ s/\r?\n$//;   # Strip off any trailing CR/LF
		  if ($version eq $importversion) {
		      print STDERR "$distpathname version '$importversion' already imported. Skipping...\n";
		      $doimport =0;
		  }
	      }
	  }
	  
	  if ($doimport == 1) {
	      if (! -d "$distpathname") {
		  &rec_mkdir("$distpathname");
	      }
	      # delete the stuff in there already.
	      # (this should really be recursive delete.)
	      
	      if ($options =~ /v/) {
		  $remheader = "\nREMOVING files in '$distpathname/' :";
		  opendir(DIR,"$distpathname") ||
		      die ("Cannot read directory $distpathname\n");
		  @filelist = readdir(DIR);
		  closedir(DIR);
		  foreach $file ( @filelist ) {
		      if (! ($file =~ m!/.?.$!) ) {
			  if (! (-d $file)) {
			      $file =~ m!([^/]*)$!;
			      print STDERR "$remheader $1";
			      $remheader = " ";
			      unlink "$distpathname/$file";
			  }
		      }
		  }
	      }


	      print STDERR "\n\n";

	      print STDERR "\nExtracting jarfile '$jarfile' to local directory $distpathname/\n";
	      
	      print STDERR "$var{UNZIP} $jartarget -d $distpathname\n";
	      system("$var{UNZIP} $jartarget -d $distpathname");
	      if ( $netfetch == 1 ) {
		unlink ("$jartarget");
	      }
	      
	      $r = $?;

	      if ($options =~ /v/) {
		  if ($r == 0) {
		      unlink ("$distpathname/version");
		      if (open(VFILE,">$distpathname/version")) {
			  print VFILE "$version\n";
			  close(VFILE);
		      }
		  }
		  else {
		      print STDERR "Could not create '$distpathname/version'. Permissions?\n";
		      $returncode ++;
		  }
	      }
	  }  # if (doimport)
	} # if (-e releasejarpathname/jarfile)
    } # foreach jarfile)
} # foreach IMPORT



exit($returncode);





