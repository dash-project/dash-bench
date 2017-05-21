#!/bin/bash
#SBATCH -o /home/hpc/ug201/ru68ceb2/logs/osu-micro-benchmarks-3.5.2/osu-bench-ompi.n1.%j.%N.out 
#SBATCH -D /home/hpc/ug201/ru68ceb2/workspaces/osu-micro-benchmarks-5.3.2
#SBATCH -J osu-bench-3.5.2
#SBATCH --get-user-env 
#SBATCH --clusters=mpp2
#SBATCH --ntasks=28
# multiples of 28 for mpp2
#SBATCH --mail-type=end 
#SBATCH --mail-user=kowalewski@nm.ifi.lmu.de
#SBATCH --export=NONE
#SBATCH --time=00:15:00 
source /etc/profile.d/modules.sh

module unload mpi.intel
module load mpi.ompi/2.0
module load papi
module load hwloc

echo ""
echo "----------------------------------DART--------------------------------------------"
echo ""

mpiexec --map-by core ./dart/osu_dart_put_latency -s flush
mpiexec --map-by core ./dart/osu_dart_put_latency -s flush_local
mpiexec --map-by core ./dart/osu_dart_put_latency -s req_handle

mpiexec --map-by core ./dart/osu_dart_get_latency -s flush
mpiexec --map-by core ./dart/osu_dart_get_latency -s flush_local
mpiexec --map-by core ./dart/osu_dart_get_latency -s req_handle

echo ""
echo "-------------------------------------------DART NO SHARED WINDOWS -----------------"
echo ""

mpiexec --map-by core ./dart/osu_dart_put_latency_noshw -s flush
mpiexec --map-by core ./dart/osu_dart_put_latency_noshw -s flush_local
mpiexec --map-by core ./dart/osu_dart_put_latency_noshw -s req_handle

mpiexec --map-by core ./dart/osu_dart_get_latency_noshw -s flush
mpiexec --map-by core ./dart/osu_dart_get_latency_noshw -s flush_local
mpiexec --map-by core ./dart/osu_dart_get_latency_noshw -s req_handle

echo ""
echo "----------------------------------MPI--------------------------------------------"
echo ""

mpiexec --map-by core ./mpi/one-sided/osu_put_latency -s flush -w dynamic 
mpiexec --map-by core ./mpi/one-sided/osu_put_latency -s flush_local -w dynamic 
mpiexec --map-by core ./mpi/one-sided/osu_put_latency -s wait -w dynamic 

mpiexec --map-by core ./mpi/one-sided/osu_get_latency -s flush -w dynamic 
mpiexec --map-by core ./mpi/one-sided/osu_get_latency -s flush_local -w dynamic 
mpiexec --map-by core ./mpi/one-sided/osu_get_latency -s wait -w dynamic 



