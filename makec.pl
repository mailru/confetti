$len = 0;
while(<>) {
	chomp $_;
	$_=~s/(["\\])/\\$1/g;

	$txt = "\t\"$_\\n\"\n";
	print $txt;

	$len += length($txt);
	if ($len > 1000) {
		print "\t,\n";
		$len=0;
	}
}
print  "\t,\n" if $len != 0;
