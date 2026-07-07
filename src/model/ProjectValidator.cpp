//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ProjectValidator.cpp
//【功能模块和目的】         实现项目校验器类，基于图算法检查依赖图无环、无孤立任务、起止任务
//                           存在及全部任务可达等调度前提。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
//ProjectValidator 类所属头文件
#include "model/ProjectValidator.hpp"

//Project 模型类所属头文件
#include "model/Project.hpp"

//std::queue 所属头文件
#include <queue>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::ProjectValidator
//【函数功能】       默认构造项目校验器；校验器无内部状态，无需额外初始化。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectValidator::ProjectValidator() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::ProjectValidator
//【函数功能】       拷贝构造项目校验器；校验器无内部状态，直接采用默认行为。
//【参数】           Source（输入参数）：作为拷贝来源的校验器对象。
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectValidator::ProjectValidator(const ProjectValidator& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::operator=
//【函数功能】       拷贝赋值项目校验器；校验器无内部状态，直接采用默认行为。
//【参数】           Source（输入参数）：作为赋值来源的校验器对象。
//【返回值】         ProjectValidator&，返回自身引用以支持连续赋值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectValidator& ProjectValidator::operator=(
    const ProjectValidator& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::~ProjectValidator
//【函数功能】       析构项目校验器；校验器不持有资源，无需额外清理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectValidator::~ProjectValidator() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::Validate
//【函数功能】       汇总执行全部检查：任务非空、依赖引用有效、依赖图无环、无孤立任务、
//                   存在起始与结束任务、所有任务均在起止路径上，逐项累积错误信息。
//【参数】           SourceProject（输入参数）：待校验的项目模型。
//【返回值】         ValidationResult，包含全部错误信息的校验结果，无错误即校验通过。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ValidationResult ProjectValidator::Validate(const Project& SourceProject) const
{
    ValidationResult Result;                          //逐项累积错误信息的校验结果
    std::size_t TaskCount = SourceProject.GetTaskCount();

    if (TaskCount == 0) {                             //空项目无需继续后续检查
        Result.AddError("Project has no tasks.");
        return Result;
    }

    //检查每条依赖引用的任务索引是否有效
    for (const Dependency& CurrentDependency
         : SourceProject.GetDependencies()) {
        if (!CurrentDependency.IsValidForTaskCount(TaskCount)) {
            Result.AddError("A dependency references a missing task.");
        }
    }

    if (!IsDag(SourceProject)) {                      //依赖图必须无环
        Result.AddError("Dependency graph has a cycle.");
    }

    //统计每个任务的入度与出度，用于孤立任务检查
    std::vector<std::vector<std::size_t>> Adjacency
        = BuildAdjacency(SourceProject);
    std::vector<int> Indegrees = BuildIndegrees(SourceProject);
    std::vector<int> Outdegrees(TaskCount, 0);        //每个任务的直接后继个数

    for (std::size_t Index = 0; Index < TaskCount; ++Index) {
        Outdegrees[Index] = static_cast<int>(Adjacency[Index].size());
    }

    if (TaskCount > 1) {                              //多任务项目才检查孤立任务
        for (std::size_t Index = 0; Index < TaskCount; ++Index) {
            //入度、出度都为 0 的任务与其他任务无任何联系
            if ((Indegrees[Index] == 0) && (Outdegrees[Index] == 0)) {
                Result.AddError("Task " + std::to_string(Index)
                    + " is isolated.");
            }
        }
    }

    if (!HasProperStartEnd(SourceProject)) {          //必须同时存在起始与结束任务
        Result.AddError("Project must have at least one start and one end.");
    }

    if (!HasReachableNodes(SourceProject)) {          //所有任务都要在起止路径上
        Result.AddError("Some tasks are not on a valid start-to-end path.");
    }

    return Result;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::IsDag
//【函数功能】       判断项目依赖图是否为有向无环图：拓扑序包含全部任务即无环。
//【参数】           SourceProject（输入参数）：待判断的项目模型。
//【返回值】         bool，依赖图无环返回 true，存在环路返回 false。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
bool ProjectValidator::IsDag(const Project& SourceProject) const
{
    //有环时环上任务的入度无法降为 0，拓扑序必然缺少任务
    return GetTopologicalOrder(SourceProject).size()
        == SourceProject.GetTaskCount();
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::GetTopologicalOrder
//【函数功能】       用 Kahn 算法求任务的一个拓扑序：从入度为 0 的任务出发逐个出队，
//                   并把出队任务的后继入度减一，入度降为 0 的后继继续入队。
//【参数】           SourceProject（输入参数）：待求拓扑序的项目模型。
//【返回值】         std::vector<std::size_t>，按拓扑序排列的任务索引序列；依赖图有环时
//                   序列长度小于任务总数。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::size_t> ProjectValidator::GetTopologicalOrder(
    const Project& SourceProject) const
{
    std::vector<std::vector<std::size_t>> Adjacency
        = BuildAdjacency(SourceProject);
    std::vector<int> Indegrees = BuildIndegrees(SourceProject);
    std::queue<std::size_t> Pending;                  //当前入度为 0、待输出的任务队列
    std::vector<std::size_t> Order;                   //拓扑序结果

    //入度为 0 的任务没有未处理的前驱，可以率先入队
    for (std::size_t Index = 0; Index < Indegrees.size(); ++Index) {
        if (Indegrees[Index] == 0) {
            Pending.push(Index);
        }
    }

    while (!Pending.empty()) {
        std::size_t Current = Pending.front();        //取出一个入度为 0 的任务
        Pending.pop();
        Order.push_back(Current);

        //当前任务输出后，其全部后继的入度各减一
        for (std::size_t Next : Adjacency[Current]) {
            --Indegrees[Next];
            if (Indegrees[Next] == 0) {               //前驱全部处理完毕，可以入队
                Pending.push(Next);
            }
        }
    }

    return Order;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::HasReachableNodes
//【函数功能】       判断是否所有任务都在起止路径上：从全部起始任务正向 BFS 求可达集合，
//                   从全部结束任务沿逆邻接表反向 BFS 求可达集合，任一任务不同时属于
//                   两个集合即不满足。
//【参数】           SourceProject（输入参数）：待检查的项目模型。
//【返回值】         bool，所有任务都位于某条起止路径上返回 true，否则返回 false。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
bool ProjectValidator::HasReachableNodes(const Project& SourceProject) const
{
    std::size_t TaskCount = SourceProject.GetTaskCount();
    std::vector<std::vector<std::size_t>> Adjacency
        = BuildAdjacency(SourceProject);
    std::vector<std::vector<std::size_t>> ReverseAdjacency
        = BuildReverseAdjacency(SourceProject);
    std::vector<int> Indegrees = BuildIndegrees(SourceProject);
    std::vector<int> Outdegrees(TaskCount, 0);        //每个任务的直接后继个数

    for (std::size_t Index = 0; Index < TaskCount; ++Index) {
        Outdegrees[Index] = static_cast<int>(Adjacency[Index].size());
    }

    std::vector<bool> ReachableFromStart(TaskCount, false); //起始任务正向可达标记
    std::queue<std::size_t> Pending;                  //BFS 待扩展的任务队列
    //所有入度为 0 的任务都是起始任务，作为正向 BFS 的源点
    for (std::size_t Index = 0; Index < TaskCount; ++Index) {
        if (Indegrees[Index] == 0) {
            Pending.push(Index);
            ReachableFromStart[Index] = true;
        }
    }
    //正向 BFS：沿后继方向扩展起始任务的可达集合
    while (!Pending.empty()) {
        std::size_t Current = Pending.front();
        Pending.pop();
        for (std::size_t Next : Adjacency[Current]) {
            if (!ReachableFromStart[Next]) {
                ReachableFromStart[Next] = true;
                Pending.push(Next);
            }
        }
    }

    std::vector<bool> CanReachEnd(TaskCount, false);  //能到达结束任务的标记
    //所有出度为 0 的任务都是结束任务，作为反向 BFS 的源点
    for (std::size_t Index = 0; Index < TaskCount; ++Index) {
        if (Outdegrees[Index] == 0) {
            Pending.push(Index);
            CanReachEnd[Index] = true;
        }
    }
    //反向 BFS：沿前驱方向扩展能到达结束任务的集合
    while (!Pending.empty()) {
        std::size_t Current = Pending.front();
        Pending.pop();
        for (std::size_t Previous : ReverseAdjacency[Current]) {
            if (!CanReachEnd[Previous]) {
                CanReachEnd[Previous] = true;
                Pending.push(Previous);
            }
        }
    }

    //每个任务都必须既能从起点到达、又能到达终点
    for (std::size_t Index = 0; Index < TaskCount; ++Index) {
        if (!ReachableFromStart[Index] || !CanReachEnd[Index]) {
            return false;
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::HasProperStartEnd
//【函数功能】       判断项目是否同时存在起始任务（入度为 0）与结束任务（出度为 0）。
//【参数】           SourceProject（输入参数）：待检查的项目模型。
//【返回值】         bool，同时存在起始与结束任务返回 true，否则返回 false。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
bool ProjectValidator::HasProperStartEnd(const Project& SourceProject) const
{
    std::vector<int> Indegrees = BuildIndegrees(SourceProject);
    std::vector<std::vector<std::size_t>> Adjacency
        = BuildAdjacency(SourceProject);
    bool HasStart = false;                            //是否存在入度为 0 的起始任务
    bool HasEnd = false;                              //是否存在出度为 0 的结束任务

    for (std::size_t Index = 0; Index < SourceProject.GetTaskCount();
         ++Index) {
        if (Indegrees[Index] == 0) {                  //无前驱即起始任务
            HasStart = true;
        }
        if (Adjacency[Index].empty()) {               //无后继即结束任务
            HasEnd = true;
        }
    }
    return HasStart && HasEnd;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::BuildAdjacency
//【函数功能】       按依赖列表构建邻接表：对每条有效依赖，把后置任务索引记入前置任务
//                   的直接后继集合。
//【参数】           SourceProject（输入参数）：提供任务数与依赖列表的项目模型。
//【返回值】         std::vector<std::vector<std::size_t>>，邻接表，第 i 项为任务 i 的
//                   直接后继任务索引集合。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::vector<std::size_t>> ProjectValidator::BuildAdjacency(
    const Project& SourceProject) const
{
    std::vector<std::vector<std::size_t>> Adjacency(
        SourceProject.GetTaskCount());
    for (const Dependency& CurrentDependency
         : SourceProject.GetDependencies()) {
        //只统计引用有效的依赖，非法依赖由 Validate 单独报错
        if (CurrentDependency.IsValidForTaskCount(
                SourceProject.GetTaskCount())) {
            Adjacency[CurrentDependency.GetPredecessor()].push_back(
                CurrentDependency.GetSuccessor());
        }
    }
    return Adjacency;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::BuildReverseAdjacency
//【函数功能】       按依赖列表构建逆邻接表：对每条有效依赖，把前置任务索引记入后置任务
//                   的直接前驱集合，供反向可达性搜索使用。
//【参数】           SourceProject（输入参数）：提供任务数与依赖列表的项目模型。
//【返回值】         std::vector<std::vector<std::size_t>>，逆邻接表，第 i 项为任务 i 的
//                   直接前驱任务索引集合。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::vector<std::size_t>> ProjectValidator::BuildReverseAdjacency(
    const Project& SourceProject) const
{
    std::vector<std::vector<std::size_t>> ReverseAdjacency(
        SourceProject.GetTaskCount());
    for (const Dependency& CurrentDependency
         : SourceProject.GetDependencies()) {
        //只统计引用有效的依赖，非法依赖由 Validate 单独报错
        if (CurrentDependency.IsValidForTaskCount(
                SourceProject.GetTaskCount())) {
            ReverseAdjacency[CurrentDependency.GetSuccessor()].push_back(
                CurrentDependency.GetPredecessor());
        }
    }
    return ReverseAdjacency;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectValidator::BuildIndegrees
//【函数功能】       按依赖列表统计每个任务的入度：每条有效依赖使其后置任务入度加一。
//【参数】           SourceProject（输入参数）：提供任务数与依赖列表的项目模型。
//【返回值】         std::vector<int>，入度表，第 i 项为任务 i 的直接前驱个数。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::vector<int> ProjectValidator::BuildIndegrees(
    const Project& SourceProject) const
{
    std::vector<int> Indegrees(SourceProject.GetTaskCount(), 0);
    for (const Dependency& CurrentDependency
         : SourceProject.GetDependencies()) {
        //只统计引用有效的依赖，非法依赖由 Validate 单独报错
        if (CurrentDependency.IsValidForTaskCount(
                SourceProject.GetTaskCount())) {
            ++Indegrees[CurrentDependency.GetSuccessor()];
        }
    }
    return Indegrees;
}
