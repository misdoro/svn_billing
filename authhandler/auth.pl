#!/usr/bin/perl

use lib "/usr/local/billing/authhandler";
use Net::LDAP;
use DBI; use DBD::mysql
IO::Socket;
require "funcs.pl";

#LDAP options:
my $ldap_server=$conf_hash{'ldap_server'};
my $ldap_binddn=$conf_hash{'ldap_binddn'};
my $ldap_searchbase=$conf_hash{'ldap_searchbase'};
my $ldap_bindpass=$conf_hash{'ldap_bindpass'};

#MySQL options:
my $db_name=$conf_hash{'mysql_database'};
my $db_host=$conf_hash{'mysql_host'};
my $db_port=$conf_hash{'mysql_port'};
my $db_user=$conf_hash{'mysql_username'};
my $db_pass=$conf_hash{'mysql_password'};


#Get calling arguments:
my %args=undef;
my %reply=undef;

while (defined($line = <STDIN>) && length($line)>2) {
	chomp $line;
	$key=(split(":", $line))[0];
	$args{"$key"}=substr($line, length($key)+1);
};

#Create LDAP object and try to bind with my details:
$ldap = Net::LDAP->new ( $ldap_server ) or die "$@";
$mesg = $ldap->bind ( $ldap_binddn, password => $ldap_bindpass, version => 3 );

sub LDAPsearch
 {
   my ($ldap,$searchString,$attrs,$base) = @_;

   # if they don't pass a base... set it for them

   if (!$base ) { $base = $ldap_searchbase; }

   # if they don't pass an array of attributes...
   # set up something for them

   if (!$attrs ) { $attrs = [ 'cn','mail' ]; }

   my $result = $ldap->search ( base    => "$base",
                                scope   => "one",
                                filter  => "$searchString",
                                attrs   =>  $attrs
                              );
  return $result;

  };

my @Attrs = ('uid','userPassword' );               # request username,userpassword

my $uid=$args{"USER_NAME"};
$uid=~s/([^a-zA-Z0-9_])//g;

my $result = LDAPsearch ( $ldap, "uid=$uid", \@Attrs );

$reply{'REPLY_MESSAGE'}='Welcome to the internet! ;)';
$reply{'RESULT'}='UNDEF';

if ($result->entries){
	my $entry = ($result->entries)[0];
	$reply{'USER_NAME'}=$entry->get_value('uid');
	$reply{'USER_PASSWORD'}=$entry->get_value ('userPassword');
}else{
	$reply{'RESULT'}='FAIL';
	$reply{'REPLY_MESSAGE'}='Sorry, user not found in LDAP database!';
};



#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

my ($money,$uin,$ipnum,$is_active)=undef;
if ($reply{'RESULT'} eq 'UNDEF') {
	#Check if user has enough money on account:
	my $query='select debit+credit, id,user_ip,active from users where login=? and active=1;';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute($uid) or die "can't execute the query:". $sth->errstr;
	($money,$uin,$ipnum,$is_active)=$sth->fetchrow_array;
	if ($money<0) {
		$reply{'RESULT'}='FAIL';
		$reply{'REPLY_MESSAGE'}='Not enough money on account, sorry :(';
	};
	if (!$uin){
		$reply{'RESULT'}='FAIL';
	        $reply{'REPLY_MESSAGE'}='Sorry, user not found in MYSQL database! :(';
	};
	if ($ipnum){
		$reply{'FRAMED_IP_ADDRESS'}=dectoip($ipnum);
	};
	if (($is_active eq '0') && ($reply{'RESULT'} eq 'UNDEF')){
		$reply{'RESULT'}='FAIL';
                $reply{'REPLY_MESSAGE'}='Sorry, user is disabled in MySQL database! :(';
	};
};

#Create session record in database (Inserting sessions only with found user_id, but possibly wrong password)
my $query='insert into sessions (user_id,user_name,session_id,nas_port,nas_linkname,called_ip,called_mac,called_ident,state) values(?,?,?,?,?,?,?,?,?)';
my $sth = $dbh->prepare($query);
my $sess_state=0;
$sess_state=1 if ($reply{'RESULT'} eq 'UNDEF');
$sth->execute($uin,$uid,$args{'ACCT_SESSION_ID'},$args{'NAS_PORT'},$args{'LINK'},iptodec($args{'PEER_ADDR'}),mactodec($args{'PEER_MAC_ADDR'}),$args{'PEER_IDENT'},$sess_state) or die "can't execute the query:". $sth->errstr;

#Reply for request
foreach my $key (keys %reply) {
        print $key.':'.$reply{$key}."\n" if $key;
};


print "\n\n";

$ldap->unbind;
$dbh->disconnect;
