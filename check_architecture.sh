#!/bin/bash
# Architecture Detection Script for OpenOCD

echo "========================================"
echo "OpenOCD Architecture Detection"
echo "========================================"
echo ""

# Detect current system architecture
ARCH=$(uname -m)
echo "Current System Architecture: $ARCH"
echo ""

# Classify architecture
case $ARCH in
    x86_64|amd64)
        ARCH_TYPE="x86-64 (Intel/AMD 64-bit)"
        COMPATIBLE_WITH="x86_64 binaries"
        ;;
    i686|i386)
        ARCH_TYPE="x86 (Intel/AMD 32-bit)"
        COMPATIBLE_WITH="i386/i686 binaries"
        ;;
    aarch64|arm64)
        ARCH_TYPE="ARM 64-bit (AArch64)"
        COMPATIBLE_WITH="arm64/aarch64 binaries"
        ;;
    armv7l|armv7*)
        ARCH_TYPE="ARM 32-bit (ARMv7)"
        COMPATIBLE_WITH="armhf/armv7l binaries"
        ;;
    armv6l|armv6*)
        ARCH_TYPE="ARM 32-bit (ARMv6)"
        COMPATIBLE_WITH="armel/armv6l binaries"
        ;;
    mips|mips64)
        ARCH_TYPE="MIPS"
        COMPATIBLE_WITH="mips binaries"
        ;;
    riscv64)
        ARCH_TYPE="RISC-V 64-bit"
        COMPATIBLE_WITH="riscv64 binaries"
        ;;
    *)
        ARCH_TYPE="Unknown"
        COMPATIBLE_WITH="Unknown"
        ;;
esac

echo "Architecture Type: $ARCH_TYPE"
echo "Compatible With: $COMPATIBLE_WITH"
echo ""

# Check if OpenOCD binary exists
if [ -f "src/openocd" ]; then
    echo "========================================"
    echo "Checking Compiled OpenOCD Binary"
    echo "========================================"
    echo ""
    
    # Get binary architecture
    BINARY_INFO=$(file src/openocd)
    echo "Binary Info: $BINARY_INFO"
    echo ""
    
    # Extract architecture from file output
    if echo "$BINARY_INFO" | grep -q "x86-64"; then
        BINARY_ARCH="x86-64"
    elif echo "$BINARY_INFO" | grep -q "x86"; then
        BINARY_ARCH="x86"
    elif echo "$BINARY_INFO" | grep -q "aarch64"; then
        BINARY_ARCH="ARM 64-bit"
    elif echo "$BINARY_INFO" | grep -q "ARM"; then
        BINARY_ARCH="ARM 32-bit"
    elif echo "$BINARY_INFO" | grep -q "MIPS"; then
        BINARY_ARCH="MIPS"
    elif echo "$BINARY_INFO" | grep -q "RISC-V"; then
        BINARY_ARCH="RISC-V"
    else
        BINARY_ARCH="Unknown"
    fi
    
    echo "Binary Architecture: $BINARY_ARCH"
    echo ""
    
    # Check compatibility
    echo "========================================"
    echo "Compatibility Check"
    echo "========================================"
    echo ""
    
    COMPATIBLE=false
    case $ARCH in
        x86_64|amd64)
            if echo "$BINARY_INFO" | grep -q "x86-64"; then
                COMPATIBLE=true
            fi
            ;;
        aarch64|arm64)
            if echo "$BINARY_INFO" | grep -q "aarch64"; then
                COMPATIBLE=true
            fi
            ;;
        armv7*|armv6*)
            if echo "$BINARY_INFO" | grep -q "ARM" && ! echo "$BINARY_INFO" | grep -q "aarch64"; then
                COMPATIBLE=true
            fi
            ;;
    esac
    
    if $COMPATIBLE; then
        echo "✅ COMPATIBLE: Binary can run on this system"
    else
        echo "❌ NOT COMPATIBLE: Binary cannot run on this system"
        echo ""
        echo "Solutions:"
        echo "  1. Compile on target device (recommended)"
        echo "  2. Cross-compile for $ARCH_TYPE"
        echo "  3. Use Docker + QEMU emulation"
        echo ""
        echo "See docs/CROSS_COMPILE_GUIDE.md for details"
    fi
    echo ""
    
    # Try to get version (will fail if not compatible)
    if $COMPATIBLE; then
        echo "========================================"
        echo "Testing Binary"
        echo "========================================"
        echo ""
        if ./src/openocd --version 2>&1 | head -3; then
            echo ""
            echo "✅ Binary works correctly!"
        else
            echo "❌ Binary failed to execute"
        fi
    fi
else
    echo "OpenOCD binary not found at: src/openocd"
    echo "You need to compile OpenOCD first."
    echo ""
    echo "Quick compilation steps:"
    echo "  ./bootstrap"
    echo "  ./configure --enable-ftdi --enable-internal-jimtcl"
    echo "  make -j\$(nproc)"
fi

echo ""
echo "========================================"
echo "System Information"
echo "========================================"
echo ""
echo "Kernel: $(uname -r)"
echo "OS: $(cat /etc/os-release 2>/dev/null | grep PRETTY_NAME | cut -d= -f2 | tr -d '\"' || echo 'Unknown')"
echo "CPU Info:"
if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i686" ]; then
    grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | xargs
else
    grep "^model name\|^Processor\|^Hardware" /proc/cpuinfo | head -1 | cut -d: -f2 | xargs
fi
echo ""

echo "For cross-compilation guide, see:"
echo "  docs/CROSS_COMPILE_GUIDE.md"
echo ""
