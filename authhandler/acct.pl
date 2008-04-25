#!/usr/bin/perl

use lib "/usr/local/billing/authhandler";
use DBI; use DBD::mysql;
use IO::Socket;
require "funcs.pl";

#Connect/disconnect event port:
my $event_host=$conf_hash{'event_host'};
my $event_port=$conf_hash{'event_port'};

#MySQL options:
my $db_name=$conf_hash{'mysql_database'};
my $db_host=$conf_hash{'mysql_host'};
my $db_port=$conf_hash{'mysql_port'};
my $db_user=$conf_hash{'mysql_username'};
my $db_pass=$conf_hash{'mysql_password'};

#Get calling arguments:
my %args=undef;

while (defined($line = <STDIN>) && length($line)>2) {
	chomp $line;
	$key=(split(":", $line))[0];
	$args{"$key"}=substr($line, length($key)+1);
};

my $uid=$args{'USER_NAME'};
$uid=~s/([^a-zA-Z0-9_])//g;

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

#If connecting:
if ($args{'ACCT_STATUS_TYPE'} eq 'START') 
{
	#Update session with user's current peer IP (for netflow), set it's state as connected:
	my $query='update sessions set ppp_ip=?,state=2 where session_id=? and user_name=? and state=1';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute(iptodec($args{'FRAMED_IP_ADDRESS'}),$args{'ACCT_SESSION_ID'},$uid) or die "can't execute the query:". $sth->errstr;
	
	#Tell billd that user is connected:
	$socket = IO::Socket::INET->new(PeerAddr => $event_host, 
		PeerPort => $event_port, 
		Proto => "tcp",
		Type => SOCK_STREAM);
	if ($socket){
	        print $socket "connect\n";
		print $socket $args{'USER_NAME'}."\n";
		print $socket $args{'ACCT_SESSION_ID'}."\n";
		print $socket $args{'FRAMED_IP_ADDRESS'}."\n";
		print $socket $args{'LINK'}."\n";
		print $socket "\n";
		close($socket);
	};
}
elsif ($args{'ACCT_STATUS_TYPE'} eq 'STOP')
{
	#Update session state, end time:
        my $query='update sessions set state=3,sess_end=CURRENT_TIMESTAMP where session_id=? and user_name=? and state=2';
        my $sth = $dbh->prepare($query);
        my $rv = $sth->execute($args{'ACCT_SESSION_ID'},$uid) or die "can't execute the query:". $sth->errstr;

	#Tell billd that user is disconnected:
	$socket = IO::Socket::INET->new(PeerAddr => $event_host,
                PeerPort => $event_port,
                Proto => "tcp",
                Type => SOCK_STREAM);
	if ($socket){
		print $socket "disconnect\n";
		print $socket $args{'USER_NAME'}."\n";
		print $socket $args{'ACCT_SESSION_ID'}."\n";
		print $socket $args{'FRAMED_IP_ADDRESS'}."\n";
		print $socket $args{'LINK'}."\n";
		print $socket "\n";
		close($socket);
	};
};



print "\n\n";
$dbh->disconnect;
