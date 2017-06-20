#!/bin/bash -l
#
# This script prepares and executes test for tMSSink.
# The execution is directly from the current node (not through SLURM).
#
# Copyright: CSIRO 2017
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#
# TODO: Add result directory (log, plot) as parameter (otherwise current directory).
##############################################################################

function show_usage() {
    echo "Usage: $0 [-c LUSTRE_CHUNK] [-d DIRECTORY] [-h] [-n NODES] [-s LUSTRE_STRIPES] [-t TIME] [-w WRITERS]"
    echo "A convenient way to run MSSink"
    echo "Optional arguments:"
    #echo "-b  Bucket size in Bytes (default: 151632 B) -- under construction"
    #echo "-c  Lustre stripe count (default: 1)"
    #echo "    Note: 0: use system default value, -1: use all OSTs"
    echo "-d  Measurement directory (default: "measurement" on current directory)"
    echo "-h  Help (this usage)"
    echo "-n  The number of nodes (default: 1)"
    #echo "-s  Lustre stripe size (default: 1m (for 1MB))"
    #echo "    Note: use with unit k, m or g (kilo, mega or giga)"
    echo "-t  Time duration in minutes (default: 1 minute)"
    echo "-w  The number of writers per node (default: 1)"
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
nwriter=1

# Bucket size
# Explanation?
bucket=151632

# Target directory is the current directory
dir="${PWD}/measurement"

# Lustre stripe count (no striping)
lustre_nstripe=1

# Lustre chunk size (1MB)
lustre_size="1m"


##############################################################################
# Get values from input arguments.

while getopts :d:hn:t:w: option; do
    case "${option}" in
        #b) bucket=${OPTARG};;
        #c) lustre_nstripe=${OPTARG};;
        d) dir=${OPTARG};;
        h) show_usage;;
        n) nnode=${OPTARG};;
        #s) lustre_size=${OPTARG};;
        t) duration=${OPTARG};;
        w) nwriter=${OPTARG};;
        *) show_usage;;
    esac
done

#echo "bucket             : ${bucket}"
echo "duration           : ${duration} minutes"
echo "nodes              : ${nnode}"
echo "writers            : ${nwriter}"
echo "Directory          : ${dir}"
#echo "Lustre stripe count: ${lustre_nstripe}"
#echo "Lustre stripe size : ${lustre_size}"


##############################################################################
# Derive parameters from the arguments.

# Check whether the input parameter is of the correct type
#catch_non_numeric ${bucket}
catch_non_numeric ${duration}
catch_non_numeric ${nnode}
catch_non_numeric ${nwriter}
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

if [[ ${nnode} -le 0 ]] || [[ ${nnode} -gt 8 ]]; then
    echo "ERROR: the number of nodes must be 1~8 (= Correlator blocks): ${nnode}"
    exit
fi

if [[ ${nwriter} -le 0 ]]; then
    echo "ERROR: the number of writers must be at least 1: ${nwriter}"
    exit
fi

#if [[ ${lustre_nstripe} -lt 0 ]]; then
#    echo "ERROR: the number of Lustre stripes is out of range: ${lustre_nstripe}"
#    exit
#fi

# The number of nodes used for writing
echo "Number of nodes: ${nnode}"

# The number of channels per node
nchannel_per_node=2592
echo "Number of channels / node: ${nchannel_per_node}"
let nchannel=nchannel_per_node*nnode
echo "Total number of channels: ${nchannel}"

# The total number of write cycle
# Cadence in second
cadence=5
echo "Cadence: ${cadence} seconds"
let ncycle=duration*60/cadence
echo "Total number of write cycle: ${ncycle}"

#let ncycle_per_node=ncycle/nnode
#echo "Number of write cycle / node: ${ncycle_per_node}"


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
stman.bucketsize = 151632
stman.tilenchan = 216
stman.tilencorr = 4
count = ${ncycle}

# put number of beams here
maxbeams = 36

# put channels per rank here
n_channels.0..95 = ${nchannel_per_node}

# just an extract from cpingest.in from one of the actual scheduling blocks,
# it fills other configuration parameters (required to write a full measurement set)
#
antenna.ant.aboriginal_name = tbd
antenna.ant.diameter = 12m
antenna.ant.mount = equatorial
antenna.ant.pointing_parameters = [9 * 0.0]
antenna.ant1.aboriginal_name = Diggidumble
antenna.ant1.location.itrf = [-2556084.669, 5097398.337, -2848424.133]
antenna.ant1.location.wgs84 = [116.6314242861317, -26.697000722524, 360.990124660544]
antenna.ant1.name = ak01
antenna.ant1.online = false
antenna.ant1.pointing_parameters = [ -0.00967,  -0.14145, 0.01161, -0.02900, -0.00483,  -0.05792,  -0.00032,  0.00570, -0.91000]
antenna.ant10.aboriginal_name = Bardi
antenna.ant10.delay = 624.215862ns
antenna.ant10.location.itrf = [-2556058.2407192, 5097558.83156939, -2848177.02569149]
antenna.ant10.location.wgs84 = [116.630464, -26.694442, 370]
antenna.ant10.name = ak10
antenna.ant10.online = true
antenna.ant10.pointing_parameters = [0.00000, 0.07669, 0.00000, -0.00108, 0.00234, 0.04571, 0.07716, -0.04390, 0.00000]
antenna.ant12.delay = -1016.41389ns
antenna.ant12.location.itrf = [-2556496.23893101, 5097333.71466669, -2848187.33832738]
antenna.ant12.location.wgs84 = [116.635412, -26.694578, 375.00]
antenna.ant12.name = ak12
antenna.ant12.online = true
antenna.ant12.pointing_parameters = [0.00000, 0.07393, 0.00000, -0.00048, 0.00215, 0.14665, 0.08890, -0.04139, 0.00000]
antenna.ant13.aboriginal_name = Jabi
antenna.ant13.delay = -1077.01348ns
antenna.ant13.location.itrf = [-2556407.35299627, 5097064.98390756, -2848756.02069474]
antenna.ant13.location.wgs84 = [116.635835825, -26.700266, 370]
antenna.ant13.name = ak13
antenna.ant13.online = true
antenna.ant13.pointing_parameters = [0.00000, -0.19786, 0.00000, 0.00000, 0.00000, 0.05712, 0.00000, 0.00000, 0.00000]
antenna.ant14.aboriginal_name = Gagu
antenna.ant14.delay = 2758.10564ns
antenna.ant14.location.itrf = [-2555972.78456557, 5097233.65481756, -2848839.88915184]
antenna.ant14.location.wgs84 = [116.631162, -26.701120, 370]
antenna.ant14.name = ak14
antenna.ant14.online = true
antenna.ant14.pointing_parameters = [0.00000, -0.15144, 0.00000, 0.00366, 0.00676, 0.06894, 0.00558, -0.01122, 0.00000]
antenna.ant16.aboriginal_name = Jindi-Jindi
antenna.ant16.delay = 3495.80185ns
antenna.ant16.location.itrf = [-2555592.88867802, 5097835.02121109, -2848098.26409648]
antenna.ant16.location.wgs84 = [116.625041, -26.693651, 370]
antenna.ant16.name = ak16
antenna.ant16.online = true
antenna.ant16.pointing_parameters = [0.00000, -0.10002, 0.00000, 0.00062, -0.00099, 0.03123, 0.08283, -0.05106, 0.00000]
antenna.ant2.delay = -190.429761ns
antenna.ant2.location.itrf = [-2556109.98244348, 5097388.70050131, -2848440.1332423]
antenna.ant2.location.wgs84 = [116.631695, -26.697119, 378.00]
antenna.ant2.name = ak02
antenna.ant2.online = true
antenna.ant2.pointing_parameters = [0.00000, 0.12390, 0.00000, 0.00115, 0.00301, 0.02041, 0.11197, -0.05287, 0.00000]
antenna.ant24.aboriginal_name = Janimaarnu
antenna.ant24.delay = 5428.21123ns
antenna.ant24.location.itrf = [-2555959.34313275, 5096979.52802882, -2849303.57702486]
antenna.ant24.location.wgs84 = [116.633326, -26.705803, 370]
antenna.ant24.name = ak24
antenna.ant24.online = true
antenna.ant24.pointing_parameters = [0.00000, -0.27485, 0.00000, 0.00000, 0.00000, 0.02264, 0.00000, 0.00000, 0.00000]
antenna.ant27.aboriginal_name = Yamaljingga
antenna.ant27.delay = 7002.74783ns
antenna.ant27.location.itrf = [-2555320.53496742, 5098257.80603434, -2847581.10811709]
antenna.ant27.location.wgs84 = [116.620692, -26.688443, 370]
antenna.ant27.name = ak27
antenna.ant27.online = true
antenna.ant27.pointing_parameters = [0.00000, 0.14571, 0.00000, -0.00052, 0.00534, 0.00208, -0.01952, 0.00171, 0.00000]
antenna.ant28.aboriginal_name = Ngurlubarndi
antenna.ant28.delay = 5133.09164ns
antenna.ant28.location.itrf = [-2556552.97431815, 5097767.23612874, -2847354.29540396]
antenna.ant28.location.wgs84 = [116.633970, -26.686153, 370]
antenna.ant28.name = ak28
antenna.ant28.online = true
antenna.ant28.pointing_parameters = [0.00000, 0.00428, 0.00000, 0.00540, 0.00655, -0.20806, -0.03319, 0.01270, 0.00000]
antenna.ant3.aboriginal_name = Balayi
antenna.ant3.location.itrf = [-2556118.1113922, 5097384.72442044, -2848417.24758565]
antenna.ant3.location.wgs84 = [116.6317858746065, -26.69693403662801, 360.4301465414464]
antenna.ant3.name = ak03
antenna.ant3.online = false
antenna.ant3.pointing_parameters = [ 0.02422, -0.09575, -0.00389, -0.00145,  0.00692, -0.16047, -0.00226, -0.00541,  1.49000]
antenna.ant30.aboriginal_name = Yamaljingga
antenna.ant30.delay = 2887.47334ns
antenna.ant30.location.itrf = [-2557348.40370367, 5097170.17682775, -2847716.21368966]
antenna.ant30.location.wgs84 = [116.643802, -26.689790, 370]
antenna.ant30.name = ak30
antenna.ant30.online = true
antenna.ant30.pointing_parameters = [0.00000, -0.09148, 0.00000, 0.00324, 0.00553, 0.12423, 0.00463, -0.02122, 0.00000]
antenna.ant4.delay = 1.50248671ns
antenna.ant4.location.itrf = [-2556087.396082, 5097423.589662, -2848396.867933]
antenna.ant4.location.wgs84 = [116.631335, -26.696684, 379.00]
antenna.ant4.name = ak04
antenna.ant4.online = true
antenna.ant4.pointing_parameters = [0.00000, 0.24331, 0.00000, -0.00401, 0.00469, -0.01258, 0.03027, -0.01035, 0.00000]
antenna.ant5.delay = 276.705545ns
antenna.ant5.location.itrf = [-2556028.60254059, 5097451.46195695, -2848399.83113161]
antenna.ant5.location.wgs84 = [116.630681, -26.696714, 381.00]
antenna.ant5.name = ak05
antenna.ant5.online = true
antenna.ant5.pointing_parameters = [0.00000, -0.02668, 0.00000, -0.00664, -0.00125, 0.03090, -0.00568, 0.00828, 0.00000]
antenna.ant90.location.itrf = [-2556496.237175, 5097333.724901, -2848187.33832738]
antenna.ant90.location.wgs84 = [116.635412, -26.694578, 375.00]
antenna.ant90.name = ak90
antenna.ant90.online = false
antennas = [ant2,ant4,ant5,ant10,ant12,ant13,ant14,ant16,ant24,ant27,ant28,ant30,ant90]
array.name = ASKAP
baselinemap.antennaidx = [ak02, ak04, ak05, ak10, ak12, ak13, ak14, ak16, ak24, ak27, ak28, ak30]
baselinemap.antennaindices = [1, 3, 4, 9, 11, 12, 13, 15, 23, 26, 27, 29]
baselinemap.name = standard
correlator.mode.standard.chan_width = 18.518518kHz
# why is the interval has this strange value?
#correlator.mode.standard.interval = 5087232
correlator.mode.standard.interval = 5000000
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
log_file="tMSSink.log"
#report_file_prefix="tMSSink_report"

# Note that the duration of slurm execution is increased by 1 minute.
let hh=duration/60
let mm=duration-hh*60+1

cat > ${mssink_batch_file} <<EOF
#!/bin/bash -l
#SBATCH --job-name=MSSink-test
#SBATCH --time=${hh}:${mm}:00
#SBATCH --nodes=${nnode}
#SBATCH --ntasks=${nnode}
#SBATCH --nodelist=galaxy-ingest07

#################################################################
# This file is automatically generated
#################################################################

module load gcc

# The root dir is inferred from the location of this script.
root_dir=\`echo "${dir}" | cut -d "/" -f2\`

log_file="tMSSink_\${SLURM_JOB_ID}.log"
report_file="tMSSink_\${SLURM_JOB_ID}.txt"

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
tMSSink -c ${parset_file}

echo "Completed MSink"
echo "At the completion of MSSink:" >> \${report_file}

#lfs df /\${root_dir} | grep ^filesystem >> \${report_file}
echo "time end: " `date` >> \${report_file}

EOF

. ${mssink_batch_file} > ${log_file}

