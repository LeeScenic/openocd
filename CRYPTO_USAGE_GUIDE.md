# OpenOCD Memory VFS 加密功能使用指南

## 📖 功能概述

OpenOCD Memory VFS 现在支持**加密文件自动解密**功能。您可以将 TCL 脚本加密为 `.bin` 文件，OpenOCD 在加载时会自动解密并存入内存 VFS。

### ✨ 核心特性

- 🔐 **AES-128-CBC 加密**: 使用标准 AES-128-CBC 算法
- 🔄 **双重 Hex 编码**: 原始内容经过两次 hex 编码
- 🔑 **灵活密钥管理**: 支持命令行、环境变量、文件等多种密钥来源
- 🎯 **自动检测**: `.bin` 文件自动尝试解密
- 🗂️ **透明使用**: 解密后的文件在 VFS 中自动去除 `.bin` 扩展名

---

## 🚀 快速开始

### 1. 加密 TCL 文件

```bash
# 使用提供的加密工具
python3 tools/encrypt_tcl.py input.tcl output.tcl.bin --key "keyskeyskeyskeys"
```

**加密过程:**
```
原始文件 → hex编码 → 零填充 → AES-128-CBC加密 → hex编码 → .bin文件
```

### 2. 使用加密文件

**方式 1: 命令行直接提供密钥 (不推荐用于生产)**
```bash
openocd --crypto-key "keyskeyskeyskeys" -D ./encrypted_scripts -f config.cfg
```

**方式 2: 环境变量 (推荐)**
```bash
export OPENOCD_CRYPTO_KEY="keyskeyskeyskeys"
openocd --crypto-key-env OPENOCD_CRYPTO_KEY -D ./encrypted_scripts -f config.cfg
```

**方式 3: 密钥文件 (最安全)**
```bash
echo -n "keyskeyskeyskeys" > /secure/keyfile
chmod 600 /secure/keyfile
openocd --crypto-key-file /secure/keyfile -D ./encrypted_scripts -f config.cfg
```

---

## 📝 详细使用说明

### 加密工具使用

#### 基本语法

```bash
python3 tools/encrypt_tcl.py <input_file> <output_file> [options]
```

#### 选项说明

| 选项 | 说明 | 示例 |
|------|------|------|
| `--key <key>` | 直接指定密钥 (16字节) | `--key "keyskeyskeyskeys"` |
| `--key-file <file>` | 从文件读取密钥 | `--key-file key.txt` |
| `--decrypt` | 解密模式 (用于验证) | `--decrypt` |

#### 使用示例

**加密单个文件:**
```bash
python3 tools/encrypt_tcl.py script.tcl script.tcl.bin --key "keyskeyskeyskeys"
```

**批量加密目录中的所有 TCL 文件:**
```bash
find ./scripts -name "*.tcl" -exec \
  python3 tools/encrypt_tcl.py {} {}.bin --key "keyskeyskeyskeys" \;
```

**使用密钥文件:**
```bash
echo -n "keyskeyskeyskeys" > mykey.txt
python3 tools/encrypt_tcl.py script.tcl script.tcl.bin --key-file mykey.txt
```

**验证加密 (解密回原文件):**
```bash
python3 tools/encrypt_tcl.py encrypted.tcl.bin decrypted.tcl \
  --key "keyskeyskeyskeys" --decrypt
```

---

### OpenOCD 命令行选项

#### 新增的加密相关选项

| 选项 | 说明 | 示例 |
|------|------|------|
| `--crypto-key <key>` | 直接指定密钥 | `--crypto-key "keyskeyskeyskeys"` |
| `--crypto-key-env <var>` | 从环境变量读取密钥 | `--crypto-key-env OPENOCD_KEY` |
| `--crypto-key-file <file>` | 从文件读取密钥 | `--crypto-key-file /secure/key` |

#### 完整使用示例

**组合使用加密和内存 VFS:**
```bash
# 1. 设置密钥环境变量
export OPENOCD_CRYPTO_KEY="keyskeyskeyskeys"

# 2. 启动 OpenOCD，加载加密脚本目录
openocd \
  --crypto-key-env OPENOCD_CRYPTO_KEY \
  -D ./encrypted_scripts \
  -f board/stm32f4discovery.cfg
```

**多目录加载:**
```bash
openocd \
  --crypto-key-file /secure/keyfile \
  -D ./encrypted_scripts \
  -D ./more_encrypted_scripts \
  -f config.cfg
```

---

## 🔧 工作原理

### 加密格式

**Python 加密流程 (tools/encrypt_tcl.py):**
```
1. 读取原始文件内容
2. 转换为十六进制字符串 (第一次 hex 编码)
3. 零字节填充到 16 字节块对齐
4. 使用 AES-128-CBC 加密 (IV = Key)
5. 再次转换为十六进制字符串 (第二次 hex 编码)
6. 写入 .bin 文件
```

**C 解密流程 (vfs_crypto.c):**
```
1. 读取 .bin 文件内容
2. 第一次 hex 解码 (得到加密数据)
3. AES-128-CBC 解密
4. 移除零字节填充
5. 第二次 hex 解码 (得到原始内容)
6. 存入内存 VFS (路径自动去除 .bin 扩展名)
```

### 文件命名约定

- **原始文件**: `script.tcl`
- **加密文件**: `script.tcl.bin`
- **VFS 中路径**: `script.tcl` (自动去除 `.bin`)

**示例:**
```
磁盘文件系统:
  encrypted_scripts/
    ├── init.tcl.bin
    ├── config.tcl.bin
    └── target/
        └── stm32.tcl.bin

OpenOCD VFS 中:
  encrypted_scripts/
    ├── init.tcl          ← 解密后，可用 source init.tcl
    ├── config.tcl        ← 解密后，可用 source config.tcl
    └── target/
        └── stm32.tcl     ← 解密后，可用 source target/stm32.tcl
```

---

## 🔐 安全最佳实践

### 密钥管理

#### ❌ 不推荐 (开发/测试)
```bash
# 密钥直接暴露在命令行
openocd --crypto-key "keyskeyskeyskeys" ...
```
**风险**: 命令行历史、进程列表可见

#### ✅ 推荐 (生产环境)

**1. 使用环境变量**
```bash
# 在启动脚本中设置
export OPENOCD_CRYPTO_KEY="keyskeyskeyskeys"
openocd --crypto-key-env OPENOCD_CRYPTO_KEY ...

# 或从安全存储读取
export OPENOCD_CRYPTO_KEY=$(vault read -field=value secret/openocd/key)
openocd --crypto-key-env OPENOCD_CRYPTO_KEY ...
```

**2. 使用密钥文件 (最佳)**
```bash
# 创建密钥文件并设置权限
echo -n "keyskeyskeyskeys" > /secure/keyfile
chmod 600 /secure/keyfile
chown openocd:openocd /secure/keyfile

# 使用密钥文件
openocd --crypto-key-file /secure/keyfile ...
```

### 密钥要求

- **长度**: 必须是 16 字节 (AES-128)
- **字符**: 可以是任意字节序列
- **存储**: 密钥文件不应包含换行符

**生成随机密钥:**
```bash
# 生成 16 字节随机密钥
openssl rand -base64 16 | head -c 16 > keyfile
```

### 文件权限

```bash
# 加密文件权限 (可以较宽松，因为已加密)
chmod 644 encrypted_scripts/*.bin

# 密钥文件权限 (必须严格)
chmod 600 /secure/keyfile
chown openocd:openocd /secure/keyfile
```

---

## 🧪 测试和验证

### 运行加密功能测试

```bash
# 运行完整的加密功能测试
./test_crypto.sh
```

**测试内容:**
1. ✅ 创建测试 TCL 文件
2. ✅ 加密文件
3. ✅ 验证解密正确性
4. ✅ 测试多文件加密
5. ✅ 测试密钥长度验证
6. ✅ 测试密钥文件

### 手动验证

**1. 加密文件**
```bash
echo 'puts "Hello, Encrypted World!"' > test.tcl
python3 tools/encrypt_tcl.py test.tcl test.tcl.bin --key "keyskeyskeyskeys"
```

**2. 验证可以解密回原文件**
```bash
python3 tools/encrypt_tcl.py test.tcl.bin decrypted.tcl \
  --key "keyskeyskeyskeys" --decrypt

diff test.tcl decrypted.tcl
# 应该没有差异
```

**3. 测试 OpenOCD 加载 (需要先编译)**
```bash
# 创建测试目录
mkdir -p /tmp/encrypted_test

# 加密测试脚本
echo 'puts "VFS Crypto Works!"' > test.tcl
python3 tools/encrypt_tcl.py test.tcl /tmp/encrypted_test/test.tcl.bin \
  --key "keyskeyskeyskeys"

# 启动 OpenOCD (会自动解密并加载到 VFS)
openocd --crypto-key "keyskeyskeyskeys" -D /tmp/encrypted_test \
  -c "source test.tcl" -c "shutdown"
```

---

## 📋 使用场景

### 场景 1: 保护专有脚本

**需求**: 分发 OpenOCD 配置，但不想暴露脚本内容

**方案**:
```bash
# 1. 加密所有专有脚本
find ./proprietary -name "*.tcl" -exec \
  python3 tools/encrypt_tcl.py {} {}.bin --key "$SECRET_KEY" \;

# 2. 分发加密文件 (不含原始 .tcl)
cp -r ./proprietary/*.bin ./distribution/

# 3. 提供密钥给授权用户
echo "使用密钥: $SECRET_KEY"

# 4. 用户使用
openocd --crypto-key "$SECRET_KEY" -D ./distribution -f config.cfg
```

### 场景 2: 临时环境中的安全执行

**需求**: 在不信任的环境中运行，不想留下脚本痕迹

**方案**:
```bash
# 1. 将加密脚本存储在安全位置
scp encrypted_scripts.tar.gz remote:/tmp/

# 2. 在远程解压
ssh remote "cd /tmp && tar xzf encrypted_scripts.tar.gz"

# 3. 运行 OpenOCD (脚本在内存中解密)
ssh remote "openocd --crypto-key '$KEY' -D /tmp/encrypted_scripts -f config.cfg"

# 4. 清理 (加密文件可以留下，密钥不会暴露)
ssh remote "rm -rf /tmp/encrypted_scripts"
```

### 场景 3: CI/CD 流水线

**需求**: 自动化测试，密钥从密钥管理系统获取

**方案**:
```bash
#!/bin/bash
# CI/CD 脚本

# 从密钥管理系统获取密钥
CRYPTO_KEY=$(vault read -field=value secret/openocd/key)

# 使用环境变量传递密钥
export OPENOCD_CRYPTO_KEY="$CRYPTO_KEY"

# 运行自动化测试
openocd \
  --crypto-key-env OPENOCD_CRYPTO_KEY \
  -D ./encrypted_test_scripts \
  -f test_config.cfg

# 密钥不会记录在日志中
```

---

## ⚠️ 故障排除

### 问题 1: 解密失败

**症状**: `Failed to decrypt file` 错误

**可能原因**:
1. 密钥不正确
2. 文件不是用相同密钥加密的
3. 文件损坏

**解决**:
```bash
# 验证密钥
echo -n "keyskeyskeyskeys" | wc -c
# 应该输出: 16

# 验证可以解密
python3 tools/encrypt_tcl.py encrypted.bin decrypted.tcl \
  --key "keyskeyskeyskeys" --decrypt

# 如果 Python 解密成功但 OpenOCD 失败，检查密钥是否完全一致
```

### 问题 2: 密钥长度错误

**症状**: `AES-128 requires 16-byte key, got X bytes`

**解决**:
```bash
# 检查密钥长度 (必须是 16 字节)
echo -n "your_key_here" | wc -c

# 如果使用密钥文件，确保没有换行符
echo -n "keyskeyskeyskeys" > keyfile  # 正确
echo "keyskeyskeyskeys" > keyfile     # 错误 (多了换行符)
```

### 问题 3: 文件未被检测为加密

**症状**: 文件被当作普通文件加载

**解决**:
- 确保加密文件有 `.bin` 扩展名
- 检查是否设置了解密密钥

### 问题 4: VFS 中找不到文件

**症状**: `source` 命令找不到文件

**检查**:
```bash
# 加密文件: encrypted_scripts/test.tcl.bin
# VFS 路径: encrypted_scripts/test.tcl (注意没有 .bin)

# 正确用法
openocd --crypto-key "$KEY" -D ./encrypted_scripts \
  -c "source test.tcl"  # 不要加 .bin

# 错误用法
openocd --crypto-key "$KEY" -D ./encrypted_scripts \
  -c "source test.tcl.bin"  # 错误
```

---

## 📚 参考资料

### 相关文档

- [VFS_USAGE_GUIDE.md](VFS_USAGE_GUIDE.md) - Memory VFS 基本使用指南
- [MEMORY_VFS_FEATURE.md](MEMORY_VFS_FEATURE.md) - 技术特性文档
- [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) - 实现状态报告

### 相关文件

- `src/helper/vfs_crypto.h` - 加密框架头文件
- `src/helper/vfs_crypto.c` - 加密实现
- `src/helper/vfs_memory.c` - VFS 实现 (包含解密集成)
- `src/helper/aes.h` / `aes.c` - AES-128 实现 (tiny-AES-c)
- `tools/encrypt_tcl.py` - Python 加密工具
- `test_crypto.sh` - 加密功能测试脚本

### 加密算法

- **算法**: AES-128-CBC
- **库**: tiny-AES-c (公共域)
- **源**: https://github.com/kokke/tiny-AES-c

---

## 📝 更新日志

### v1.0.0 (2025-11-10)

**新增功能**:
- ✨ AES-128-CBC 加密/解密支持
- ✨ 双重 hex 编码
- ✨ 多种密钥来源 (命令行、环境变量、文件)
- ✨ 自动检测和解密 .bin 文件
- ✨ Python 加密工具
- ✨ 完整的测试脚本

**实现的文件**:
- `vfs_crypto.c` / `vfs_crypto.h` - 加密框架
- `aes.c` / `aes.h` - AES 实现
- `encrypt_tcl.py` - 加密工具
- `test_crypto.sh` - 测试脚本

---

## 💡 常见问题 (FAQ)

**Q: 为什么使用双重 hex 编码?**  
A: 为了兼容现有的 Python 加密脚本格式，确保加密文件的格式统一。

**Q: 可以使用其他加密算法吗?**  
A: 当前只实现了 AES-128-CBC。未来可以扩展支持 AES-256-CBC 等。

**Q: 密钥可以是中文吗?**  
A: 可以，密钥是字节序列。但必须确保长度为 16 字节 (UTF-8 编码)。

**Q: 加密文件可以嵌套在子目录中吗?**  
A: 可以。目录结构会被保留，解密后的 VFS 路径自动去除 `.bin` 扩展名。

**Q: 如果忘记密钥怎么办?**  
A: 无法解密。请妥善保管密钥。

**Q: 性能开销如何?**  
A: 解密发生在启动时一次性完成，运行时从内存读取，性能影响极小。

---

**文档版本**: 1.0.0  
**最后更新**: 2025-11-10  
**作者**: OpenOCD Contributors
