# 快速回答：VFS 路径匹配

## 问题
`openocd -D test` 加载了 `test/test1.tcl` 和 `test/test2.tcl`  
应该用哪个？
- `source test1.tcl`
- `source test/test1.tcl`

## 答案：`source test1.tcl`（不带目录前缀）

## 原因
VFS 存储文件时会**自动去掉 `-D` 指定的目录前缀**。

## 可视化说明

```
命令: openocd -D test
            ▼
磁盘文件:        VFS 存储:       使用方式:
test/           test1.tcl    →  source test1.tcl      ✅
├─test1.tcl     test2.tcl    →  source test2.tcl      ✅
└─test2.tcl     
                
                              ✗  source test/test1.tcl  ❌ 找不到！
```

## 代码逻辑

```c
// vfs_memory.c 第 348-358 行
/* Calculate virtual path (relative to base_path) */
virtual_path = file_path + base_len;  // 去掉 base_path
while (*virtual_path == '/')
    virtual_path++;  // 去掉前导斜杠
```

## 不同场景对比

| 命令 | 文件 | VFS 路径 | source 命令 |
|------|------|----------|------------|
| `-D test` | `test/file.tcl` | `file.tcl` | `source file.tcl` |
| `-D test` | `test/sub/file.tcl` | `sub/file.tcl` | `source sub/file.tcl` |
| `-D .` | `./test/file.tcl` | `test/file.tcl` | `source test/file.tcl` |

## 验证方法

```bash
# 方法 1: 查看调试日志
openocd -d3 -D test -c shutdown 2>&1 | grep "VFS: Added file"
# 输出: Debug: VFS: Added file 'test1.tcl' (123 bytes)
#                                   ^^^^^ 没有 test/ 前缀！

# 方法 2: 实际测试
mkdir test
echo 'puts "OK"' > test/test1.tcl

openocd -D test -c "source test1.tcl" -c shutdown      # ✅ 成功
openocd -D test -c "source test/test1.tcl" -c shutdown # ❌ 失败
```

## 记忆方法

**规则：** VFS 路径 = 完整路径 - (`-D` 参数) - 前导斜杠

**示例：**
```
文件: test/sub/file.tcl
-D:   test
      ----
VFS:       sub/file.tcl
```

## 最佳实践

**推荐：** 直接加载目标目录，脚本中不使用目录前缀

```bash
# 好的做法
openocd -D jtag_procs -f config.cfg

# config.cfg 或脚本中:
source init.tcl    # ✅ 简洁清晰
source utils.tcl   # ✅
```

**不推荐：** 加载父目录，脚本中使用完整路径
```bash
# 不推荐（但可以工作）
openocd -D . -f config.cfg

# 脚本中:
source jtag_procs/init.tcl   # 可以，但路径更长
```

## 详细文档

完整说明请查看：`PATH_MATCHING_EXPLANATION.md`
