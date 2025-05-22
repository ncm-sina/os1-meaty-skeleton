#!/bin/bash

# Script to count lines of code in .c, .h, .asm, .S, .ld, and Makefile/.mk files
# Run from the project root folder in Cygwin

# Initialize counters
c_lines=0
h_lines=0
asm_lines=0
s_lines=0
ld_lines=0
makefile_lines=0
total_lines=0

# Function to count lines for a given file pattern
count_lines() {
    local pattern="$1"
    local count=0

    # Find files matching the pattern, count lines, handle errors
    if [ -n "$(find . -type f -name "$pattern" 2>/dev/null)" ]; then
        count=$(find . -type f -name "$pattern" -exec cat {} \; 2>/dev/null | wc -l)
    fi

    # Output only the count for variable capture
    echo "$count"
}

# Print header
echo "Line counts for source files in $(pwd)"
echo "-------------------------------------"

# Count lines for each file type and capture only the raw count
c_lines=$(count_lines "*.c")
h_lines=$(count_lines "*.h")
asm_lines=$(count_lines "*.asm")
s_lines=$(count_lines "*.S")
ld_lines=$(count_lines "*.ld")

# Count lines for Makefiles (exact name 'Makefile' or *.mk)
makefile_count=0
if [ -n "$(find . -type f -name "Makefile" 2>/dev/null)" ]; then
    makefile_count=$(find . -type f -name "Makefile" -exec cat {} \; 2>/dev/null | wc -l)
fi
if [ -n "$(find . -type f -name "*.mk" 2>/dev/null)" ]; then
    makefile_count=$((makefile_count + $(find . -type f -name "*.mk" -exec cat {} \; 2>/dev/null | wc -l)))
fi
echo "Makefiles (*.mk, Makefile): $makefile_count"
makefile_lines=$makefile_count

# Calculate total
total_lines=$((c_lines + h_lines + asm_lines + s_lines + ld_lines + makefile_lines))

# Print total
echo "-------------------------------------"
echo "Total lines: $total_lines"

# Debug output to verify counts
echo "Debug: Individual counts"
echo "c_lines: $c_lines"
echo "h_lines: $h_lines"
echo "asm_lines: $asm_lines"
echo "s_lines: $s_lines"
echo "ld_lines: $ld_lines"
echo "makefile_lines: $makefile_lines"

# Exit successfully
exit 0