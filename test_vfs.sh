#!/bin/bash
# Test script for Memory VFS feature

set -e

echo "=== OpenOCD Memory VFS Test ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create test directory
TEST_DIR="test_vfs_temp"
echo "1. Creating test directory: $TEST_DIR"
mkdir -p "$TEST_DIR"

# Create test scripts
echo "2. Creating test TCL scripts..."
cat > "$TEST_DIR/test1.tcl" << 'EOF'
puts "Test 1: Hello from memory VFS!"
set test1_var "Test 1 completed"
EOF

cat > "$TEST_DIR/test2.tcl" << 'EOF'
puts "Test 2: Sourcing from memory"
set test2_var "Test 2 completed"
source test1.tcl
puts "Test 1 variable: $test1_var"
EOF

mkdir -p "$TEST_DIR/subdir"
cat > "$TEST_DIR/subdir/test3.tcl" << 'EOF'
puts "Test 3: Script in subdirectory"
set test3_var "Test 3 completed"
EOF

echo "3. Test scripts created:"
find "$TEST_DIR" -name "*.tcl" -exec echo "   - {}" \;
echo

# Test 1: Basic memory VFS usage
echo "=== Test 1: Basic Memory VFS Load ==="
echo "Command: ./src/openocd -D $TEST_DIR -c 'source test1.tcl' -c 'shutdown'"
if ./src/openocd -D "$TEST_DIR" -c "source test1.tcl" -c "shutdown" 2>&1 | grep -q "Test 1: Hello from memory VFS!"; then
    echo -e "${GREEN}✓ Test 1 PASSED${NC}"
else
    echo -e "${RED}✗ Test 1 FAILED${NC}"
    exit 1
fi
echo

# Test 2: Script sourcing another script
echo "=== Test 2: Nested Script Sourcing ==="
echo "Command: ./src/openocd -D $TEST_DIR -c 'source test2.tcl' -c 'shutdown'"
if ./src/openocd -D "$TEST_DIR" -c "source test2.tcl" -c "shutdown" 2>&1 | grep -q "Test 1 variable: Test 1 completed"; then
    echo -e "${GREEN}✓ Test 2 PASSED${NC}"
else
    echo -e "${RED}✗ Test 2 FAILED${NC}"
    exit 1
fi
echo

# Test 3: Subdirectory support
echo "=== Test 3: Subdirectory Scripts ==="
echo "Command: ./src/openocd -D $TEST_DIR -c 'source subdir/test3.tcl' -c 'shutdown'"
if ./src/openocd -D "$TEST_DIR" -c "source subdir/test3.tcl" -c "shutdown" 2>&1 | grep -q "Test 3: Script in subdirectory"; then
    echo -e "${GREEN}✓ Test 3 PASSED${NC}"
else
    echo -e "${RED}✗ Test 3 FAILED${NC}"
    exit 1
fi
echo

# Test 4: Memory persistence (file deleted from disk but still in memory)
echo "=== Test 4: Memory Persistence After Disk Delete ==="
echo "Removing test1.tcl from disk..."
rm "$TEST_DIR/test1.tcl"
echo "Command: ./src/openocd -D $TEST_DIR -c 'source test1.tcl' -c 'shutdown'"
echo "(File deleted from disk but should still work from memory)"
if ./src/openocd -D "$TEST_DIR" -c "source test1.tcl" -c "shutdown" 2>&1 | grep -q "Test 1: Hello from memory VFS!"; then
    echo -e "${GREEN}✓ Test 4 PASSED${NC}"
    echo -e "${GREEN}  → Script executed from memory despite disk deletion!${NC}"
else
    echo -e "${RED}✗ Test 4 FAILED${NC}"
    exit 1
fi
echo

# Cleanup
echo "Cleaning up test directory..."
rm -rf "$TEST_DIR"

echo
echo -e "${GREEN}=== All Tests PASSED ===${NC}"
echo
echo "Memory VFS feature is working correctly!"
