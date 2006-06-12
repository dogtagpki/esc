#!/usr/sbin/perl
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

# Compose lowercase alphabet
@alphabet = ( "a", "b", "c", "d", "e", "f", "g", "h",
              "i", "j", "k", "l", "m", "n", "o", "p",
              "q", "r", "s", "t", "u", "v", "w", "x",
              "y", "z" );

# Compute year
$year = (localtime)[5] + 1900;

# Compute month
$month = (localtime)[4] + 1;

# Compute day
$day = (localtime)[3];

# Compute base build number
$version = sprintf( "%d%02d%02d", $year, $month, $day );
$directory = sprintf( "%s\/%s\/%d%02d%02d", $ARGV[0], $ARGV[1], $year, $month, $day );

# Print out the name of the first version directory which does not exist
#if( ! -e  $directory )
#{
    print $version;
#}
#else
#{
#    # Loop through combinations
#    foreach $ch1 (@alphabet)
#    {
#	foreach $ch2 (@alphabet)
#	{
#	    $version = sprintf( "%d%02d%02d%s%s", $year, $month, $day, $ch1, $ch2 );
#	    $directory = sprintf( "%s\/%s\/%d%02d%02d%s%s", $ARGV[0], $ARGV[1], $year, $month, $day, $ch1, $ch2 );
#	    if( ! -e  $directory )
#	    {
#		print STDOUT $version;
#		exit;
#	    }
#	}
#    }
#}

