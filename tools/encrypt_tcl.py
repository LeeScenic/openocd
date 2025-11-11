#!/usr/bin/env python3
"""
TCL File Encryption Tool for OpenOCD Memory VFS

This script encrypts TCL files using AES-128-CBC encryption with double hex encoding.
The encrypted files are saved with .bin extension and can be loaded by OpenOCD's
Memory VFS with decryption support.

This version uses the openssl command-line tool for encryption, requiring no Python dependencies.

Encryption Process:
1. Read original file content
2. Convert to hex (first encoding)
3. Pad to 16-byte blocks with zero bytes
4. Encrypt using AES-128-CBC (via openssl)
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
    find ./scripts -name "*.tcl" -exec python3 encrypt_tcl.py {} {}.bin --key "keyskeyskeyskeys" \\;
    
    # Use with OpenOCD
    openocd --crypto-key "keyskeyskeyskeys" -D ./encrypted_scripts -f config.cfg
"""

import sys
import os
import argparse
import binascii
import subprocess
import tempfile


def check_openssl():
    """Check if openssl is available."""
    try:
        subprocess.run(['openssl', 'version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def hex_encode(data):
    """Convert bytes to hex string."""
    return binascii.hexlify(data)


def hex_decode(hex_data):
    """Convert hex string to bytes."""
    return binascii.unhexlify(hex_data)


def pad_data(data, block_size=16):
    """Pad data to block_size with zero bytes."""
    padding_len = (block_size - len(data) % block_size) % block_size
    if padding_len > 0:
        data = data + b'\x00' * padding_len
    return data


def unpad_data(data):
    """Remove zero-byte padding."""
    return data.rstrip(b'\x00')


def aes_encrypt(data, key):
    """
    Encrypt data using AES-128-CBC with openssl.
    Uses key as both key and IV.
    """
    # Create temporary files for openssl
    with tempfile.NamedTemporaryFile(delete=False) as tmp_in:
        tmp_in.write(data)
        tmp_in_path = tmp_in.name
    
    with tempfile.NamedTemporaryFile(delete=False) as tmp_out:
        tmp_out_path = tmp_out.name
    
    with tempfile.NamedTemporaryFile(delete=False) as tmp_key:
        tmp_key.write(key)
        tmp_key_path = tmp_key.name
    
    try:
        # Convert key to hex for openssl
        key_hex = binascii.hexlify(key).decode('ascii')
        
        # Run openssl encryption
        # -K: key in hex, -iv: IV in hex, -nopad: no additional padding
        cmd = [
            'openssl', 'enc', '-aes-128-cbc',
            '-K', key_hex,
            '-iv', key_hex,  # Use key as IV
            '-nopad',  # We already padded
            '-in', tmp_in_path,
            '-out', tmp_out_path
        ]
        
        result = subprocess.run(cmd, capture_output=True, check=True)
        
        # Read encrypted data
        with open(tmp_out_path, 'rb') as f:
            encrypted = f.read()
        
        return encrypted
    
    finally:
        # Clean up temporary files
        try:
            os.unlink(tmp_in_path)
            os.unlink(tmp_out_path)
            os.unlink(tmp_key_path)
        except:
            pass


def aes_decrypt(encrypted_data, key):
    """
    Decrypt data using AES-128-CBC with openssl.
    Uses key as both key and IV.
    """
    # Create temporary files for openssl
    with tempfile.NamedTemporaryFile(delete=False) as tmp_in:
        tmp_in.write(encrypted_data)
        tmp_in_path = tmp_in.name
    
    with tempfile.NamedTemporaryFile(delete=False) as tmp_out:
        tmp_out_path = tmp_out.name
    
    try:
        # Convert key to hex for openssl
        key_hex = binascii.hexlify(key).decode('ascii')
        
        # Run openssl decryption
        cmd = [
            'openssl', 'enc', '-aes-128-cbc', '-d',
            '-K', key_hex,
            '-iv', key_hex,  # Use key as IV
            '-nopad',  # We handle padding ourselves
            '-in', tmp_in_path,
            '-out', tmp_out_path
        ]
        
        result = subprocess.run(cmd, capture_output=True, check=True)
        
        # Read decrypted data
        with open(tmp_out_path, 'rb') as f:
            decrypted = f.read()
        
        return decrypted
    
    finally:
        # Clean up temporary files
        try:
            os.unlink(tmp_in_path)
            os.unlink(tmp_out_path)
        except:
            pass


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
    hex_data = hex_encode(data)
    print(f"After hex encode: {len(hex_data)} bytes")
    
    # Step 2: Pad to 16-byte blocks with zero bytes
    padded_data = pad_data(hex_data, 16)
    print(f"After padding: {len(padded_data)} bytes")
    
    # Step 3: Encrypt using AES-128-CBC (use key as IV)
    encrypted = aes_encrypt(padded_data, key)
    print(f"After encryption: {len(encrypted)} bytes")
    
    # Step 4: Convert to hex again (second encoding)
    hex_encrypted = hex_encode(encrypted)
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
    encrypted = hex_decode(hex_encrypted)
    print(f"After hex decode: {len(encrypted)} bytes")
    
    # Step 2: Decrypt using AES-128-CBC
    decrypted = aes_decrypt(encrypted, key)
    print(f"After decryption: {len(decrypted)} bytes")
    
    # Step 3: Remove zero-byte padding
    decrypted = unpad_data(decrypted)
    print(f"After removing padding: {len(decrypted)} bytes")
    
    # Step 4: Hex decode (first encoding)
    original = hex_decode(decrypted)
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
    
    # Check if openssl is available
    if not check_openssl():
        print("Error: openssl command not found. Please install openssl.", file=sys.stderr)
        print("On Debian/Ubuntu: sudo apt-get install openssl", file=sys.stderr)
        return 1
    
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
