#!/bin/bash
# Test script for VFS crypto functionality

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="/tmp/test_vfs_crypto_$$"
KEY="keyskeyskeyskeys"

echo "======================================"
echo "VFS Crypto Functionality Test"
echo "======================================"
echo

# Create test directory
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo "Test directory: $TEST_DIR"
echo

# Test 1: Create a test TCL file
echo "Test 1: Creating test TCL file..."
cat > test.tcl << 'EOF'
# Test TCL script
puts "Hello from encrypted VFS!"
set test_var "This is a test"
puts "Test variable: $test_var"
EOF

echo "Created test.tcl:"
cat test.tcl
echo

# Test 2: Encrypt the file
echo "Test 2: Encrypting test.tcl..."
python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" test.tcl test.tcl.bin --key "$KEY"
echo

# Check encrypted file exists
if [ ! -f test.tcl.bin ]; then
    echo "ERROR: Encrypted file not created"
    exit 1
fi

echo "Encrypted file created: test.tcl.bin"
echo "Size: $(wc -c < test.tcl.bin) bytes"
echo "First 100 bytes:"
head -c 100 test.tcl.bin
echo
echo

# Test 3: Verify encryption (decrypt back)
echo "Test 3: Verifying encryption (decrypt back)..."
python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" test.tcl.bin test_decrypted.tcl --key "$KEY" --decrypt
echo

# Compare original and decrypted
if diff test.tcl test_decrypted.tcl > /dev/null; then
    echo "✓ Decrypted file matches original"
else
    echo "✗ Decrypted file does NOT match original"
    echo "Original:"
    cat test.tcl
    echo "Decrypted:"
    cat test_decrypted.tcl
    exit 1
fi
echo

# Test 4: Create multiple encrypted files
echo "Test 4: Creating multiple encrypted test files..."
mkdir -p encrypted_dir

cat > file1.tcl << 'EOF'
puts "File 1"
EOF

cat > file2.tcl << 'EOF'
puts "File 2"
EOF

python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" file1.tcl encrypted_dir/file1.tcl.bin --key "$KEY"
python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" file2.tcl encrypted_dir/file2.tcl.bin --key "$KEY"

echo "✓ Created encrypted files:"
ls -lh encrypted_dir/
echo

# Test 5: Test with different key lengths
echo "Test 5: Testing key length validation..."

# This should fail (wrong key length)
if python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" test.tcl test_wrong_key.bin --key "wrongkey" 2>&1 | grep -q "16 bytes"; then
    echo "✓ Correctly rejected wrong key length"
else
    echo "✗ Failed to detect wrong key length"
    exit 1
fi
echo

# Test 6: Test with key file
echo "Test 6: Testing key from file..."
echo -n "$KEY" > keyfile.txt
python3 "$SCRIPT_DIR/tools/encrypt_tcl.py" test.tcl test_keyfile.bin --key-file keyfile.txt
echo "✓ Encrypted with key from file"
echo

# Verify it produces the same result
if diff test.tcl.bin test_keyfile.bin > /dev/null; then
    echo "✓ Key file produces same result as direct key"
else
    echo "✗ Key file produces different result"
    exit 1
fi
echo

# Test 7: Create usage example
echo "Test 7: Creating usage example..."
cat > README_CRYPTO.txt << EOF
# VFS Crypto Usage Example

## Encrypt TCL files

1. Encrypt a single file:
   python3 tools/encrypt_tcl.py script.tcl script.tcl.bin --key "$KEY"

2. Encrypt all TCL files in a directory:
   find ./scripts -name "*.tcl" -exec python3 tools/encrypt_tcl.py {} {}.bin --key "$KEY" \;

## Use with OpenOCD

1. Direct key (not recommended for production):
   openocd --crypto-key "$KEY" -D ./encrypted_dir -f config.cfg

2. Key from environment variable (recommended):
   export OPENOCD_KEY="$KEY"
   openocd --crypto-key-env OPENOCD_KEY -D ./encrypted_dir -f config.cfg

3. Key from file (most secure):
   echo -n "$KEY" > /secure/keyfile
   chmod 600 /secure/keyfile
   openocd --crypto-key-file /secure/keyfile -D ./encrypted_dir -f config.cfg

## File naming convention

- Original: script.tcl
- Encrypted: script.tcl.bin
- In VFS: script.tcl (automatic .bin removal)

When you load encrypted_dir/ with -D flag:
- Files: script.tcl.bin, config.tcl.bin
- VFS paths: script.tcl, config.tcl
- source script.tcl  → works (loaded from VFS, decrypted)
EOF

cat README_CRYPTO.txt
echo

echo "======================================"
echo "All tests passed! ✓"
echo "======================================"
echo
echo "Test directory: $TEST_DIR"
echo "Encrypted files ready for testing with OpenOCD"
echo
echo "To test with OpenOCD (after building):"
echo "  cd $TEST_DIR"
echo "  openocd --crypto-key \"$KEY\" -D ./encrypted_dir -f yourconfig.cfg"
echo

# Cleanup instructions
echo "To cleanup: rm -rf $TEST_DIR"
