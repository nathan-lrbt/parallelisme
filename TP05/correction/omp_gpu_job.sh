#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=omp_gpu_job
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

echo "Running on GPU (OMP)"

# module load GCCcore/12.3.0 Clang/16.0.6-CUDA-12.1.1
# clang++ -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda --offload-arch=sm_70 -O3 main_omp_gpu.cpp -o main_omp_gpu

module load NVHPC
nvc++ -std=c++20 -mp=gpu -fopenmp -O3 -gpu=cc75 -Minfo=accel main_omp_gpu.cpp -o main_omp_gpu

export OMP_TARGET_OFFLOAD=MANDATORY
# num_threads = 0 (no effect on the device)
srun ./main_omp_gpu 20000 15000 300 -2.2 1.0 -1.5 1.5 0 mandelbrot_omp_gpu.bmp
