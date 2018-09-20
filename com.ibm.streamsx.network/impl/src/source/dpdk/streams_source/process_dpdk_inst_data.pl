#!/usr/bin/perl

use strict;
use warnings;

# Feed the pec.pe.*.stdouterr file to this script on STDIN
# It will ignore the startup data, and process just the DPDK METRICS data that pops out.

our $TSC_HZ = 2194714309;
our $old_new = undef;

our $state = {entries=>{}, all=>[]};

while(<>) {
    chomp;
    if(/^STREAMS_SOURCE: TSC Hz = ([[:digit:]]+)$/) {
        $TSC_HZ = $1;
    } elsif(/^RXTX: receive_loop: METRICS (.*)$/) {
        my @groups = split / : /,$1;

        if(!defined $old_new) {
            $old_new = scalar @groups;
        }

        if(scalar @groups <= $old_new) {
            if(scalar @groups > 0) {
                if($old_new == 6) {
                    # old code
                    processOldEntry($state, @groups);
                } elsif($old_new == 7 || $old_new == 8) {
                    # new code
                    processNewEntry($state, @groups);
                } else {
                    # ?
                    die "ERROR: Initial line had invalid number of groups ($old_new)\n";
                }
            } else {
                printf STDERR "WARNING: Found line with no initial group\n";
            }
        } else {
            # too many groups!  bad!
            die "ERROR: Found line with too many groups (".(scalar @groups).", expected $old_new)\n";
        }
    } else {
        # ignore everything else.
    }
}

if($old_new == 6) {
    postProcessOld($state);
    displayOld($state);
} elsif($old_new == 7 || $old_new == 8) {
    postProcessNew($state);
    displayNew($state);
} else {
    die "ERROR: How did I get this far with the initial group count at $old_new?\n";
}

exit(0);




sub processOldEntry {
    my ($s, @g) = @_;

    my $e = {};
    my $qid;
    (undef, undef, $qid, $e->{ts}, $e->{waste}, $e->{dur}) = split /\s+/,$g[0];
    #print "Entry for $qid\n";
    if(scalar @g == 6) {
        ($e->{none_count}, $e->{none_time}, $e->{none_tsquare}, $e->{none_min}, $e->{none_max}) = split /\s+/,$g[1];
        ($e->{burst_count}, $e->{burst_time}, $e->{burst_tsquare}, $e->{burst_min}, $e->{burst_max}) = split /\s+/,$g[2];
        ($e->{pkt_count}, $e->{pkt_square}, $e->{pkt_min}, $e->{pkt_max}) = split /\s+/,$g[3];
        ($e->{call_time}, $e->{call_tsquare}, $e->{call_min}, $e->{call_max}) = split /\s+/,$g[4];
        ($e->{free_time}, $e->{free_tsquare}, $e->{free_min}, $e->{free_max}) = split /\s+/,$g[5];
    }
    if($qid !~ /^[[:digit:]]+/) {
        print STDERR "WARNING: Found invalid queue id [$qid]\n";
        return;
    }
    if(!defined $e->{free_max}) {
        $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
        push @{$s->{entries}->{$qid}}, undef;

        print STDERR "WARNING: Found line for $qid with short last group.\n";
        return;
    } elsif((scalar (split /\s+/,$g[5])) > 4) {
        $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
        push @{$s->{entries}->{$qid}}, undef;

        print STDERR "WARNING: Found line for $qid with long last group.\n";
        return;
    }

    # Can already compute the averages and stddev
    if($e->{none_count}) {
        $e->{avg_none_time} = $e->{none_time} / $e->{none_count};
        $e->{sdev_none_time} = sqrt($e->{none_tsquare}/$e->{none_count} - $e->{avg_none_time} * $e->{avg_none_time});
    }
    $e->{avg_burst_time} = $e->{burst_time} / $e->{burst_count};
    $e->{avg_burst_pkts} = $e->{pkt_count} / $e->{burst_count};
    $e->{avg_burst_time_per_pkt} = $e->{burst_time} / $e->{pkt_count};
    $e->{avg_call_time} = $e->{call_time} / $e->{pkt_count};
    $e->{avg_free_time} = $e->{free_time} / $e->{pkt_count};
    $e->{sdev_burst_time} = sqrt($e->{burst_tsquare}/$e->{burst_count} - $e->{avg_burst_time} * $e->{avg_burst_time});
    $e->{sdev_burst_pkts} = sqrt($e->{pkt_square}/$e->{burst_count} - $e->{avg_burst_pkts} * $e->{avg_burst_pkts});
    $e->{sdev_burst_time_per_pkt} = $e->{sdev_burst_time}/$e->{avg_burst_pkts};
    $e->{sdev_call_time} = sqrt($e->{call_tsquare}/$e->{pkt_count} - $e->{avg_call_time} * $e->{avg_call_time});
    $e->{sdev_free_time} = sqrt($e->{free_tsquare}/$e->{pkt_count} - $e->{avg_free_time} * $e->{avg_free_time});

    $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
    push @{$s->{entries}->{$qid}}, $e;
}

sub postProcessOld {
    my ($s) = @_;

    my @qids = keys %{$s->{entries}};
#    print "ppo: $qids[0] has ".(scalar @{$s->{entries}->{$qids[0]}})." entries.\n";
    GROUP: for(my $i = 0; $i < scalar @{$s->{entries}->{$qids[0]}}; ++$i) {
        foreach(@qids) {
#            print "Checking $_\n";
            if(!defined $s->{entries}->{$_}->[$i]) {
                next GROUP;
            }
#            print "$_ has entry $i\n";
        }
#        print "Handling entry $i\n";
        # combine all the queue data into data for this point.
        # We want to use the timestamp of the first queue, I guess
        # - sum of pkt_count
        # - new average of burst time per pkt
        # - new stddev of burst time per pkt
        # - new average of call time per pkt
        # - new stddev of call time per pkt
        # - new average of free time per pkt
        # - new stddev of free time per pkt

        # - average total time per pkt
        # - sdev total time per pkt
        # - back-computed pkt rate possible
        # - forward-computed pkt rate in interval

        my $e = {};
        $e->{ts} = $s->{entries}->{$qids[0]}->[$i]->{ts};

        $e->{dur} = 0;
        $e->{pkts} = 0;
        $e->{bursts} = 0;
        $e->{avg_ppb} = 0;
        $e->{avg_rxpp} = 0;
        $e->{avg_callpp} = 0;
        $e->{avg_freepp} = 0;
        $e->{sdev_ppb} = 0;
        $e->{sdev_rxpp} = 0;
        $e->{sdev_callpp} = 0;
        $e->{sdev_freepp} = 0;
        foreach(@qids) {
            my $d = $s->{entries}->{$_}->[$i];
            $e->{dur} += $d->{dur};
            $e->{pkts} += $d->{pkt_count};
            $e->{bursts} += $d->{burst_count};
            $e->{avg_ppb} += $d->{avg_burst_pkts} * $d->{burst_count};
            $e->{avg_rxpp} += $d->{avg_burst_time_per_pkt} * $d->{pkt_count};
            $e->{avg_callpp} += $d->{avg_call_time} * $d->{pkt_count};
            $e->{avg_freepp} += $d->{avg_free_time} * $d->{pkt_count};
        }

        $e->{dur} /= scalar @qids;
        $e->{avg_ppb} /= $e->{bursts};
        $e->{avg_rxpp} /= $e->{pkts};
        $e->{avg_callpp} /= $e->{pkts};
        $e->{avg_freepp} /= $e->{pkts};

        foreach(@qids) {
            my $d = $s->{entries}->{$_}->[$i];
            $e->{sdev_ppb} += ($d->{sdev_burst_pkts} * $d->{sdev_burst_pkts} + (($d->{avg_burst_pkts} - $e->{avg_ppb})**2.0)) * $d->{burst_count};
            $e->{sdev_rxpp} += ($d->{sdev_burst_time_per_pkt} * $d->{sdev_burst_time_per_pkt}  + (($d->{avg_burst_time_per_pkt} - $e->{avg_rxpp})**2.0)) * $d->{pkt_count};
            $e->{sdev_callpp} += ($d->{sdev_call_time} * $d->{sdev_call_time}  + (($d->{avg_call_time} - $e->{avg_callpp})**2.0)) * $d->{pkt_count};
            $e->{sdev_freepp} += ($d->{sdev_free_time} * $d->{sdev_free_time}  + (($d->{avg_free_time} - $e->{avg_freepp})**2.0)) * $d->{pkt_count};
        }

        $e->{sdev_ppb} /= $e->{bursts};
        $e->{sdev_rxpp} /= $e->{pkts};
        $e->{sdev_callpp} /= $e->{pkts};
        $e->{sdev_freepp} /= $e->{pkts};
        $e->{sdev_ppb} = sqrt($e->{sdev_ppb});
        $e->{sdev_rxpp} = sqrt($e->{sdev_rxpp});
        $e->{sdev_callpp} = sqrt($e->{sdev_callpp});
        $e->{sdev_freepp} = sqrt($e->{sdev_freepp});


        $e->{avg_total} = $e->{avg_rxpp} + $e->{avg_callpp} + $e->{avg_freepp};
        $e->{sdev_total} = sqrt($e->{sdev_rxpp}*$e->{sdev_rxpp} + $e->{sdev_callpp}*$e->{sdev_callpp} + $e->{sdev_freepp}*$e->{sdev_freepp});

        $e->{avg_rate} = $e->{pkts}/($e->{dur}/$TSC_HZ);

        $e->{avg_comp_rate} = (scalar @qids) * $TSC_HZ / $e->{avg_total};
        $e->{min_comp_rate} = (scalar @qids) * $TSC_HZ / ($e->{avg_total} + $e->{sdev_total});
        $e->{max_comp_rate} = (scalar @qids) * $TSC_HZ / ($e->{avg_total} - $e->{sdev_total});

        push @{$s->{all}}, $e;

    }

}

sub displayOld {
    my ($s) = @_;

    foreach(@{$s->{all}}) {
        my $e = $_;
        printf("%20.6f %10d %3d %3d    %9d %9d    %9d %9d    %9d %9d    %9d %9d    %12.6f %12.6f\n",
               $e->{ts}, $e->{pkts}, $e->{avg_ppb}, $e->{sdev_ppb}, $e->{avg_rxpp}, $e->{sdev_rxpp}, $e->{avg_callpp}, $e->{sdev_callpp}, $e->{avg_freepp}, $e->{sdev_freepp},
               $e->{avg_total}, $e->{sdev_total}, $e->{avg_rate}/1000000, $e->{avg_comp_rate}/1000000);
    }
}






sub processNewEntry {
    my ($s, @g) = @_;

    my $e = {};
    my $qid;

    (undef, undef, $qid, $e->{ts_offs}, $e->{waste}, $e->{dur}) = split /\s+/,$g[0];
    if($qid !~ /^[[:digit:]]+$/ || $e->{ts_offs} !~ /^[[:digit:].-]+$/ || $e->{dur} !~ /^[[:digit:]]+$/) {
        print STDERR "WARNING: Found invalid queue id [$qid] or ts_offs [$e->{ts_offs}] or dur [$e->{dur}]\n";
        return;
    }
    ($e->{ts}, $e->{offs}) = split '-',$e->{ts_offs};
    $e->{ts} -= $e->{dur}/$TSC_HZ * $e->{offs};
    if(scalar @g == 7 || scalar @g == 8) {
#        ($e->{nic_packets},$e->{nic_bytes}, $e->{nic_missed}, $e->{nic_errors}, $e->{nic_rnb}) = split /\s+/, $g[1];

        $e->{m} = [];
        for(my $i = 0; $i < 5; ++$i) {
            $e->{m}->[$i] = {};
            ($e->{m}->[$i]->{count}, $e->{m}->[$i]->{total}, $e->{m}->[$i]->{tsquare}, $e->{m}->[$i]->{min}, $e->{m}->[$i]->{max}) = split /\s+/, $g[$i+2];
        }
    }
    if(!defined $e->{m}->[4]->{max} || $e->{m}->[4]->{max} !~ /^[[:digit:]]+$/) {
        $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
        push @{$s->{entries}->{$qid}}, undef;

        print STDERR "WARNING: Found line with short last group.\n";
        return;
    } elsif((scalar (split /\s+/,$g[6])) > 5) {
        $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
        push @{$s->{entries}->{$qid}}, undef;

        print STDERR "WARNING: Found line with long last group.\n";
        return;
    }


    foreach(@{$e->{m}}) {
        my $m = $_;
        if($m->{count}) {
            $m->{avg} = $m->{total}/$m->{count};
            $m->{sdev} = sqrt($m->{tsquare}/$m->{count} - $m->{avg}*$m->{avg});
        }
    }

    # For burst time metric, compute the per-pkt averages
    # Not sure how to handle the standard deviation here...
    if($e->{m}->[0]->{total}) {
        $e->{avg_burst_time_per_pkt} = $e->{m}->[1]->{total}/$e->{m}->[0]->{total};
        $e->{sdev_burst_time_per_pkt} = $e->{m}->[1]->{sdev}/$e->{m}->[0]->{avg};
    }

    $s->{entries}->{$qid} = [] if !defined $s->{entries}->{$qid};
    push @{$s->{entries}->{$qid}}, $e;
}

sub postProcessNew {
    my ($s) = @_;



    my @qids = keys %{$s->{entries}};
#    print "ppo: $qids[0] has ".(scalar @{$s->{entries}->{$qids[0]}})." entries.\n";
    GROUP: for(my $i = 0; $i < scalar @{$s->{entries}->{$qids[0]}}; ++$i) {
        foreach(@qids) {
#            print "Checking $_\n";
            if(!defined $s->{entries}->{$_}->[$i]) {
                next GROUP;
            }
#            print "$_ has entry $i\n";
        }
#        print "Handling entry $i\n";
        # combine all the queue data into data for this point.
        # We want to use the timestamp of the first queue, I guess
        # - sum of pkt_count
        # - new average of burst time per pkt
        # - new stddev of burst time per pkt
        # - new average of call time per pkt
        # - new stddev of call time per pkt
        # - new average of free time per pkt
        # - new stddev of free time per pkt

        # - average total time per pkt
        # - sdev total time per pkt
        # - back-computed pkt rate possible
        # - forward-computed pkt rate in interval

        my $e = {};
        $e->{ts} = $s->{entries}->{$qids[0]}->[$i]->{ts};

        $e->{dur} = 0;
        $e->{pkts} = 0;
        $e->{bursts} = 0;
        $e->{avg_ppb} = 0;
        $e->{avg_rxpp} = 0;
        $e->{avg_callpp} = 0;
        $e->{avg_freepp} = 0;
        $e->{sdev_ppb} = 0;
        $e->{sdev_rxpp} = 0;
        $e->{sdev_callpp} = 0;
        $e->{sdev_freepp} = 0;
        foreach(@qids) {
            my $d = $s->{entries}->{$_}->[$i];
            $e->{dur} += $d->{dur};
            $e->{pkts} += $d->{m}->[0]->{total};
            $e->{bursts} += $d->{m}->[0]->{count};
            $e->{avg_ppb} += $d->{m}->[0]->{avg} * $d->{m}->[0]->{count};
            $e->{avg_rxpp} += $d->{avg_burst_time_per_pkt} * $d->{m}->[0]->{total};
            $e->{avg_callpp} += $d->{m}->[3]->{avg} * $d->{m}->[0]->{total};
            $e->{avg_freepp} += $d->{m}->[4]->{avg} * $d->{m}->[0]->{total};
        }

        $e->{dur} /= scalar @qids;
        $e->{avg_ppb} /= $e->{bursts};
        $e->{avg_rxpp} /= $e->{pkts};
        $e->{avg_callpp} /= $e->{pkts};
        $e->{avg_freepp} /= $e->{pkts};

        foreach(@qids) {
            my $d = $s->{entries}->{$_}->[$i];
            $e->{sdev_ppb} += ($d->{m}->[0]->{sdev} * $d->{m}->[0]->{sdev} + (($d->{m}->[0]->{avg} - $e->{avg_ppb})**2.0)) * $d->{m}->[0]->{count};
            $e->{sdev_rxpp} += ($d->{sdev_burst_time_per_pkt} * $d->{sdev_burst_time_per_pkt}  + (($d->{avg_burst_time_per_pkt} - $e->{avg_rxpp})**2.0)) * $d->{m}->[0]->{total};
            $e->{sdev_callpp} += ($d->{m}->[3]->{sdev} * $d->{m}->[3]->{sdev}  + (($d->{m}->[3]->{avg} - $e->{avg_callpp})**2.0)) * $d->{m}->[0]->{total};
            $e->{sdev_freepp} += ($d->{m}->[4]->{sdev} * $d->{m}->[4]->{sdev}  + (($d->{m}->[4]->{avg} - $e->{avg_freepp})**2.0)) * $d->{m}->[0]->{total};
        }

        $e->{sdev_ppb} /= $e->{bursts};
        $e->{sdev_rxpp} /= $e->{pkts};
        $e->{sdev_callpp} /= $e->{pkts};
        $e->{sdev_freepp} /= $e->{pkts};
        $e->{sdev_ppb} = sqrt($e->{sdev_ppb});
        $e->{sdev_rxpp} = sqrt($e->{sdev_rxpp});
        $e->{sdev_callpp} = sqrt($e->{sdev_callpp});
        $e->{sdev_freepp} = sqrt($e->{sdev_freepp});


        $e->{avg_total} = $e->{avg_rxpp} + $e->{avg_callpp} + $e->{avg_freepp};
        $e->{sdev_total} = sqrt($e->{sdev_rxpp}*$e->{sdev_rxpp} + $e->{sdev_callpp}*$e->{sdev_callpp} + $e->{sdev_freepp}*$e->{sdev_freepp});

        $e->{avg_rate} = $e->{pkts}/($e->{dur}/$TSC_HZ);

        $e->{avg_comp_rate} = (scalar @qids) * $TSC_HZ / $e->{avg_total};
        $e->{min_comp_rate} = (scalar @qids) * $TSC_HZ / ($e->{avg_total} + $e->{sdev_total});
        $e->{max_comp_rate} = (scalar @qids) * $TSC_HZ / ($e->{avg_total} - $e->{sdev_total});

        push @{$s->{all}}, $e;

    }





}

sub displayNew {
    my ($s) = @_;

    foreach(@{$s->{all}}) {
        my $e = $_;
        printf("%20.6f %10d %3d %3d    %9d %9d    %9d %9d    %9d %9d    %9d %9d    %12.6f %12.6f\n",
               $e->{ts}, $e->{pkts}, $e->{avg_ppb}, $e->{sdev_ppb}, $e->{avg_rxpp}, $e->{sdev_rxpp}, $e->{avg_callpp}, $e->{sdev_callpp}, $e->{avg_freepp}, $e->{sdev_freepp},
               $e->{avg_total}, $e->{sdev_total}, $e->{avg_rate}/1000000, $e->{avg_comp_rate}/1000000);
    }
}

