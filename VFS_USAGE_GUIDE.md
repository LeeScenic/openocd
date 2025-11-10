# OpenOCD Memory VFS 使用指南

## 功能简介

OpenOCD Memory VFS (虚拟文件系统) 功能允许您将 TCL 脚本预加载到内存中，从而防止脚本内容泄露到磁盘。这对于保护敏感配置脚本特别有用。

## 核心特性

✅ **内存加载**: 将整个目录的 TCL 脚本加载到内存  
✅ **透明执行**: 脚本从内存执行，无需磁盘文件  
✅ **自动回退**: 内存中找不到的文件自动从磁盘加载  
✅ **递归支持**: 自动加载子目录中的所有脚本  
✅ **零修改**: 现有脚本无需任何修改即可使用  

## 快速开始

### 基本用法

```bash
# 加载 tcl 目录到内存，然后运行配置
openocd -D ./tcl -f board/stm32f4discovery.cfg
```

### 加载多个目录

```bash
# 可以多次使用 -D 参数加载多个目录
openocd -D ./tcl -D ./custom_scripts -f my_config.cfg
```

### 保护敏感脚本

```bash
# 1. 将敏感脚本目录加载到内存
openocd -D /secure/jtag_procs -f config.cfg

# 2. 脚本中使用 source 命令时，会优先从内存读取
#    source jtag_procs/my_secure_script.tcl
#    → 从内存执行，不访问磁盘！
```

## 工作原理

### 1. 启动阶段

```
openocd -D /path/to/scripts
    ↓
初始化内存 VFS
    ↓
递归扫描目录
    ↓
将所有文件加载到内存
    ↓
构建路径→内容映射表
```

### 2. 执行阶段

```
source target/stm32f4x.cfg
    ↓
检查内存 VFS
    ↓
找到？ → 从内存执行 ✓
    ↓
未找到？ → 从磁盘加载 (传统方式)
```

## 使用场景

### 场景 1: 保护专有 JTAG 配置

```bash
# 您有专有的 JTAG 配置脚本不想暴露在磁盘上
openocd -D /company/proprietary/jtag_procs \
        -f /company/proprietary/jtag_procs/main.cfg
        
# 好处：
# - 脚本只存在于内存中
# - 进程结束后内容消失
# - 无法从磁盘恢复脚本内容
```

### 场景 2: 临时环境中的安全执行

```bash
# 在不信任的环境中运行，不想留下脚本痕迹
openocd -D ./sensitive_configs \
        -c "source board_init.tcl" \
        -c "program firmware.bin verify reset" \
        -c "shutdown"
        
# 执行完成后，sensitive_configs 中的内容不会留在磁盘上
```

### 场景 3: 加密脚本保护

```bash
# 预处理：解密脚本到临时内存目录
./decrypt_scripts.sh /tmp/decrypted_$$

# 使用：从内存加载，不写入磁盘
openocd -D /tmp/decrypted_$$ -f main.cfg

# 清理：删除临时目录
rm -rf /tmp/decrypted_$$

# 好处：脚本已在 OpenOCD 内存中，删除临时目录不影响执行
```

## 命令行选项

### -D, --directory-to-memory <目录>

将指定目录及其所有子目录的内容加载到内存 VFS 中。

**语法:**
```bash
openocd -D <directory_path> [其他选项]
```

**示例:**
```bash
# 单个目录
openocd -D ./tcl -f board/stm32f4discovery.cfg

# 多个目录
openocd -D ./tcl -D ./custom -f config.cfg

# 绝对路径
openocd -D /opt/openocd/scripts -f my_config.cfg
```

**注意事项:**
- 目录必须存在，否则会报错
- 会递归加载所有子目录
- 文件路径以相对于加载目录的形式存储

## 调试和监控

### 查看加载统计

启动时，OpenOCD 会输出 VFS 统计信息：

```
Info : Loading directory into memory VFS: ./tcl
Info : VFS: Loaded directory './tcl' (245 files, 187523 bytes total)
```

### 启用调试日志

```bash
# 使用 -d 选项查看详细的 VFS 操作
openocd -d3 -D ./tcl -f config.cfg
```

调试输出示例：
```
Debug: VFS: Added file 'target/stm32f4x.cfg' (3251 bytes)
Debug: VFS: Read file 'target/stm32f4x.cfg' (3251 bytes)
Debug: VFS: Evaluating 'target/stm32f4x.cfg' from memory (3251 bytes)
```

## 性能考虑

### 内存使用

- **内存开销**: 约等于加载文件的总大小
- **查找性能**: O(1) 平均情况（哈希表）
- **启动时间**: 与文件数量成线性关系

**示例:**
```
tcl 目录 (OpenOCD 标准脚本):
- 文件数: ~245 个
- 总大小: ~187 KB
- 加载时间: < 100ms
- 内存占用: ~200 KB
```

### 性能优化建议

1. **选择性加载**: 只加载需要保护的目录
```bash
# 不要加载整个 /usr/share/openocd
openocd -D ./sensitive_only -f config.cfg
```

2. **避免大文件**: VFS 适合脚本，不适合大型数据文件
```bash
# 好: TCL 脚本 (几 KB)
openocd -D ./tcl_scripts

# 不好: 包含大型二进制文件的目录
# openocd -D ./firmware_images  # 避免这样做
```

## 限制和注意事项

### 1. 只读访问
- 内存 VFS 是只读的
- 不支持运行时创建或修改文件
- 如需写入，使用正常的磁盘路径

### 2. 启动时加载
- 文件在启动时加载
- 不支持动态加载新文件
- 要更新内容，需要重启 OpenOCD

### 3. 路径匹配
```bash
# 加载时的路径
-D /path/to/scripts

# 脚本中的引用必须匹配
source target/stm32.cfg          # ✓ 正确: 相对路径
source /path/to/scripts/target/stm32.cfg  # ✓ 正确: 完整路径

# 路径不匹配会回退到磁盘
source /other/path/stm32.cfg     # → 从磁盘加载
```

## 故障排除

### 问题 1: 文件未从内存加载

**症状:** 脚本仍从磁盘读取

**解决:**
1. 检查路径是否正确
```bash
# 确认文件已加载
openocd -d3 -D ./tcl -f config.cfg 2>&1 | grep "VFS: Added file"
```

2. 检查路径匹配
```bash
# source 命令中的路径必须匹配 VFS 中的路径
source target/stm32.cfg  # 如果加载时用 -D ./tcl
```

### 问题 2: 内存不足

**症状:** 加载大目录时失败

**解决:**
1. 只加载必要的目录
2. 检查可用内存
3. 考虑分批加载

### 问题 3: 启动变慢

**症状:** OpenOCD 启动时间增加

**解决:**
1. 减少加载的文件数量
2. 只在需要时使用 VFS
3. 考虑脚本优化

## 高级用法

### 与环境变量配合

```bash
#!/bin/bash
# 设置脚本目录
SCRIPT_DIR="/opt/company/openocd_scripts"

# 检查目录是否存在
if [ -d "$SCRIPT_DIR" ]; then
    openocd -D "$SCRIPT_DIR" -f "$SCRIPT_DIR/main.cfg"
else
    echo "Error: Script directory not found"
    exit 1
fi
```

### 在脚本中检测 VFS

虽然 VFS 是透明的，但您可以通过日志判断：

```tcl
# 在 TCL 脚本中
puts "Loading configuration..."
# OpenOCD 日志会显示是否从内存加载
```

### 组合使用

```bash
# 组合多种加载方式
openocd \
    -D ./protected_scripts \      # 受保护的脚本
    -s ./additional_scripts \     # 搜索路径
    -f ./protected_scripts/main.cfg
```

## 安全建议

1. **权限管理**: 确保内存加载的脚本目录权限正确
```bash
chmod 700 /secure/scripts
openocd -D /secure/scripts -f config.cfg
```

2. **清理临时文件**: 如果解密脚本到临时目录，及时清理
```bash
trap "rm -rf /tmp/scripts_$$" EXIT
openocd -D /tmp/scripts_$$ -f main.cfg
```

3. **避免日志泄露**: 注意 OpenOCD 日志可能包含脚本内容
```bash
# 不记录详细日志到文件
openocd -D ./secure -f config.cfg
# 而不是:
# openocd -d3 -D ./secure -l logfile.txt -f config.cfg
```

## 总结

Memory VFS 功能为 OpenOCD 提供了一种简单有效的方式来保护 TCL 脚本内容：

- ✅ 使用简单: 只需添加 `-D` 参数
- ✅ 性能优秀: 内存访问比磁盘更快
- ✅ 安全可靠: 脚本内容不会泄露到磁盘
- ✅ 兼容性好: 现有脚本无需修改

如有问题或建议，请参考 MEMORY_VFS_FEATURE.md 了解更多技术细节。
