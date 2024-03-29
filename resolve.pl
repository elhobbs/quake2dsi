#!/usr/bin/perl

#
# cyg-resolve.pl - CygProfiler resolver
# 
# Michal Ludvig <michal@logix.cz>
# http://www.logix.cz/michal/devel
# 
# Use this script to parse the output of the CygProfile suite.
# 

use strict;
use warnings;
no warnings 'portable';
use diagnostics;
use English;

my %symtab;
my ($binfile, $textfile, $levelcorr);

$OUTPUT_AUTOFLUSH=1;

&help() if($#ARGV < 1);
$binfile=$ARGV[0];
&help() if($binfile =~ /--help/);
$textfile=($#ARGV > 0 ? $ARGV[1] : "cygprof.log");
$levelcorr=($#ARGV > 1 ? $ARGV[2] : -1);

&main();

# ==== Subs

sub help()
{
	printf("CygProgile parser, Michal Ludvig <michal\@logix.cz>, 5192-5193\n");
	printf("Usage: %s <bin-file> <log-file> [<correction>]\n", $0);
	printf("\t<bin-file>   Program that generated the logfile.\n");
	printf("\t<log-file>   Logfile generated by the profiled program.\n");
	printf("\t<correction> Correction of the nesting level.\n");
	exit;
}

sub main()
{
	my($offset, $type, $function);
	my($nsym, $nfunc);

	$nsym=0;
	$nfunc=0;

        printf("starting up\n");
	
	open(NM, "arm-eabi-nm $binfile|") or die("Unable to run 'nm $binfile': $!\n");
	printf("Loading symbols from $binfile ... ");
	
	while(<NM>)
	{
		$nsym++;
		next if(!/^([0-9A-F]+) (.) (.+)$/i);
		$offset=hex($1); $type=$2; $function=$3;
		next if($type !~ /[tT]/);
		$nfunc++;
		$symtab{$offset}=$function;
	}
	printf("OK\nSeen %d symbols stored %d function offsets\n", $nsym, $nfunc);
	close(NM);

	open(TEXT, "$textfile")
		or die("Unable to open '$textfile': $!\n");

	my $total = 0;
	printf("\taddress\tcalls\tms\tms/call\tus/call\tclocks/call\tms/frame\tlowest stack\thighest stack\tfunction name\n\n");
	while(<TEXT>)
	{
		# Change the pattern if the output format
		# of __cyg_...() functions has changed.
		if(!/(\d*)\s*([[:xdigit:]]+)\s*(\d*)\s*(\d*)\s*(\d*)/)
			{ print $_; next; }
		else
		{
			my $off1=hex($2);
			my $hits=$3;
			my $ticks=$1;

			# Don't print exits
			my $sym=(defined($symtab{$off1})?$symtab{$off1}:"???");

			printf("\t0x%x\t%6d\t%10.4f\t%.3f\t%.3f\t%.3f\t%.3f\t%d\t%d\t%s\n",
				$off1, $hits, $ticks / 15.720,
				($ticks / 15.720) / $hits,
				($ticks / 15.720) / $hits * 1000,
				($ticks / 15.720) / $hits * 67000,
				$ticks / 15.720 / 519,
				$4, $5, $sym);
			$total += $ticks / 15720;
		}
	}
	close(TEXT);

	printf("total time %.2f, %.2f ms per frame, %.2f fps\n", $total, $total / 519 * 1000, 519 / $total);
	printf("done\n");
}
