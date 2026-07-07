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

测试覆盖 7 个基础功能场景 + 14 个边界和异常场景，共 **21 个测试用例**。

---

## 作业要求对应表

本表明确映射大作业要求到实现代码位置，方便评阅。

### 2.1 基本功能（10%）

| 要求 | 功能 | 代码位置 | 测试 |
|-----|------|--------|------|
| 2.1.1 | PPM 导入 | `include/model/PPMImporter.hpp`、`src/model/PPMImporter.cpp` | ✓ run_tests.ps1 用例 1-3 |
| 2.1.2 | PPM 导出 | `include/model/PPMExporter.hpp`、`src/model/PPMExporter.cpp` | ✓ run_tests.ps1 用例 3 |
| 2.1.3 | 任务管理 | `include/controller/ProjectController.hpp` 菜单 4-7 | ✓ ConsoleUI |
| 2.1.4 | 依赖管理 | `include/controller/ProjectController.hpp` 菜单 9-11 | ✓ ConsoleUI |
| 2.1.5 | 资源管理 | `include/controller/ProjectController.hpp` 菜单 12-14 | ✓ ConsoleUI |

### 2.2 类设计实现（60%）

#### 2.2.1 继承与多态（15%）

| 要求 | 设计 | 代码位置 |
|-----|------|--------|
| Task 体系 | 抽象基类 + BasicTask/MilestoneTask 派生 | `include/model/Task.hpp`、`BasicTask.hpp`、`MilestoneTask.hpp` |
| 虚析构 | 所有基类析构均为 virtual | `include/model/Task.hpp` 第 47 行 |
| 纯虚函数 | GetDuration、CanAllocateResource、Clone、Print | `include/model/Task.hpp` 第 66-72 行 |
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
| DTO | TaskInfo/DependencyInfo/... 嵌套类 | `include/controller/ProjectController.hpp` 第 62-...行 |
| 错误信息 | LastError 常引用只读成员 | `include/controller/ProjectController.hpp` § 3.2 |

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

## 架构详细说明

详见 `docs/design.md`，包含：
- 完整 UML 类图与关系分析（§ 2-4）
- 关键设计决策与原理（§ 3）
- CPM 计算规则（§ 5）
- SOLID 原则映射（§ 6）
- 编码规范落实（§ 7）
