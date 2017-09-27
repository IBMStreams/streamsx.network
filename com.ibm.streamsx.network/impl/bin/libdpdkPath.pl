#!/usr/bin/perl

## Copyright (C) 2011, 2017  International Business Machines Corporation
## All Rights Reserved

use strict;
use English;
use feature "switch";

my $env = "RTE_SDK";

sub help() {

    print "

This script is called by the Streams compiler (that is, the 'sc' command) when
it compiles a PacketDPDKSource operator.  The compiler passes one of these
arguments to the script:

  includePath -- The compiler requests the directory path for the 'pam.h' file.

  libPath -- The compiler requests the directory path for the 'iss-pam1.so' file.

  lib -- The compiler requests the name of the library, which is 'iss-pam'.

The script responds by writing either the requested directory or an empty line
to STDOUT, depending upon the presence or absence of this environment variable:

  * If the environment variable $env is defined, and the directory it points to
    exists, and the directory contains the requested files, then that directory
    is written to STDOUT.

";
}

#-------------------------------------------------------------------------------

# display help text, if requested

help(), exit 1 if scalar(@ARGV)!=1 || $ARGV[0] =~ m/^(\?|-h|-help|--help)$/;

# check for required directory and files

my $directory = $ENV{$env} . "/$ENV{RTE_TARGET}/lib";
my $error = "";
if ( ! exists $ENV{$env} ) { $error = "sorry, enviroment variable '$env' not found by PacketDPDKSource operator\n"; }
if ( ! $error && ! -d $directory ) { $error = "sorry, enviroment variable '$env' set, but directory '$directory' not found by PacketDPDKSource operator\n"; }
if ( ! $error && ! -f "$directory/libdpdk.a" ) { $error = "sorry, enviroment variable '$env' set, but file '$directory/libdpdk.a' not found by PacketDPDKSource operator\n"; }

# write the directory to STDOUT, and the error message to STDERR if there is one

my ($request) = $ARGV[0];
for ($request) {
    when (/^includePath/) { print STDERR $error if $error; print STDOUT "\n"; }
    when (/^libPath/) { print STDOUT "$directory\n"; }
    when (/^lib/) { print STDOUT "dpdk\n"; }
    default { print STDERR "sorry, unrecognized request '$request'\n"; exit 1; }
}

exit 0;
