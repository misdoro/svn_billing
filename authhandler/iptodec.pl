#!/usr/bin/perl

use lib "/usr/local/billing/authhandler";
require "funcs.pl";
while (defined($line = <STDIN>) && length($line)>2) {
        chomp $line;
	print iptodec($line)."\n";
};
