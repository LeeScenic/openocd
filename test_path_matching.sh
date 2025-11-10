#!/bin/bash
# Test path matching in VFS

echo "=== VFS Path Matching Test ==="
echo

# Clean up
rm -rf test_path_demo
mkdir -p test_path_demo

# Create test files
echo 'puts "test1.tcl executed!"' > test_path_demo/test1.tcl
echo 'puts "test2.tcl executed!"' > test_path_demo/test2.tcl

# Build OpenOCD if not already built
if [ ! -f ./src/openocd ]; then
    echo "Building OpenOCD first..."
    ./bootstrap && ./configure && make
fi

echo "Created test structure:"
echo "  test_path_demo/"
echo "    ├── test1.tcl"
echo "    └── test2.tcl"
echo

# Test 1: Load with -D test_path_demo
echo "═══════════════════════════════════════════════════"
echo "Test 1: openocd -D test_path_demo"
echo "═══════════════════════════════════════════════════"
echo

echo "Question: Should we use 'source test1.tcl' or 'source test_path_demo/test1.tcl'?"
echo

echo "Trying: source test1.tcl"
./src/openocd -D test_path_demo -c "source test1.tcl" -c "shutdown" 2>&1 | grep -E "(test1.tcl executed|VFS:|Error|Can't find)" | head -5
echo

echo "Trying: source test_path_demo/test1.tcl"
./src/openocd -D test_path_demo -c "source test_path_demo/test1.tcl" -c "shutdown" 2>&1 | grep -E "(test1.tcl executed|VFS:|Error|Can't find)" | head -5
echo

# Test 2: Show what paths are stored in VFS
echo "═══════════════════════════════════════════════════"
echo "Test 2: Check VFS debug output to see stored paths"
echo "═══════════════════════════════════════════════════"
./src/openocd -d3 -D test_path_demo -c "shutdown" 2>&1 | grep "VFS: Added file" | head -10

echo
echo "═══════════════════════════════════════════════════"
echo "Analysis:"
echo "═══════════════════════════════════════════════════"
echo "When you use: -D test_path_demo"
echo "Files are stored as: 'test1.tcl', 'test2.tcl' (WITHOUT directory prefix)"
echo "So you should use: source test1.tcl"
echo
echo "If you want to keep the prefix, use: -D ."
echo "Then files are stored as: 'test_path_demo/test1.tcl'"
echo "And you use: source test_path_demo/test1.tcl"

# Cleanup
rm -rf test_path_demo
