# Memory VFS Feature for OpenOCD

## Overview

This feature adds an in-memory Virtual File System (VFS) to OpenOCD to protect TCL script content from being leaked to disk. When enabled, OpenOCD can load TCL scripts into memory at startup and execute them directly from memory, without requiring the files to exist on disk.

## Motivation

The original issue with Jim Tcl's `source` command is that it requires files to exist on disk to be executed. This creates a security concern where sensitive TCL script content could be exposed. By implementing an in-memory VFS, we can:

1. **Protect Script Content**: Scripts are loaded into memory and never written to disk
2. **Prevent Leakage**: No temporary files or disk access required for script execution
3. **Maintain Compatibility**: Falls back to disk access if file not found in memory

## Architecture

The implementation consists of several components:

### 1. Memory VFS Module (`vfs_memory.c/h`)
- Hash table-based file storage in memory
- Fast file lookup and retrieval
- Directory recursion support for bulk loading
- Path normalization for cross-platform compatibility

### 2. Jim Tcl Source Hook (`jim_source_hook.c/h`)
- Replaces Jim Tcl's built-in `source` command
- Checks memory VFS first before disk access
- Maintains full compatibility with original behavior

### 3. Configuration Support (`configuration.c`)
- Extended `find_file()` to check memory VFS
- Seamless integration with OpenOCD's file search paths

### 4. Command Line Interface (`options.c`)
- New `-D` / `--directory-to-memory` option
- Load entire directories into memory at startup

## Usage

### Command Line Option

```bash
# Load a single directory into memory
openocd -D /path/to/tcl/scripts -f board/stm32f4discovery.cfg

# Load multiple directories
openocd -D /path/to/scripts1 -D /path/to/scripts2 -f config.cfg

# Example: Load the standard tcl directory
openocd -D ./tcl -f board/stm32f4discovery.cfg
```

### How It Works

1. **Initialization**: When `-D <directory>` is specified, OpenOCD:
   - Initializes the memory VFS
   - Recursively loads all files from the specified directory into memory
   - Stores files with their relative paths

2. **Script Execution**: When a `source` command is executed:
   - Jim Tcl's hooked `source` command checks memory VFS first
   - If file exists in memory, it's executed directly from memory
   - If not found in memory, falls back to disk access (normal behavior)

3. **File Finding**: The `find` command (used by `script`):
   - Checks memory VFS first
   - Then checks search paths on disk
   - Returns the first match found

## Example Workflow

```bash
# 1. Load sensitive scripts into memory
openocd -D /secure/scripts -f /secure/scripts/my_config.cfg

# 2. During execution, all script commands work as normal:
#    source target/stm32f4x.cfg
#    source interface/stlink.cfg
#
#    These files are served from memory, not disk!

# 3. Files not in memory still work from disk:
#    source /other/path/custom.cfg
#    (Falls back to disk if not in memory)
```

## Security Benefits

1. **No Disk Footprint**: Scripts never touch disk during execution
2. **Memory Protection**: Scripts are stored in process memory only
3. **Selective Loading**: Choose which directories to protect
4. **Transparent Operation**: Existing scripts work without modification

## Implementation Details

### Memory VFS Hash Table

- **Size**: 256 buckets for efficient lookup
- **Collision Handling**: Linked list chaining
- **Path Normalization**: Handles `/`, `\`, and relative paths
- **Memory Management**: Proper allocation and cleanup

### File Loading

```c
int vfs_memory_load_directory(const char *directory_path);
```

- Recursively scans directory
- Loads all regular files
- Stores with relative paths from base directory
- Reports statistics (file count, total size)

### Hook Installation

```c
int jim_source_hook_install(Jim_Interp *interp);
```

- Saves reference to original `source` command
- Installs VFS-aware version
- Maintains full backward compatibility

## Performance Considerations

- **Fast Lookup**: O(1) average case hash table lookup
- **Memory Usage**: Approximately 1:1 ratio with file sizes
- **Startup Time**: Linear with directory size (O(n) files)
- **Runtime**: No disk I/O for in-memory files

## Testing

To test the feature:

```bash
# 1. Build OpenOCD with the new feature
./bootstrap
./configure
make

# 2. Create test scripts
mkdir -p test_vfs
echo 'puts "Hello from memory VFS!"' > test_vfs/test.tcl

# 3. Run OpenOCD with memory VFS
src/openocd -D test_vfs -c "source test.tcl" -c "shutdown"

# 4. Verify it works without disk access
rm test_vfs/test.tcl
src/openocd -D test_vfs -c "source test.tcl" -c "shutdown"
# Should still work because file is in memory!
```

## Limitations

1. **Memory Usage**: Large script directories consume RAM
2. **No Dynamic Updates**: Files loaded at startup only
3. **Read-Only**: Memory VFS is read-only (no write support)

## Future Enhancements

Possible future improvements:

1. **Encrypted Loading**: Decrypt scripts as they're loaded into memory
2. **Compression**: Compress scripts in memory to save RAM
3. **Dynamic Loading**: API to load/unload scripts at runtime
4. **Write Support**: Allow creating files in memory VFS
5. **Statistics**: Runtime statistics on VFS usage

## API Reference

### Initialization

```c
int vfs_memory_init(void);
void vfs_memory_cleanup(void);
```

### File Operations

```c
int vfs_memory_add_file(const char *virtual_path, const char *content, size_t size);
int vfs_memory_read_file(const char *virtual_path, const char **content, size_t *size);
bool vfs_memory_file_exists(const char *virtual_path);
int vfs_memory_remove_file(const char *virtual_path);
```

### Directory Operations

```c
int vfs_memory_load_directory(const char *directory_path);
```

### Statistics

```c
void vfs_memory_get_stats(int *file_count, size_t *total_size);
void vfs_memory_set_debug(bool enable);
```

## Contributing

Contributions are welcome! Please ensure:

1. Code follows OpenOCD style guidelines
2. Memory leaks are avoided (valgrind clean)
3. Backward compatibility is maintained
4. Documentation is updated

## License

This feature is part of OpenOCD and is licensed under GPL-2.0-or-later.

## Authors

- OpenOCD Contributors (2024)
