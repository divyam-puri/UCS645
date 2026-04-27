#!/bin/bash

# Cluster execution script for password cracker
# Usage: ./run_cluster.sh <num_nodes> <num_processes_per_node> <hostfile>

set -e

NUM_NODES=${1:-4}
PROCS_PER_NODE=${2:-4}
HOSTFILE=${3:-"./hostfile.txt"}
NUM_TOTAL_PROCS=$((NUM_NODES * PROCS_PER_NODE))

echo "=========================================="
echo "Cluster Password Cracker Execution"
echo "=========================================="
echo "Nodes: $NUM_NODES"
echo "Processes per node: $PROCS_PER_NODE"
echo "Total processes: $NUM_TOTAL_PROCS"
echo "Hostfile: $HOSTFILE"
echo "=========================================="

# Check if hostfile exists
if [ ! -f "$HOSTFILE" ]; then
    echo "ERROR: Hostfile '$HOSTFILE' not found"
    echo "Create a hostfile with cluster nodes:"
    echo "  node1 slots=4"
    echo "  node2 slots=4"
    echo "  node3 slots=4"
    echo "  node4 slots=4"
    exit 1
fi

# Check if executable exists
if [ ! -f "build/password_cracker" ]; then
    echo "ERROR: Executable not found. Run ./scripts/build.sh first"
    exit 1
fi

# Run with MPI
echo "Launching MPI job..."
mpirun -np $NUM_TOTAL_PROCS \
        -hostfile $HOSTFILE \
        --bind-to core \
        --map-by node \
        ./build/password_cracker

echo ""
echo "=========================================="
echo "Cluster execution complete!"
echo "=========================================="
