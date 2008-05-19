
#
#Read config variables into a hash:
#
$config = '/etc/billd.conf' if length($config)==0;
$conf_hash = undef;

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


#Convert IP to 4-byte number
sub iptodec{
	my $ip=@_[0];
	my @octets = split(/\./, $ip);
	return ($octets[0]*1<<24)+($octets[1]*1<<16)+($octets[2]*1<<8)+($octets[3]);
}

#Convert IP to 4-byte number
sub dectoip{
        my $num=@_[0];
	$d = $num % 256; $num -= $d; $num /= 256;
	$c = $num % 256; $num -= $c; $num /= 256;
	$b = $num % 256; $num -= $b; $num /= 256;
	$a = $num;
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

1;
