# 项目调度器（Project Scheduler）

## 概述

这是一个用 C++17 实现的项目调度管理系统，支持通过关键路径法（CPM）对项目进行任务调度、时间分析和成本评估。项目采用 MVC 架构，具有规范的类设计、严格的输入校验和完整的测试覆盖。

## 快速开始

### 编译

```bash
# 使用 Makefile（推荐）
mingw32-make

# 或直接使用 g++ 编译
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude -o project_scheduler main.cpp src/model/*.cpp src/controller/*.cpp src/view/*.cpp
```

### 运行

```bash
./project_scheduler.exe  # Windows
./project_scheduler      # Linux/macOS
```

### 运行测试

```bash
cd tests
powershell -ExecutionPolicy Bypass -File run_tests.ps1
```

测试覆盖 3 个基础功能场景 + 26 个边界和异常场景，共 **29 个测试用例**，**全部通过**，包括：
- 基础功能：项目导入、任务关系、项目导出
- 依赖管理：按索引删除、按任务对删除（新增）
- 图合法性：孤立任务、环路、缺失任务、多起点多终点（新增）
- 数据校验：负 ID、重复 ID、格式错误、非法依赖、非法工期、非法成本等
- CPM 算法：正负 Lag、里程碑任务、资源分配、任务类型转换（新增）
- 错误处理：导出失败错误捕获（新增）、文件顺序错误（新增）

---

## 作业要求对应表

本表明确映射大作业要求到实现代码位置，方便评阅。

### 2.1 基本功能（10%）

| 要求 | 功能 | 代码位置 | 测试 |
|-----|------|--------|------|
| 2.1.1 | PPM 导入 | `include/model/PPMImporter.hpp`、`src/model/PPMImporter.cpp` | ✓ run_tests.ps1 用例 1-3 |
| 2.1.2 | PPM 导出 | `include/model/PPMExporter.hpp`、`src/model/PPMExporter.cpp` | ✓ run_tests.ps1 用例 3；导出错误捕获（新增） |
| 2.1.3 | 任务管理 | `include/controller/ProjectController.hpp` 菜单 4-8 | ✓ ConsoleUI |
| 2.1.4 | 依赖管理 | `include/controller/ProjectController.hpp` 菜单 9-12 | ✓ ConsoleUI；支持按索引删除与按任务对删除（新增） |
| 2.1.5 | 资源管理 | `include/controller/ProjectController.hpp` 菜单 13-15 | ✓ ConsoleUI |

### 2.2 类设计实现（60%）

#### 2.2.1 继承与多态（15%）

| 要求 | 设计 | 代码位置 |
|-----|------|--------|
| Task 体系 | 抽象基类 + BasicTask/MilestoneTask 派生 | `include/model/Task.hpp`、`BasicTask.hpp`、`MilestoneTask.hpp` |
| 虚析构 | 所有基类析构均为 virtual | `include/model/Task.hpp` 第 47 行 |
| 纯虚函数 | GetDuration、IsResourceAllocatable、Clone | `include/model/Task.hpp` |
| 里氏替换 | BasicTask/MilestoneTask 可作 Task* 使用 | `src/model/Project.cpp:GetTask` |
| 多态应用 | 工期 0 自动转为里程碑（保留下标和依赖） | `src/model/Project.cpp:UpdateTask` |

#### 2.2.2 模板与工厂（15%）

| 要求 | 设计 | 代码位置 |
|-----|------|--------|
| 导入/导出继承体系 | 模板基类 FilePorter<T> | `include/model/FilePorter.hpp` |
| Importer 模板 | 统一的导入流程：LoadFromFile → LoadFromStream | `include/model/Importer.hpp` |
| Exporter 模板 | 统一的导出流程：SaveToFile → SaveToStream | `include/model/Exporter.hpp` |
| PPMImporter 派生 | 实现 LoadFromStream | `include/model/PPMImporter.hpp`、`src/model/PPMImporter.cpp` |
| PPMExporter 派生 | 实现 SaveToStream | `include/model/PPMExporter.hpp`、`src/model/PPMExporter.cpp` |
| 工厂注册 | Register<T>() 在控制器构造中调用 | `src/controller/ProjectController.cpp` 构造函数 |
| 扩展原则 | 新格式只需派生 + 一行注册，无需改现有代码 | 设计文档 § 3.1 |

#### 2.2.3 职责分离（15%）

| 模块 | 职责 | 代码位置 |
|-----|------|--------|
| Model | 数据结构与业务规则 | `include/model/`、`src/model/` |
| Controller | 单例，业务流程协调 | `include/controller/ProjectController.hpp`（§ 3.2） |
| View | 用户交互与格式化 | `include/view/ConsoleUI.hpp`、`src/view/ConsoleUI.cpp` |
| Validator | 图合法性校验 + 拓扑排序 | `include/model/ProjectValidator.hpp` |
| CPMScheduler | 正推/逆推/关键路径 | `include/model/CPMScheduler.hpp` |

#### 2.2.4 MVC 与接口设计（15%）

| 要求 | 实现 | 代码位置 |
|-----|------|--------|
| 单例 | ProjectController::GetInstance() | `src/controller/ProjectController.cpp` |
| 状态返回 | RES 枚举（SUCCESS/INVALID_ARGUMENT/...） | `include/controller/ProjectController.hpp` 第 47-57 行 |
| DTO | TaskInfo/DependencyInfo/... 嵌套信息类，私有字段 + 读写接口 | `include/controller/ProjectController.hpp` 第 62-...行 |
| 错误信息 | GetLastError() 只读访问失败详情 | `include/controller/ProjectController.hpp` § 3.2 |

### 2.3 代码规范（30%）

| 项 | 检查 | 状态 |
|---|------|------|
| 2.3.1 | 无全局变量 | ✓ |
| 2.3.2 | 无宏常量/宏函数 | ✓ |
| 2.3.3 | 无 goto | ✓ |
| 2.3.4 | 无 C 风格强转 | ✓ |
| 2.3.5 | 无裸 new/delete | ✓（全用 make_unique/make_shared） |
| 2.3.6 | 显式构造/析构/赋值 | ✓（每个类有明确声明或 = delete + 注释） |
| 2.3.7 | 虚析构 | ✓（Task、FilePorter、Importer、Exporter） |
| 2.3.8 | override 关键字 | ✓（所有派生类） |
| 2.3.9 | 后置 const | ✓（所有查询接口） |
| 2.3.10 | IsXxx/HasXxx 命名 | ✓ |
| 2.3.11 | m_前缀命名 | ✓（m_iDuration、m_sName、m_mTaskIdToIndex） |
| 2.3.12 | 类/函数大写 | ✓（CamelCase） |
| 2.3.13 | 文件注释块 | ✓（所有文件顶部） |
| 2.3.14 | 类注释块 | ✓（所有类前） |
| 2.3.15 | 函数注释块 | ✓（所有函数实现前） |
| 2.3.16 | 注释密度 > 1/3 | ✓ |
| 2.3.17 | UTF-8 编码 | ✓ |

---

## 样例验证

使用 `samples/simple.PPM`：

```bash
# 菜单选择 2（导入 PPM）→ samples/simple.PPM
# 菜单选择 15（校验项目）
# 菜单选择 16（运行 CPM 调度）
# 菜单选择 17（统计）
```

**预期结果**：

```text
Project duration: 22
Total cost: 9030.00
Critical path: 0 1 2 3 4
```

---

## 设计模式应用

本项目应用了以下 4 种经典设计模式，提升代码的可扩展性与可维护性：

### 1. 工厂模式（Factory Pattern）

**目的**：按文件扩展名动态选择合适的导入/导出器，支持无限扩展新格式。

**位置**：
- `include/model/Importer.hpp`：`Register<T>()`、`GetInstanceByFileName()`
- `include/model/Exporter.hpp`：`Register<T>()`、`GetInstanceByFileName()`
- `src/controller/ProjectController.cpp` 构造函数：注册 PPMImporter/PPMExporter

**扩展示例**：支持 XML/JSON 格式只需：
1. 继承 `Importer<Project>`，实现 `LoadFromStream()`
2. 在 ProjectController 构造函数中一行注册：`Importer<Project>::Register<XMLImporter>()`
3. 无需修改现有任何代码 ✓ 符合开闭原则

### 2. 模板方法模式（Template Method Pattern）

**目的**：定义统一的文件导入/导出流程，避免各个具体导入/导出器重复实现文件打开、关闭等通用逻辑。

**位置**：
- `include/model/Importer.hpp` 的 `LoadFromFile()`：打开文件 → 调用派生类的 `LoadFromStream()` → 返回结果
- `include/model/Exporter.hpp` 的 `SaveToFile()`：创建文件 → 调用派生类的 `SaveToStream()` → 返回结果

**效果**：通用部分与特定部分清晰分离，减少代码重复。

### 3. 策略模式（Strategy Pattern）

**目的**：不同文件格式的导入/导出策略可以相互替换，客户端代码无需关心具体策略。

**位置**：
- `include/model/PPMImporter.hpp`、`include/model/PPMExporter.hpp`
- 控制器通过工厂获取合适的导入/导出器后调用统一接口，无需 if-else 判断

**效果**：如需支持新格式，仅添加新策略类，无需改动现有逻辑。

### 4. 单例模式（Singleton Pattern）

**目的**：确保项目中仅有一个控制器实例，方便全局访问业务接口。

**位置**：`include/controller/ProjectController.hpp` 的 `GetInstance()`

**实现**：函数内静态对象，在 C++11 及后续标准下线程安全。

**效果**：不需全局变量，控制器状态唯一。

---

## 架构详细说明

详见 `docs/design.md`，包含：
- 完整 UML 类图与关系分析（§ 2-4）
- 关键设计决策与原理（§ 3）
- CPM 计算规则（§ 5）
- SOLID 原则映射（§ 6）
- 编码规范落实（§ 7）
