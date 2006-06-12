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

require('coreconf.pl');

#######-- read in variables on command line into %var

$var{ZIP} = "zip";

&parse_argv;

 
######-- Do the packaging of jars.

foreach $jarfile (split(/ /,$var{FILES}) ) {
    print STDERR "---------------------------------------------\n";
    print STDERR "Packaging jar file $jarfile....\n";

    $jarinfo = $var{$jarfile};

    ($jardir,$jaropts) = split(/\|/,$jarinfo);

    $zipoptions = "-T";
    if ($jaropts =~ /a/) {
	if ($var{OS_ARCH} eq 'WINNT') {
	    $zipoptions .= ' -ll';
	}
    }

# just in case the directory ends in a /, remove it
    if ($jardir =~ /\/$/) {
	chop $jardir;
    }

    $dirdepth --;
    
    print STDERR "jardir = $jardir\n";
    system("ls $jardir");

    if (-d $jardir) {


# count the number of slashes

	$slashes =0;
	
	foreach $i (split(//,$jardir)) {
	    if ($i =~ /\//) {
		$slashes++;
	    }
	}

	$dotdots =0;
	
	foreach $i (split(m|/|,$jardir)) {
	    if ($i eq '..') {
		$dotdots ++;
	    }
	}

	$dirdepth = ($slashes +1) - (2*$dotdots);

	print STDERR "changing dir $jardir\n";
	chdir($jardir);
	print STDERR "making dir META-INF\n";
	mkdir("META-INF",0755);

	$filelist = "";
	opendir(DIR,".");
	while ($_ = readdir(DIR)) {
	    if (! ( ($_ eq '.') || ($_ eq '..'))) {
		if ( $jaropts =~ /i/) {
		    if (! /^include$/) {
			$filelist .= "$_ ";
		    }
		}
		else {
		    $filelist .= "$_ ";
		}
	    }
	}
	closedir(DIR);	

	print STDERR "zip $zipoptions -r $jarfile $filelist\n";
	system("zip $zipoptions -r $jarfile $filelist");
	rmdir("META-INF");
	    for $i (1 .. $dirdepth) {
	    chdir("..");
	    print STDERR "chdir ..\n";
	}
    }
    else {
        print STDERR "Directory $jardir doesn't exist\n";
    }

}

