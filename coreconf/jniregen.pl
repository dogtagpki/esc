#!/usr/local/bin/perl
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

# Input: -d dir -j javahcmd foo1 foo2 . . .
#        Compares generated "_jni/foo1.h" file with "foo1.class", and
#        generated "_jni/foo2.h" file with "foo2.class", etc.
#        (NOTE:  unlike its closely related cousin, outofdate.pl,
#                the "-d dir" must always be specified)
#        Runs the javahcmd on all files that are different.
#
# Returns: list of headers which are OLDER than corresponding class
#          files (non-existant class files are considered to be real old :-)

my $javah = "";
my $classdir = "";

while(1) {
    if ($ARGV[0] eq '-d') {
        $classdir = $ARGV[1];
        $classdir .= "/";
        shift;
        shift;
    } elsif($ARGV[0] eq '-j') {
        $javah = $ARGV[1];
        shift;
        shift;
    } else {
        last;
    }
}

if( $javah  eq "") {
    die "Must specify -j <javah command>";
}
if( $classdir eq "") {
    die "Must specify -d <classdir>";
}

foreach $filename (@ARGV)
{
    $headerfilename = "_jni/";
    $headerfilename .= $filename;
    $headerfilename =~ s/\./_/g;
    $headerfilename .= ".h";

    $classfilename = $filename;
    $classfilename =~ s|\.|/|g;
    $classfilename .= ".class";

    $classfilename = $classdir . $classfilename;


    ( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $headermtime,
      $ctime, $blksize, $blocks ) = stat( $headerfilename );

    ( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $classmtime,
      $ctime, $blksize, $blocks ) = stat( $classfilename );

    if( $headermtime < $classmtime )
    {
	# NOTE:  Since this is used by "javah", and "javah" refuses to overwrite
	#        an existing file, we force an unlink from this script, since
	#        we actually want to regenerate the header file at this time.
        unlink $headerfilename;
        push @filelist, $filename;
    }
}

if( @filelist ) {
    $cmd = "$javah " . join(" ",@filelist);
    $cmd =~ s/\'/\"/g;  # because windows doesn't understand single quote
    print "$cmd\n";
    exit (system($cmd) >> 8);
} else {
    print "All JNI header files up to date.\n"
}
