//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 Project.cpp
//【功能模块和目的】         实现项目模型类，完成任务、依赖关系与资源的增删改查及一致性维护。
//【开发者及日期】           刘江宇, 2026-07-05
//【更改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#include "model/Project.hpp"
#include "model/BasicTask.hpp"
#include "model/MilestoneTask.hpp"
#include <cctype>
#include <algorithm>
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::Project
//【函数功能】       默认构造项目：以默认名称 "UntitledProject" 创建不含任务、依赖与资源的空项目。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project::Project()
    : m_Name("UntitledProject")
{
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::Project
//【函数功能】       以给定名称构造空项目，并校验名称非空且无空白字符。
//【参数】           Name（输入参数）：项目名称，不允许为空或含空白字符。
//【返回值】         无；名称非法时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为调用 SetName 确保构造函数与 SetName 校验规则一致。
//-------------------------------------------------------------------------------------------------------------------
Project::Project(const std::string& Name)
    : m_Name("")
{
    SetName(Name);                                      //使用 SetName 统一校验规则
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::Project
//【函数功能】       拷贝构造项目：复制名称、依赖与资源列表，并对任务列表逐个 Clone 深拷贝。
//【参数】           Source（输入参数）：作为拷贝来源的项目对象。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project::Project(const Project& Source)
    : m_Name(Source.m_Name),
      m_Dependencies(Source.m_Dependencies),
      m_Resources(Source.m_Resources)
{
    //任务以多态指针持有，必须逐个 Clone 完成深拷贝
    for (const auto& CurrentTask : Source.m_Tasks) {
        m_Tasks.push_back(CurrentTask->Clone());
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::operator=
//【函数功能】       拷贝赋值：按"先拷贝构造临时对象、再移动赋值"的方式深拷贝整个项目，
//                   自赋值时不做任何操作。
//【参数】           Source（输入参数）：作为赋值来源的项目对象。
//【返回值】         Project&，返回自身引用以支持连续赋值。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project& Project::operator=(const Project& Source)
{
    if (this != &Source) {                            //自赋值保护
        Project Copied(Source);                       //先深拷贝出临时对象
        *this = std::move(Copied);                    //再整体移动接管
    }
    return *this;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::Project
//【函数功能】       移动构造项目：接管源项目的名称、任务、依赖与资源容器。
//【参数】           Source（输入参数）：被移动的项目对象，移动后处于有效但未指定状态。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project::Project(Project&& Source) noexcept = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::operator=
//【函数功能】       移动赋值：接管源项目的名称、任务、依赖与资源容器。
//【参数】           Source（输入参数）：被移动的项目对象，移动后处于有效但未指定状态。
//【返回值】         Project&，返回自身引用以支持连续赋值。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project& Project::operator=(Project&& Source) noexcept = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::~Project
//【函数功能】       析构项目；任务由智能指针自动释放，无需额外清理。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Project::~Project() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetName
//【函数功能】       读取项目名称。
//【参数】           无
//【返回值】         const std::string&，项目名称的只读引用。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const std::string& Project::GetName() const
{
    return m_Name;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::SetName
//【函数功能】       设置项目名称，并校验名称非空。
//【参数】           Name（输入参数）：新的项目名称，不允许为空字符串。
//【返回值】         无；名称为空时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::SetName(const std::string& Name)
{
    if (Name.empty()) {                               //项目名称不允许为空
        throw std::invalid_argument("Project name cannot be empty.");
    }
    //检查名称中是否含有空白字符
    for (char Char : Name) {
        if (std::isspace(static_cast<unsigned char>(Char)) != 0) {
            throw std::invalid_argument(
                "Name must not contain whitespace characters.");
        }
    }
    m_Name = Name;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::AddBasicTask
//【函数功能】       新增一个普通任务：校验任务名不重复后追加到任务列表末尾，
//                   并重建任务前驱后继缓存。
//【参数】           Name（输入参数）：任务名称，项目内必须唯一；
//                   Duration（输入参数）：任务工期，正整数，单位：天。
//【返回值】         std::size_t，新任务在任务列表中的索引；任务名重复时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::AddBasicTask(const std::string& Name, int Duration)
{
    if (HasTaskName(Name)) {                          //任务名在项目内必须唯一
        throw std::invalid_argument("Task name already exists.");
    }
    std::size_t Index = m_Tasks.size();               //新任务追加在列表末尾
    m_Tasks.push_back(std::make_unique<BasicTask>(Name, Duration));
    RebuildTaskRelations();
    return Index;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::AddMilestoneTask
//【函数功能】       新增一个里程碑任务（工期为 0）：校验任务名不重复后追加到任务列表末尾，
//                   并重建任务前驱后继缓存。
//【参数】           Name（输入参数）：任务名称，项目内必须唯一。
//【返回值】         std::size_t，新任务在任务列表中的索引；任务名重复时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::AddMilestoneTask(const std::string& Name)
{
    if (HasTaskName(Name)) {                          //任务名在项目内必须唯一
        throw std::invalid_argument("Task name already exists.");
    }
    std::size_t Index = m_Tasks.size();               //新任务追加在列表末尾
    m_Tasks.push_back(std::make_unique<MilestoneTask>(Name));
    RebuildTaskRelations();
    return Index;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::RemoveTask
//【函数功能】       删除指定索引的任务：删除涉及该任务的全部依赖，并把索引大于它的
//                   依赖端点整体前移一位，最后重建任务前驱后继缓存。
//【参数】           TaskIndex（输入参数）：待删除任务在任务列表中的索引。
//【返回值】         无；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::RemoveTask(std::size_t TaskIndex)
{
    CheckTaskIndex(TaskIndex);
    m_Tasks.erase(m_Tasks.begin() + static_cast<long long>(TaskIndex));

    std::vector<Dependency> UpdatedDependencies;      //删除后仍保留的依赖，端点已平移
    for (const Dependency& CurrentDependency : m_Dependencies) {
        if (CurrentDependency.HasTask(TaskIndex)) {
            continue;                                 //涉及被删任务的依赖直接丢弃
        }

        std::size_t Predecessor = CurrentDependency.GetPredecessor();
        std::size_t Successor = CurrentDependency.GetSuccessor();
        if (Predecessor > TaskIndex) {                //被删任务之后的索引整体前移一位
            --Predecessor;
        }
        if (Successor > TaskIndex) {
            --Successor;
        }
        UpdatedDependencies.push_back(
            Dependency(Predecessor,
                       Successor,
                       CurrentDependency.GetType(),
                       CurrentDependency.GetLag()));
    }
    m_Dependencies = UpdatedDependencies;
    RebuildTaskRelations();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::UpdateTask
//【函数功能】       更新指定任务的名称与工期：按新工期重建任务对象（工期 0 转为里程碑，
//                   否则为普通任务），并迁移原任务的前驱后继、调度结果与资源分配。
//【参数】           TaskIndex（输入参数）：待更新任务在任务列表中的索引；
//                   Name（输入参数）：任务的新名称，不得与其他任务重名；
//                   Duration（输入参数）：任务的新工期，非负整数，0 表示里程碑。
//【返回值】         无；索引越界、重名或工期为负时抛出异常。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::UpdateTask(std::size_t TaskIndex,
                         const std::string& Name,
                         int Duration)
{
    CheckTaskIndex(TaskIndex);
    if (HasTaskName(Name, TaskIndex)) {               //允许与自身同名，不得与他人重名
        throw std::invalid_argument("Task name already exists.");
    }
    if (Duration < 0) {                               //工期不允许为负
        throw std::invalid_argument("Task duration cannot be negative.");
    }

    std::unique_ptr<Task> NewTask;                    //按新工期重建的任务对象
    if (Duration == 0) {                              //工期 0 一律作为里程碑任务
        NewTask = std::make_unique<MilestoneTask>(Name);
    }
    else {
        NewTask = std::make_unique<BasicTask>(Name, Duration);
    }

    //迁移原任务的前驱后继关系与调度结果
    NewTask->SetPredecessors(m_Tasks[TaskIndex]->GetPredecessors());
    NewTask->SetSuccessors(m_Tasks[TaskIndex]->GetSuccessors());
    NewTask->SetSchedule(m_Tasks[TaskIndex]->GetES(),
                         m_Tasks[TaskIndex]->GetEF(),
                         m_Tasks[TaskIndex]->GetLS(),
                         m_Tasks[TaskIndex]->GetLF());
    if (NewTask->IsResourceAllocatable()) {           //里程碑不能占用资源，迁移前先判断
        for (const ResourceAllocation& Allocation :
             m_Tasks[TaskIndex]->GetResources()) {
            NewTask->AddResource(Allocation.GetResourceIndex(),
                                 Allocation.GetQuantity());
        }
    }
    m_Tasks[TaskIndex] = std::move(NewTask);          //用新任务对象替换原任务
    RebuildTaskRelations();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::AddResource
//【函数功能】       新增一个资源：校验资源名不重复后追加到资源列表末尾。
//【参数】           Name（输入参数）：资源名称，项目内必须唯一；
//                   UnitCost（输入参数）：资源单位成本。
//【返回值】         std::size_t，新资源在资源列表中的索引；资源名重复时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::AddResource(const std::string& Name, double UnitCost)
{
    if (HasResourceName(Name)) {                      //资源名在项目内必须唯一
        throw std::invalid_argument("Resource name already exists.");
    }
    std::size_t Index = m_Resources.size();           //新资源追加在列表末尾
    m_Resources.push_back(Resource(Name, UnitCost));
    return Index;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::AssignResource
//【函数功能】       给指定任务分配指定资源及占用数量。
//【参数】           TaskIndex（输入参数）：目标任务在任务列表中的索引；
//                   ResourceIndex（输入参数）：被占用资源在资源列表中的索引；
//                   Quantity（输入参数）：占用数量，正整数。
//【返回值】         无；任务或资源索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::AssignResource(std::size_t TaskIndex,
                             std::size_t ResourceIndex,
                             int Quantity)
{
    CheckTaskIndex(TaskIndex);
    CheckResourceIndex(ResourceIndex);
    m_Tasks[TaskIndex]->AddResource(ResourceIndex, Quantity);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::AddDependency
//【函数功能】       新增一条依赖关系：校验其引用的任务索引有效且同一对任务间不存在
//                   重复依赖，然后加入依赖列表并重建任务前驱后继缓存。
//【参数】           CurrentDependency（输入参数）：待加入的依赖关系对象。
//【返回值】         无；引用非法或依赖重复时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::AddDependency(const Dependency& CurrentDependency)
{
    if (!CurrentDependency.IsValidForTaskCount(m_Tasks.size())) {
        //依赖引用了不存在的任务索引
        throw std::invalid_argument("Dependency references invalid tasks.");
    }
    if (HasDependency(CurrentDependency.GetPredecessor(),
                      CurrentDependency.GetSuccessor())) {
        //同一对任务之间的依赖不允许重复
        throw std::invalid_argument("Dependency already exists.");
    }
    m_Dependencies.push_back(CurrentDependency);
    RebuildTaskRelations();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::RemoveDependency
//【函数功能】       按依赖列表索引删除一条依赖关系，并重建任务前驱后继缓存。
//【参数】           DependencyIndex（输入参数）：待删除依赖在依赖列表中的索引。
//【返回值】         无；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::RemoveDependency(std::size_t DependencyIndex)
{
    if (DependencyIndex >= m_Dependencies.size()) {   //依赖索引必须在范围内
        throw std::out_of_range("Dependency index is out of range.");
    }
    m_Dependencies.erase(
        m_Dependencies.begin() + static_cast<long long>(DependencyIndex));
    RebuildTaskRelations();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::RemoveDependency
//【函数功能】       按前置任务与后置任务索引查找并删除对应依赖关系，并重建任务
//                   前驱后继缓存。
//【参数】           Predecessor（输入参数）：依赖的前置任务索引；
//                   Successor（输入参数）：依赖的后置任务索引。
//【返回值】         无；对应依赖不存在时抛出 std::invalid_argument。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::RemoveDependency(std::size_t Predecessor, std::size_t Successor)
{
    CheckTaskIndex(Predecessor);
    CheckTaskIndex(Successor);

    //在依赖列表中查找首条匹配给定前置、后置任务的依赖
    auto iter = std::find_if(
        m_Dependencies.begin(),
        m_Dependencies.end(),
        [Predecessor, Successor](const Dependency& CurrentDependency) {
            return CurrentDependency.HasEndpoints(Predecessor, Successor);
        });
    if (iter == m_Dependencies.end()) {               //未找到匹配依赖
        throw std::invalid_argument("Dependency does not exist.");
    }
    m_Dependencies.erase(iter);
    RebuildTaskRelations();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetTaskCount
//【函数功能】       读取项目中的任务数量。
//【参数】           无
//【返回值】         std::size_t，任务列表中的任务个数。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::GetTaskCount() const
{
    return m_Tasks.size();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetDependencyCount
//【函数功能】       读取项目中的依赖关系数量。
//【参数】           无
//【返回值】         std::size_t，依赖列表中的依赖个数。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::GetDependencyCount() const
{
    return m_Dependencies.size();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetResourceCount
//【函数功能】       读取项目中的资源数量。
//【参数】           无
//【返回值】         std::size_t，资源列表中的资源个数。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
std::size_t Project::GetResourceCount() const
{
    return m_Resources.size();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetTask
//【函数功能】       按索引读取任务（只读版本）。
//【参数】           Index（输入参数）：任务在任务列表中的索引。
//【返回值】         const Task&，对应任务的只读引用；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const Task& Project::GetTask(std::size_t Index) const
{
    CheckTaskIndex(Index);
    return *m_Tasks[Index];
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetTask
//【函数功能】       按索引读取任务（可修改版本）。
//【参数】           Index（输入参数）：任务在任务列表中的索引。
//【返回值】         Task&，对应任务的可修改引用；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
Task& Project::GetTask(std::size_t Index)
{
    CheckTaskIndex(Index);
    return *m_Tasks[Index];
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetDependency
//【函数功能】       按索引读取依赖关系。
//【参数】           Index（输入参数）：依赖在依赖列表中的索引。
//【返回值】         const Dependency&，对应依赖的只读引用；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const Dependency& Project::GetDependency(std::size_t Index) const
{
    if (Index >= m_Dependencies.size()) {             //依赖索引必须在范围内
        throw std::out_of_range("Dependency index is out of range.");
    }
    return m_Dependencies[Index];
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetResource
//【函数功能】       按索引读取资源。
//【参数】           Index（输入参数）：资源在资源列表中的索引。
//【返回值】         const Resource&，对应资源的只读引用；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const Resource& Project::GetResource(std::size_t Index) const
{
    CheckResourceIndex(Index);
    return m_Resources[Index];
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetTasks
//【函数功能】       读取整个任务列表。
//【参数】           无
//【返回值】         const std::vector<std::unique_ptr<Task>>&，任务列表的只读引用。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const std::vector<std::unique_ptr<Task>>& Project::GetTasks() const
{
    return m_Tasks;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetDependencies
//【函数功能】       读取整个依赖关系列表。
//【参数】           无
//【返回值】         const std::vector<Dependency>&，依赖列表的只读引用。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const std::vector<Dependency>& Project::GetDependencies() const
{
    return m_Dependencies;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetResources
//【函数功能】       读取整个资源列表。
//【参数】           无
//【返回值】         const std::vector<Resource>&，资源列表的只读引用。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
const std::vector<Resource>& Project::GetResources() const
{
    return m_Resources;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetTaskTotalCost
//【函数功能】       计算指定任务占用资源的总成本（各项占用数量乘以资源单位成本求和）。
//【参数】           TaskIndex（输入参数）：目标任务在任务列表中的索引。
//【返回值】         double，该任务的资源总成本；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
double Project::GetTaskTotalCost(std::size_t TaskIndex) const
{
    CheckTaskIndex(TaskIndex);
    return m_Tasks[TaskIndex]->CalculateTotalCost(m_Resources);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::GetProjectTotalCost
//【函数功能】       计算全项目所有任务的资源总成本之和。
//【参数】           无
//【返回值】         double，项目的资源总成本。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
double Project::GetProjectTotalCost() const
{
    double TotalCost = 0.0;                           //各任务成本的累加结果
    for (std::size_t Index = 0; Index < m_Tasks.size(); ++Index) {
        TotalCost += GetTaskTotalCost(Index);
    }
    return TotalCost;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::HasTaskName
//【函数功能】       判断给定任务名是否已被其他任务使用；可指定一个忽略比较的任务索引，
//                   用于任务改名时允许与自身同名。
//【参数】           Name（输入参数）：待检查的任务名称；
//                   IgnoredIndex（输入参数）：忽略比较的任务索引，默认取 std::size_t
//                   最大值表示不忽略任何任务。
//【返回值】         bool，存在同名任务返回 true，否则返回 false。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
bool Project::HasTaskName(const std::string& Name,
                          std::size_t IgnoredIndex) const
{
    for (std::size_t Index = 0; Index < m_Tasks.size(); ++Index) {
        //跳过被忽略的任务后逐个比较名称
        if ((Index != IgnoredIndex) && (m_Tasks[Index]->GetName() == Name)) {
            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::HasResourceName
//【函数功能】       判断给定资源名是否已被某个资源使用。
//【参数】           Name（输入参数）：待检查的资源名称。
//【返回值】         bool，存在同名资源返回 true，否则返回 false。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
bool Project::HasResourceName(const std::string& Name) const
{
    for (const Resource& CurrentResource : m_Resources) {
        if (CurrentResource.HasSameName(Name)) {      //逐个资源比较名称
            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::HasDependency
//【函数功能】       判断指定前置、后置任务之间是否已存在依赖关系。
//【参数】           Predecessor（输入参数）：依赖的前置任务索引；
//                   Successor（输入参数）：依赖的后置任务索引。
//【返回值】         bool，已存在对应依赖返回 true，否则返回 false。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
bool Project::HasDependency(std::size_t Predecessor,
                            std::size_t Successor) const
{
    for (const Dependency& CurrentDependency : m_Dependencies) {
        if (CurrentDependency.HasEndpoints(Predecessor, Successor)) {
            return true;                              //找到匹配的依赖
        }
    }
    return false;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::RebuildTaskRelations
//【函数功能】       按依赖列表重建各任务的前驱、后继索引缓存：先汇总每个任务的前驱
//                   与后继集合，再统一写回各任务对象。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::RebuildTaskRelations()
{
    std::vector<std::vector<std::size_t>> Predecessors(m_Tasks.size());
    std::vector<std::vector<std::size_t>> Successors(m_Tasks.size());

    //遍历依赖列表，汇总每个任务的前驱与后继集合
    for (const Dependency& CurrentDependency : m_Dependencies) {
        if (CurrentDependency.IsValidForTaskCount(m_Tasks.size())) {
            Successors[CurrentDependency.GetPredecessor()].push_back(
                CurrentDependency.GetSuccessor());
            Predecessors[CurrentDependency.GetSuccessor()].push_back(
                CurrentDependency.GetPredecessor());
        }
    }

    //把汇总结果统一写回各任务对象
    for (std::size_t Index = 0; Index < m_Tasks.size(); ++Index) {
        m_Tasks[Index]->SetPredecessors(Predecessors[Index]);
        m_Tasks[Index]->SetSuccessors(Successors[Index]);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::ClearSchedule
//【函数功能】       清空所有任务的调度结果（ES/EF/LS/LF），使项目回到未调度状态。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::ClearSchedule()
{
    for (auto& CurrentTask : m_Tasks) {
        CurrentTask->ClearSchedule();
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::CheckTaskIndex
//【函数功能】       内部校验：任务索引越界时抛出异常，供各任务访问接口复用。
//【参数】           Index（输入参数）：待校验的任务索引。
//【返回值】         无；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::CheckTaskIndex(std::size_t Index) const
{
    if (Index >= m_Tasks.size()) {                    //任务索引必须在范围内
        throw std::out_of_range("Task index is out of range.");
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Project::CheckResourceIndex
//【函数功能】       内部校验：资源索引越界时抛出异常，供各资源访问接口复用。
//【参数】           Index（输入参数）：待校验的资源索引。
//【返回值】         无；索引越界时抛出 std::out_of_range。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       无
//-------------------------------------------------------------------------------------------------------------------
void Project::CheckResourceIndex(std::size_t Index) const
{
    if (Index >= m_Resources.size()) {                //资源索引必须在范围内
        throw std::out_of_range("Resource index is out of range.");
    }
}
