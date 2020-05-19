#!/bin/bash

# Exit if a command fails (-e)
# Exit if an undeclared variable is used (-u)
set -eu

# Define the install directory and output directory for building process
install_dir="install"
output_dir="output"

# Determine whether the output folder exists, create if it doesn't exist
if [ ! -d "$output_dir" ]; then
        mkdir $output_dir
fi

# change work directory to source/
cd ./source

# Clean the prebuild files
make clean

# Determine whether the install folder exists, create if it doesn't exist
if [ ! -d "$install_dir" ]; then
        mkdir $install_dir
fi

# Build the QNX image for s32g evb
make all

# move the created QNX image into output directory
mv ./images/ifs-s32g-evb.ui ./images/qnx-ifs ../output/
