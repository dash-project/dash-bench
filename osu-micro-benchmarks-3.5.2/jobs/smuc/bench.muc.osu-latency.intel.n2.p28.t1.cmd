#!/bin/bash
#@ energy_policy_tag=NONE
#@ minimize_time_to_solution=yes

#-- Intel MPI: ------------
#@ job_type=MPICH
#--------------------------

#@ class=test

#@ node=2
#@ tasks_per_node=28

#@ node_usage=not_shared

# not needed for class general:
#@ island_count=1

#@ wall_clock_limit=0:30:00
#@ job_name=osu-benchmarks.intel
#@ network.MPI=sn_all,not_shared,us
#@ initialdir=$(home)
#@ output=./logs/$(job_name)/job.n2p28t1.$(schedd_host).$(jobid).out
#@ error=./logs/$(job_name)/job.n2p28t1.$(schedd_host).$(jobid).err
#@ notification=error
#@ notify_user=kowalewski@nm.ifi.lmu.de
#@ queue

#-- MPI environment settings: --
#export MP_CSS_INTERRUPT=yes
#export MP_USE_BULK_XFER=yes
#export MP_BULK_MIN_MSG_SIZE=4096

# Increases communication latency but allows overlap of
# communication and computation:
# export MPICH_ASYNC_PROGRESS=1
# export MV2_USE_RDMA_CM=1
# export MV2_USE_IWARP_MODE=1
# export MV2_RDMA_CM_MAX_PORT=65535
# export MV2_VBUF_TOTAL_SIZE=9216
# export I_MPI_DEVICE=rdma:OpenIB-iwarp
# export I_MPI_RENDEZVOUS_RDMA_WRITE=1
# export I_MPI_RDMA_USE_EVD_FALLBACK=1
# export I_MPI_RDMA_TRANSLATION_CACHE=1
# export I_MPI_DAPL_CHECK_MAX_RDMA_SIZE=1
#-------------------------------

#-- MKL environment settings: --
#export MP_SINGLE_THREAD=yes
#export OMP_NUM_THREADS=1
#export MKL_NUM_THREADS=1
#-------------------------------

#Intel MPI Specific Settings
export I_MPI_SCALABLE_OPTIMIZATION=off
export I_MPI_ADJUST_BARRIER=4

# Setup environment:
. /etc/profile
. /etc/profile.d/modules.sh

#-- Intel MPI: ------------
module switch mpi.ibm mpi.intel
module load hwloc/1.11
module load papi/5.3
#--------------------------

echo ""
echo "------------------------------------------DART----------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s flush
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s flush_local
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s req_handle

echo ""
echo "-------------------------------------------------------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s flush
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s flush_local
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s req_handle

echo ""
echo "------------------------------------------DART (noshw) ----------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency_noshw -s flush
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency_noshw -s flush_local
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency_noshw -s req_handle

echo ""
echo "------------------------------------------------------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency_noshw -s flush
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency_noshw -s flush_local
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency_noshw -s req_handle

echo ""
echo "------------------------------------------MPI----------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s flush -w dynamic
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s flush_local -w dynamic
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s wait -w dynamic


echo ""
echo "-----------------------------------------------------------------------------------------"
echo ""

mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s flush -w dynamic
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s flush_local -w dynamic
mpiexec -n $((28 * 2)) ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s wait -w dynamic
