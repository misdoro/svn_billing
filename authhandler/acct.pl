#!/usr/bin/perl

use lib "/usr/local/billing/authhandler";
use DBI; use DBD::mysql;
require "funcs.pl";

#MySQL options:
my $db_name='billing';
my $db_host='localhost';
my $db_port=3306;
my $db_user='billing';
my $db_pass='billadm';

#Get calling arguments:
my %args=undef;

while (defined($line = <STDIN>) && length($line)>2) {
	chomp $line;
	$key=(split(":", $line))[0];
	$args{"$key"}=substr($line, length($key)+1);
};

my $uid=$args{'USER_NAME'};
$uid=~s/([^a-zA-Z])//g;

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

#If connecting:
if ($args{'ACCT_STATUS_TYPE'} eq 'START') {
	#Update session with user's current peer IP (for netflow), set it's state as connected:
	my $query='update sessions set ppp_ip=?,state=2 where session_id=? and user_name=? and state=1';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute(iptodec($args{'FRAMED_IP_ADDRESS'}),$args{'ACCT_SESSION_ID'},$uid) or die "can't execute the query:". $sth->errstr;
}elsif ($args{'ACCT_STATUS_TYPE'} eq 'STOP'){
	#Update session state, end time:
        my $query='update sessions set state=3,sess_end=CURRENT_TIMESTAMP where session_id=? and user_name=? and state=2';
        my $sth = $dbh->prepare($query);
        my $rv = $sth->execute($args{'ACCT_SESSION_ID'},$uid) or die "can't execute the query:". $sth->errstr;
	
};



print "\n\n";
$dbh->disconnect;
