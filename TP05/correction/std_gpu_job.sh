#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=std_gpu_job
#SBATCH --nodes=1                     # Number of node, default: 1
#SBATCH --ntasks=1                    # Number of task, default: 1
#SBATCH --cpus-per-task=1             # Requesting 1 thread, default: 1
#SBATCH --time=00:03:30               # Maximum run time
#SBATCH --gpus=nvidia_geforce_rtx_2080_ti:1   # Optional: set the GPU type for the study
#SBATCH --partition=shared-gpu   # Partition to use: cpu or gpu for the course
#SBATCH --mem=6000M

#SBATCH --account=latt_teach_parallelisme
#SBATCH --output=%x-%j.out    # Output of the job: {job name}-{jobid}.out (job name = script name)
#SBATCH --error=%x-%j.err     # Error of the job: {job name}-{jobid}.err

echo "Running on GPU."

module load NVHPC

srun nvc++ -std=c++20 -stdpar=gpu -O3 main_std_gpu.cpp -o main_std_gpu

srun ./main_std_gpu 20000 15000 300 -2.2 1.0 -1.5 1.5 mandelbrot_std_gpu.bmp
