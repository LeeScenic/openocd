# 重要变更：目录层次结构现在被保留

## 🎯 您的需求已实现！

现在使用 `-D test` 加载目录后，可以用 `source test/test1.tcl` 来访问文件。

## 📊 变更对比

### 之前的行为（已修改）

```bash
openocd -D test

磁盘结构:         VFS 存储:      使用:
test/            test1.tcl   →  source test1.tcl
├─test1.tcl      test2.tcl   →  source test2.tcl
└─test2.tcl
```

### 现在的行为（新版本）✅

```bash
openocd -D test

磁盘结构:         VFS 存储:           使用:
test/            test/test1.tcl   →  source test/test1.tcl  ✅
├─test1.tcl      test/test2.tcl   →  source test/test2.tcl  ✅
└─test2.tcl
```

## 🔍 实际示例

### 示例 1：您的使用场景

```bash
# 目录结构
jtag_procs/
├── init.tcl
├── utils.tcl
└── board/
    └── stm32.tcl

# 启动命令
openocd -D jtag_procs -f config.cfg

# VFS 中存储为:
# - jtag_procs/init.tcl
# - jtag_procs/utils.tcl
# - jtag_procs/board/stm32.tcl

# 在脚本中使用（包含完整路径）:
source jtag_procs/init.tcl         ✅ 正确！
source jtag_procs/utils.tcl        ✅
source jtag_procs/board/stm32.tcl  ✅
```

### 示例 2：嵌套目录

```bash
# 目录结构
test/
├── test1.tcl
├── test2.tcl
└── subdir/
    └── nested.tcl

# 命令
openocd -D test

# 使用
source test/test1.tcl           ✅
source test/test2.tcl           ✅
source test/subdir/nested.tcl   ✅
```

### 示例 3：多个目录（避免冲突）

```bash
# 目录结构
dir1/
└── init.tcl  # 内容: puts "from dir1"

dir2/
└── init.tcl  # 内容: puts "from dir2"

# 命令
openocd -D dir1 -D dir2

# 使用（不会冲突！）
source dir1/init.tcl  # 输出: from dir1  ✅
source dir2/init.tcl  # 输出: from dir2  ✅
```

## ✅ 优点

### 1. 更直观
```bash
# 命令行指定 test，脚本中也用 test
openocd -D test
source test/file.tcl  # 很清楚来自 test 目录
```

### 2. 避免冲突
```bash
# 多个目录有同名文件？没问题！
openocd -D scripts1 -D scripts2
source scripts1/init.tcl  # 清楚区分
source scripts2/init.tcl
```

### 3. 结构清晰
```bash
# 保持文件组织结构
source jtag_procs/board/stm32.tcl
source jtag_procs/interface/jlink.tcl
```

## 🔧 技术实现

修改了 `vfs_memory.c` 中的 `vfs_load_file_from_disk()` 函数：

**核心逻辑：**
```c
/* 提取目录名（最后一级） */
const char *dir_name = strrchr(base_path, '/');
if (!dir_name) dir_name = base_path;
else dir_name++;

/* 获取相对路径 */
const char *rel_path = file_path + base_len;
while (*rel_path == '/') rel_path++;

/* 构建虚拟路径: dir_name/rel_path */
snprintf(vpath, vpath_len, "%s/%s", dir_name, rel_path);
```

**结果：**
- `/path/to/test/file.tcl` + base=`/path/to/test` → VFS: `test/file.tcl`
- `./test/file.tcl` + base=`./test` → VFS: `test/file.tcl`

## ⚠️ 重要提示

这是一个**破坏性变更**！如果您有旧的脚本，需要更新。

### 迁移示例

**旧脚本：**
```tcl
# openocd -D jtag_procs
source init.tcl     # 旧版本可以工作
source utils.tcl
```

**新脚本（需要修改）：**
```tcl
# openocd -D jtag_procs
source jtag_procs/init.tcl    # 新版本需要完整路径
source jtag_procs/utils.tcl
```

## 🧪 测试验证

### 运行测试脚本
```bash
./test_hierarchy.sh
```

### 查看 VFS 存储的路径
```bash
openocd -d3 -D test -c "shutdown" 2>&1 | grep "VFS: Added file"
```

**期望输出：**
```
Debug: VFS: Added file 'test/test1.tcl' (123 bytes)  ← 注意有 test/ 前缀
Debug: VFS: Added file 'test/test2.tcl' (456 bytes)
```

### 实际测试
```bash
# 创建测试
mkdir -p test
echo 'puts "Success!"' > test/file.tcl

# 测试（应该成功）
openocd -D test -c "source test/file.tcl" -c "shutdown"
# 输出: Success!  ✅
```

## 📚 相关文档

- **详细说明：** `HIERARCHY_PRESERVED.md`
- **测试脚本：** `test_hierarchy.sh`
- **旧版行为：** `PATH_MATCHING_EXPLANATION.md`（已过时）

## 🎉 总结

现在，当您使用 `-D test` 时：

```bash
✅ source test/test1.tcl      # 正确
✅ source test/test2.tcl      # 正确
✅ source test/sub/file.tcl   # 正确

❌ source test1.tcl           # 找不到（需要完整路径）
```

**这正是您想要的行为！** 🎯
