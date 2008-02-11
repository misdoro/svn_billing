sub iptodec{
	my $ip=@_[0];
	my @octets = split(/\./, $ip);
	return ($octets[0]*1<<24)+($octets[1]*1<<16)+($octets[2]*1<<8)+($octets[3]);
}
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
