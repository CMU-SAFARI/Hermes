#!/usr/bin/perl

use warnings;
use Getopt::Long;
use Trace;
use Exp;
use Metric;
use Statistics::Descriptive;
use Data::Dumper;

# defaults
my $tlist_file;
my $exp_file;
my $mfile;
my $ext = "out";

GetOptions('tlist=s' => \$tlist_file,
	   'exp=s' => \$exp_file,
	   'ext=s' => \$ext,
) or die "Usage: $0 --exe <executable> --exp <exp file> --tlist <trace list>\n";

die "\$HERMES_HOME env variable is not defined.\nHave you sourced setvars.sh?\n" unless defined $ENV{'HERMES_HOME'};

die "Supply tlist\n" unless defined $tlist_file;
die "Supply exp\n" unless defined $exp_file;

my @trace_info = Trace::parse($tlist_file);
my @exp_info = Exp::parse($exp_file);

print "Trace,Exp,total_runtime_dyn_power,icache,dcache,L2,L3,bus,Filter\n";

for $trace (@trace_info)
{
	my $trace_name = $trace->{"NAME"};
	my %per_trace_result;
	my $all_exps_passed = 1;

	for $exp (@exp_info)
	{
		my $exp_name = $exp->{"NAME"};
		my $log_file = "${trace_name}_${exp_name}.mcpat.${ext}";

		if (-e $log_file)
		{
            $cmd1 = "grep \"Runtime Dynamic\" $log_file | head -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # total
            $cmd2 = "grep \"Runtime Dynamic\" $log_file | head -7  | tail -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # icache
            $cmd3 = "grep \"Runtime Dynamic\" $log_file | head -25 | tail -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # dcache
            $cmd4 = "grep \"Runtime Dynamic\" $log_file | head -43 | tail -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # L2
            $cmd5 = "grep \"Runtime Dynamic\" $log_file | head -44 | tail -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # L3
            $cmd6 = "grep \"Runtime Dynamic\" $log_file | head -45 | tail -1 | cut -d\"=\" -f2 | cut -d\" \" -f2"; # bus

            $total_runtime_dyn_power = `$cmd1`; $total_runtime_dyn_power = trim($total_runtime_dyn_power);
            $icache = `$cmd2`; $icache = trim($icache);
            $dcache = `$cmd3`; $dcache = trim($dcache);
            $l2 = `$cmd4`; $l2 = trim($l2);
            $l3 = `$cmd5`; $l3 = trim($l3);
            $bus = `$cmd6`; $bus = trim($bus);
		}
		else
		{
			$all_exps_passed = 0;
		}
		$per_trace_result{$exp_name} = "$total_runtime_dyn_power,$icache,$dcache,$l2,$l3,$bus";
	}

	# print stats
	for $exp (@exp_info)
	{
		my $exp_name = $exp->{"NAME"};
		my $result = sprintf("%s,%s,%s,%d\n", $trace_name, $exp_name, $per_trace_result{$exp_name}, $all_exps_passed);
		print $result;
	}
}

sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };
