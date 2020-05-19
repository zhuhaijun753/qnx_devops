#!/bin/bash

# Exit if a command fails (-e)
# Exit if an undeclared variable is used (-u)
set -eu

# Define the install directory and output directory for building process
INSTALL_DIR="install"
OUTPUT_DIR="output"

# Get the absolute path of git branche
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd ../ && pwd)"

# Determine whether the output folder exists, create if it doesn't exist
if [ ! -d "$OUTPUT_DIR" ]; then
        mkdir $OUTPUT_DIR
fi

# change work directory to source/
cd $WORK_DIR/source

# Clean the prebuild files
make clean

# Determine whether the install folder exists, create if it doesn't exist
if [ ! -d "$INSTALL_DIR" ]; then
        mkdir $INSTALL_DIR
fi

# Build the QNX image for s32g evb
make all

# move the created QNX image into output directory
mv $WORK_DIR/source/images/ifs-s32g-evb.ui $WORK_DIR/output/
