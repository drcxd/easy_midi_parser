#!/bin/perl

use strict;
use warnings;
use 5.010;
use POSIX;

die "Usage: emp2zc [empfile]\n" unless @ARGV > 0;

my %h2p = qw(
    30 0
    32 1
    34 2
    35 3
    37 4
    39 5
    3b 6

    3c 7
    3e 8
    40 9
    41 10
    43 11
    45 12
    47 13

    48 14
    4a 15
    4c 16
    4d 17
    4f 18
    51 19
    53 20
    );

my $empfile = shift @ARGV;
open EMP, "<", $empfile;
my $tpq = 0;
my $micspq = 0;

my %channel_hash;
while (<EMP>)
{
    if (m/(\d+) ticks/)
    {
        $tpq = $1;
    }
    if (m/(\d+) microseconds/)
    {
        $micspq = $1;
    }
    if (m/channel (\d+)/)
    {
        $channel_hash{$1} = 1;
    }
}

if (%channel_hash > 1)
{
    say "[WARNING] more than one channel occur, check emp source code or ask the musicians!";
}


close EMP;
my $mspt = $micspq / $tpq / 1000;

open EMP, "<", $empfile;
my $cts = 0;
my $ticks = 0;
my @transactions;
while (<EMP>)
{
    if (m/delta\s+(\d+)/)
    {
        $ticks += $1;
        $cts += $mspt * $1;
        if (m/\[midi event\]\s+Note\s+0x(\p{Hex}+)\s+(\w+)/)
        {
            my $ts = sprintf "%.0f", $cts;
            my $pitch = $h2p{$1};
            my $mode = $2 eq "press" ? 1 : 0;
            push @transactions, "$ts\tpitch\t$pitch\tmode\t$mode";
            # say "$ts pitch\t$pitch\tmode\t$mode";
        }
    }
}

my %visited;
my @out;
for (my $i = 0; $i < @transactions; ++$i)
{
    if (exists $visited{$i})
    {
        next;
    }
    $transactions[$i] =~ m/(\d+)\s+pitch\s+(\d+)\s+mode\s+(\d+)/;
    $visited{$i} = 1;
    my $ts = $1;
    my $pitch = $2;
    my $mode = $3;
    for (my $j = $i + 1; $j < @transactions; ++$j)
    {
        if (exists $visited{$j})
        {
            next;
        }
        $transactions[$j] =~ m/(\d+)\s+pitch\s+(\d+)\s+mode\s+(\d+)/;
        my $ts2 = $1;
        my $pitch2 = $2;
        my $mode2 = $3;
        if ($pitch2 == $pitch)
        {
            $visited{$j} = 1;
            my $last = $ts2 - $ts;
            push @out, "Time:$ts Pitch:$pitch Last:$last";
            last;
        }
    }
}

for (@out)
{
    say;
}
# for (@transactions)
# {
#     say;
# }
