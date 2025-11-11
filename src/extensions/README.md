# OpenOCD Extensions

This directory contains extended features that are not part of the core OpenOCD functionality. These extensions provide additional capabilities while maintaining separation from the core codebase.

## 📦 Available Extensions

### Memory Virtual File System (Memory VFS)

**Purpose**: Protect TCL script content from disk exposure by loading them into memory.

**Files**:
- `vfs_memory.h` / `vfs_memory.c` - Core memory VFS implementation
- `jim_source_hook.h` / `jim_source_hook.c` - Jim Tcl `source` command hook
- `jim_vfs_wrapper.h` / `jim_vfs_wrapper.c` - Jim_EvalFile VFS wrapper

**Features**:
- ✅ Hash table-based in-memory file storage (O(1) lookup)
- ✅ Recursive directory loading with `-D` option
- ✅ Directory hierarchy preservation
- ✅ Transparent file access (automatic fallback to disk)
- ✅ Zero modification required for existing scripts

**Command-Line Option**:
```bash
openocd -D <directory> -f config.cfg
```

**Documentation**: See [MEMORY_VFS_FEATURE.md](../../MEMORY_VFS_FEATURE.md) and [VFS_USAGE_GUIDE.md](../../VFS_USAGE_GUIDE.md)

---

### Encryption Support for Memory VFS

**Purpose**: Provide encryption/decryption capabilities for secure TCL script storage.

**Files**:
- `vfs_crypto.h` / `vfs_crypto.c` - Cryptography framework
- `aes.h` / `aes.c` - AES-128 implementation (tiny-AES-c)

**Features**:
- ✅ AES-128-CBC encryption/decryption
- ✅ Double-hex encoding (matches Python encryption script)
- ✅ Flexible key management:
  - Command line: `--crypto-key <key>`
  - Environment variable: `--crypto-key-env <var>`
  - Key file: `--crypto-key-file <file>`
  - Callback function (for custom key retrieval)
- ✅ Automatic `.bin` file detection and decryption
- ✅ Zero external dependencies (uses tiny-AES-c)

**Command-Line Options**:
```bash
# Direct key (testing only)
openocd --crypto-key "keyskeyskeyskeys" -D ./encrypted -f config.cfg

# Environment variable (recommended)
export OPENOCD_CRYPTO_KEY="keyskeyskeyskeys"
openocd --crypto-key-env OPENOCD_CRYPTO_KEY -D ./encrypted -f config.cfg

# Key file (most secure)
openocd --crypto-key-file /secure/keyfile -D ./encrypted -f config.cfg
```

**Encryption Tool**:
```bash
# Encrypt TCL files
python3 tools/encrypt_tcl.py script.tcl script.tcl.bin --key "keyskeyskeyskeys"
```

**Documentation**: See [CRYPTO_USAGE_GUIDE.md](../../CRYPTO_USAGE_GUIDE.md)

---

## 🏗️ Architecture

### Integration Points

The extensions integrate with OpenOCD through:

1. **Command Line Options** (`src/helper/options.c`)
   - `-D` / `--directory-to-memory` - Load directory to memory VFS
   - `--crypto-key` / `--crypto-key-env` / `--crypto-key-file` - Set encryption key

2. **Configuration System** (`src/helper/configuration.c`)
   - `find_file()` checks memory VFS before disk

3. **Command System** (`src/helper/command.c`)
   - Installs Jim Tcl hooks during initialization
   - Cleanup on exit

### Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ User starts OpenOCD with -D and --crypto-key-env options       │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ options.c: Parse command line                                   │
│   • Initialize VFS                                              │
│   • Load encryption key                                         │
│   • Load directory into memory                                  │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ vfs_memory.c: Load files from disk                              │
│   • Detect .bin files                                           │
│   • Call vfs_crypto_decrypt() for encrypted files              │
│   • Store decrypted content in hash table                      │
│   • Remove .bin extension from VFS path                        │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ command.c: Install Jim Tcl hooks                                │
│   • Hook 'source' command (jim_source_hook.c)                  │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ Runtime: User TCL script executes                               │
│   • 'source file.tcl' → jim_source_hook intercepts             │
│   • Check VFS first (vfs_memory_read_file)                     │
│   • If found in VFS: execute from memory                       │
│   • If not found: fallback to original source command (disk)   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Building

The extensions are automatically built as part of the OpenOCD build process.

### Build Configuration

The extensions library is defined in `Makefile.am` and linked into the main OpenOCD binary.

```makefile
# In src/extensions/Makefile.am
noinst_LTLIBRARIES += %D%/libextensions.la

%C%_libextensions_la_SOURCES = \
    %D%/vfs_memory.c \
    %D%/vfs_crypto.c \
    %D%/aes.c \
    ...
```

### Include Path

Extensions can access helper headers:
```c
#include "log.h"           // From src/helper/
#include "configuration.h" // From src/helper/
#include "vfs_memory.h"    // From src/extensions/
```

---

## 🧪 Testing

### Test Scripts

Located in the repository root:

- `test_vfs.sh` - Basic VFS functionality tests
- `test_no_conflict.sh` - Verify no conflicts with jimtcl
- `test_path_matching.sh` - Path matching tests
- `test_hierarchy.sh` - Directory hierarchy tests
- `test_crypto.sh` - Encryption/decryption tests

### Running Tests

```bash
# Run all VFS tests
./test_vfs.sh
./test_no_conflict.sh
./test_path_matching.sh
./test_hierarchy.sh

# Run crypto tests
./test_crypto.sh
```

---

## 📚 Documentation

### User Documentation

- [VFS_USAGE_GUIDE.md](../../VFS_USAGE_GUIDE.md) - Memory VFS usage guide (Chinese)
- [CRYPTO_USAGE_GUIDE.md](../../CRYPTO_USAGE_GUIDE.md) - Encryption usage guide (Chinese)
- [QUICK_START.md](../../QUICK_START.md) - Quick start guide

### Technical Documentation

- [MEMORY_VFS_FEATURE.md](../../MEMORY_VFS_FEATURE.md) - VFS technical documentation
- [IMPLEMENTATION_SUMMARY.md](../../IMPLEMENTATION_SUMMARY.md) - Implementation summary
- [CRYPTO_IMPLEMENTATION_COMPLETE.md](../../CRYPTO_IMPLEMENTATION_COMPLETE.md) - Crypto completion report

### Design Documentation

- [NO_CONFLICT_EXPLANATION.md](../../NO_CONFLICT_EXPLANATION.md) - Why there's no conflict with jimtcl
- [HIERARCHY_PRESERVED.md](../../HIERARCHY_PRESERVED.md) - Directory hierarchy explanation
- [WHAT_CHANGED.md](../../WHAT_CHANGED.md) - Breaking changes summary

---

## 🔐 Security Considerations

### Memory VFS Security

- Files stored in memory are cleared on exit
- No permanent traces left on disk (except original source files)
- Useful for preventing script content exposure in untrusted environments

### Encryption Security

- **Key Management**: Use environment variables or key files in production
- **Key Length**: 16 bytes for AES-128 (strictly enforced)
- **Key Storage**: Never hardcode keys in scripts or command line history
- **File Permissions**: Set restrictive permissions on key files (chmod 600)

### Best Practices

1. **Use Key Files for Production**:
   ```bash
   echo -n "keyskeyskeyskeys" > /secure/keyfile
   chmod 600 /secure/keyfile
   openocd --crypto-key-file /secure/keyfile -D ./encrypted -f config.cfg
   ```

2. **Environment Variables for CI/CD**:
   ```bash
   export OPENOCD_CRYPTO_KEY=$(vault read -field=value secret/openocd/key)
   openocd --crypto-key-env OPENOCD_CRYPTO_KEY -D ./encrypted -f config.cfg
   ```

3. **Avoid Command Line Keys**:
   ```bash
   # ❌ Bad: Key visible in process list and history
   openocd --crypto-key "mysecretkey" ...
   
   # ✅ Good: Use environment variable or file
   export KEY="mysecretkey"
   openocd --crypto-key-env KEY ...
   ```

---

## 📊 Performance

### Memory VFS Performance

- **Initialization**: O(n) where n = number of files
- **File Lookup**: O(1) average case (hash table with 256 buckets)
- **Memory Overhead**: ~equal to total file size + hash table overhead
- **Runtime Impact**: Minimal (faster than disk access)

### Encryption Performance

- **Decryption**: One-time cost during initialization
- **Runtime Impact**: Zero (files already decrypted in memory)
- **Typical Overhead**: ~100ms for 245 files (~187KB)

---

## 🤝 Contributing

### Adding New Extensions

1. Create source files in `src/extensions/`
2. Update `src/extensions/Makefile.am`
3. Add documentation to this README
4. Create test scripts
5. Update main documentation files

### Code Style

Follow OpenOCD coding standards:
- Use tabs for indentation
- Add SPDX license headers
- Include comprehensive comments
- Use `LOG_*` macros for logging

---

## 📝 License

All extensions follow the OpenOCD licensing:

```
SPDX-License-Identifier: GPL-2.0-or-later
```

See [COPYING](../../COPYING) for full license text.

---

## 📞 Support

For issues or questions:

1. Check the documentation files in the repository root
2. Run test scripts to verify functionality
3. Review code comments in source files
4. Submit issues on GitHub

---

**Last Updated**: 2025-11-10  
**Maintainer**: OpenOCD Contributors
