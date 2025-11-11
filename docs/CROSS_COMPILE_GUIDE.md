# OpenOCD 交叉编译指南

## 📖 概述

OpenOCD 是架构相关的，需要为目标平台编译特定的二进制文件。

### 支持的架构

- **x86_64** (AMD64, Intel 64-bit)
- **ARM** (32-bit, ARMv7, ARMv8)
- **AArch64** (ARM 64-bit)
- **MIPS**
- **RISC-V**
- 其他 Linux 支持的架构

---

## 🎯 方案选择

### 方案 1: 本地编译（推荐）⭐

**适用场景**: 你有 ARM 开发板/设备，可以直接在上面编译

**优点**:
- ✅ 简单直接
- ✅ 不需要交叉编译工具链
- ✅ 自动匹配目标系统
- ✅ 避免库依赖问题

**缺点**:
- ⚠️ ARM 设备编译可能较慢
- ⚠️ 需要在目标设备上安装开发工具

### 方案 2: 交叉编译

**适用场景**: 在 x86 机器上编译 ARM 版本

**优点**:
- ✅ 编译速度快（x86 性能好）
- ✅ 可以批量编译多个架构

**缺点**:
- ⚠️ 配置复杂
- ⚠️ 容易出现库依赖问题
- ⚠️ 需要安装交叉编译工具链

---

## 📝 方案 1: ARM 设备上本地编译

### 步骤 1: 准备 ARM 设备

```bash
# 检查架构
uname -m
# 输出示例:
# - armv7l (32-bit ARM)
# - aarch64 (64-bit ARM)
# - x86_64 (Intel/AMD 64-bit)

# 检查系统信息
cat /proc/cpuinfo | grep -i "model name\|processor" | head -5
```

### 步骤 2: 安装开发工具

```bash
# 更新包管理器
sudo apt-get update

# 安装基础编译工具
sudo apt-get install -y \
    build-essential \
    autoconf \
    automake \
    libtool \
    pkg-config \
    git

# 安装 OpenOCD 依赖
sudo apt-get install -y \
    libusb-1.0-0-dev \
    libftdi1-dev \
    libhidapi-dev \
    libgpiod-dev

# 可选: 安装 openssl (用于加密工具)
sudo apt-get install -y openssl
```

### 步骤 3: 获取源码

```bash
# 克隆仓库
git clone https://github.com/LeeScenic/openocd.git
cd openocd

# 切换到功能分支
git checkout feature/memory-vfs-protection

# 初始化子模块
git submodule update --init --recursive
```

### 步骤 4: 编译 jimtcl

```bash
cd jimtcl
./configure --disable-lineedit
make -j$(nproc)
cd ..
```

### 步骤 5: 配置 OpenOCD

```bash
# 运行 bootstrap
./bootstrap

# 配置（根据需要选择）
# 基础配置
./configure --enable-internal-jimtcl

# 或带 FTDI 支持
./configure --enable-ftdi --enable-internal-jimtcl

# 或完整配置
./configure \
    --enable-ftdi \
    --enable-stlink \
    --enable-ti-icdi \
    --enable-ulink \
    --enable-usb-blaster-2 \
    --enable-ft232r \
    --enable-vsllink \
    --enable-xds110 \
    --enable-cmsis-dap \
    --enable-osbdm \
    --enable-opendous \
    --enable-jlink \
    --enable-rlink \
    --enable-usbprog \
    --enable-armjtagew \
    --enable-buspirate \
    --enable-internal-jimtcl
```

### 步骤 6: 编译

```bash
# 编译（使用所有 CPU 核心）
make -j$(nproc)

# 编译可能需要 10-30 分钟（取决于 ARM 设备性能）

# 可选: 安装到系统
sudo make install
```

### 步骤 7: 验证

```bash
# 检查二进制文件架构
file src/openocd

# 应该显示类似:
# ARM aarch64, version 1 (SYSV)
# 或 ARM, EABI5 version 1 (SYSV)

# 测试运行
./src/openocd --version
```

---

## 🔧 方案 2: x86 上交叉编译 ARM 版本

### 步骤 1: 安装交叉编译工具链

#### For 32-bit ARM (ARMv7)

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf

# Fedora/RHEL
sudo dnf install -y \
    gcc-arm-linux-gnu \
    binutils-arm-linux-gnu
```

#### For 64-bit ARM (AArch64)

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu

# Fedora/RHEL
sudo dnf install -y \
    gcc-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu
```

### 步骤 2: 安装目标架构的库

```bash
# 添加目标架构（以 arm64 为例）
sudo dpkg --add-architecture arm64
sudo apt-get update

# 安装 ARM64 版本的依赖库
sudo apt-get install -y \
    libusb-1.0-0-dev:arm64 \
    libftdi1-dev:arm64 \
    libhidapi-dev:arm64

# 注意: 这可能在某些系统上不可行
# 如果失败，需要手动编译依赖库
```

### 步骤 3: 配置交叉编译

```bash
cd openocd

# 设置环境变量
export CROSS_COMPILE=aarch64-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export AR=${CROSS_COMPILE}ar
export RANLIB=${CROSS_COMPILE}ranlib
export STRIP=${CROSS_COMPILE}strip

# 配置
./bootstrap
./configure \
    --host=aarch64-linux-gnu \
    --build=x86_64-linux-gnu \
    --enable-internal-jimtcl \
    --enable-ftdi \
    LDFLAGS="-static" \
    --prefix=/opt/openocd-arm64
```

### 步骤 4: 编译

```bash
make -j$(nproc)

# 验证架构
file src/openocd
# 应该显示: ARM aarch64
```

### 常见问题

#### 问题 1: 找不到库

```bash
# 解决方案: 静态链接
./configure --enable-static --disable-shared ...

# 或手动指定库路径
./configure LDFLAGS="-L/usr/aarch64-linux-gnu/lib" ...
```

#### 问题 2: jimtcl 编译失败

```bash
# 需要先交叉编译 jimtcl
cd jimtcl
./configure \
    --host=aarch64-linux-gnu \
    --build=x86_64-linux-gnu
make
cd ..
```

---

## 📦 方案 3: 使用容器/QEMU（最简单的交叉编译）

### 使用 Docker + QEMU

```bash
# 安装 QEMU 用户模式
sudo apt-get install qemu-user-static

# 启用 ARM 容器支持
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 使用 ARM64 容器编译
docker run --rm -it \
    -v $(pwd):/workspace \
    arm64v8/debian:bullseye \
    /bin/bash

# 在容器内执行编译步骤
cd /workspace
apt-get update
apt-get install -y build-essential autoconf libtool pkg-config \
    libusb-1.0-0-dev libftdi1-dev libhidapi-dev git

# 按照方案 1 的步骤编译
```

---

## 🎯 推荐方案对比

| 方案 | 难度 | 速度 | 可靠性 | 推荐度 |
|------|------|------|--------|--------|
| ARM 设备上编译 | ⭐ 简单 | ⭐⭐ 较慢 | ⭐⭐⭐ 高 | ⭐⭐⭐⭐⭐ |
| 交叉编译 | ⭐⭐⭐ 复杂 | ⭐⭐⭐ 快 | ⭐⭐ 中 | ⭐⭐ |
| Docker+QEMU | ⭐⭐ 中等 | ⭐⭐ 较慢 | ⭐⭐⭐ 高 | ⭐⭐⭐⭐ |

---

## 📊 不同架构的二进制文件识别

```bash
# x86_64
file openocd
# 输出: ELF 64-bit LSB executable, x86-64

# ARM 32-bit
file openocd
# 输出: ELF 32-bit LSB executable, ARM, EABI5

# ARM 64-bit (AArch64)
file openocd
# 输出: ELF 64-bit LSB executable, ARM aarch64

# MIPS
file openocd
# 输出: ELF 32-bit MSB executable, MIPS

# RISC-V
file openocd
# 输出: ELF 64-bit LSB executable, UCB RISC-V
```

---

## 🔍 检查当前系统架构

```bash
# 方法 1: uname
uname -m
# x86_64, armv7l, aarch64, mips, riscv64

# 方法 2: dpkg (Debian/Ubuntu)
dpkg --print-architecture
# amd64, armhf, arm64

# 方法 3: file
file /bin/ls
# 显示系统二进制文件架构

# 方法 4: lscpu
lscpu | grep Architecture
# Architecture: x86_64
# Architecture: aarch64
```

---

## 💡 最佳实践

### 1. 为目标设备编译

- ✅ 始终在目标架构设备上编译（如果可能）
- ✅ 或使用相同架构的容器环境

### 2. 静态链接（可移植性）

```bash
./configure --enable-static --disable-shared ...
```

这样编译的二进制文件包含所有依赖，可以在没有开发库的系统上运行。

### 3. 打包分发

```bash
# 创建架构特定的发布包
make install DESTDIR=/tmp/openocd-arm64
cd /tmp
tar -czf openocd-arm64-v0.12.0.tar.gz openocd-arm64/
```

### 4. 测试不同架构

```bash
# 使用 QEMU 测试 ARM 二进制
qemu-aarch64-static ./openocd-arm64 --version
```

---

## 🚀 快速参考

### 树莓派 (ARM)

```bash
# Raspberry Pi 4 (ARMv7 或 ARMv8)
uname -m  # armv7l 或 aarch64

# 按方案 1 编译即可
```

### Orange Pi / Banana Pi

```bash
# 通常是 ARM Cortex-A 系列
# 按方案 1 编译
```

### NVIDIA Jetson (ARM64)

```bash
# AArch64 架构
uname -m  # aarch64

# 按方案 1 编译
```

---

## 📞 获取帮助

如果遇到编译问题:

1. 检查架构: `uname -m`
2. 查看编译日志: `make V=1`
3. 查看配置日志: `cat config.log | less`
4. 检查依赖: `ldd src/openocd`

---

**总结**: 对于 ARM 设备，最简单可靠的方法是直接在 ARM 设备上编译，避免交叉编译的复杂性。
