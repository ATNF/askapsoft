#!/bin/bash
#
# Script to list the galaxy usage by user from the squeue command. 
#
# This can be called using the squser alias that has been setup in the askaputils module
#
# ----
#
# @copyright (c) 2017 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Eric Bastholm <eric.bastholm@csiro.au>
#

squser() {

  # This awk command will take the output from squeue and add up the CPUs and NODEs columns
  # and group by the user.
  #
  # The index for the cpus_per_user and the nodes_per_user arrays is "username cpus nodes"
  # values from the squeue output. 
  awk_command='
    {
      cpus_per_user[$1" "$2] += $4; 
      nodes_per_user[$1" "$2] += $5;
      threads_per_cpu = 2; # need to divide by this else the output is actually threads not cpu
    } END {
      printf "Current galaxy usage by user and project\n"
      printf "\n"
      printf"%-20s %-10s %-5s %-4s\n", "USER", "PROJECT", "CPUs", "NODES";

      # Sort the array indices so we have the names in alpha order. idx[i] will contain "username project".
      n=asorti(cpus_per_user, idx)
      for (i = 1; i <= n; i++) {
        split(idx[i], id, " "); # Split the username and project out into id array for printf later.
        cpus+=cpus_per_user[idx[i]];
        nodes+=nodes_per_user[idx[i]];
        printf"%-20s %-10s %-5s %-4s\n", id[1], id[2], cpus_per_user[idx[i]]/threads_per_cpu, nodes_per_user[idx[i]];
      } 
      printf"\nTotal: CPUS %s\tNodes %s\n", cpus/threads_per_cpu, nodes; 
    } '
  squeue -o "%u %a %T %C %D" -p workq | egrep "RUNNING" | sort -b | awk "$awk_command"
}

squser "$@"