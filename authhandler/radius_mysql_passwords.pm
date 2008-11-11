use strict;
# use ...
# This is very important ! Without this script will not get the filled  hashesh from main.
use vars qw(%RAD_REQUEST %RAD_REPLY %RAD_CHECK);
use Data::Dumper;
use DBI; use DBD::mysql;
use IO::Socket;




# This is hash wich hold original request from radius
#my %RAD_REQUEST;
# In this hash you add values that will be returned to NAS.
#my %RAD_REPLY;
#This is for check items
#my %RAD_CHECK;

#
# This the remapping of return values
#
       use constant    RLM_MODULE_REJECT=>    0;#  /* immediately reject the request */
       use constant    RLM_MODULE_FAIL=>      1;#  /* module failed, don't reply */
       use constant    RLM_MODULE_OK=>        2;#  /* the module is OK, continue */
       use constant    RLM_MODULE_HANDLED=>   3;#  /* the module handled the request, so stop. */
       use constant    RLM_MODULE_INVALID=>   4;#  /* the module considers the request invalid. */
       use constant    RLM_MODULE_USERLOCK=>  5;#  /* reject the request (user is locked out) */
       use constant    RLM_MODULE_NOTFOUND=>  6;#  /* user not found */
       use constant    RLM_MODULE_NOOP=>      7;#  /* module succeeded without doing anything */
       use constant    RLM_MODULE_UPDATED=>   8;#  /* OK (pairs modified) */
       use constant    RLM_MODULE_NUMCODES=>  9;#  /* How many return codes there are */

# Function to handle authorize
sub authorize {
# For debugging purposes only
&log_request_attributes;

my (%conf_hash) =&readconf();
my $resp=0;

#MySQL options:
my $db_name=$conf_hash{'mysql_database'};
my $db_host=$conf_hash{'mysql_host'};
my $db_port=$conf_hash{'mysql_port'};
my $db_user=$conf_hash{'mysql_username'};
my $db_pass=$conf_hash{'mysql_password'};

my $uid=$RAD_REQUEST{'User-Name'};
$uid=~s/([^a-zA-Z0-9_])//g;

$RAD_REPLY{'Reply-Message'}='Welcome to the internet! ;)';

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

my ($money,$uin,$password,$ipnum,$is_active,$parent,$parent_money)=undef;
#Check if user has enough money on account:
my $query='select a.debit+a.credit, a.id, a.password, a.user_ip, a.active, a.parent, b.credit+ b.debit from users as a left join users as b on a.parent = b.id where a.login=? limit 1';
my $sth = $dbh->prepare($query);
my $rv = $sth->execute($uid) or die "can't execute the query:". $sth->errstr;
($money,$uin,$password,$ipnum,$is_active,$parent,$parent_money)=$sth->fetchrow_array;
if ($uin && $password)
{	
	if (($money < 0 && !$parent) || ($parent && $parent_money < 0)) {
		$resp=1;
		$RAD_REPLY{'Reply-Message'}='Not enough money on account, sorry :(';
	};
	if ($ipnum){
		$RAD_REPLY{'Framed-IP-Address'}=dectoip($ipnum);
	};
	if (($is_active eq '0') && ($resp == 1)){
		$resp=1;
		$RAD_REPLY{'Reply-Message'}='Sorry, user is disabled in MySQL database! :(';
	};
	$RAD_REPLY{'User-Name'}=$uid;
	$RAD_REPLY{'User-Password'}=$password;
	$RAD_CHECK{'Cleartext-Password'} = $password;
	$RAD_CHECK{'User-Password'}=$password;

}else{
	$resp=1;
        $RAD_REPLY{'Reply-Message'}='Sorry, user not found in MYSQL database! :(';
};

#Create session record in database (Inserting sessions only with found user_id, but possibly wrong password)
my $query='insert into sessions (user_id,user_name,session_id,nas_port,nas_linkname,called_ip,called_mac,called_ident,state) values(?,?,?,?,?,?,?,?,?)';
my $sth = $dbh->prepare($query);
my $sess_state=0;
$sess_state=1 if ($resp == 0);
#$sth->execute($uin,$uid,$RAD_REQUEST{'Acct-Session-Id'},$RAD_REQUEST{'Nas-Port'},$RAD_REQUEST{'Link'},iptodec($RAD_REQUEST{'Peer-Addr'}),mactodec($RAD_REQUEST{'Peer-Mac-Addr'}),$RAD_REQUEST{'Peer-Ident'},$sess_state) or die "can't execute the query:". $sth->errstr;
$sth->execute($uin,$uid,$RAD_REQUEST{'Acct-Session-Id'},$RAD_REQUEST{'NAS-Port'},'pptp_link-'.$RAD_REQUEST{'NAS-Port'},iptodec($RAD_REQUEST{'Calling-Station-Id'}),0,$RAD_REQUEST{'Calling-Station-Id'},$sess_state) or die "can't execute the query:". $sth->errstr;

$dbh->disconnect;
if ($resp == 1) {
	return RLM_MODULE_REJECT;
};

return RLM_MODULE_OK;
}

# Function to handle authenticate
sub authenticate {
# For debugging purposes only
&log_request_attributes;

return RLM_MODULE_OK;

}

# Function to handle preacct
sub preacct {
# For debugging purposes only
       &log_request_attributes;

       return RLM_MODULE_OK;
       #return &accounting;
}

# Function to handle accounting
sub accounting {
# For debugging purposes only
       &log_request_attributes;

# You can call another subroutine from here
#       &test_call;
my $event_host="127.0.0.1";
my $event_port="10203";

my (%conf_hash) =readconf();

#MySQL options:
my $db_name=$conf_hash{'mysql_database'};
my $db_host=$conf_hash{'mysql_host'};
my $db_port=$conf_hash{'mysql_port'};
my $db_user=$conf_hash{'mysql_username'};
my $db_pass=$conf_hash{'mysql_password'};

my $uid=$RAD_REQUEST{'User-Name'};
$uid=~s/([^a-zA-Z0-9_])//g;

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

#If connecting:
if ($RAD_REQUEST{'Acct-Status-Type'} eq 'Start') 
{
	#Update session with user's current peer IP (for netflow), set it's state as connected:
	my $query='update sessions set ppp_ip=?,state=2 where session_id=? and user_name=? and state=1';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute(iptodec($RAD_REQUEST{'Framed-IP-Address'}),$RAD_REQUEST{'Acct-Session-Id'},$uid) or die "can't execute the query:". $sth->errstr;
#	&radiusd::radlog(1, "update sessions set ppp_ip='".iptodec($RAD_REQUEST{'Framed-Ip-Address'}.',state=2 where session_id='.$RAD_REQUEST{'Acct-Session-Id'}.' and user_name='.$uid );
	#Tell billd that user is connected:
	my $socket = IO::Socket::INET->new(PeerAddr => $event_host, 
		PeerPort => $event_port, 
		Proto => "tcp",
		Type => SOCK_STREAM);
	if ($socket){
	        print $socket "connect\n";
#		&radiusd::radlog(1, "connect");
		print $socket $RAD_REQUEST{'User-Name'}."\n";
#		&radiusd::radlog(1,$RAD_REQUEST{'User-Name'});
		print $socket $RAD_REQUEST{'Acct-Session-Id'}."\n";
#		&radiusd::radlog(1,$RAD_REQUEST{'Acct-Session-Id'});
		print $socket $RAD_REQUEST{'Framed-IP-Address'}."\n";
#		&radiusd::radlog(1,$RAD_REQUEST{'Framed-IP-Address'});
		print $socket 'pptp_link-'.$RAD_REQUEST{'NAS-Port'}."\n";
#		&radiusd::radlog(1,'pptp_link-'.$RAD_REQUEST{'NAS-Port'});
		print $socket "\n";
		close($socket);
	};
}
elsif ($RAD_REQUEST{'Acct-Status-Type'} eq 'Stop')
{
	#Update session state, end time:
        my $query='update sessions set state=3,sess_end=CURRENT_TIMESTAMP where session_id=? and user_name=? and state=2';
        my $sth = $dbh->prepare($query);
        my $rv = $sth->execute($RAD_REQUEST{'Acct-Session-Id'},$uid) or die "can't execute the query:". $sth->errstr;

	#Tell billd that user is disconnected:
	my $socket = IO::Socket::INET->new(PeerAddr => $event_host,
                PeerPort => $event_port,
                Proto => "tcp",
                Type => SOCK_STREAM);
	if ($socket){
		print $socket "disconnect\n";
		print $socket $RAD_REQUEST{'User-Name'}."\n";
		print $socket $RAD_REQUEST{'Acct-Session-Id'}."\n";
		print $socket $RAD_REQUEST{'Framed-IP-Address'}."\n";
		print $socket 'pptp_link-'.$RAD_REQUEST{'NAS-Port'}."\n";
		print $socket "\n";
		close($socket);
	};
};
$dbh->disconnect;
       return RLM_MODULE_OK;
}

# Function to handle checksimul
sub checksimul {
# For debugging purposes only
#       &log_request_attributes;

       return RLM_MODULE_OK;
}

# Function to handle pre_proxy
sub pre_proxy {
# For debugging purposes only
#       &log_request_attributes;

       return RLM_MODULE_OK;
}

# Function to handle post_proxy
sub post_proxy {
# For debugging purposes only
#       &log_request_attributes;

       return RLM_MODULE_OK;
}

# Function to handle post_auth
sub post_auth {
# For debugging purposes only
#       &log_request_attributes;

       return RLM_MODULE_OK;
}

# Function to handle xlat
sub xlat {
# For debugging purposes only
#       &log_request_attributes;

# Loads some external perl and evaluate it
       my ($filename,$a,$b,$c,$d) = @_;
       &radiusd::radlog(1, "From xlat $filename ");
       &radiusd::radlog(1,"From xlat $a $b $c $d ");
       local *FH;
       open FH, $filename or die "open '$filename' $!";
       local($/) = undef;
       my $sub = <FH>;
       close FH;
       my $eval = qq{ sub handler{ $sub;} };
       eval $eval;
       eval {main->handler;};
}

# Function to handle detach
sub detach {
# For debugging purposes only
#       &log_request_attributes;

# Do some logging.
       &radiusd::radlog(0,"rlm_perl::Detaching. Reloading. Done.");
} 

#
# Some functions that can be called from other functions
#


#
#Read config variables into a hash:
#
sub readconf {
	my $config = '/etc/billd.conf';
	my %conf_hash = undef;
	open(CONF,'<',$config);
	while (my $line=<CONF>){
		chomp($line);
		#Delete comments:
		$line=~s/#[\S\s]*//;
		my $key=(split('=',$line))[0];
		my $value=substr($line,(length($key)+1));
		$key=~ s/^\s+//;
		$key=~ s/\s+$//;
		$value=~ s/^\s+//;
		$value=~ s/\s+$//;
		$conf_hash{$key}=$value if length($key)>0;
	};
	close CONF;
	return %conf_hash;
}

#Convert IP to 4-byte number
sub iptodec{
	my $ip=@_[0];
	my @octets = split(/\./, $ip);
	return ( $octets[0]*1 << 24 )+($octets[1]*1 << 16)+($octets[2]*1 << 8)+($octets[3]);
}

#Convert IP to 4-byte number
sub dectoip{
        my $num=@_[0];
	my	$d = $num % 256; $num -= $d; $num /= 256;
	my $c = $num % 256; $num -= $c; $num /= 256;
	my $b = $num % 256; $num -= $b; $num /= 256;
	my $a = $num;
	return "$a.$b.$c.$d";
}

#Convert MAC address to 6-byte number:
sub mactodec{
	my $mac=@_[0];
	my @octets = split(/:/, $mac);
	my $mac_num = 0;
	
	foreach my $octet (@octets)
{	
		$octet=hex($octet);
		$mac_num <<= 8;
		$mac_num |= $octet;
}
	return $mac_num;
}

sub LDAPsearch
{
   my ($ldap,$searchString,$attrs,$base) = @_;

# if they don't pass a base... set it for them

#   if (!$base ) { $base = $ldap_searchbase; }

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

sub test_call {
# Some code goes here
}

sub log_request_attributes {
# This shouldn't be done in production environments!
# This is only meant for debugging!
       for (keys %RAD_REQUEST) {
               &radiusd::radlog(1, "RAD_REQUEST: $_ = $RAD_REQUEST{$_}");
}
}
 
