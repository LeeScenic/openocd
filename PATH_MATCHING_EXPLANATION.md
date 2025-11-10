# VFS 路径匹配说明

## 问题

如果执行：`openocd -D test`，其中 test 目录下有 `test1.tcl` 和 `test2.tcl`

应该使用：
- `source test1.tcl` ？
- 还是 `source test/test1.tcl` ？

## 答案

**使用 `source test1.tcl`**（不带目录前缀）

## 原因分析

### 代码实现逻辑

在 `vfs_memory.c` 的 `vfs_load_file_from_disk()` 函数中（第 348-358 行）：

```c
/* Calculate virtual path (relative to base_path) */
const char *virtual_path = file_path;
if (base_path) {
    size_t base_len = strlen(base_path);
    if (strncmp(file_path, base_path, base_len) == 0) {
        virtual_path = file_path + base_len;  // ← 跳过 base_path
        /* Skip leading slashes */
        while (*virtual_path == '/' || *virtual_path == '\\')
            virtual_path++;  // ← 跳过开头的斜杠
    }
}
```

**关键点：** VFS 存储文件时，会**去掉 base_path 前缀**！

### 具体例子

#### 场景 1：`openocd -D test`

```
磁盘文件结构:
test/
├── test1.tcl
└── test2.tcl

加载过程:
1. 读取文件: test/test1.tcl
2. base_path = "test"
3. 计算虚拟路径:
   - file_path = "test/test1.tcl"
   - 去掉 base_path "test" → "/test1.tcl"
   - 去掉开头的 "/" → "test1.tcl"
4. 存储为: "test1.tcl" ← 不带目录前缀！

VFS 中存储的路径:
- "test1.tcl"
- "test2.tcl"

使用方法:
source test1.tcl  ✅ 正确
source test/test1.tcl  ❌ 找不到
```

#### 场景 2：`openocd -D .`（加载当前目录）

```
磁盘文件结构:
./test/
├── test1.tcl
└── test2.tcl

加载过程:
1. 读取文件: ./test/test1.tcl
2. base_path = "."
3. 计算虚拟路径:
   - file_path = "./test/test1.tcl"
   - 去掉 base_path "." → "/test/test1.tcl"
   - 去掉开头的 "/" → "test/test1.tcl"
4. 存储为: "test/test1.tcl" ← 保留了目录！

VFS 中存储的路径:
- "test/test1.tcl"
- "test/test2.tcl"

使用方法:
source test/test1.tcl  ✅ 正确
source test1.tcl  ❌ 找不到
```

#### 场景 3：`openocd -D /path/to/test`（绝对路径）

```
磁盘文件结构:
/path/to/test/
├── test1.tcl
└── test2.tcl

加载过程:
1. 读取文件: /path/to/test/test1.tcl
2. base_path = "/path/to/test"
3. 计算虚拟路径:
   - file_path = "/path/to/test/test1.tcl"
   - 去掉 base_path → "/test1.tcl"
   - 去掉开头的 "/" → "test1.tcl"
4. 存储为: "test1.tcl"

使用方法:
source test1.tcl  ✅ 正确
```

## 路径匹配规则总结

| -D 参数 | 磁盘文件 | VFS 存储路径 | source 使用 |
|---------|----------|-------------|-------------|
| `-D test` | `test/file.tcl` | `file.tcl` | `source file.tcl` |
| `-D test` | `test/sub/file.tcl` | `sub/file.tcl` | `source sub/file.tcl` |
| `-D .` | `./test/file.tcl` | `test/file.tcl` | `source test/file.tcl` |
| `-D /abs/path` | `/abs/path/file.tcl` | `file.tcl` | `source file.tcl` |

## 通用规则

```
VFS 存储路径 = 文件完整路径 - base_path - 前导斜杠
```

举例：
```bash
-D /home/user/scripts

文件: /home/user/scripts/board/stm32.cfg
      └─────────┬─────────┘└─────┬─────────┘
           base_path          相对路径

VFS 路径: board/stm32.cfg

使用: source board/stm32.cfg
```

## 实际使用建议

### 推荐方式 1：直接指定目录

```bash
# 如果您的脚本在 jtag_procs 目录下
openocd -D jtag_procs -f config.cfg

# 脚本中使用（不带目录前缀）
source init.tcl
source utils.tcl
```

### 推荐方式 2：保留目录结构

```bash
# 加载当前目录
openocd -D . -f config.cfg

# 脚本中使用（带完整相对路径）
source jtag_procs/init.tcl
source jtag_procs/utils.tcl
```

### 推荐方式 3：多个目录

```bash
# 分别加载多个目录
openocd -D jtag_procs -D custom_scripts -f config.cfg

# 脚本中使用（各自不带目录前缀）
source init.tcl         # 从 jtag_procs
source custom.tcl       # 从 custom_scripts
```

## 如何验证？

### 方法 1：使用调试日志

```bash
openocd -d3 -D test -c "shutdown" 2>&1 | grep "VFS: Added file"
```

输出会显示 VFS 中存储的路径：
```
Debug: VFS: Added file 'test1.tcl' (123 bytes)
Debug: VFS: Added file 'test2.tcl' (456 bytes)
```

### 方法 2：测试实际加载

```bash
# 创建测试文件
mkdir -p test_dir
echo 'puts "Success!"' > test_dir/test.tcl

# 测试不带前缀
openocd -D test_dir -c "source test.tcl" -c "shutdown"
# ✅ 应该成功

# 测试带前缀
openocd -D test_dir -c "source test_dir/test.tcl" -c "shutdown"
# ❌ 会失败（找不到文件）
```

### 方法 3：运行测试脚本

```bash
./test_path_matching.sh
```

## 潜在陷阱

### ❌ 错误示例 1

```bash
# 错误：加载 test 目录但使用了完整路径
openocd -D test -f config.cfg

# config.cfg 中:
source test/init.tcl  # ❌ 错误！找不到
```

**正确做法：**
```bash
source init.tcl  # ✅ 正确
```

### ❌ 错误示例 2

```bash
# 混淆：同时加载父目录和子目录
openocd -D . -D test -f config.cfg

# VFS 中会有:
# 从 "." 加载的: "test/file.tcl"
# 从 "test" 加载的: "file.tcl"
# 同一个文件存储了两次！

source file.tcl       # ✅ 找到（从 test 加载的）
source test/file.tcl  # ✅ 也找到（从 . 加载的）
```

**建议：** 避免重复加载，选择一种方式。

## 最佳实践

### 1. 脚本自包含

```bash
# 将相关脚本放在一个目录
jtag_procs/
├── init.tcl
├── utils.tcl
└── board.tcl

# 加载整个目录
openocd -D jtag_procs -f jtag_procs/init.tcl

# init.tcl 中使用相对路径（不带目录前缀）
source utils.tcl   # ✅
source board.tcl   # ✅
```

### 2. 保持一致性

**选择一种风格并坚持使用：**

**风格 A：** 直接加载目录，脚本中不用前缀
```bash
openocd -D scripts
source init.tcl
```

**风格 B：** 加载父目录，脚本中用完整路径
```bash
openocd -D .
source scripts/init.tcl
```

### 3. 文档化

在 README 中说明：
```markdown
## Usage

Load scripts with:
```bash
openocd -D jtag_procs -f config.cfg
```

In your TCL scripts, use:
```tcl
source init.tcl      # NOT: source jtag_procs/init.tcl
source utils.tcl
```
```

## 总结

回到您的原始问题：

**问：** `openocd -D test` 后，应该用 `source test1.tcl` 还是 `source test/test1.tcl`？

**答：** **`source test1.tcl`**（不带目录前缀）

**原因：** VFS 加载文件时会自动去掉 base_path（即 `-D` 指定的目录），所以文件存储在 VFS 中的路径不包含该前缀。

**验证方法：** 运行 `./test_path_matching.sh` 或使用 `-d3` 查看调试日志。
