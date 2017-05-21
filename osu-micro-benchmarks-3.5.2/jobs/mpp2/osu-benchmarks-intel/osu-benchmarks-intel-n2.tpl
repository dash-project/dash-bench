#!/bin/bash
#SBATCH -o /home/hpc/ug201/ru68ceb2/logs/osu-micro-benchmarks-3.5.2/osu-bench-intel.n2.%j.%N.out 
#SBATCH -D /home/hpc/ug201/ru68ceb2/workspaces/osu-micro-benchmarks-5.3.2
#SBATCH -J osu-bench-3.5.2
#SBATCH --get-user-env 
#SBATCH --clusters=mpp2
#SBATCH --ntasks=56
# multiples of 28 for mpp2
#SBATCH --mail-type=end 
#SBATCH --mail-user=kowalewski@nm.ifi.lmu.de
#SBATCH --export=NONE
#SBATCH --time=00:15:00 

source /etc/profile.d/modules.sh

module load papi
module load hwloc

export I_MPI_BARRIER=4
export I_MPI_SCALABLE_OPTIMIZATION=off

echo ""
echo "-------------------------------------------DART----------------------------------"
echo ""

mpiexec  ./dart/osu_dart_put_latency -s flush
mpiexec  ./dart/osu_dart_put_latency -s flush_local
mpiexec  ./dart/osu_dart_put_latency -s req_handle

mpiexec ./dart/osu_dart_get_latency -s flush
mpiexec  ./dart/osu_dart_get_latency -s flush_local
mpiexec  ./dart/osu_dart_get_latency -s req_handle

echo ""
echo "-------------------------------------------DART NO SHARED WINDOWS -----------------"
echo ""

mpiexec  ./dart/osu_dart_put_latency_noshw -s flush
mpiexec  ./dart/osu_dart_put_latency_noshw -s flush_local
mpiexec  ./dart/osu_dart_put_latency_noshw -s req_handle

mpiexec  ./dart/osu_dart_get_latency_noshw -s flush
mpiexec  ./dart/osu_dart_get_latency_noshw -s flush_local
mpiexec  ./dart/osu_dart_get_latency_noshw -s req_handle

echo ""
echo "-----------------------------------------MPI------------------------------------"
echo ""


mpiexec ./mpi/one-sided/osu_put_latency -s flush -w dynamic 
mpiexec ./mpi/one-sided/osu_put_latency -s flush_local -w dynamic 
mpiexec ./mpi/one-sided/osu_put_latency -s wait -w dynamic 

mpiexec ./mpi/one-sided/osu_get_latency -s flush -w dynamic 
mpiexec ./mpi/one-sided/osu_get_latency -s flush_local -w dynamic 
mpiexec ./mpi/one-sided/osu_get_latency -s wait -w dynamic 


