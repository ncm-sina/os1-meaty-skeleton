#!/bin/bash

# Check if a path is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <path>"
    exit 1
fi

# Resolve the provided path relative to current working directory
target_path=$(realpath "$1")

# Check if the path exists
if [ ! -d "$target_path" ]; then
    echo "Error: Directory '$target_path' does not exist"
    exit 1
fi

# Function to print directory structure recursively
print_structure() {
    local dir="$1"
    local prefix="$2"
    
    # List all entries in the directory
    for entry in "$dir"/*; do
        if [ -e "$entry" ]; then
            local basename=$(basename "$entry")
            
            # Print the current entry
            echo "${prefix}├── $basename"
            
            # If it's a directory, recurse into it
            if [ -d "$entry" ]; then
                print_structure "$entry" "$prefix│   "
            fi
        fi
    done
}

# Print the root directory name
echo "$(basename "$target_path")"
# Start printing the structure
print_structure "$target_path" ""