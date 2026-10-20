#!/bin/bash
#
# To learn more, refere to UNIGE HPC documentation: https://doc.eresearch.unige.ch/hpc/slurm
#
#SBATCH --job-name=cpu_study_job
#SBATCH --nodes=1                     # Number of node, default: 1
#SBATCH --ntasks=1                    # Number of task, default: 1
#SBATCH --time=00:03:30               # Maximum run time
#SBATCH --partition=shared-cpu        # Partition to use: cpu or gpu for the course
#SBATCH --mem=6000M
#SBATCH --constraint=GOLD-6240        # Optional: set the CPU type for the study

#SBATCH --account=latt_teach_parallelisme
#SBATCH --output=%x-%j.out    # Output of the job: {job name}-{jobid}.out (job name = script name)
#SBATCH --error=%x-%j.err     # Error of the job: {job name}-{jobid}.err

# NOTE: Used to vary the number of thread by using ./run_cpu_study.sh

echo "=== Strong scaling study: std algorithms on CPU ==="
echo "CPUs/task:  $SLURM_CPUS_PER_TASK (visible to the process: $(nproc))"

# Needed by tbb (see module spider tbb/2022.2.0)
module load GCCcore/14.3.0
module load tbb/2022.2.0

# Binary built beforehand by std_cpu_job.sh (prevent multiple compilation)
if [ ! -x "./main_std_cpu" ]; then
    echo "ERROR: ./main_std_cpu not found, run std_cpu_job.sh first to build it." >&2
    exit 1
fi

# One image per CPU count (e.g. mandelbrot_cpu_study_4cpu.bmp)
srun ./main_std_cpu 20000 15000 300 -2.2 1.0 -1.5 1.5 mandelbrot_cpu_study_${SLURM_CPUS_PER_TASK}cpu.bmp
