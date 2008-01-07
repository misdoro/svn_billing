#!/usr/bin/perl

use Net::LDAP;
use DBI; use DBD::mysql

#LDAP options:
my $ldap_binddn='uid=billing,ou=admins,dc=openlan,dc=nnov,dc=ru';
my $ldap_searchbase='ou=users,dc=openlan,dc=nnov,dc=ru';
my $ldap_bindpass='vOVWpAk2g6';

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

#Create LDAP object and try to bind with my details:
$ldap = Net::LDAP->new ( 'server2' ) or die "$@";
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
$uid=~s/([^a-zA-Z])//g;


my $result = LDAPsearch ( $ldap, "uid=$uid", \@Attrs );

my $maylogin=1;
my $errormessage='Welcome to the internet!;)';
if ($result->entries){
	my $entry = ($result->entries)[0];
	print 'USER_NAME:'.$entry->get_value ('uid')."\n";
	print 'USER_PASSWORD:'.$entry->get_value ('userPassword')."\n";
}else{
	$maylogin=0;
	$errormessage='user not found in LDAP database!';
};

#Check if user may login, get his params:
my %result=('FRAMED_ROUTE','192.168.5.1');

#Connect to MYSQL
my $dbh = DBI->connect("DBI:mysql:$db_name:$db_host:$db_port", $db_user, $db_pass);

#Check if user has enough money on account:

my $query='select debit+credit from users where login=?';
my $sth = $dbh->prepare($query);
my $rv = $sth->execute($uid) or die "can't execute the query:". $sth->errstr;
my $money=($sth->fetchrow_array)[0];
if ($money<=0) {
	$maylogin=0;
	$errormessage='Not enough money on account, sorry :(';
};




print 'REPLY_MESSAGE:'.$errormessage."\n";
if ($maylogin){
	print 'RESULT:UNDEF'."\n";
}else{
	print 'RESULT:FAIL'."\n";
};

print "\n\n";

$ldap->unbind;
