#!/bin/bash
# Test VFS with directory hierarchy preserved

echo "=== Testing VFS with Directory Hierarchy ==="
echo

# Clean up
rm -rf test_hierarchy
mkdir -p test_hierarchy/subdir

# Create test files
echo 'puts "test1.tcl in test_hierarchy/"' > test_hierarchy/test1.tcl
echo 'puts "test2.tcl in test_hierarchy/"' > test_hierarchy/test2.tcl
echo 'puts "nested.tcl in test_hierarchy/subdir/"' > test_hierarchy/subdir/nested.tcl

echo "Directory structure:"
echo "test_hierarchy/"
echo "├── test1.tcl"
echo "├── test2.tcl"
echo "└── subdir/"
echo "    └── nested.tcl"
echo

# Test 1: Load with -D test_hierarchy
echo "═══════════════════════════════════════════════════"
echo "Test 1: Check what paths are stored in VFS"
echo "═══════════════════════════════════════════════════"
echo "Command: openocd -d3 -D test_hierarchy -c shutdown"
./src/openocd -d3 -D test_hierarchy -c "shutdown" 2>&1 | grep "VFS: Added file" | head -10
echo

echo "═══════════════════════════════════════════════════"
echo "Test 2: Try sourcing with hierarchy"
echo "═══════════════════════════════════════════════════"

echo "A) Trying: source test_hierarchy/test1.tcl"
./src/openocd -D test_hierarchy -c "source test_hierarchy/test1.tcl" -c "shutdown" 2>&1 | grep -E "(test1.tcl|Error|Can't find)" | head -3
echo

echo "B) Trying: source test1.tcl (without directory)"
./src/openocd -D test_hierarchy -c "source test1.tcl" -c "shutdown" 2>&1 | grep -E "(test1.tcl|Error|Can't find)" | head -3
echo

echo "C) Trying: source test_hierarchy/subdir/nested.tcl"
./src/openocd -D test_hierarchy -c "source test_hierarchy/subdir/nested.tcl" -c "shutdown" 2>&1 | grep -E "(nested.tcl|Error|Can't find)" | head -3
echo

# Cleanup
rm -rf test_hierarchy

echo "═══════════════════════════════════════════════════"
echo "Expected behavior after modification:"
echo "═══════════════════════════════════════════════════"
echo "VFS stores files as:"
echo "  - test_hierarchy/test1.tcl"
echo "  - test_hierarchy/test2.tcl"
echo "  - test_hierarchy/subdir/nested.tcl"
echo
echo "So you should use:"
echo "  source test_hierarchy/test1.tcl  ✅"
echo "  source test_hierarchy/subdir/nested.tcl  ✅"
