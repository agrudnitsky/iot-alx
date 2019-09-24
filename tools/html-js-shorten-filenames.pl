#!/usr/bin/perl

use warnings;
use File::Find;
use strict;

die unless $#ARGV == 0;
my $dir = $ARGV[0];
my %replacements = ('chunk-vendors.' => 'c-v.');
my %sched_replacements = ();

sub get_candidates {
	my $file = $_;
	my ($mrk, $mrknew) = (undef, undef);
	my $fn;

	return unless -f $file;
	for my $rk (sort keys %replacements) {
		$file =~ /\/([^\/]+)$/;
		$fn = $1;
		if (substr($fn, 0, length($rk)) eq $rk) {
			$mrk = $rk;
			$mrknew = $file;
			$mrknew =~ s/\Q$rk\E/$replacements{$rk}/g;
			last;
		}
	}
	if (defined($mrk)) {
		$sched_replacements{$file} = $mrknew;
	}
}

find({wanted => \&get_candidates, no_chdir => 1}, $dir);

my $rep_cmd;
my ($old, $new);
for my $srk (sort keys %sched_replacements) {
	print $srk . " -> " . $sched_replacements{$srk} . "\n";
	rename $srk, $sched_replacements{$srk};
	$srk =~ /\/([^\/]+)$/;
	$old = $1;
	$sched_replacements{$srk} =~ /\/([^\/]+)$/;
	$new = $1;
	$rep_cmd = "find $dir -type f | xargs -n1 perl -i -p -e 's/$old/$new/g;'";
	`$rep_cmd`;
#	print $rep_cmd . "\n";
}


exit 1;

