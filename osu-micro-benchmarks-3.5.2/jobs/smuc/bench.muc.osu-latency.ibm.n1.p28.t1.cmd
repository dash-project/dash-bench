#!/bin/bash
#@ energy_policy_tag=NONE
#@ minimize_time_to_solution=yes

#-- IBM MPI: ------------
#@ job_type=parallel
#--------------------------

#@ class=test

#@ node=1
#@ tasks_per_node=28

#@ node_usage=not_shared

# not needed for class general:
#@ island_count=1

#@ wall_clock_limit=0:30:00
#@ job_name=osu-benchmarks.ibm
#@ network.MPI=sn_all,not_shared,us
#@ initialdir=$(home)
#@ output=./logs/$(job_name)/job.n1p28t1.$(schedd_host).$(jobid).out
#@ error=./logs/$(job_name)/job.n1p28t1.$(schedd_host).$(jobid).err
#@ notification=error
#@ notify_user=kowalewski@nm.ifi.lmu.de
#@ queue

#-- MPI environment settings: --
#export MP_CSS_INTERRUPT=yes

export MP_USE_BULK_XFER=yes
export MP_BULK_MIN_MSG_SIZE=4096

# Increases communication latency but allows overlap of
# communication and computation:
#export MPICH_ASYNC_PROGRESS=1
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

# Setup environment:
. /etc/profile
. /etc/profile.d/modules.sh

module load hwloc/1.11
module load papi/5.3
#--------------------------

echo ""
echo "----------------------------------DART_PUT-------------------------------------------"
echo ""

poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s flush
poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s flush_local
poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_put_latency -s req_handle


echo ""
echo "----------------------------------DART_GET-------------------------------------------"
echo ""


poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s flush
poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s flush_local
poe ./workspaces/osu-micro-benchmarks-3.5.2/dart/osu_dart_get_latency -s req_handle


echo ""
echo "----------------------------------MPI_PUT-------------------------------------------"
echo ""

poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s flush -w dynamic
poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s flush_local -w dynamic
poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_put_latency -s wait -w dynamic
#

echo ""
echo "----------------------------------MPI_GET-------------------------------------------"
echo ""

poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s flush -w dynamic
poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s flush_local -w dynamic
poe ./workspaces/osu-micro-benchmarks-3.5.2/mpi/one-sided/osu_get_latency -s wait -w dynamic
