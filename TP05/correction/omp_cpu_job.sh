#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=omp_cpu_job
#SBATCH --nodes=1                     # Number of node, default: 1
#SBATCH --ntasks=1                    # Number of task, default: 1
#SBATCH --cpus-per-task=1             # Requesting 1 thread, default: 1
#SBATCH --time=00:03:30               # Maximum run time
#SBATCH --partition=shared-cpu        # Partition to use: cpu or gpu for the course
#SBATCH --mem=6000M
#SBATCH --constraint=GOLD-6240        # Optional: set the CPU type for the study

#SBATCH --account=latt_teach_parallelisme
#SBATCH --output=%x-%j.out    # Output of the job: {job name}-{jobid}.out (job name = script name)
#SBATCH --error=%x-%j.err     # Error of the job: {job name}-{jobid}.err

echo "Running on CPU (OMP)."

# Same compiler version as the std version, to keep the CPU comparison consistent
module load GCCcore/14.3.0

srun g++ -std=c++20 -fopenmp -O3 main_omp_cpu.cpp -o main_omp_cpu

# num_threads = 0 keeps the OpenMP default (taken from the SLURM allocation)
srun ./main_omp_cpu 20000 15000 300 -2.2 1.0 -1.5 1.5 0 mandelbrot_omp_cpu.bmp
