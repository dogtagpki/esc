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

#Input: [-d dir] foo1.java foo2.java
#Compares with: foo1.class foo2.class (if -d specified, checks in 'dir', 
#  otherwise assumes .class files in same directory as .java files)
#Returns: list of input arguments which are newer than corresponding class
#files (non-existant class files are considered to be real old :-)

$found = 1;

if ($ARGV[0] eq '-d') {
    $classdir = $ARGV[1];
    $classdir .= "/";
    shift;
    shift;
} else {
    $classdir = "./";
}

foreach $filename (@ARGV) {
    $classfilename = $classdir;
    $classfilename .= $filename;
    $classfilename =~ s/.java$/.class/;
    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,
     $ctime,$blksize,$blocks) = stat($filename);
    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$classmtime,
     $ctime,$blksize,$blocks) = stat($classfilename);
#    print $filename, " ", $mtime, ", ", $classfilename, " ", $classmtime, "\n";
    if ($mtime > $classmtime) {
        print $filename, " ";
        $found = 0;
    }
}

print "\n";
