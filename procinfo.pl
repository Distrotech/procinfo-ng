#!/usr/bin/perl

##############################################################################
#                                                                            #
# AUTHOR: Adam Schrotenboer <tabris@tabris.net>                              #
# Copyright ( c ) 2007                                                       #
#                                                                            #
# Licensed under the GNU Public License v2. Please see LICENSE.GPL for the   #
# full text of terms and conditions.                                         #
#                                                                            #
##############################################################################

use strict;

use Math::BigFloat lib => 'GMP';
use Event;
use Term::ReadKey;

=head1 NAME

ProcInfo NG - display system status gathered from /proc

=cut

=head1 SYNOPSIS

None yet.

=cut

=head1 DESCRIPTION

B<procinfo> gathers some system data from the /proc directory and prints it nicely formatted on the console.

=cut

use constant {
	PROCDIR => '/proc', # It shouldn't move, but... who knows?
	INTERVAL => 2,
};

my $irqHash = getIRQs();
sub mainLoop() {

	print "\e[H\n\n"; # goto home position on screen
	printMeminfo(getMeminfo());

	my $uptime = getUptime();
	my $bootup = getBootup($uptime);
	my $loadAvg = getLoadAvg();
	prettyPrint([ [$bootup, $loadAvg] ]);

	my ($cpuDiffs, $irqDiffs, $ctxtDiff) = getStat();
	my ($user, $nice, $system, $idle, $uptime) = renderCPUstats($cpuDiffs, $uptime);

	my ($pageIn, $pageOut, $swapIn, $swapOut) = getPageSwapStats();

	prettyPrint([ ['user  :', $user, 'page in :', $pageIn],
		['nice  :', $nice, 'page out:', $pageOut],
		['system:', $system, 'swap in :', $swapIn ],
		['idle  :', $idle, 'swap out:', $swapOut],
		['uptime:', $uptime, 'context :', $ctxtDiff]
		]);

	prettyPrint(prepareIRQs($irqHash, $irqDiffs), undef, 1);
}

my $keyboard;
main();
sub main() {
	open($keyboard, '<', '/dev/tty');
	ReadMode 'raw';

	print "\e[2J"; # clear screen
	my $timerWatcher = Event->timer(interval => INTERVAL(), hard=> 1, cb => \&mainLoop );
	my $keyboardWatcher = Event->io( fd => $keyboard, cb => \&keyboardHandle );
	mainLoop();
	Event::loop();
	exit 0;
}

sub keyboardHandle() {
	my $key = ReadKey (0, $keyboard);
	if (lc($key) eq 'q') {
		Event::unloop_all();
	}
}

END { ReadMode 'normal' }

my @cpuStats;
my @irqStats;
my $ctxtStat;
sub getStat() {
	my (@cpuDiffs, @irqDiffs, $ctxtDiff);
	open((my $fh), '<', PROCDIR.'/stat');
	while(my $l = <$fh>) {
		chomp $l;
		my @fields = split(' ', $l);
		my $key = shift @fields;
		my @fields = map( { Math::BigFloat->new($_) } @fields);
		getStat_Switch: { # switch/case hack. faster than using Switch's switch(...)
			($key eq 'cpu') and do {
				@cpuDiffs = subArrays(\@cpuStats, \@fields);
				@cpuStats = @fields;
				last getStat_Switch;
			};
			($key eq 'intr') and do {
				@irqDiffs = subArrays(\@irqStats, \@fields);
				@irqStats = @fields;
				last getStat_Switch;
			};
			($key eq 'ctxt') and do {
				$ctxtDiff = $fields[0] - $ctxtStat;
				$ctxtStat = $fields[0];
				last getStat_Switch;
			};
		}
	}
	close $fh;
	return (\@cpuDiffs, \@irqDiffs, $ctxtDiff);
}

my %oldVMstat;
sub getPageSwapStats() {
	my %curVMstat = getVMstat();
	my %diffVMstat = subHashes(\%oldVMstat, \%curVMstat);
	%oldVMstat = %curVMstat;
	return ($diffVMstat{'pgpgin'} / INTERVAL(), $diffVMstat{'pgpgout'} / INTERVAL(),
		$diffVMstat{'pswpin'} / INTERVAL(), $diffVMstat{'pswpout'} / INTERVAL()
		);
}

sub getVMstat() {
	open((my $fh), '<', PROCDIR.'/vmstat');
	my %hash;
	keys %hash = 40; # pre-allocate enough buckets.
	while(my $l = <$fh>) {
		chomp $l;
		my ($key, $val) = split(' ', $l);
		$hash{$key} = new Math::BigInt($val); # we use Math::BigInt for the possibility of 64bit values;
	}
	close $fh;
	return %hash;
}

sub getIRQs() {
	open((my $fh), '<', PROCDIR.'/interrupts');
	my %hash;
	keys %hash = 16; # pre-allocate enough buckets.
	while(my $l = <$fh>) {
		chomp $l;
		if($l =~ /(\d+): .+PIC.*? (\S.+)$/) {
			my ($key, $devs) = ($1, $2);
			$hash{int $key} = $devs;
			$hash{int $key} =~ /^(.{1,20})/ ; $hash{int $key} = $1;
		}
	}
	close $fh;
	return \%hash;
}

sub prepareIRQs($$) {
	my ($hashRef, $irqDiffs) = @_;
	my %hash = %$hashRef;
	my @irqList = sort( {$a <=> $b}  keys ( %hash ) );
	my $count = scalar @irqList;
	my $split = ($count & 1 ? ($count / 2) + 1 : $count / 2);
	my @rows;
	for(my $i = 0; $i < $split; $i++) {
		push @rows, [sprintf('irq %3d:',$irqList[$i]) . sprintf('%9s', int($irqDiffs->[$irqList[$i]+1]/ INTERVAL)) .' '. sprintf('%-20s', $hash{$irqList[$i]}),
			sprintf('irq %3d:',$irqList[$i+$split]) . sprintf('%9s', int($irqDiffs->[$irqList[$i+$split]+1] / INTERVAL)) .' '. sprintf('%-20s', $hash{$irqList[$i+$split]})];
#		push @rows, [$irqList[$i].':', $irqList[$i], sprintf('%-20s', $hash{$irqList[$i]}),
#			$irqList[$i+$split].':', $irqList[$i+$split], sprintf('%-20s', $hash{$irqList[$i+$split]})];
	}
	return \@rows;
}

sub renderCPUstats($$) {
	my ($cpuDiffs, $uptime) = @_;
	my ($user, $nice, $system, $idle, $iowait, $irq, $softirq) = @$cpuDiffs;
	my ($weeks, $days, $hours, $minutes, $seconds) = splitTime($user / (100.0 * INTERVAL()));
	my $user = ($weeks ? $weeks.'w ' : '').($days ? $days.'d ' : '').
		sprintf("%02d:%02d:%02.2f %5s%%", $hours, $minutes, $seconds, sprintf("%3.1f", $user / INTERVAL()));

	my ($weeks, $days, $hours, $minutes, $seconds) = splitTime($nice / (100.0 * INTERVAL()));
	my $nice = ($weeks ? $weeks.'w ' : '').($days ? $days.'d ' : '').
		sprintf("%02d:%02d:%02.2f %5s%%", $hours, $minutes, $seconds, sprintf("%3.1f", $nice / INTERVAL()));
	
	my ($weeks, $days, $hours, $minutes, $seconds) = splitTime($system / (100.0 * INTERVAL()));
	my $system = ($weeks ? $weeks.'w ' : '').($days ? $days.'d ' : '').
		sprintf("%02d:%02d:%02.2f %5s%%", $hours, $minutes, $seconds, sprintf("%3.1f", $system / INTERVAL()));

	my ($weeks, $days, $hours, $minutes, $seconds) = splitTime($idle / (100.0 * INTERVAL()));
	my $idle = ($weeks ? $weeks.'w ' : '').($days ? $days.'d ' : '').
		sprintf("%02d:%02d:%02.2f %5s%%", $hours, $minutes, $seconds, sprintf("%3.1f", $idle / INTERVAL()));

	my ($weeks, $days, $hours, $minutes, $seconds) = splitTime($uptime);
	my $uptime = ($weeks ? $weeks.'w ' : '').($days ? $days.'d ' : '').
		sprintf("%02d:%02d:%02.2f".' 'x7, $hours, $minutes, $seconds);

	return ($user, $nice, $system, $idle, $uptime);
}

sub subArrays($$) {
	my ($array1, $array2) = @_;
	my @array3;
	for(my $i = 0; exists $array2->[$i] or exists $array1->[$i]; $i++) {
		$array3[$i] = (defined $array2->[$i] ? $array2->[$i] : 0) - (defined $array1->[$i] ? $array1->[$i] : 0);
	}
	return @array3;
}

sub subHashes($$) {
	my ($hash1, $hash2) = @_;
	my %hash3;
	foreach my $key (keys(%$hash2)) {
		#print STDERR "$key $hash2->{$key}\n";
		$hash3{$key} = (defined $hash2->{$key} ? $hash2->{$key} : 0) - (defined $hash1->{$key} ? $hash1->{$key} : 0);
	}
	return %hash3;
}

sub getMeminfo() {
	open((my $fh), '<', PROCDIR.'/meminfo');
	my %hash;
	keys %hash = 30; # pre-allocate enough buckets.
	while(my $l = <$fh>) {
		chomp $l;
		$l =~ /^(\S+):\s*(\S+) kB$/; # grabs the keys w/o all the padding;
		$hash{$1} = new Math::BigInt($2); # we use Math::BigInt for the possibility of 64bit values;
	}
	close $fh;
	return %hash;
}

sub getBootup(;$) {
	my $uptime = shift or getUptime();
	return 'Bootup: '.scalar localtime(int(Math::BigFloat->new(time()) - $uptime));
}

sub getUptime() {
	open((my $fh), '<', PROCDIR.'/uptime');
	my $l = <$fh>; chomp $l; close $fh;
	my ($uptime) = split(' ', $l);
	return Math::BigFloat->new($uptime)
}

sub getLoadAvg() {
	open((my $fh), '<', PROCDIR.'/loadavg');
	my $l = <$fh>; chomp $l; close $fh;
	return "Load average: $l";
}

sub printMeminfo(%) {
	my (%hash) = @_;
	my @rows;
	push @rows, ['Memory:', 'Total', 'Used', 'Free', 'Buffers'];
	push @rows, ['RAM:', $hash{'MemTotal'}, $hash{'MemTotal'} - $hash{'MemFree'}, $hash{'MemFree'}, $hash{'Buffers'}];
	push @rows, ['Swap:', $hash{'SwapTotal'}, $hash{'SwapTotal'} - $hash{'SwapFree'}, $hash{'SwapFree'}];
	prettyPrint(\@rows, [4, 10, 10, 10, 10]);
}

sub prettyPrint($;$$) {
	my ($rowsRef, $colWidthRef, $leftJustify) = @_;
	my @colWidth = ($colWidthRef ? @$colWidthRef : getMaxWidths(@$rowsRef));
	foreach my $ref (@$rowsRef) {
		my @row = @$ref;
		my $line = '';
		for (my $colNum = 0; exists $row[$colNum]; $colNum++) {
			unless($leftJustify) {
				$line .= sprintf('%'.(!$colNum ? '-' : '') .($colWidth[$colNum]+3).'s', $row[$colNum]);
			} else {
				$line .= sprintf('%-'.($colWidth[$colNum]+3).'s', $row[$colNum]);
			}
		}
		print $line.' 'x(80-length($line))."\n";
	}
	print "\n";
}

sub getMaxWidths(@) {
	my (@rows) = @_;
	my @colWidth;
	foreach my $ref (@rows) {
		my @row = @$ref;
		for(my $colNum = 0; exists $row[$colNum]; $colNum++) {
			if (length($row[$colNum]) > $colWidth[$colNum]) {
				$colWidth[$colNum] = length($row[$colNum]);
			}
		}
	}
	return @colWidth;
}

sub splitTime($) {
	my ($difference) = @_;
	my ($weeks, $days, $hours, $minutes, $seconds);
	$seconds	=  $difference % 60;
	$difference	= ($difference - $seconds) / 60;
	$minutes	=  $difference % 60;
	$difference	= ($difference - $minutes) / 60;
	$hours		=  $difference % 24;
	$difference     = ($difference - $hours) / 24;
	$days	   	= $difference % 7;
	$weeks		= ($difference - $days) /  7;

	return ($weeks, $days, $hours, $minutes, $seconds);
}
