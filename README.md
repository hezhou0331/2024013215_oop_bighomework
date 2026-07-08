# Project Scheduler 项目调度管理系统

本项目是一个用 C++17 实现的项目调度管理系统。程序支持 PPM 文件导入/导出、任务/依赖/资源管理、项目合法性校验，以及关键路径法（CPM）调度计算。

本 README 是仓库的主入口，面向第一次阅读项目的人，重点说明项目能做什么、如何运行、源码如何分层以及应该按什么顺序阅读。

## 快速阅读

| 想了解什么 | 推荐入口 |
| --- | --- |
| 项目怎么运行 | 本 README 的“编译、运行与测试” |
| 类设计和 OOP 思想 | `docs/design.md` |
| 人工交互测试流程 | `docs/manual_test.md` |
| 核心源码 | `include/`、`src/` |
| PPM 样例 | `samples/simple.PPM`、`samples/roundtrip.PPM` |
| 自动化测试 | `tests/run_tests.ps1`、`tests/*.PPM` |

## 编译、运行与测试

### 编译

```bash
# Windows / MinGW
mingw32-make

# 或在支持 make 的环境中
make
```

也可以直接使用 g++：

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude \
    -o project_scheduler \
    main.cpp src/model/*.cpp src/controller/*.cpp src/view/*.cpp
```

### 运行

```bash
./project_scheduler.exe   # Windows
./project_scheduler       # Linux / macOS
```

### 自动化测试

```powershell
cd tests
powershell -ExecutionPolicy Bypass -File run_tests.ps1
```

测试脚本会自动回到项目根目录、清理旧构建、重新编译并运行用例。当前测试覆盖 29 个交互场景，并额外校验导出文件内容，主要包括：

- PPM 导入/导出与导出失败处理
- 任务、依赖、资源的基础操作
- FS/SS/FF/SF 四种依赖类型与正负 Lag
- CPM 调度、关键路径、总成本统计
- 环路、孤立任务、重复 ID、非法引用、非法工期、非法资源数量等异常场景

## 功能概览

程序提供一个控制台菜单界面：

| 菜单 | 功能 |
| --- | --- |
| 1-3 | 新建项目、导入 PPM、导出 PPM |
| 4-8 | 添加、查看、修改、删除任务，查看任务前驱/后继 |
| 9-12 | 添加依赖、查看依赖、按索引删除依赖、按任务对删除依赖 |
| 13-15 | 添加资源、查看资源、为任务分配资源 |
| 16-18 | 验证项目、运行 CPM 调度、查看统计信息 |

使用样例项目可快速验证：

```text
2
samples\simple.PPM
16
17
18
0
```

预期关键结果：

```text
Project is valid.
Project duration: 22
Critical path: 0 1 2 3 4
Total cost: 9030.00
```

## 项目架构

项目采用 MVC 分层：

- `ConsoleUI` 是 View 层，只负责菜单、输入读取和文本展示。
- `ProjectController` 是 Controller 层，用 `RES` 状态枚举和信息类 DTO 与界面通信。
- `Project`、`Task`、`Dependency`、`Resource`、`ProjectValidator`、`CPMScheduler` 等是 Model 层，负责业务规则、图校验和 CPM 计算。

一次典型操作的数据流如下：

1. `ConsoleUI` 读取用户菜单和输入。
2. `ProjectController` 接收请求，调用模型层或文件导入/导出组件。
3. `PPMImporter` / `PPMExporter` 负责把 PPM 文本文件和 `Project` 对象互相转换。
4. `ProjectValidator` 检查任务、依赖和资源是否合法。
5. `CPMScheduler` 在合法项目上计算最早/最晚时间、项目工期和关键路径。
6. `ProjectController` 把结果整理成状态码和信息类，再交给 `ConsoleUI` 展示。

主要 OOP 设计点：

- `Task` 是抽象基类，`BasicTask` 和 `MilestoneTask` 通过虚函数表达不同任务行为。
- `Project` 使用 `std::unique_ptr<Task>` 保存多态任务，并通过 `Clone()` 实现深拷贝。
- `Importer<T>` / `Exporter<T>` 是模板化导入/导出基类，`PPMImporter` / `PPMExporter` 只负责 PPM 格式。
- 导入/导出器通过扩展名工厂注册，新增格式只需添加派生类并注册。
- `ProjectValidator` 专注图合法性校验，`CPMScheduler` 专注关键路径法计算，职责分离清晰。

## 目录结构

```text
2024013215/
├── main.cpp              程序入口
├── Makefile              编译脚本
├── requirement.txt       开发环境说明
├── include/
│   ├── controller/       控制器接口
│   ├── model/            模型层头文件
│   └── view/             控制台界面头文件
├── src/
│   ├── controller/       控制器实现
│   ├── model/            模型层实现
│   └── view/             控制台界面实现
├── samples/              可直接导入的 PPM 示例
├── tests/                自动化测试脚本和测试 PPM 文件
└── docs/                 设计报告与人工测试说明
```

## PPM 文件格式简述

本项目使用文本 PPM 格式描述项目：

- `P` 行：项目名称，必须位于文件首个内容行。
- `T` 行：普通任务，包含任务 ID、名称、工期。
- `M` 行：里程碑任务，工期为 0。
- `R` 行：资源，包含资源 ID、名称、单位时间成本。
- `D` 行：依赖关系，支持 FS、SS、FF、SF 和 Lag。
- `A` 行：资源分配，表示任务占用资源及数量。

导入器会检查区块顺序、字段数量、数值范围、重复 ID、引用有效性、任务名称合法性和里程碑约束。解析失败时会返回带行号的错误信息。

## 架构阅读建议

如果想快速理解项目架构，可以按下面顺序阅读：

1. `main.cpp`：程序入口，只创建 `ConsoleUI` 并启动主循环。
2. `include/view/ConsoleUI.hpp`、`src/view/ConsoleUI.cpp`：查看菜单、输入读取和结果展示方式。
3. `include/controller/ProjectController.hpp`：理解界面层如何通过状态枚举和信息类访问业务功能。
4. `include/model/Project.hpp`：理解项目如何持有任务、依赖和资源。
5. `include/model/Task.hpp`、`BasicTask.hpp`、`MilestoneTask.hpp`：理解任务继承体系和多态行为。
6. `include/model/Importer.hpp`、`Exporter.hpp`、`PPMImporter.hpp`、`PPMExporter.hpp`：理解文件导入/导出扩展结构。
7. `ProjectValidator` 和 `CPMScheduler`：理解项目合法性检查和关键路径计算。

项目中比较重要的设计关系：

- View 层只负责交互，不直接操作模型对象。
- Controller 层负责协调流程，把模型层异常转换为统一状态码。
- Model 层保存数据和业务规则，不依赖界面层。
- `Task` 用抽象基类和虚函数表达不同任务类型，而不是用布尔标志区分。
- 导入/导出器通过模板基类和扩展名工厂组织，方便以后增加新文件格式。

更完整的类关系、关键设计决策和 CPM 计算规则见 `docs/design.md`。

## 说明

本目录已经只保留项目源码、设计说明、样例数据和测试文件。首次阅读时，建议优先关注：

1. `README.md`
2. `docs/design.md`
3. `include/` 和 `src/`
4. `samples/` 和 `tests/`
