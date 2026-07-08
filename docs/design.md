# 项目调度器设计报告（Project Scheduler Design Report）

学号：2024013215　　文档更新日期：2026-07-07

本报告与当前代码同步，描述最终版架构：MVC 三层、模板化导入/导出器工厂、
"状态枚举 + 信息类"控制器接口，以及关键路径法（CPM）调度计算的实现要点。

## 1. 需求概述

按大作业要求实现一个项目调度器：管理任务（Task，含里程碑 MilestoneTask）、
依赖（Dependency，FS/SS/FF/SF 四种类型带 Lag）与资源（Resource）三类核心元素；
支持 PPM 格式文件的导入导出、人工增删改查、项目合理性验证（IsValid）与
CPM 调度计算（总工期与关键路径）。设计目标不只是"够用"，而是产出一组
可脱离本作业复用的模型类（我为人人）。

## 2. 分层结构（文本 UML）

```text
View（界面层）
  ConsoleUI
    仅依赖 ProjectController；负责菜单、输入读取与全部文本格式化

Controller（业务流程/控制器层）
  ProjectController <<Singleton>>
    组合 ProjectRepository
    以 RES 状态枚举报告结果，以嵌套信息类（TaskInfo 等）回传数据
    构造时向工厂注册 PPMImporter / PPMExporter
    使用 Importer<Project> / Exporter<Project> / ProjectValidator / CPMScheduler

Model（可重用类层）
  ProjectRepository ── 组合 Project（当前可编辑项目的持有者）

  Project
    组合 std::unique_ptr<Task>*（多态任务表，深拷贝经 Task::Clone）
    组合 Dependency*、Resource*

  Task <<abstract>>
    ├── BasicTask      普通任务：正整数工期，可占用资源
    └── MilestoneTask  里程碑：工期恒 0，不可占用资源
    聚合 Resource（按资源下标 + ResourceAllocation 记录占用）
    缓存前驱/后继任务下标（由 Project::RebuildTaskRelations 重建）

  Dependency
    关联前置/后置任务（按 Project 容器下标）
    内嵌枚举 Dependency::Type（FS/SS/FF/SF）
    封装 CPM 正推/逆推的时间换算规则

  FilePorter<FilePorterType> <<模板基类>>
    扩展名校验、文件可打开性检测、内嵌异常类（INVALID_FILE_TYPE / FILE_OPEN_FAIL）
    ├── Importer<T> <<模板>>  统一导入流程 + 按扩展名注册/查找工厂
    │     └── PPMImporter : public Importer<Project>
    └── Exporter<T> <<模板>>  统一导出流程 + 按扩展名注册/查找工厂
          └── PPMExporter : public Exporter<Project>

  ProjectValidator ── 无环 / 悬挂节点 / 依赖引用有效性检查，产出拓扑序
  CPMScheduler     ── 正推 ES/EF、逆推 LS/LF、总工期与关键任务
  ValidationResult / ScheduleResult / TaskScheduleInfo ── 结果值对象
```

## 3. 关键设计决策

### 3.1 导入/导出器：模板 + 扩展名注册工厂

作业要求"从可能支持多种模型格式的角度考虑导入器继承体系，而非专用 PPM
导入器"。本设计比单纯的抽象基类更进一步，采用与课程 Model3D 示例一致的
三层结构：

- `FilePorter<FilePorterType>`：与模型类型无关的文件接口工具（扩展名截取、
  大小写无关的类型校验、按方向的可打开性检测、统一异常）。
- `Importer<T>` / `Exporter<T>`：类模板，`T` 为任意模型类型。提供
  `LoadFromFile`/`SaveToFile` 统一流程（校验 → 开流 → 调派生类纯虚
  `LoadFromStream`/`SaveToStream`），以及静态工厂
  `Register<DERIVED>()`、`GetInstanceByFileName/ByExtName`。
- `PPMImporter`/`PPMExporter`：仅实现 PPM 的流解析与流写出。

新增一种格式（如 XML）只需新写一个派生类并在控制器构造函数中追加一行
注册，控制器与界面代码零修改（开闭原则）。`FilePorterType` 枚举位于
FilePorter.hpp 顶层而非类内，是因为它是 FilePorter 的非类型模板参数，
必须先于类模板声明；该组织方式与课程示例代码一致。

### 3.2 控制器接口："状态枚举 + 信息类"，不做格式化

`ProjectController` 全部业务接口返回 `RES` 枚举（SUCCESS / INVALID_ARGUMENT /
INDEX_OUT_OF_RANGE / SELF_DEPENDENCY / CYCLE_DETECTED / FILE_ERROR /
PARSE_ERROR / INVALID_PROJECT / UNKNOWN_ERROR），数据经嵌套信息类回传：
TaskInfo、DependencyInfo、ResourceInfo、ValidationInfo、ScheduleInfo、
StatisticsInfo。文本格式化全部由 ConsoleUI 完成，控制器不含任何
输入输出。该模式与课程 Model3D 控制器示例一致。

### 3.2.0 MVC 三层分离的具体实现

**View 层（用户界面）**

- `ConsoleUI`：独立负责全部用户交互、菜单导航、输入读取与结果展示。
- 仅依赖 `ProjectController` 公开接口，不涉及模型细节。
- 所有文本格式化、错误提示、统计显示由该层实现。
- `ConsoleUI::ShowTaskInfo(const ProjectController::TaskInfo&)` 等方法组织显示逻辑。

**Controller 层（业务流程协调）**

- `ProjectController`：单例，不持有任何用户交互代码，无 printf/cout/cin。
- 接收界面层指令 → 调用模型层 → 返回状态与数据结构。
- 状态由 `RES` 枚举表达，数据由嵌套 DTO 类承载，使控制器接口完全与 UI 解耦。
- 异常捕获与转换发生在此层：模型异常 → `RES` 状态码；错误详情 → `GetLastError()`。
- 单例通过 `GetInstance()` 访问，管理 `ProjectRepository`（当前项目的持有者）。

**Model 层（数据结构与业务规则）**

- `Project`：持有任务、依赖、资源的生命周期（RAII），维护图一致性。
- `Task`（抽象基类）、`BasicTask`、`MilestoneTask`：任务多态，由虚函数表达行为差异。
- `Dependency`：依赖关系与 CPM 时间换算规则的统一封装。
- `Resource`：资源定义与成本计算。
- `ProjectValidator`：无状态工具，检验图合法性（无环、无孤立节点、引用有效）。
- `CPMScheduler`：无状态工具，执行关键路径法计算（正推、逆推、关键路径）。
- `Importer<T>`/`Exporter<T>`：模板化文件 I/O，使格式扩展无需改动其他层。
- 模型层不导入 `ProjectController.hpp`，避免循环依赖；不包含任何格式化或 I/O。

**关键分离点：**

1. **格式化完全在 View**：模型与控制器返回的都是原始数据结构，显示格式（对齐、着色、分组）由 UI 决定。
2. **异常处理在 Controller**：模型层抛出异常（`std::invalid_argument`、`std::runtime_error` 等），控制器捕获并转为 `RES` 枚举。
3. **业务规则在 Model**：图校验、CPM 计算、任务多态行为都在模型层；控制器只是调用与异常映射。
4. **单一责任**：每层聚焦自己的核心职责，修改一层不波及其他层。

### 3.2.1 DTO 设计理由

项目定义了六个信息类（DTO）：`TaskInfo`、`DependencyInfo`、`ResourceInfo`、
`ValidationInfo`、`ScheduleInfo`、`StatisticsInfo`。这些类只负责承载控制器
返回给界面层的数据，但仍统一采用**私有数据成员 + getter/setter** 的形式，
以消除“公有数据成员”在课程规范 review 中可能产生的扣分风险。

**纯数据结构，边界仍然清晰**  
DTO 的唯一职责是承载数据——将模型层计算结果传递给表现层。这些类既不维护
复杂业务不变量，也不承担数据校验；数据校验与业务逻辑全部由模型层负责
（Task、Dependency、Resource、ProjectValidator、CPMScheduler 等）。DTO
仅表达“控制器可以提供哪些信息，界面可以读取哪些信息”。

**规范优先的封装选择**  
作业设计要求 3.3 与编码规范 2.8.9 均强调不应暴露受访问规则限制的数据成员。
因此，即使 DTO 本身接近纯数据容器，本项目仍将其数据成员全部私有化，并通过
`GetXxx` / `SetXxx` / `IsXxx` 成员函数访问，避免评审时需要依赖“例外条款”
进行解释。

**界面与控制器的契约内聚**  
DTO 内嵌于 ProjectController 类（规范 2.8.13），使数据交换协议在一个头文件中
完整定义。接收端（ConsoleUI）导入 ProjectController.hpp 后，可以通过 DTO 的
只读接口完成格式化输出；写入接口仅由控制器在组装返回数据时使用。

**显式特殊成员函数**  
每个 DTO 仍显式声明默认构造、拷贝构造、拷贝赋值与析构函数，确保类作者已经
考虑对象初始化、复制和销毁行为，符合编码规范对类特殊成员函数的要求。

综上，DTO 既保持了轻量的数据传递职责，又符合课程对类封装性的严格要求。

### 3.2.2 依赖删除：两种路径设计

`ProjectController::RemoveDependency` 提供两种重载，使界面层能灵活选择删除策略：

- `RemoveDependency(std::size_t DependencyIndex)`：按依赖在容器中的位置删除。
  适用于用户已知确切索引的场景（如命令行 UI 列表后选择）。
  
- `RemoveDependency(std::size_t Predecessor, std::size_t Successor)`：按前置与后置
  任务的索引删除该任务对之间的依赖关系。若不存在则返回 `INVALID_ARGUMENT`。
  适用于脚本化自动化测试或高层业务流程直接按任务语义操作的场景。

两条路径同时保留，遵循"最小改动、最大兼容"的进化策略。下层 `Project` 类
同样提供两种方法（`RemoveDependency(Index)` 与 `RemoveDependency(Pred, Succ)`），
控制器仅做异常映射，不重复业务逻辑。

### 3.3 任务多态而非布尔标志

里程碑与普通任务的差异用真实类型差异表达：`Task` 抽象基类声明
`GetDuration`/`IsResourceAllocatable`/`Clone` 纯虚函数，
`MilestoneTask::GetDuration()` 恒 0、`IsResourceAllocatable()` 恒 false。
修改工期导致类型变化时，`Project::UpdateTask` 原地重建对象并保留原下标、
依赖与（转为里程碑时丢弃）资源分配。深拷贝经 `Task::Clone()` 完成，
避免多态对象切片。

### 3.4 依赖语义收敛在 Dependency 内

四种依赖类型的 CPM 时间换算（正推 `GetSuccessorEarliestStart`、逆推
`GetPredecessorLatestStart`）是 Dependency 自己的行为，Project 与调度器
不知道 FS/SS/FF/SF 的公式细节。依赖类型枚举 `Dependency::Type` 内嵌于类，
满足"一个头文件只声明一个类"的组织要求。

### 3.5 严格的 PPM 格式校验

导入器执行字段级检查：
- 字段完整性：缺失字段拒绝；行尾多余字段拒绝。
- 数值范围：ID 必须非负；分配数量必须为正；工期非负。
- 唯一性约束：任务 ID、资源 ID 在各自范围内不重复。
- 引用有效性：依赖/分配引用的任务或资源必须存在。
- 里程碑规则：M 行工期自动视为 0，可省略或显式写 0；T 行工期必须为正整数。
- 名称规范：任务/资源/项目名称不允许为空、不允许含空白字符
  （因 PPM 采用空格分隔，名称中的空格会破坏格式）。

同时执行附录 1 的区块规则：P 行有且仅有一行且为首个内容行；T/M 块先于 D 块；
各同类区块必须连续；A 块必须位于文件末尾。所有错误统一包装为带行号的解析异常。

## 4. 类间关系一览

- 泛化：BasicTask、MilestoneTask → Task；PPMImporter → Importer<Project>；
  PPMExporter → Exporter<Project>；Importer/Exporter → FilePorter。
- 组合：Project 拥有任务/依赖/资源的生命周期（RAII)；ProjectRepository
  拥有 Project；ProjectController 拥有 ProjectRepository。
- 聚合：Task 按下标引用 Resource（使用但不拥有）。
- 关联：Dependency 按容器下标关联两个 Task。
- 依赖：ProjectValidator、CPMScheduler 在操作期间使用 Project，不持有状态。
- 缓存说明：Task 内的前驱/后继下标只是显示用缓存，权威数据在
  Project::m_Dependencies，每次图变更后由 RebuildTaskRelations 重建。

## 5. CPM 计算规则

1. 先经 ProjectValidator 校验（无环、无孤立/悬挂节点、依赖引用有效），
   并获得拓扑序；
2. 正推：入度为 0 的任务（项目起点）ES = 0，其他任务初值为 INT_MIN；
   按拓扑序经依赖换算传播 ES/EF，允许任务 ES/EF 为负值（支持相对第 0 天的
   提前准备时间，例如 SS -1 使后继在第 -1 天开始）；
3. 总工期 = 全体任务 EF 的最大值（可能为负，表示全体任务实际提前完工）；
4. 逆推：各任务 LF 先取总工期为上界，再按逆拓扑序由后继依赖收紧；
   有后继的任务取"后继推出的最晚开始"与"总工期上界"的较小值，
   保证 SS/SF/负 Lag 场景下 LS/LF 不越过项目完工时间；
5. 关键任务判据：EF == LF 或 ES == LS；关键路径按拓扑序输出。

作业示例 simple.PPM 的验证结果：总工期 22 天，总成本 9030.00，
关键路径（容器下标）0 1 2 3 4，对应样例任务 ID 1 → 2 → 3 → 4 → 5。

## 6. SOLID 与课程知识点对应

- 单一职责：PPM 解析变化只改 PPMImporter，图校验只改 ProjectValidator，
  调度只改 CPMScheduler，显示格式只改 ConsoleUI。
- 开闭原则：新增文件格式 = 新派生类 + 一行注册；新增任务种类 = 新 Task
  派生类。
- 里氏替换：BasicTask/MilestoneTask 可经 Task&/unique_ptr<Task> 统一使用。
- 依赖倒置：控制器面向 Importer<Project>/Exporter<Project> 抽象编程，
  具体格式类只在注册处出现一次。
- 封装：全部数据成员私有，DTO 也通过 getter/setter 访问；
  不变量由成员函数维护。
- 知识点覆盖：抽象类与纯虚函数、虚析构、override、类模板与模板成员函数、
  非类型模板参数、static_assert/type_traits、单例模式、工厂注册、
  内嵌类与内嵌枚举、深拷贝（Clone）、异常体系、STL 容器与智能指针。

## 7. 编码规范落实要点

- 仅使用 C++17 标准库；无全局变量；无宏常量/宏函数；无 goto；
  无 C 风格强制转换；无裸 new/delete（统一 make_unique/make_shared）。
- 每个类显式声明构造、拷贝构造、拷贝赋值与析构（不适用者显式 = delete
  并注释原因）；被继承的基类析构均为 virtual。
- 类声明在 hpp、实现在 cpp；类模板与内嵌类按规范在 hpp 中声明与实现
  分离（声明在前、实现在后）。
- 只读成员函数全部后置 const；bool 命名以 Is/Has 开头；成员变量按
  m_ + 类型前缀命名；函数与类型大写开头。
- 文件、类、函数三级注释块齐全；注释行数量高于程序行数量的 1/3。
- 源文件编码统一为 UTF-8。

## 8. 目录结构

```text
2024013215/
├── main.cpp              程序入口（仅创建 ConsoleUI 并启动）
├── Makefile              g++ -std=c++17 -Wall -Wextra -pedantic
├── requirement.txt       开发环境说明
├── include/
│   ├── controller/       ProjectController.hpp
│   ├── model/            模型层全部头文件（含 FilePorter/Importer/Exporter 模板）
│   └── view/             ConsoleUI.hpp
├── src/
│   ├── controller/       ProjectController.cpp
│   ├── model/            模型层实现（模板类无对应 cpp）
│   └── view/             ConsoleUI.cpp
├── samples/              simple.PPM、roundtrip.PPM
├── tests/                非法用例 PPM 与 run_tests.ps1 自动测试脚本
└── docs/                 本设计报告
```
