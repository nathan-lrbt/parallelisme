#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=parallel_class_job
#SBATCH --nodes=1                     # Number of node, default: 1
#SBATCH --ntasks=1                    # Number of task, default: 1
#SBATCH --cpus-per-task=1             # Requesting 1 thread, default: 1
#SBATCH --time=00:10:00               # Maximum run time
#SBATCH --partition=shared-cpu   # Partition to use: shared-cpu or shared-gpu for the course
#SBATCH --mem=6000M                   # Total memory requested, default: 3G / cpu

# Using reservation made for this course
# Check that the reservation matches today's session with: scontrol show reservation
#SBATCH --account=latt_teach_parallelisme
#SBATCH --reservation=latt_teach_parallel_wednesday_20261014

#SBATCH --output=slurm-%j.out # Output of the job, default: slurm-{jobid}.out
#SBATCH --error=slurm-%j.err  # Error of the job, default: error and output are merged

echo "Hello HPC"

# TODO: load the modules

# TODO: compile your program

# TODO: run your program
