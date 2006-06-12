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

sub recursive_copy {
    local($fromdir);
    local($todir);
    local(@dirlist);
    $fromdir = shift;
    $todir = shift;
  
    print STDERR "recursive copy called with $fromdir, $todir\n";

#remove any trailing slashes.
    $fromdir =~ s/\/$//;
    $todir =~ s/\/$//;
    
    opendir(DIR, $fromdir);
    @dirlist = readdir DIR;
    close DIR;


    foreach $file (@dirlist) {
	if  (! (($file eq "." ) || ($file eq "..") )) {
	    
	    if (-d "$fromdir/$file") {
		print STDERR "handling directory $todir/$file\n";
		&rec_mkdir("$todir/$file");
		&recursive_copy("$fromdir/$file","$todir/$file");
	    }
	    else {
		print STDERR "handling file $fromdir/$file\n";
		&my_copy("$fromdir/$file","$todir/$file");
	    }
	}
    }
}

sub parse_argv {

#    print STDERR "Parsing Variables\n";

    foreach $q ( @ARGV ) {
	if (! ($q =~ /=/)) {
	    $var{$lastassigned} .= " $q";
	}
	else {
	   $q =~ /^([^=]*)=(.*)/;
	   $left = $1;
	   $right = $2;
	
	   $right =~ s/ *$//;
	   $var{$left} = $right;

	   $lastassigned = $left;
	
	   }
	print STDERR "Assigned $lastassigned = $var{$lastassigned}\n";
    }
}


# usage: &my_copy("dir/fromfile","dir2/tofile");
# do a 'copy' - files only, 'to' MUST be a filename, not a directory.

# fix this to be able to use copy on win nt.

sub my_copy {
    local($from);
    local($to);
    local($cpcmd);

    $from = shift;
    $to = shift;

    if ( ! defined $var{OS_ARCH}) {
	die "OS_ARCH not defined!";
    }
    else {
	if ($var{OS_ARCH} eq 'WINNT') {
	    $cpcmd = 'cp';
	    	}
	else {
	    $cpcmd = 'cp';
	    }
	print STDERR "COPYING: $cpcmd $from $to\n";
	system("$cpcmd $from $to");
    }
}


sub old_my_copy {
    local($from);
    local($to);

    $from = shift;
    $to = shift;
    open(FIN, "<$from") || die("Can't read from file $from\n");
    if ( ! open(FOUT,">$to")) {
	close FIN;
	die "Can't write to file $to\n";
    }
    while (read(FIN, $buf, 100000)) {
	print FOUT $buf;
    }
    close (FIN);
    close (FOUT);
}

sub rec_mkdir {
    local($arg);
    local($t);
    local($q);

    $arg = shift;
    $t = "";
    foreach $q (split(/\//,$arg)) {
	$t .= $q;
	if (! ($t =~ /\.\.$/)) {
	    if ($t =~ /./) {
		mkdir($t,0775);
	    }
	}
	$t.= '/';
    }
}

1;
