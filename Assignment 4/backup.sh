#!/bin/bash

backup_directory=~/home/backup
complete_backup_directory=$backup_directory/cb
incremental_backup_directory=$backup_directory/ib
backup_log=$backup_directory/backup.log
source_directory=$HOME
log_date_format="%a %d %b %Y %I:%M:%S %p %Z"
complete_backup_counter=20000
incremental_backup_counter=10000

# creating complete backup and incremental backup directory
mkdir -p $complete_backup_directory
mkdir -p $incremental_backup_directory

# Step 1 : creating complete backup
complete_backup_counter=$((complete_backup_counter + 1))
cb_tar_file=cb$complete_backup_counter.tar

# search for all txt files in the home directory and create a tar file and save it in /home/backup/cb directory
find $source_directory -type f -name "*.txt" -print0 | tar -czf $complete_backup_directory/$cb_tar_file --null -T -

# add the message to the log file
echo "$(date +"$log_date_format") $cb_tar_file was created" >> $backup_log
sleep 2m

while true; do

    for i in {1..4}; do

        # creating complete backup
        if [ $i -eq 4 ]; then
            complete_backup_counter=$((complete_backup_counter + 1))
            previous_backup=$complete_backup_directory/$cb_tar_file
            cb_tar_file=cb$complete_backup_counter.tar

            # search for all txt files in the home directory and create a tar file and save it in /home/backup/cb directory
            find $source_directory -type f -name "*.txt" -print0 | tar -czf $complete_backup_directory/$cb_tar_file --null -T -

            # add the message to the log file
            echo "$(date +"$log_date_format") $cb_tar_file was created" >> $backup_log
        else
            # check if any new txt files are created or any txt file is modified
            previous_backup=$incremental_backup_directory/$previous_ib_tar_file
            if [ $i -eq 1 ]; then
                previous_backup=$complete_backup_directory/$cb_tar_file
            fi        
            new_files=$(find $source_directory -type f -name "*.txt" -newer $previous_backup)

            # if new files are created, create an incremental backup
            if [ -n "$new_files" ]; then
                incremental_backup_counter=$((incremental_backup_counter + 1))
                ib_tar_file=ib$incremental_backup_counter.tar

                # for new/modified files create a tar file for those files and save it in /home/backup/ib directory
                echo "$new_files" | tr '\n' '\0' | tar -czf $incremental_backup_directory/$ib_tar_file --null -T -

                # add the message to the log file
                echo "$(date +"$log_date_format") $ib_tar_file was created" >> $backup_log
                previous_ib_tar_file=$ib_tar_file
            else
                # else just add the message in the log file
                echo "$(date +"$log_date_format") No changes-Incremental backup was not created" >> $backup_log
            fi
        fi

        # sleep for 2 minutes after each step
        sleep 2m
    done
done

