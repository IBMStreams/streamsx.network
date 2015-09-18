#!/usr/bin/perl

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

use strict;
use English;
use feature "switch";

sub help() {

    print " 

This script is called by the Streams compiler (that is, the 'sc' command) when
it compiles a PacketLiveSource or PacketFileSource operator.  The compiler
passes one of these arguments to the script:

  includePath -- The compiler requests a directory path for the 'pcap.h' file.

  libPath -- The compiler requests the directory path for the 'libpcap.so' file.

  lib -- The compiler requests the name of the library, which is 'pcap'.

The script responds by writing either the requested path or an empty line to
STDOUT, depending upon the presence or absence of these environment variables:

  STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH -- If this environment variable is
    present, the script will print its value to STDOUT and the operator will be
    compiled with the version of 'pcap.h' in that directory.  Otherwise, the
    script will print an empty line to STDOUT and the operator will be compiled
    with the system version of 'pcap.h', most likely in either
    /usr/local/include or /usr/include.

  STREAMS_ADAPTERS_LIBPCAP_LIBPATH -- If this environment variable is present,
    the script will print its value to STDOUT and the operator will be linked
    with the version of 'libpcap.so' in that directory.  Otherwise, the script
    will print an empty line to STDOUT and the operator will be compiled with
    the system version of 'libpcap.so', most likely in either /usr/local/lib or
    /usr/lib64.  "; }

#-------------------------------------------------------------------------------

# display help text, if requested

help(), exit 1 if scalar(@ARGV)!=1 || $ARGV[0] =~ m/^(\?|-h|-help|--help)$/;

# set local variables

my ($request) = $ARGV[0];
my $includePath = $ENV{'STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH'};
my $libPath = $ENV{'STREAMS_ADAPTERS_LIBPCAP_LIBPATH'};

# handle the request

for ($request) {
    when (/^includePath/) { print STDOUT ($includePath ? $includePath : "") . "\n"; }
    when (/^libPath/) { print STDOUT ($libPath ? $libPath : "") . "\n"; }
    when (/^lib/) { print STDOUT "pcap\n"; }
    default { print STDERR "sorry, unrecognized request '$request'\n"; exit 1; } 
}

exit 0;


