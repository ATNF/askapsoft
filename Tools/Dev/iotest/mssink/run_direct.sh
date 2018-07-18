#!/bin/bash -l
#
# This script prepares and executes test for tMSSink.
# The execution is directly from the current node (not through SLURM).
#
# Copyright: CSIRO 2017
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#
# Modified: Mark Wieringa - changes to run on ingest machines using mpirun_rsh
# Note this now needs a file hosts.txt with one or more hosts to start jobs on
# to exist in the current directory
##############################################################################

function show_usage() {
    echo "Usage: $0 [-c LUSTRE_CHUNK] [-d DIRECTORY] [-h] [-n NODES] [-s LUSTRE_STRIPES] [-t TIME] [-w WRITERS]"
    echo "A convenient way to run MSSink"
    echo "Optional arguments:"
    echo "-a  Number of antennas (default to 16 - maximum 36)"
    #echo "-b  Bucket size in Bytes (default: 151632 B) -- under construction"
    #echo "-c  Lustre stripe count (default: 1)"
    #echo "    Note: 0: use system default value, -1: use all OSTs"
    echo "-C  Number of channels"
    echo "-d  Measurement directory (default: "measurement" on current directory)"
    echo "-h  Help (this usage)"
    echo "-m  mpi-like runner (default:srun, e.g. mpiexec)"
    echo "-n  The number of nodes (default: 1)"
    #echo "-s  Lustre stripe size (default: 1m (for 1MB))"
    #echo "    Note: use with unit k, m or g (kilo, mega or giga)"
    echo "-t  Time duration in minutes (default: 1 minute)"
    #echo "-w  The number of writers per node (default: 1)"
    exit
}

regex_number='^-?[0-9]+([.][0-9]+)?$'

function catch_non_numeric() {
    if ! [[ $1 =~ ${regex_number} ]] ; then
        echo "ERROR: $1 is not a number"
        exit 1
    fi
}


##############################################################################
# Set the default values for arguments.

# Duration (in minutes).
# This determines the number of MSSink cycles.
duration=1

# The number of computational nodes, which map to Correlator's blocks.
# Note about Correlator:
# - It has 8 blocks, out of which 7 are used (= 84 cards).
# - A block has 12 cards (= 48 coarse channels = 2592 fine channels).
#   This maps to computational node.
# - A card has 4 coarse channels (= 216 fine channels).
# - A coarse channel has a bandwidth of 1 MHz, and has 54 fine channels.
# Thus the node count determines bandwidth and channel count.
nnode=1

# The number of writers per computational node
#nwriter=1

# Bucket size
# Explanation?
bucket=6469632 #151632

# Target directory is the current directory
dir="${PWD}/measurement"

# Lustre stripe count (no striping)
lustre_nstripe=0

# Lustre chunk size (1MB)
lustre_size="1m"

# default number of antennas
nant=16

# default maximum number of channels
nchannel=12960

# default execution environment
runner="mpirun_rsh -export-all -hostfile hosts.txt "
 
# ASKAP_ROOT: use Max's version for now - getting missing library errors with mine
ASKAP_ROOT=/group/askap/vor010/ASKAPsdp_ingest

##############################################################################
# Get values from input arguments.

while getopts :a:C:d:hm:n:t:w: option; do
    case "${option}" in
        a) nant=${OPTARG};;
        b) bucket=${OPTARG};;
        #c) lustre_nstripe=${OPTARG};;
        C) nchannel=${OPTARG};;
        d) dir=${OPTARG};;
        h) show_usage;;
        m) runner=${OPTARG};;
        n) nnode=${OPTARG};;
        #s) lustre_size=${OPTARG};;
        t) duration=${OPTARG};;
        w) nwriter=${OPTARG};;
        *) show_usage;;
    esac
done

echo "bucket             : ${bucket}"
echo "duration           : ${duration} minutes"
echo "nodes              : ${nnode}"
#echo "writers            : ${nwriter}"
echo "Directory          : ${dir}"
echo "Lustre stripe count: ${lustre_nstripe}"
echo "Lustre stripe size : ${lustre_size}"


##############################################################################
# Derive parameters from the arguments.

# Check whether the input parameter is of the correct type
#catch_non_numeric ${bucket}
catch_non_numeric ${duration}
catch_non_numeric ${nnode}
#catch_non_numeric ${nwriter}
#catch_non_numeric ${lustre_nstripe}

# Check whether the input parameter is within the correct range
#if [[ ${bucket} -le 0 ]]; then
#    echo "ERROR: bucket size is out of range: ${bucket}"
#    exit
#fi

if [[ ${duration} -le 0 ]]; then
    echo "ERROR: duration must be at least 1 minute: ${duration}"
    exit
fi

if [[ ${nnode} -le 0 ]]; then
    echo "ERROR: the number of nodes must be at least 1: ${nnode}"
    exit
fi

#if [[ ${nwriter} -le 0 ]]; then
#    echo "ERROR: the number of writers must be at least 1: ${nwriter}"
#    exit
#fi

#if [[ ${lustre_nstripe} -lt 0 ]]; then
#    echo "ERROR: the number of Lustre stripes is out of range: ${lustre_nstripe}"
#    exit
#fi

# The number of nodes used for writing
echo "Number of nodes: ${nnode}"


# The number of channels per node
#let nchannel_per_node=nchannel/nnode
#echo "Number of channels / node: ${nchannel_per_node}"
#echo "Total number of channels: ${nchannel}"

# The total number of write cycle

# Cadence in second
cadence=10
echo "Cadence: ${cadence} seconds"
let ncycle=duration*60/cadence
echo "Total number of write cycle: ${ncycle}"

#let ncycle_per_node=ncycle/nnode
#echo "Number of write cycle / node: ${ncycle_per_node}"

# number of antennas

antenna_indices="[0"

for (( c=1; c<${nant}; c++ ))
do
   antenna_indices="${antenna_indices}, $c" 
    
done
antenna_indices="${antenna_indices}]"

antennas="["

for (( c=1; c<=${nant}; c++ ))
do
   if [[ ${c} -lt 2 ]]; then
        antennas="${antennas}ant$c"
   else    
        antennas="${antennas}, ant$c" 
   fi
 
done
antennas="${antennas}]"
idx="["
for (( c=1; c<=${nant}; c++ ))
do
   if [[ ${c} -lt 2 ]]; then
        idx="${idx}ak0$c"
   elif [[ ${c} -lt 10 ]]; then
        idx="${idx}, ak0$c"
   else
        idx="${idx}, ak$c"
   fi

done

idx="$idx]"

echo ${antenna_indices}
echo ${antennas}
echo ${idx}

##############################################################################
# Attempt to make target directory if it does not exist.
# Note that if the directory already exists, it is not an error.

mkdir -p ${dir}

# Complain and exit if directory creation fails (eg. a file with same name exists)
if [[ $? -ne 0 ]]; then
    echo "Cannot create directory ${dir}"
    exit
fi

# Set Lustre properties on the directory
#lfs setstripe -c ${lustre_nstripe} -s ${lustre_size} ${dir}
#if [[ $? -ne 0 ]]; then
#    echo "Cannot set Lustre property on directory ${dir}"
#    exit
#fi


##############################################################################
# Create parset file
#
parset_file="tMSSink.in"
cat > ${parset_file} <<EOF
#################################################################
# This file is automatically generated
#################################################################
#filename = %d_%t_%s.ms
filename = ${dir}/test_%t_%s.ms
pointingtable.enable = true
# bucketsize 8 (complex) * 2592 (chan) * 4 (pol) * 12*13/2 (vis) = 1 integration for 48 MHz with 12 antenna array
# for 16 antennas this should be: 11280384
stman.bucketsize = 6469632
# nchan = 54 * 48
stman.tilenchan = 2592
stman.tilencorr = 4
count = ${ncycle}
# synchronize the ranks
#syncranks = true
# put number of beams here : we get each rank to write one beam and all channels, so use 1 here
maxbeams = 1

# put channels per rank here
n_channels.0..95 = ${nchannel}

# just an extract from cpingest.in from one of the actual scheduling blocks,
# it fills other configuration parameters (required to write a full measurement set)
#
antenna.ant1.aboriginal_name=Biyarli
antenna.ant1.location.itrf=[-2556088.4762342, 5097405.97130136, -2848428.39801755]
antenna.ant1.location.wgs84=[116.6314242861317, -26.697000722524, 360.990124660544]
antenna.ant1.name=ak01
antenna.ant2.aboriginal_name=Birliya
antenna.ant2.location.itrf=[-2556109.98178715, 5097388.7000003, -2848440.13680826]
antenna.ant2.location.wgs84=[116.631695, -26.697119, 378.00]
antenna.ant2.name=ak02
antenna.ant3.aboriginal_name=Bundara
antenna.ant3.location.itrf=[-2556121.91125146, 5097392.34895714, -2848421.5318665]
antenna.ant3.location.wgs84=[116.6317858746065, -26.69693403662801, 360.4301465414464]
antenna.ant3.name=ak03
antenna.ant4.aboriginal_name=Bimba
antenna.ant4.location.itrf=[-2556087.39654401, 5097423.58670027, -2848396.86663533]
antenna.ant4.location.wgs84=[116.631335, -26.696684, 379.00]
antenna.ant4.name=ak04
antenna.ant5.aboriginal_name=Gagurla
antenna.ant5.location.itrf=[-2556028.6090681, 5097451.46354903, -2848399.83329766]
antenna.ant5.location.wgs84=[116.630681, -26.696714, 381.00]
antenna.ant5.name=ak05
antenna.ant6.aboriginal_name=Wilara
antenna.ant6.location.itrf=[-2556231.67490635, 5097388.01641451, -2848327.62307273]
antenna.ant6.location.wgs84=[116.632791, -26.695960, 367.89]
antenna.ant6.name=ak06
antenna.ant10.aboriginal_name=Bardi
antenna.ant10.location.itrf=[-2556058.24844042, 5097558.82335391, -2848177.00922141]
antenna.ant10.location.wgs84=[116.630464, -26.694442, 370]
antenna.ant10.name=ak10
antenna.ant12.aboriginal_name=Yalibirri
antenna.ant12.location.itrf=[-2556496.23566172, 5097333.70823193, -2848187.32794448]
antenna.ant12.location.wgs84=[116.635412, -26.694578, 375.00]
antenna.ant12.name=ak12
antenna.ant13.aboriginal_name=Jabi
antenna.ant13.location.itrf=[-2556407.35111752, 5097064.98293269, -2848756.02069474]
antenna.ant13.location.wgs84=[116.635835825, -26.700266, 370]
antenna.ant13.name=ak13
antenna.ant14.aboriginal_name=Gagu
antenna.ant14.location.itrf=[-2555972.77804049, 5097233.66590982, -2848839.91426791]
antenna.ant14.location.wgs84=[116.631162, -26.701120, 370]
antenna.ant14.name=ak14
antenna.ant16.aboriginal_name=Jindi-Jindi
antenna.ant16.location.itrf=[-2555592.92591807, 5097835.03063972, -2848098.26095972]
antenna.ant16.location.wgs84=[116.625041, -26.693651, 370]
antenna.ant16.name=ak16
antenna.ant17.aboriginal_name=Marlu
antenna.ant17.location.itrf=[-2555749.04170034, 5097834.52204658, -2847957.80690714]
antenna.ant17.location.wgs84=[116.626445, -26.692237, 370]
antenna.ant17.name=ak17
antenna.ant19.aboriginal_name=Biji-Biji
antenna.ant19.location.itrf=[-2556379.59847295, 5097497.286368, -2847996.55112378]
antenna.ant19.location.wgs84=[116.633628, -26.692626, 370]
antenna.ant19.name=ak19
antenna.ant24.aboriginal_name=Janimaarnu
antenna.ant24.location.itrf=[-2555959.32694933, 5096979.54433336, -2849303.6263752]
antenna.ant24.location.wgs84=[116.633326, -26.705803, 370]
antenna.ant24.name=ak24
antenna.ant26.aboriginal_name=Yamatji Nyarlu
antenna.ant26.location.itrf=[-2555870.76220996, 5097845.42279761, -2847828.14149782]
antenna.ant26.location.wgs84=[116.627491, -26.690931, 370]
antenna.ant26.name=ak26
antenna.ant27.aboriginal_name=Yamaljingga
antenna.ant27.location.itrf=[-2555320.5971097, 5098257.8170561, -2847581.04700044]
antenna.ant27.location.wgs84=[116.620692, -26.688443, 370]
antenna.ant27.name=ak27
antenna.ant28.aboriginal_name=Ngurlubarndi
antenna.ant28.location.itrf=[-2556552.96761381, 5097767.18910421, -2847354.23205508]
antenna.ant28.location.wgs84=[116.633970, -26.686153, 370]
antenna.ant28.name=ak28
antenna.ant30.aboriginal_name=Nyarluwarri
antenna.ant30.location.itrf=[-2557348.3458112, 5097170.10575597, -2847716.17698495]
antenna.ant30.location.wgs84=[116.643802, -26.689790, 370]
antenna.ant30.name=ak30
antenna.ant90.location.itrf=[-2556496.237175, 5097333.724901, -2848187.33832738]
antenna.ant90.location.wgs84=[116.635412, -26.694578, 375.00]
antenna.ant90.name=ak90


# old BETA antennas
antenna.ant15.location.itrf = [-2555389.850372, 5097664.627578, -2848561.95991566]
antenna.ant15.name = ak15
antenna.ant8.location.itrf = [-2556002.509816,5097320.293832,-2848637.482106]
antenna.ant8.name = ak08
antenna.ant9.location.itrf = [-2555888.891875,5097552.280516,-2848324.679547]
antenna.ant9.name = ak09

# other to get 12-antennas
antenna.ant7.location.itrf = [-2556282.880670, 5097252.290820, -2848527.104272]
antenna.ant7.name = ak07
antenna.ant11.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant11.name = ak11

# Other to get all 36 antennas
antenna.ant18.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant18.name = ak18

antenna.ant20.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant20.name = ak20
antenna.ant21.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant21.name = ak21
antenna.ant22.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant22.name = ak22
antenna.ant23.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant23.name = ak23
antenna.ant25.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant25.name = ak25
antenna.ant29.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant29.name = ak29

antenna.ant31.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant31.name = ak31
antenna.ant32.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant32.name = ak32
antenna.ant33.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant33.name = ak33
antenna.ant34.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant34.name = ak34
antenna.ant35.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant35.name = ak35
antenna.ant36.location.itrf = [-2556397.233607, 5097421.429903, -2848124.497319]
antenna.ant36.name = ak36

antenna.ant.aboriginal_name = tbd
antenna.ant.diameter = 12m
antenna.ant.mount = equatorial
antenna.ant.pointing_parameters = [9 * 0.0]

antennas = ${antennas}
array.name = ASKAP
baselinemap.antennaidx = ${idx}
baselinemap.antennaindices = ${antenna_indices}
baselinemap.name = standard
correlator.mode.standard.chan_width = 18.518518kHz
correlator.mode.standard.interval = 9953280
correlator.mode.standard.n_chan = 216
correlator.mode.standard.stokes = [XX, XY, YX, YY]
feeds.feed0 = [0., 0.]
feeds.feed1 = [0., 0.]
feeds.feed10 = [0., 0.]
feeds.feed11 = [0., 0.]
feeds.feed12 = [0., 0.]
feeds.feed13 = [0., 0.]
feeds.feed14 = [0., 0.]
feeds.feed15 = [0., 0.]
feeds.feed16 = [0., 0.]
feeds.feed17 = [0., 0.]
feeds.feed18 = [0., 0.]
feeds.feed19 = [0., 0.]
feeds.feed2 = [0., 0.]
feeds.feed20 = [0., 0.]
feeds.feed21 = [0., 0.]
feeds.feed22 = [0., 0.]
feeds.feed23 = [0., 0.]
feeds.feed24 = [0., 0.]
feeds.feed25 = [0., 0.]
feeds.feed26 = [0., 0.]
feeds.feed27 = [0., 0.]
feeds.feed28 = [0., 0.]
feeds.feed29 = [0., 0.]
feeds.feed3 = [0., 0.]
feeds.feed30 = [0., 0.]
feeds.feed31 = [0., 0.]
feeds.feed32 = [0., 0.]
feeds.feed33 = [0., 0.]
feeds.feed34 = [0., 0.]
feeds.feed35 = [0., 0.]
feeds.feed4 = [0., 0.]
feeds.feed5 = [0., 0.]
feeds.feed6 = [0., 0.]
feeds.feed7 = [0., 0.]
feeds.feed8 = [0., 0.]
feeds.feed9 = [0., 0.]
feeds.n_feeds = 36
feeds.names = [PAF36]
feeds.spacing = 1deg
monitoring.enabled = false
tasks.tasklist = [MSSink]
tasks.MSSink.type = MSSink
correlator.modes=[standard]
EOF

echo "Created parset file: ${parset_file}"


##############################################################################
# Create batch file
# Note that the quote sign in 'EOF' is used to prevent automatic substitution of ${}.
#
mssink_batch_file="tMSSink.sbatch"
log_file="tMSSink_${dir}.log"
#report_file_prefix="tMSSink_report"

# Note that the duration of slurm execution is increased by 1 minute.
let hh=duration/60
let mm=duration-hh*60+1

cat > ${mssink_batch_file} <<EOF
#!/bin/bash -l

#################################################################
# This file is automatically generated
#################################################################
module use /group/pawsey0233/software/sles12sp2/modulefiles
module load sandybridge gcc/4.8.5 mvapich/2.3b
export MV2_ENABLE_AFFINITY=0

# The root dir is inferred from the location of this script.
root_dir=\`echo "${dir}" | cut -d "/" -f2\`

log_file="tMSSink_${dir}.log"
report_file="tMSSink_${dir}.txt"

#time_stamp=\`date +%Y%m%d_%H%M%S\`
#echo "time stamp: \${time_stamp}"

echo "Report created by ${mssink_batch_file}" > \${report_file}
echo "time start: " `date` >> \${report_file}

# Write main parameters to log file to ensure repeatability
echo "parameter: bucket size         = ${bucket}"         >> \${report_file}
#echo "parameter: lustre stripe size  = ${lustre_size}"    >> \${report_file}
echo "parameter: node count          = ${nnode}"          >> \${report_file}
#echo "parameter: lustre stripe count = ${lustre_nstripe}" >> \${report_file}
echo "parameter: duration            = ${duration}"       >> \${report_file}
echo "parameter: writer count        = ${nwriter}"        >> \${report_file}
echo "log file: \${log_file}" >> \${report_file}

#lfs df /\${root_dir} | grep ^filesystem >> \${report_file}

echo "Executing MSSink ..."

#srun -N ${nnode} -n ${nnode} tMSSink -c ${parset_file} > \${log_file}
#tMSSink -c ${parset_file}
#mpirun_rsh -export-all -n %i -hostfile %s %s -c %s
${runner} -n ${nnode} ${ASKAP_ROOT}/Code/Components/Services/ingest/current/apps/tMSSink.sh -c ${parset_file}

echo "Completed MSink"
echo "At the completion of MSSink:" >> \${report_file}

#lfs df /\${root_dir} | grep ^filesystem >> \${report_file}
echo "time end: " \`date\` >> \${report_file}

EOF

. ${mssink_batch_file} > ${log_file}

##############################################################################
# Post processing:
# - Plot time performance of MSSink
# - Produce report
# - Delete measurement set (big!)
# TODO: Delete SLURM output too

log_file="tMSSink_${dir}.log"
report_file="tMSSink_${dir}.txt"
plot_file="tMSSink_${dir}.png"
post_batch_file="post.sbatch"

cat > ${post_batch_file} <<EOF
#!/bin/bash -l
#SBATCH --job-name=MSSink-post
#SBATCH --account=askaprt
#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks=1

#################################################################
# This file is automatically generated
#################################################################

echo "Plotting MSSink log"

module unload PrgEnv-cray
module unload gcc

module load PrgEnv-gnu
module load argparse
module load numpy
module load matplotlib

echo >> ${report_file}
echo "Report appended by ${post_batch_file}" >> ${report_file}

echo "Plotting data from log file: ${log_file}"

${ASKAP_ROOT}/Tools/Dev/iotest/mssink/ingestPlot.py ${log_file} -p ${plot_file} >> ${report_file}
echo "Top 10 worst times: ">> ${report_file}
grep took ${log_file} | sort -bk 11 -nr | head -10 >> ${report_file}
echo "Deleting test directory: ${dir}"
find ${dir} -type f -print0 | xargs -0 munlink
find ${dir} -depth -type d -empty -delete

EOF
if [[ $testmode -ne 1 ]]; then
    . ${post_batch_file}
fi

