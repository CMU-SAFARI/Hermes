#!/usr/bin/perl

use warnings;
use Getopt::Long;
use Data::Dumper;
use File::Slurp;

sub read_stats
{
	my $record_ref = shift;
	my $stat_name = shift;
	if(exists($record_ref->{$stat_name}))
	{
		return $record_ref->{$stat_name};
	}
	else
	{
		return 0;
	}
}
sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

# Getopt::Long::Configure(qw( posix_default ));
# GetOptions('tracename=s' => \$trace_name,
# 	        'expname=s' => \$exp_name,
# 	        'ext=s' => \$ext,
#             'statsdir' => \$stats_dir,
#             'outdir' => \$out_dir,
#             'template' => \$template_file,
# ) or die "Usage: $0 --exe <executable> --exp <exp file> --tlist <trace list>\n";

# die "Supply tracename\n" unless defined $trace_name;
# die "Supply expname\n" unless defined $exp_name;
# die "Supply statsdir\n" unless defined $stats_dir;
# die "Supply outdir\n" unless defined $out_dir;
# die "Supply template\n" unless defined $template_file;

my $arguments = @ARGV;
# print Dumper \@ARGV;
die "Usage: ${0} <tracename> <expname> <statsdir> <outdir> <XML template> <McPAT executable>\n" unless $arguments == 6;

my ($trace_name, $exp_name, $stats_dir, $out_dir, $template_file, $mcpat_exe) = @ARGV;

my $stats_file = "${stats_dir}/${trace_name}_${exp_name}.out";
my $mcpat_xml_file = "${out_dir}/${trace_name}_${exp_name}.mcpat.xml";
my %records;

if (-e $stats_file)
{
	open(my $fh, '<', $stats_file) or die "cannot open $stats_file\n";
	while($line = <$fh>)
	{
		chomp($line);
		my $space_count = () = $line =~ / /g;
		if ($space_count == 1)
		{
			my ($key, $value) = split / /, $line;
			$key = trim($key);
			$value = trim($value);
			$records{$key} = $value;						
		}
	}
	close($fh);
}
else
{
	die "stats file does not exsists: $stats_file\n";
}

########## GENERAL STATS ##########
my $total_cycle = read_stats(\%records, "Core_0_cycles");
my $total_instructions = read_stats(\%records, "Core_0_total_instructions");
my $total_non_branch_instructions = read_stats(\%records, "Core_0_not_branch");
my $branch_prediction_accuracy = read_stats(\%records, "Core_0_branch_pred_accuracy");
my $branch_instructions = $total_instructions - $total_non_branch_instructions;
my $branch_mispredictions = int($branch_instructions * (1 - $branch_prediction_accuracy));
my $int_instructions = $total_non_branch_instructions;
########## L1I STATS ##########
my $L1I_loads = read_stats(\%records, "Core_0_L1I_loads");
my $L1I_load_hit = read_stats(\%records, "Core_0_L1I_load_hit");
my $L1I_load_miss = read_stats(\%records, "Core_0_L1I_load_miss");
my $L1I_RFOs = read_stats(\%records, "Core_0_L1I_RFOs");
my $L1I_RFO_hit = read_stats(\%records, "Core_0_L1I_RFO_hit");
my $L1I_RFO_miss = read_stats(\%records, "Core_0_L1I_RFO_miss");
my $L1I_prefetches = read_stats(\%records, "Core_0_L1I_prefetches");
my $L1I_prefetch_hit = read_stats(\%records, "Core_0_L1I_prefetch_hit");
my $L1I_prefetch_miss = read_stats(\%records, "Core_0_L1I_prefetch_miss");
my $L1I_writebacks = read_stats(\%records, "Core_0_L1I_writebacks");
my $L1I_writeback_hit = read_stats(\%records, "Core_0_L1I_writeback_hit");
my $L1I_writeback_miss = read_stats(\%records, "Core_0_L1I_writeback_miss");
my $icache_read_accesses = $L1I_loads + $L1I_prefetches + $L1I_RFOs;
my $icache_read_misses   = $L1I_load_miss + $L1I_prefetch_miss + $L1I_RFO_miss;
########## L1D STATS ##########
my $L1D_loads = read_stats(\%records, "Core_0_L1D_loads");
my $L1D_load_hit = read_stats(\%records, "Core_0_L1D_load_hit");
my $L1D_load_miss = read_stats(\%records, "Core_0_L1D_load_miss");
my $L1D_RFOs = read_stats(\%records, "Core_0_L1D_RFOs");
my $L1D_RFO_hit = read_stats(\%records, "Core_0_L1D_RFO_hit");
my $L1D_RFO_miss = read_stats(\%records, "Core_0_L1D_RFO_miss");
my $L1D_prefetches = read_stats(\%records, "Core_0_L1D_prefetches");
my $L1D_prefetch_hit = read_stats(\%records, "Core_0_L1D_prefetch_hit");
my $L1D_prefetch_miss = read_stats(\%records, "Core_0_L1D_prefetch_miss");
my $L1D_writebacks = read_stats(\%records, "Core_0_L1D_writebacks");
my $L1D_writeback_hit = read_stats(\%records, "Core_0_L1D_writeback_hit");
my $L1D_writeback_miss = read_stats(\%records, "Core_0_L1D_writeback_miss");
my $dcache_read_accesses = $L1D_loads + $L1D_prefetches + $L1D_RFOs;
my $dcache_write_accesses = $L1D_writebacks;
my $dcache_read_misses   = $L1D_load_miss + $L1D_prefetch_miss + $L1D_RFO_miss;
my $dcache_write_misses = $L1D_writeback_miss;
########## L2C STATS ##########
my $L2C_loads = read_stats(\%records, "Core_0_L2C_loads");
my $L2C_load_hit = read_stats(\%records, "Core_0_L2C_load_hit");
my $L2C_load_miss = read_stats(\%records, "Core_0_L2C_load_miss");
my $L2C_RFOs = read_stats(\%records, "Core_0_L2C_RFOs");
my $L2C_RFO_hit = read_stats(\%records, "Core_0_L2C_RFO_hit");
my $L2C_RFO_miss = read_stats(\%records, "Core_0_L2C_RFO_miss");
my $L2C_prefetches = read_stats(\%records, "Core_0_L2C_prefetches");
my $L2C_prefetch_hit = read_stats(\%records, "Core_0_L2C_prefetch_hit");
my $L2C_prefetch_miss = read_stats(\%records, "Core_0_L2C_prefetch_miss");
my $L2C_writebacks = read_stats(\%records, "Core_0_L2C_writebacks");
my $L2C_writeback_hit = read_stats(\%records, "Core_0_L2C_writeback_hit");
my $L2C_writeback_miss = read_stats(\%records, "Core_0_L2C_writeback_miss");
my $l20_read_accesses = $L2C_loads + $L2C_prefetches + $L2C_RFOs;
my $l20_read_misses   = $L2C_load_miss + $L2C_prefetch_miss + $L2C_RFO_miss;
my $l20_write_accesses = $L2C_writebacks;
my $l20_write_misses = $L2C_writeback_miss;
########## LLC STATS ##########
my $LLC_loads = read_stats(\%records, "Core_0_LLC_loads");
my $LLC_load_hit = read_stats(\%records, "Core_0_LLC_load_hit");
my $LLC_load_miss = read_stats(\%records, "Core_0_LLC_load_miss");
my $LLC_RFOs = read_stats(\%records, "Core_0_LLC_RFOs");
my $LLC_RFO_hit = read_stats(\%records, "Core_0_LLC_RFO_hit");
my $LLC_RFO_miss = read_stats(\%records, "Core_0_LLC_RFO_miss");
my $LLC_prefetches = read_stats(\%records, "Core_0_LLC_prefetches");
my $LLC_prefetch_hit = read_stats(\%records, "Core_0_LLC_prefetch_hit");
my $LLC_prefetch_miss = read_stats(\%records, "Core_0_LLC_prefetch_miss");
my $LLC_writebacks = read_stats(\%records, "Core_0_LLC_writebacks");
my $LLC_writeback_hit = read_stats(\%records, "Core_0_LLC_writeback_hit");
my $LLC_writeback_miss = read_stats(\%records, "Core_0_LLC_writeback_miss");
my $l30_read_accesses = $LLC_loads + $LLC_prefetches + $LLC_RFOs;
my $l30_read_misses   = $LLC_load_miss + $LLC_prefetch_miss + $LLC_RFO_miss;
my $l30_write_accesses = $LLC_writebacks;
my $l30_write_misses = $LLC_writeback_miss;
########## MEMORY CONTROLLER STATS ##########
my $RQ_row_buffer_hit = read_stats(\%records, "Channel_0_RQ_row_buffer_hit");
my $RQ_row_buffer_miss = read_stats(\%records, "Channel_0_RQ_row_buffer_miss");
my $WQ_row_buffer_hit = read_stats(\%records, "Channel_0_WQ_row_buffer_hit");
my $WQ_row_buffer_miss = read_stats(\%records, "Channel_0_WQ_row_buffer_miss");
my $WQ_full = read_stats(\%records, "Channel_0_WQ_full");
my $mc_memory_accesses = $RQ_row_buffer_hit + $RQ_row_buffer_miss + $WQ_row_buffer_hit + $WQ_row_buffer_miss + $WQ_full;
my $mc_memory_reads = $RQ_row_buffer_hit + $RQ_row_buffer_miss;
my $mc_memory_writes = $WQ_row_buffer_hit + $WQ_row_buffer_miss;
#############################################

# read the template
my $template = read_file($template_file);
# print $template;

# change the handlers appropriately

# system
$template =~ s/CS_SYSTEM_TOTAL_CYCLE/$total_cycle/g;
$template =~ s/CS_SYSTEM_IDLE_CYCLE/0/g;
$template =~ s/CS_SYSTEM_BUSY_CYCLE/$total_cycle/g;

# system.core0
$template =~ s/CS_SYSTEM_CORE0_TOTAL_INSTRUCTIONS/$total_instructions/g;
$template =~ s/CS_SYSTEM_CORE0_INT_INSTRUCTIONS/$int_instructions/g;
$template =~ s/CS_SYSTEM_CORE0_FP_INSTRUCTIONS/0/g;
$template =~ s/CS_SYSTEM_CORE0_BRANCH_INSTRUCTIONS/$branch_instructions/g;
$template =~ s/CS_SYSTEM_CORE0_BRANCH_MISPREDICTIONS/$branch_mispredictions/g;
$template =~ s/CS_SYSTEM_CORE0_LOAD_INSTRUCTIONS/0/g;
$template =~ s/CS_SYSTEM_CORE0_STORE_INSTRUCTIONS/0/g;
$template =~ s/CS_SYSTEM_CORE0_COMMITTED_INSTRUCTIONS/$total_instructions/g;
$template =~ s/CS_SYSTEM_CORE0_COMMITTED_INT_INSUTRCTIONS/$int_instructions/g;
$template =~ s/CS_SYSTEM_CORE0_COMMITTED_FP_INSTRUCTIONS/0/g;
$template =~ s/CS_SYSTEM_CORE0_TOTAL_CYCLES/$total_cycle/g;
$template =~ s/CS_SYSTEM_CORE0_IDLE_CYCLES/0/g;
$template =~ s/CS_SYSTEM_CORE0_BUSY_CYCLES/$total_cycle/g;

# system.core0.icache
$template =~ s/CS_SYSTEM_CORE0_ICACHE_READ_ACCESSES/$icache_read_accesses/g;
$template =~ s/CS_SYSTEM_CORE0_ICACHE_READ_MISSES/$icache_read_misses/g;

# system.core0.dcache
$template =~ s/CS_SYSTEM_CORE0_DCACHE_READ_ACCESSES/$dcache_read_accesses/g;
$template =~ s/CS_SYSTEM_CORE0_DCACHE_WRITE_ACCESSES/$dcache_write_accesses/g;
$template =~ s/CS_SYSTEM_CORE0_DCACHE_READ_MISSES/$dcache_read_misses/g;
$template =~ s/CS_SYSTEM_CORE0_DCACHE_WRITE_MISSES/$dcache_write_misses/g;

# system.core0.L20
$template =~ s/CS_SYSTEM_L20_READ_ACCESSES/$l20_read_accesses/g;
$template =~ s/CS_SYSTEM_L20_WRITE_ACCESSES/$l20_write_accesses/g;
$template =~ s/CS_SYSTEM_L20_READ_MISSES/$l20_read_misses/g;
$template =~ s/CS_SYSTEM_L20_WRITE_MISSES/$l20_write_misses/g;

# system.core0.L30
$template =~ s/CS_SYSTEM_L30_READ_ACCESSES/$l30_read_accesses/g;
$template =~ s/CS_SYSTEM_L30_WRITE_ACCESSES/$l30_write_accesses/g;
$template =~ s/CS_SYSTEM_L30_READ_MISSES/$l30_read_misses/g;
$template =~ s/CS_SYSTEM_L30_WRITE_MISSES/$l30_write_misses/g;

# system.core0.mc
$template =~ s/CS_SYSTEM_MC_MEMORY_ACCESSES/$mc_memory_accesses/g;
$template =~ s/CS_SYSTEM_MC_MEMORY_READS/$mc_memory_reads/g;
$template =~ s/CS_SYSTEM_MC_MEMORY_WRITES/$mc_memory_writes/g;

# writeback the file
write_file($mcpat_xml_file, $template);

# execute McPAT
$cmd = "$mcpat_exe -infile $mcpat_xml_file -print_level 5\n";
# print $cmd;
$mcpat_output = `$cmd`;
print $mcpat_output;