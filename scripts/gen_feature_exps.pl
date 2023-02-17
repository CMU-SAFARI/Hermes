#!/usr/bin/perl
use v5.10;
use lib '/home/rahbera/skipper/ChampSim/scripts';
use warnings;
use Math::Combinatorics;

my @feature_names = ("PC", "Offset", "Page", "Addr", "FirstAccess", "PC_Offset", "PC_Page", "PC_Addr", "PC_FirstAccess", "Offset_FirstAccess", "CLOffset", "PC_CLOffset", "CLWordOffset", "PC_CLWordOffset", "CLDWordOffset", "PC_CLDWordOffset", "LastNLoadPCs", "LastNPCs");

############# CHANGE ONLY THESE PARAMS #############
# my @selected_features = ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "16", "17");
my @selected_features = ("0", "5", "8", "9", "11", "16", "17");
# my @num_combs = ("1", "2");
my @num_combs = ("3", "4", "5", "6", "7");
my $weight_array_size = 4096;
####################################################

foreach $num_comb (@num_combs)
{
	my $arg_weight_array_size = join(",", ($weight_array_size)x$num_comb);
	my $arg_hash_types = join(",", (2)x$num_comb);
	my $arg_activation_thresh = 0;
	my $arg_pos_train_thresh = 0;
	my $arg_neg_train_thresh = 0;

	given ($num_comb)
	{
		when('1') { 
			$arg_activation_thresh = -2;
			$arg_pos_train_thresh = 10;
			$arg_neg_train_thresh = -10;
		}
		when('2') { 
			$arg_activation_thresh = -4;
			$arg_pos_train_thresh = 14;
			$arg_neg_train_thresh = -15;
		}
		when('3') { 
			$arg_activation_thresh = -8;
			$arg_pos_train_thresh = 18;
			$arg_neg_train_thresh = -20;
		}
		when('4') { 
			$arg_activation_thresh = -9;
			$arg_pos_train_thresh = 30;
			$arg_neg_train_thresh = -27;
		}
		when('5') { 
			$arg_activation_thresh = -11;
			$arg_pos_train_thresh = 40;
			$arg_neg_train_thresh = -35;
		}
		when('6') { 
			$arg_activation_thresh = -13;
			$arg_pos_train_thresh = 50;
			$arg_neg_train_thresh = -42;
		}
		when('7') { 
			$arg_activation_thresh = -15;
			$arg_pos_train_thresh = 60;
			$arg_neg_train_thresh = -50;
		}
		default{ 
			$arg_activation_thresh = -2;
			$arg_pos_train_thresh = 10;
			$arg_neg_train_thresh = -10;
		}
	}


	my $more_knobs = "--ocp_perc_weight_array_sizes=$arg_weight_array_size  --ocp_perc_feature_hash_types=$arg_hash_types --ocp_perc_activation_threshold=$arg_activation_thresh --ocp_perc_pos_train_thresh=$arg_pos_train_thresh --ocp_perc_neg_train_thresh=$arg_neg_train_thresh";

	my @combinations = combine($num_comb, @selected_features);

	print "#Combinations using $num_comb features\n";
	foreach my $combo (@combinations)
	{
		my @combo2 = @$combo;
		$sel_feat_ids = join(",", @combo2);
		$sel_feat_ids_string = join("_", @combo2);
		# $sel_feat_names = join("+", map { $feature_names[$_] } @combo2);
		# $exp_name = "f_${sel_feat_names}";
		$exp_name = "f_${sel_feat_ids_string}";
		print "${exp_name}  \$(BASE) \$(PYTHIA) \$(OCP_PERC) \$(HERMES_O) --ocp_perc_activated_features=${sel_feat_ids} $more_knobs\n";
	}
	print "\n";
}
