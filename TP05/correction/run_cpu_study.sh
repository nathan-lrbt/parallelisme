for ncpu in 1 2 4 8 16; do
    sbatch --job-name=cpu_study_${ncpu}cpu --cpus-per-task=$ncpu cpu_study_job.sh
done

