# JajaCC

独立 C 编译器（Standalone C Compiler），**不调用 gcc / clang / MinGW**，
自己完成 词法分析 -> 语法分析 -> 代码生成 -> PE32 写出，直接产出 Windows `.exe`。

- 作者 / 版权持有人：**student25-ch**
- 许可证：**GNU GPLv3**（见 `LICENSE`）
- 目标平台：Windows（输出 32 位 PE，32/64 位 Windows 均可直接运行）

---

## 1. 它到底做了什么

| 阶段 | 文件 | 说明 |
| ---- | ---- | ---- |
| 词法分析 | `src/lexer.cpp` | 标识符、数字、字符、字符串、运算符、注释 |
| 语法分析 | `src/parser.cpp` | 递归下降，生成 AST |
| 代码生成 | `src/codegen.cpp` | x86-32 机器码，栈帧 + eax/ebx 求值 |
| PE 写出 | `src/pe.cpp` | DOS 头 / PE 头 / 节表 / 导入表（kernel32!ExitProcess） |
| 入口 | `src/main.cpp` | 读源码 -> 编译 -> 写 exe |

生成的 exe 入口直接调用 `ExitProcess`，所以 `return` 的值就是进程退出码，
在 cmd 里用 `echo %ERRORLEVEL%` 能看到。

---

## 2. 编译 JajaCC 本身

只需要一个 C++17 编译器来编译**编译器**（这一步用 g++ 是允许的，
就像 gcc 本身也是用编译器编译出来的）：

```bash
cd jajacc
make
```

产物：`./jajacc`（Linux/WSL）或 `jajacc.exe`（Windows/MinGW）

---

## 3. 使用

```bash
./jajacc examples/test.c out.exe
```

Linux 下无法直接运行 exe，但可以用 `file out.exe` 检查格式；
拷到 Windows 上双击或在 cmd 里运行即可。

---

## 4. 目前支持的 C 语法

- 类型：`int` / `char` / `void`（只做占位，全部按 32 位整数处理）
- 变量声明与初始化：`int a = 10;`
- 赋值与复合赋值：`=  +=  -=  *=  /=`
- 算术：`+ - * / %`，一元 `-`、`!`
- 比较：`== != < > <= >=`，逻辑 `&& ||`
- 自增自减：`++x` / `--x`
- 控制流：`if / else`、`while`、`for`、`break`、`continue`
- 代码块 `{}`、注释 `//` 与 `/* */`
- 字符串字面量（生成到 .rdata，返回其地址）
- `return`（值作为进程退出码）

### 暂未支持（后续迭代方向）

- 函数参数传递与自定义函数调用（`main` 之外暂时忽略）
- 指针、数组、结构体、`float/double`
- 预处理指令 `#include` / `#define`
- 标准库函数（`printf` 等，需要额外的导入表项）

如果源码里出现不支持的写法，编译器会直接报
`[JajaCC] syntax error` 并退出，不会静默产出错误 exe。

---

## 5. 目录结构

```
jajacc/
├── LICENSE
├── README.md
├── Makefile
├── examples/
│   └── test.c
└── src/
    ├── lexer.h      lexer.cpp
    ├── ast.h
    ├── parser.h     parser.cpp
    ├── codegen.h    codegen.cpp
    ├── pe.h         pe.cpp
    └── main.cpp
```

---

## 6. 许可证

JajaCC 以 **GNU General Public License v3** 发布。
你可以自由使用、修改、再分发，但衍生作品必须同样以 GPLv3 发布，
并保留原始版权声明。

    JajaCC - Standalone C Compiler
    Copyright (C) 2026 student25-ch

完整条款见 `LICENSE` 或 <https://www.gnu.org/licenses/gpl-3.0.txt>。
