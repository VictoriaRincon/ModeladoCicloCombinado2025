#!/bin/bash

# This script runs the optimization experiment multiple times with different
# numbers of processes (cores).

# Exit immediately if a command exits with a non-zero status.
set -e

# Compile the project first, only if the executable is missing or sources are newer.
echo "--- Ensuring project is compiled ---"
make
echo "--- Compilation check finished ---"
echo

# --- Configuration ---
NUM_TRIALS=1
TIME_LIMIT="60m" # 60 minutes timeout for each run

# --- Directories ---
# Ensure output directories exist
mkdir -p ./resultados
mkdir -p ./scores
mkdir -p ./logs

# --- Main Loop ---
echo "--- Starting Experiment Series ---"
echo "Number of Trials: $NUM_TRIALS"
echo "Time Limit per Run: $TIME_LIMIT"
echo "------------------------------------"
echo

for trial in $(seq 1 $NUM_TRIALS); do
    for np in 1; do
        echo ">>> Running Trial #$trial with NP=$np Cores <<<"

        # Define unique filenames for this specific run
        SCORES_FILE="scores/scores_trial_${trial}_np_${np}.csv"
        SCHEDULE_FILE="resultados/schedule_trial_${trial}_np_${np}.csv"
        LOG_FILE="logs/run_log_trial_${trial}_np_${np}.txt"

        # The mpirun command with a timeout.
        # It redirects all stdout/stderr from the run to a log file.
        # The C++ program itself will write the timestamped scores to the scores file.
        timeout --foreground "$TIME_LIMIT" mpirun -np "$np" --oversubscribe ./bin/optimizer "$SCORES_FILE" "$SCHEDULE_FILE" > "$LOG_FILE" 2>&1 || \
        {
            # This block executes if the command fails (e.g., timeout)
            echo "--- !!! Run for Trial #$trial, NP=$np either timed out or failed. Check log: $LOG_FILE !!! ---"
        }
        
        echo "--- Finished Trial #$trial, NP=$np. Output logged to $LOG_FILE ---"
        echo
    done
done

echo "--- All Experiments Finished ---"
