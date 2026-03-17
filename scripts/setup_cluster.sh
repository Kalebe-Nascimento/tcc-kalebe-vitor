#!/usr/bin/env bash
# Setup script for Beowulf cluster nodes.
# Run on the master node; it will configure all nodes via SSH.
#
# Usage:
#   ./setup_cluster.sh [master|node]
#
# Environment variables:
#   NODES      - Space-separated list of node hostnames/IPs (default: node1 node2 node3)
#   MASTER     - Master node hostname (default: master)
#   SSH_USER   - SSH user (default: current user)
#   PROJECT_DIR - Project directory on all nodes (default: ~/tcc-hpc)

set -euo pipefail

NODES="${NODES:-node1 node2 node3}"
MASTER="${MASTER:-master}"
SSH_USER="${SSH_USER:-$(whoami)}"
PROJECT_DIR="${PROJECT_DIR:-$HOME/tcc-hpc}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

log() { echo "[setup] $*"; }
run_on() {
    local host="$1"; shift
    ssh "$SSH_USER@$host" "$@"
}

install_dependencies_local() {
    log "Installing dependencies on local node..."

    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq
        sudo apt-get install -y \
            build-essential \
            openmpi-bin \
            openmpi-common \
            libopenmpi-dev \
            python3 \
            python3-pip \
            default-jdk \
            nfs-common \
            openssh-server \
            2>&1
        pip3 install --user mpi4py numpy Pillow
    elif command -v yum &>/dev/null; then
        sudo yum install -y \
            gcc gcc-c++ make \
            openmpi openmpi-devel \
            python3 python3-pip \
            java-11-openjdk-devel \
            nfs-utils \
            openssh-server
        pip3 install --user mpi4py numpy Pillow
    else
        log "Unknown package manager. Install dependencies manually."
    fi
    log "Local dependencies installed."
}

setup_ssh_keys() {
    log "Setting up SSH keys for passwordless access..."
    if [ ! -f "$HOME/.ssh/id_rsa" ]; then
        ssh-keygen -t rsa -b 4096 -N "" -f "$HOME/.ssh/id_rsa"
        log "SSH key generated."
    fi

    for node in $NODES; do
        log "Copying SSH key to $node..."
        ssh-copy-id "$SSH_USER@$node" || log "Warning: Could not copy key to $node"
    done
}

configure_hosts_file() {
    log "Configuring /etc/hosts..."
    MASTER_IP=$(hostname -I | awk '{print $1}')
    # Only append if the entry does not already exist
    if ! grep -q " $MASTER$" /etc/hosts 2>/dev/null; then
        echo "# TCC Cluster Nodes" | sudo tee -a /etc/hosts
        echo "$MASTER_IP $MASTER" | sudo tee -a /etc/hosts
    fi
    for node in $NODES; do
        if ! grep -q " $node$" /etc/hosts 2>/dev/null; then
            echo "# Add IP for $node manually if needed" | sudo tee -a /etc/hosts
        fi
    done
}

setup_nfs() {
    log "Setting up NFS for shared project directory..."
    MASTER_IP=$(hostname -I | awk '{print $1}')

    if command -v apt-get &>/dev/null; then
        sudo apt-get install -y nfs-kernel-server 2>&1
    fi

    mkdir -p "$PROJECT_DIR"
    # Restrict NFS access to the local subnet for security
    SUBNET=$(hostname -I | awk '{split($1,a,"."); printf "%s.%s.%s.0/24", a[1], a[2], a[3]}')
    echo "$PROJECT_DIR $SUBNET(rw,sync,no_subtree_check)" | sudo tee -a /etc/exports
    sudo exportfs -ra
    sudo systemctl restart nfs-kernel-server 2>/dev/null || sudo service nfs-kernel-server restart 2>/dev/null || true
    log "NFS server configured. Project dir: $PROJECT_DIR"
    log "On each node, run: sudo mount ${MASTER_IP}:${PROJECT_DIR} ${PROJECT_DIR}"
}

install_on_nodes() {
    log "Installing dependencies on cluster nodes..."
    for node in $NODES; do
        log "Configuring $node..."
        run_on "$node" "
            if command -v apt-get &>/dev/null; then
                sudo apt-get update -qq && sudo apt-get install -y \
                    build-essential openmpi-bin libopenmpi-dev \
                    python3 python3-pip default-jdk nfs-common 2>&1
                pip3 install --user mpi4py
            fi
        " || log "Warning: Could not configure $node"
    done
}

create_hostfile() {
    log "Creating MPI hostfile..."
    HOSTFILE="$ROOT_DIR/hostfile"
    echo "$MASTER slots=4" > "$HOSTFILE"
    for node in $NODES; do
        echo "$node slots=4" >> "$HOSTFILE"
    done
    log "Hostfile created: $HOSTFILE"
    log "Run MPI with: mpirun --hostfile $HOSTFILE -np <N> <program>"
}

MODE="${1:-local}"

case "$MODE" in
    master)
        log "Setting up master node..."
        install_dependencies_local
        setup_ssh_keys
        configure_hosts_file
        setup_nfs
        install_on_nodes
        create_hostfile
        log "Master setup complete!"
        ;;
    node)
        log "Setting up worker node..."
        install_dependencies_local
        log "Node setup complete! Mount NFS share from master."
        ;;
    local)
        log "Setting up local single-machine environment..."
        install_dependencies_local
        create_hostfile
        log "Local setup complete!"
        ;;
    *)
        echo "Usage: $0 [master|node|local]"
        exit 1
        ;;
esac
