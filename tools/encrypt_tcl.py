#!/usr/bin/env python3
"""
TCL File Encryption Tool for OpenOCD Memory VFS

This script encrypts TCL files using AES-128-CBC encryption with double hex encoding.
The encrypted files are saved with .bin extension and can be loaded by OpenOCD's
Memory VFS with decryption support.

Encryption Process:
1. Read original file content
2. Convert to hex (first encoding)
3. Pad to 16-byte blocks with zero bytes
4. Encrypt using AES-128-CBC
5. Convert to hex again (second encoding)
6. Write to .bin file

Usage:
    python3 encrypt_tcl.py <input_file> <output_file> --key <encryption_key>
    python3 encrypt_tcl.py input.tcl output.tcl.bin --key "keyskeyskeyskeys"
    python3 encrypt_tcl.py input.tcl output.tcl.bin --key-file key.txt

Example:
    # Encrypt a single file
    python3 encrypt_tcl.py test.tcl test.tcl.bin --key "keyskeyskeyskeys"
    
    # Encrypt all TCL files in a directory
    find ./scripts -name "*.tcl" -exec python3 encrypt_tcl.py {} {}.bin --key "keyskeyskeyskeys" \;
    
    # Use with OpenOCD
    openocd --crypto-key "keyskeyskeyskeys" -D ./encrypted_scripts -f config.cfg
"""

import sys
import os
import argparse
import binascii
from Crypto.Cipher import AES


def encrypt_file(input_file, output_file, key):
    """
    Encrypt a file using AES-128-CBC with double hex encoding.
    
    Args:
        input_file: Path to input file
        output_file: Path to output file (.bin)
        key: 16-byte encryption key
    """
    # Validate key length
    if len(key) != 16:
        raise ValueError(f"Key must be exactly 16 bytes, got {len(key)} bytes")
    
    # Read input file
    with open(input_file, 'rb') as f:
        data = f.read()
    
    print(f"Input file: {input_file} ({len(data)} bytes)")
    
    # Step 1: Convert to hex (first encoding)
    hex_data = binascii.hexlify(data)
    print(f"After hex encode: {len(hex_data)} bytes")
    
    # Step 2: Pad to 16-byte blocks with zero bytes
    padding_len = (16 - len(hex_data) % 16) % 16
    if padding_len > 0:
        hex_data = hex_data + b'\x00' * padding_len
    print(f"After padding: {len(hex_data)} bytes")
    
    # Step 3: Encrypt using AES-128-CBC (use key as IV)
    cipher = AES.new(key, AES.MODE_CBC, key)  # Use key as IV
    encrypted = cipher.encrypt(hex_data)
    print(f"After encryption: {len(encrypted)} bytes")
    
    # Step 4: Convert to hex again (second encoding)
    hex_encrypted = binascii.hexlify(encrypted)
    print(f"After hex encode: {len(hex_encrypted)} bytes")
    
    # Write output file
    with open(output_file, 'wb') as f:
        f.write(hex_encrypted)
    
    print(f"Output file: {output_file} ({len(hex_encrypted)} bytes)")
    print(f"Encryption successful!")


def decrypt_file(input_file, output_file, key):
    """
    Decrypt a file for verification purposes.
    
    Args:
        input_file: Path to encrypted .bin file
        output_file: Path to decrypted output file
        key: 16-byte encryption key
    """
    # Validate key length
    if len(key) != 16:
        raise ValueError(f"Key must be exactly 16 bytes, got {len(key)} bytes")
    
    # Read encrypted file
    with open(input_file, 'rb') as f:
        hex_encrypted = f.read()
    
    print(f"Encrypted file: {input_file} ({len(hex_encrypted)} bytes)")
    
    # Step 1: Hex decode (second encoding)
    encrypted = binascii.unhexlify(hex_encrypted)
    print(f"After hex decode: {len(encrypted)} bytes")
    
    # Step 2: Decrypt using AES-128-CBC
    cipher = AES.new(key, AES.MODE_CBC, key)  # Use key as IV
    decrypted = cipher.decrypt(encrypted)
    print(f"After decryption: {len(decrypted)} bytes")
    
    # Step 3: Remove zero-byte padding
    decrypted = decrypted.rstrip(b'\x00')
    print(f"After removing padding: {len(decrypted)} bytes")
    
    # Step 4: Hex decode (first encoding)
    original = binascii.unhexlify(decrypted)
    print(f"After hex decode: {len(original)} bytes")
    
    # Write output file
    with open(output_file, 'wb') as f:
        f.write(original)
    
    print(f"Decrypted file: {output_file} ({len(original)} bytes)")
    print(f"Decryption successful!")


def main():
    parser = argparse.ArgumentParser(
        description='Encrypt/Decrypt TCL files for OpenOCD Memory VFS',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Encrypt a file
  %(prog)s input.tcl output.tcl.bin --key "keyskeyskeyskeys"
  %(prog)s input.tcl output.tcl.bin --key-file key.txt
  
  # Decrypt a file (for verification)
  %(prog)s encrypted.tcl.bin decrypted.tcl --key "keyskeyskeyskeys" --decrypt
  
  # Use with OpenOCD
  openocd --crypto-key "keyskeyskeyskeys" -D ./encrypted_scripts -f config.cfg
  openocd --crypto-key-env OPENOCD_KEY -D ./encrypted_scripts -f config.cfg
  openocd --crypto-key-file key.txt -D ./encrypted_scripts -f config.cfg
        """
    )
    
    parser.add_argument('input', help='Input file path')
    parser.add_argument('output', help='Output file path')
    
    key_group = parser.add_mutually_exclusive_group(required=True)
    key_group.add_argument('--key', help='Encryption key (16 bytes)')
    key_group.add_argument('--key-file', help='File containing encryption key')
    
    parser.add_argument('--decrypt', action='store_true',
                        help='Decrypt instead of encrypt (for verification)')
    
    args = parser.parse_args()
    
    # Get encryption key
    if args.key:
        key = args.key.encode('utf-8')
    else:
        with open(args.key_file, 'rb') as f:
            key = f.read().strip()
    
    # Validate key length
    if len(key) != 16:
        print(f"Error: Key must be exactly 16 bytes, got {len(key)} bytes", file=sys.stderr)
        print(f"Current key: {key!r}", file=sys.stderr)
        return 1
    
    # Check input file exists
    if not os.path.exists(args.input):
        print(f"Error: Input file not found: {args.input}", file=sys.stderr)
        return 1
    
    try:
        if args.decrypt:
            decrypt_file(args.input, args.output, key)
        else:
            encrypt_file(args.input, args.output, key)
        return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
