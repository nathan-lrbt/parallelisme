#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=gpu_study_job
#SBATCH --nodes=1                     # Number of node, default: 1
#SBATCH --ntasks=1                    # Number of task, default: 1
#SBATCH --time=00:03:30               # Maximum run time
#SBATCH --gpus=nvidia_geforce_rtx_2080_ti:1   # Optional: set the GPU type for the study
#SBATCH --partition=shared-gpu   # Partition to use: cpu or gpu for the course
#SBATCH --mem=12000M

#SBATCH --account=latt_teach_parallelisme
#SBATCH --output=%x-%j.out    # Output of the job: {job name}-{jobid}.out (job name = script name)
#SBATCH --error=%x-%j.err     # Error of the job: {job name}-{jobid}.err


module load NVHPC

srun nvc++ -std=c++20 -stdpar=gpu -O3 main_std_gpu.cpp -o main_std_gpu

for ratio in 0.5 1.0 1.5; do
    sqrt_ratio=$(echo "scale=4; sqrt($ratio)" | bc)

        size1=$(echo "20000 * $sqrt_ratio" | bc)
        size2=$(echo "15000 * $sqrt_ratio" | bc)
    echo "Running on GPU (algo par) with size=($size1, $size2)."
    # Runs are done sequentially, only the last image is kept
    srun ./main_std_gpu $size1 $size2 300 -2.2 1.0 -1.5 1.5 mandelbrot_gpu_study.bmp
done
