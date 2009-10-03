use strict;
# use ...
# This is very important ! Without this script will not get the filled  hashesh from main.
use vars qw(%RAD_REQUEST %RAD_REPLY %RAD_CHECK);
use Data::Dumper;
use Net::LDAP;
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

#LDAP options:

my $ldap_server = $conf_hash{'ldap_server'};
my $ldap_binddn = $conf_hash{'ldap_binddn'};
my $ldap_searchbase = $conf_hash{'ldap_searchbase'};
my $ldap_bindpass = $conf_hash{'ldap_bindpass'};

#MySQL options:
my $db_name=$conf_hash{'mysql_database'};
my $db_host=$conf_hash{'mysql_host'};
my $db_port=$conf_hash{'mysql_port'};
my $db_user=$conf_hash{'mysql_username'};
my $db_pass=$conf_hash{'mysql_password'};

#Create LDAP object and try to bind with my details:
my $ldap = Net::LDAP->new ( $ldap_server );
if (!$ldap){
	&radiusd::radlog(1, $@);
	return RLM_MODULE_REJECT;
};
my $mesg = $ldap->bind ( $ldap_binddn, password => $ldap_bindpass, version => 3 );

my @Attrs = ('uid','userPassword' );               # request username,userpassword

my $uid=$RAD_REQUEST{'User-Name'};
$uid=~s/([^a-zA-Z0-9_])//g;

my $result = LDAPsearch ( $ldap, "uid=$uid", \@Attrs, $ldap_searchbase );

$RAD_REPLY{'Reply-Message'}='Welcome to the internet! ;)';
#$RAD_REPLY{'Result'}='UNDEF';

if ($result->entries){
	my $entry = ($result->entries)[0];
	$RAD_REPLY{'User-Name'}=$entry->get_value('uid');
	$RAD_REPLY{'User-Password'}=$entry->get_value ('userPassword');
	$RAD_CHECK{'Cleartext-Password'} = $entry->get_value ('userPassword');
	$RAD_CHECK{'User-Password'}=$entry->get_value ('userPassword');
}else{
	$resp=1;
	$RAD_REPLY{'Reply-Message'}='Sorry, user not found in LDAP database!';
};

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

my ($money,$uin,$ipnum,$is_active,$parent,$parent_money)=undef;
if ($resp == 0) {
#Check if user has enough money on account:
my $query='select a.debit+a.credit, a.id, a.user_ip, a.active, a.parent, b.credit+ b.debit from users as a left join users as b on a.parent = b.id where a.login=? limit 1';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute($uid);
	if ($rv==undef){
		&radiusd::radlog(1,"can't execute the query $query:". $sth->errstr);
		$ldap->unbind;
		$dbh->disconnect;
		return RLM_MODULE_REJECT;
	};
	($money,$uin,$ipnum,$is_active,$parent,$parent_money)=$sth->fetchrow_array;
	$sth->finish;
	if (($money < 0 && !$parent) || ($parent && $parent_money < 0)) {
		$resp=1;
		$RAD_REPLY{'Reply-Message'}='Not enough money on account, sorry :(';
	};
	if (!$uin){
		$resp=1;
		$RAD_REPLY{'Reply-Message'}='Sorry, user not found in MYSQL database! :(';
	};
	if ($ipnum){
		$RAD_REPLY{'Framed-IP-Address'}=dectoip($ipnum);
	};
	if (($is_active eq '0') && ($resp == 1)){
		$resp=1;
		$RAD_REPLY{'Reply-Message'}='Sorry, user is disabled in MySQL database! :(';
	};
};

#Create session record in database (Inserting sessions only with found user_id, but possibly wrong password)
my $query='insert into sessions (user_id,user_name,session_id,nas_port,nas_linkname,called_ip,called_mac,called_ident,state) values(?,?,?,?,?,?,?,?,?)';
my $sth = $dbh->prepare($query);
my $sess_state=0;
$sess_state=1 if ($resp == 0);
my $rv=$sth->execute($uin,$uid,$RAD_REQUEST{'Acct-Session-Id'},$RAD_REQUEST{'NAS-Port'},'pptp_link-'.$RAD_REQUEST{'NAS-Port'},iptodec($RAD_REQUEST{'Calling-Station-Id'}),0,$RAD_REQUEST{'Calling-Station-Id'},$sess_state);
#if ($rv==undef){
#	&radiusd::radlog(1,"can't execute the query $query:'. $sth->errstr);
#	$ldap->unbind;
#	$dbh->disconnect;
#	return RLM_MODULE_REJECT;
#};

if ($resp == 1) {
	$ldap->unbind;
	$dbh->disconnect;
	return RLM_MODULE_REJECT;
};

###############
# Traffic shaping:
###############
#Query for users price

if ($uin){
	my @mpdflt;
	my @mpdlim;
	my $query='SELECT inet_ntoa(a.`ip`), a.`mask`, a.`dstport`, p.`group_id` FROM allzones a, zone_groups g, price_groups p, users u where u.active_price=p.price_id and g.group_id=p.group_id and u.id=? and g.zone_id=a.id order by g.priority desc';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute($uin);
#	if ($rv==undef) {
#		&radiusd::radlog(1,'can\'t execute the query '.$query.':'. $sth->errstr);
#		$ldap->unbind;
#		$dbh->disconnect;
#		return RLM_MODULE_REJECT;
#	};
	my %fltidx;
	my %fltno=undef;
	my $fltnocnt=0;
	my $str;
	while(my @row = $sth -> fetchrow_array) {
		#user->server
		if (@row[1] || @row[2]){
			if (!$fltno{@row[3].'u'}){
				$fltno{@row[3].'u'}=++$fltnocnt;
			};
			my $filter=$fltno{@row[3].'u'};
			$str=$filter.'#'.(++$fltidx{$filter}).'=match ';
			if (@row[1]){
				$str.='dst net '.@row[0].'/'.@row[1];
			};
			if (@row[2]){
				$str.=' and ' if @row[1];
				$str.='dst port '.@row[2];
			};
			push(@mpdflt,$str);
		};
		#server->user
		if (@row[1] || @row[2]){
			if (!$fltno{@row[3].'d'}){
				$fltno{@row[3].'d'}=++$fltnocnt;
			};
			my $filter=$fltno{@row[3].'d'};
			$str=$filter.'#'.(++$fltidx{$filter}).'=match ';		
			if (@row[1]){
				$str.='src net '.@row[0].'/'.@row[1];
			};
			if (@row[2]) {
				$str.=' and ' if @row[1];
				$str.='src port '.@row[2];
			};
			push(@mpdflt,$str);
		};
	};
	$sth->finish;

	my $query='SELECT p.bw_in, p.bw_out, p.group_id, p.priority FROM price_groups p, users u where u.active_price=p.price_id and u.id=? order by p.priority desc';
	my $sth = $dbh->prepare($query);
	my $rv = $sth->execute($uin);

	my $in_idx=0;
	my $out_idx=0;
	my $limit;
	my $str;
	while(my @row = $sth -> fetchrow_array) {
		# user->server
		if (@row[1]) {
			$limit=' shape '.(@row[1]*1024).' 4000 ';
		} else {
			$limit='';
		};
		if ($fltno{@row[2].'u'}){
			push(@mpdlim,'in#'.++$in_idx.'=flt'.$fltno{@row[2].'u'}.$limit.' pass');
		}else {
			push(@mpdlim,'in#'.++$in_idx.'=all '.$limit.' pass');
		};
		# server->user
		if (@row[0]) {
			$limit=' shape '.(@row[0]*1024).' 4000 ';
		}else{
			$limit='';
		};
		if ($fltno{@row[2].'d'}){
			push(@mpdlim,'out#'.++$out_idx.'=flt'.$fltno{@row[2].'d'}.$limit.' pass');
		}else{
			push(@mpdlim,'out#'.++$out_idx.'=all '.$limit.' pass');
		};
	};
	$sth->finish;
	#Last limit is deny
#	push(@mpdlim,'in#'.++$in_idx.'=all deny');
#	push(@mpdlim,'out#'.++$out_idx.'=all deny');

	$RAD_REPLY{'mpd-limit'}=\@mpdlim;
	$RAD_REPLY{'mpd-filter'}=\@mpdflt;

};
$ldap->unbind;
$dbh->disconnect;
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
 
