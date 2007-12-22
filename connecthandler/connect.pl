#!/usr/bin/perl
use IO::Socket;

$remote_host='127.0.0.1';
$remote_port='10203';

if (true){
	$socket = IO::Socket::INET->new(PeerAddr => $remote_host, PeerPort => $remote_port, Proto => "tcp",Type => SOCK_STREAM) or die ("cant create socket");
	print $socket "connect\n";
	foreach $arg(@ARGV) {
		print $socket "$arg\n";
	};

	print $socket "\n";

	close($socket);
}else{
	print "Call only from MPD";
}
