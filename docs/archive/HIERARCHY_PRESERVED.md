# VFS 目录层次结构保留

## 修改说明

原先的实现会**去掉 `-D` 参数指定的目录前缀**，现在修改为**保留目录名称**。

## 修改前后对比

### 修改前（旧行为）

```bash
openocd -D test

磁盘:              VFS存储:         使用:
test/             test1.tcl    →   source test1.tcl
├─test1.tcl       test2.tcl    →   source test2.tcl
└─test2.tcl
```

### 修改后（新行为）

```bash
openocd -D test

磁盘:              VFS存储:              使用:
test/             test/test1.tcl    →   source test/test1.tcl
├─test1.tcl       test/test2.tcl    →   source test/test2.tcl
└─test2.tcl
```

## 实现原理

### 代码修改（vfs_memory.c）

**修改前：**
```c
/* Calculate virtual path (relative to base_path) */
if (base_path) {
    virtual_path = file_path + base_len;  // 直接跳过整个base_path
    while (*virtual_path == '/') 
        virtual_path++;
}
// 结果: test/file.tcl → file.tcl
```

**修改后：**
```c
/* Extract the directory name from base_path */
const char *dir_name = strrchr(base_path, '/');
if (!dir_name) dir_name = base_path;
else dir_name++;  // Skip the slash

/* Get relative path after base_path */
const char *rel_path = file_path + base_len;
while (*rel_path == '/') rel_path++;

/* Build virtual path as: dir_name/rel_path */
snprintf(vpath, vpath_len, "%s/%s", dir_name, rel_path);
// 结果: test/file.tcl → test/file.tcl
```

## 详细示例

### 示例 1：简单目录

```bash
# 目录结构
jtag_procs/
├── init.tcl
└── utils.tcl

# 命令
openocd -D jtag_procs -f config.cfg

# VFS 存储
jtag_procs/init.tcl
jtag_procs/utils.tcl

# 脚本中使用
source jtag_procs/init.tcl   ✅
source jtag_procs/utils.tcl  ✅

# 不再支持（与旧版本不同）
source init.tcl   ❌ 找不到
```

### 示例 2：嵌套目录

```bash
# 目录结构
test/
├── file1.tcl
└── subdir/
    └── file2.tcl

# 命令
openocd -D test

# VFS 存储
test/file1.tcl
test/subdir/file2.tcl

# 使用
source test/file1.tcl         ✅
source test/subdir/file2.tcl  ✅
```

### 示例 3：绝对路径

```bash
# 目录结构
/home/user/scripts/
└── init.tcl

# 命令
openocd -D /home/user/scripts

# VFS 存储
scripts/init.tcl   # 只保留最后一级目录名

# 使用
source scripts/init.tcl  ✅
```

### 示例 4：多个目录

```bash
# 目录结构
dir1/
└── file1.tcl

dir2/
└── file2.tcl

# 命令
openocd -D dir1 -D dir2

# VFS 存储
dir1/file1.tcl
dir2/file2.tcl

# 使用（不会冲突）
source dir1/file1.tcl  ✅
source dir2/file2.tcl  ✅
```

## 优点

### 1. 更符合直觉

```bash
# 加载 jtag_procs 目录
openocd -D jtag_procs

# 使用时带上目录名，更清晰
source jtag_procs/init.tcl
```

### 2. 避免命名冲突

```bash
# 加载多个目录
openocd -D dir1 -D dir2

# 即使两个目录都有 init.tcl，也不会冲突
source dir1/init.tcl  # 来自 dir1
source dir2/init.tcl  # 来自 dir2
```

### 3. 保持文件组织

```bash
# 目录结构一目了然
source jtag_procs/init.tcl
source jtag_procs/board/stm32.tcl
source jtag_procs/interface/jlink.tcl
```

## 与旧版本的兼容性

⚠️ **破坏性变更！** 旧脚本需要修改。

### 迁移指南

**旧版本脚本：**
```tcl
# openocd -D test
source init.tcl
source utils.tcl
```

**新版本脚本：**
```tcl
# openocd -D test
source test/init.tcl   # 需要加上目录前缀
source test/utils.tcl
```

### 迁移建议

**方案 1：修改脚本（推荐）**
```tcl
# 添加目录前缀
source test/init.tcl
```

**方案 2：修改命令行**
```bash
# 使用父目录，避免直接暴露目录名
# （如果您不想在脚本中写目录名）

# 不推荐：加载子目录
openocd -D test

# 推荐：加载父目录
openocd -D .
source test/init.tcl
```

## 验证修改

### 方法 1：查看调试日志

```bash
openocd -d3 -D test -c "shutdown" 2>&1 | grep "VFS: Added file"
```

**期望输出：**
```
Debug: VFS: Added file 'test/test1.tcl' (123 bytes)  ← 包含 test/ 前缀
Debug: VFS: Added file 'test/test2.tcl' (456 bytes)
```

### 方法 2：实际测试

```bash
# 创建测试
mkdir -p test
echo 'puts "Success!"' > test/file.tcl

# 测试带前缀（应该成功）
openocd -D test -c "source test/file.tcl" -c "shutdown"
# 期望: Success!  ✅

# 测试不带前缀（应该失败）
openocd -D test -c "source file.tcl" -c "shutdown"
# 期望: Error: Can't find file.tcl  ❌
```

### 方法 3：运行测试脚本

```bash
./test_hierarchy.sh
```

## 为什么这样修改？

### 原因 1：用户反馈

用户期望：
```bash
openocd -D test
source test/file.tcl  # 符合直觉
```

而不是：
```bash
openocd -D test
source file.tcl  # 不清楚文件来自哪里
```

### 原因 2：避免冲突

多个目录有同名文件时：
```bash
openocd -D dir1 -D dir2

# 旧版本：冲突，后加载的覆盖前面的
# 新版本：不冲突
source dir1/init.tcl  ✅
source dir2/init.tcl  ✅
```

### 原因 3：保持结构清晰

```bash
# 清楚知道文件来自哪个目录
source jtag_procs/board/stm32.tcl
source jtag_procs/interface/jlink.tcl
```

## 总结

### 修改内容
✅ VFS 现在保留目录名称  
✅ source 命令需要包含完整路径  
✅ 避免多目录命名冲突  

### 使用示例
```bash
# 命令
openocd -D test

# 使用（新版本）
source test/file.tcl  ✅ 正确

# 使用（旧版本，不再支持）
source file.tcl  ❌ 找不到
```

### 迁移步骤
1. 检查所有 source 命令
2. 添加目录前缀
3. 测试验证

### 测试方法
```bash
./test_hierarchy.sh
```
