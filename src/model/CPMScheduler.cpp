//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 CPMScheduler.cpp
//【功能模块和目的】         实现关键路径法（CPM）调度器：校验项目、按拓扑序正推最早时间、
//                           逆推最晚时间，计算总工期、各任务时间参数与关键任务集合。
//【开发者及日期】           刘江宇, 2026-07-05
//【修改记录】               2026-07-05 修复逆推取最晚开始需与总工期上界取小的问题，
//                           并按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#include "model/CPMScheduler.hpp"
#include "model/Project.hpp"
#include "model/ProjectValidator.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::CPMScheduler
//【函数功能】       默认构造 CPM 调度器；调度器无内部状态，无需额外初始化。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler::CPMScheduler() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::CPMScheduler
//【函数功能】       拷贝构造 CPM 调度器；调度器无内部状态，采用默认逐成员拷贝。
//【参数】           Source（输入参数）：被拷贝的调度器对象。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler::CPMScheduler(const CPMScheduler& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::operator=
//【函数功能】       拷贝赋值 CPM 调度器；调度器无内部状态，采用默认逐成员赋值。
//【参数】           Source（输入参数）：赋值来源的调度器对象。
//【返回值】         返回自身引用，支持连续赋值。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler& CPMScheduler::operator=(const CPMScheduler& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::~CPMScheduler
//【函数功能】       析构 CPM 调度器；调度器不持有资源，无需额外清理。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler::~CPMScheduler() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::Calculate
//【函数功能】       对项目执行完整的 CPM 计算：先校验项目合法性并取得拓扑序；正推阶段
//                   按拓扑序沿依赖传播各任务最早开始/最早完成时间；总工期取全体任务
//                   最早完成的最大值；逆推阶段先以总工期为各任务最晚完成的初值，再按
//                   逆拓扑序由后继依赖收紧最晚开始/最晚完成时间；最后把时间参数写回
//                   各任务，收集任务时间表与关键任务（松弛为零）到结果对象。
//【参数】           TargetProject（输入/输出参数）：待调度的项目，计算后其中各任务的
//                   时间参数被写回更新。
//【返回值】         返回 ScheduleResult，含成功标志、总工期、拓扑序、各任务时间表与
//                   关键路径；项目不合法时抛出 std::logic_error。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-05 逆推阶段最晚开始与总工期上界取小，修复越过完工时间的问题。
//-------------------------------------------------------------------------------------------------------------------
ScheduleResult CPMScheduler::Calculate(Project& TargetProject) const
{
    ProjectValidator Validator;
    //调度前必须通过合法性校验（含环路检测），否则拓扑序无意义
    ValidationResult Validation = Validator.Validate(TargetProject);
    if (!Validation.IsValid()) {
        throw std::logic_error("Cannot schedule an invalid project.");
    }

    ScheduleResult Result;
    Result.MarkSuccessful();
    Result.SetTopologicalOrder(Validator.GetTopologicalOrder(TargetProject));

    TargetProject.ClearSchedule();

    std::vector<int> EarliestStarts(TargetProject.GetTaskCount(),
                                    std::numeric_limits<int>::min());
    std::vector<int> EarliestFinishes(TargetProject.GetTaskCount(), 0);
    std::vector<int> LatestStarts(TargetProject.GetTaskCount(), 0);
    std::vector<int> LatestFinishes(TargetProject.GetTaskCount(), 0);

    InitializeEarliestStarts(TargetProject, EarliestStarts);
    RunForwardPass(TargetProject,
                   Result.GetTopologicalOrder(),
                   EarliestStarts,
                   EarliestFinishes);
    SetDurationFromEF(EarliestFinishes, Result);
    InitializeLatestTimes(TargetProject,
                          EarliestStarts,
                          EarliestFinishes,
                          LatestStarts,
                          LatestFinishes,
                          Result.GetProjectDuration());
    RunBackwardPass(TargetProject,
                    Result.GetTopologicalOrder(),
                    EarliestStarts,
                    EarliestFinishes,
                    LatestStarts,
                    LatestFinishes);
    CollectScheduleResult(TargetProject, Result.GetTopologicalOrder(), Result);

    return Result;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::InitializeEarliestStarts
//【函数功能】       统计任务入度，把所有入度为 0 的起始任务最早开始时间设置为 0。
//【参数】           SourceProject（输入参数）：待调度项目；
//                   EarliestStarts（输出参数）：最早开始时间数组。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::InitializeEarliestStarts(
    const Project& SourceProject,
    std::vector<int>& EarliestStarts) const
{
    std::vector<int> Indegrees(SourceProject.GetTaskCount(), 0);
    for (const Dependency& CurrentDependency : SourceProject.GetDependencies()) {
        ++Indegrees[CurrentDependency.GetSuccessor()];
    }
    for (std::size_t Index = 0; Index < SourceProject.GetTaskCount();
         ++Index) {
        if (Indegrees[Index] == 0) {
            EarliestStarts[Index] = 0;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::RunForwardPass
//【函数功能】       按拓扑序执行 CPM 正推，计算各任务最早完成时间，并沿依赖传播后继
//                   任务的最早开始下界。
//【参数】           TargetProject（输入/输出参数）：待调度项目；
//                   TopologicalOrder（输入参数）：任务拓扑序；
//                   EarliestStarts（输入/输出参数）：最早开始时间数组；
//                   EarliestFinishes（输出参数）：最早完成时间数组。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::RunForwardPass(
    Project& TargetProject,
    const std::vector<std::size_t>& TopologicalOrder,
    std::vector<int>& EarliestStarts,
    std::vector<int>& EarliestFinishes) const
{
    for (std::size_t Index : TopologicalOrder) {
        const Task& CurrentTask = TargetProject.GetTask(Index);
        EarliestFinishes[Index] = EarliestStarts[Index]
            + CurrentTask.GetDuration();
        TargetProject.GetTask(Index).SetSchedule(EarliestStarts[Index],
                                                 EarliestFinishes[Index],
                                                 0,
                                                 0);

        //把当前任务作为前置的每条依赖换算成后继的最早开始下界，取最大值
        for (const Dependency& CurrentDependency
             : TargetProject.GetDependencies()) {
            if (CurrentDependency.GetPredecessor() == Index) {
                std::size_t SuccessorIndex = CurrentDependency.GetSuccessor();
                int NextStart = CurrentDependency.GetSuccessorEarliestStart(
                    TargetProject.GetTask(Index),
                    TargetProject.GetTask(SuccessorIndex));
                EarliestStarts[SuccessorIndex] = std::max(
                    EarliestStarts[SuccessorIndex],
                    NextStart);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::SetDurationFromEF
//【函数功能】       将全体任务最早完成时间的最大值写为项目总工期。
//【参数】           EarliestFinishes（输入参数）：最早完成时间数组；
//                   Result（输出参数）：待写入总工期的调度结果。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::SetDurationFromEF(
    const std::vector<int>& EarliestFinishes,
    ScheduleResult& Result) const
{
    int ProjectDuration = std::numeric_limits<int>::min();
    for (std::size_t Index = 0; Index < EarliestFinishes.size(); ++Index) {
        ProjectDuration = std::max(ProjectDuration, EarliestFinishes[Index]);
    }
    Result.SetProjectDuration(ProjectDuration);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::InitializeLatestTimes
//【函数功能】       以项目总工期为上界初始化各任务最晚完成时间，并按工期折算最晚开始
//                   时间，同时把初始调度时间写回项目任务。
//【参数】           TargetProject（输入/输出参数）：待调度项目；
//                   EarliestStarts（输入参数）：最早开始时间数组；
//                   EarliestFinishes（输入参数）：最早完成时间数组；
//                   LatestStarts（输出参数）：最晚开始时间数组；
//                   LatestFinishes（输出参数）：最晚完成时间数组；
//                   ProjectDuration（输入参数）：项目总工期。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::InitializeLatestTimes(
    Project& TargetProject,
    const std::vector<int>& EarliestStarts,
    const std::vector<int>& EarliestFinishes,
    std::vector<int>& LatestStarts,
    std::vector<int>& LatestFinishes,
    int ProjectDuration) const
{
    for (std::size_t Index = 0; Index < TargetProject.GetTaskCount();
         ++Index) {
        LatestFinishes[Index] = ProjectDuration;
        LatestStarts[Index] = LatestFinishes[Index]
            - TargetProject.GetTask(Index).GetDuration();
        TargetProject.GetTask(Index).SetSchedule(EarliestStarts[Index],
                                                 EarliestFinishes[Index],
                                                 LatestStarts[Index],
                                                 LatestFinishes[Index]);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::RunBackwardPass
//【函数功能】       按逆拓扑序执行 CPM 逆推，由后继依赖收紧每个任务的最晚开始和最晚
//                   完成时间。
//【参数】           TargetProject（输入/输出参数）：待调度项目；
//                   TopologicalOrder（输入参数）：任务拓扑序；
//                   EarliestStarts（输入参数）：最早开始时间数组；
//                   EarliestFinishes（输入参数）：最早完成时间数组；
//                   LatestStarts（输入/输出参数）：最晚开始时间数组；
//                   LatestFinishes（输入/输出参数）：最晚完成时间数组。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::RunBackwardPass(
    Project& TargetProject,
    const std::vector<std::size_t>& TopologicalOrder,
    const std::vector<int>& EarliestStarts,
    const std::vector<int>& EarliestFinishes,
    std::vector<int>& LatestStarts,
    std::vector<int>& LatestFinishes) const
{
    for (auto iter = TopologicalOrder.rbegin();
         iter != TopologicalOrder.rend();
         ++iter) {
        std::size_t Current = *iter;
        bool HasSuccessor = false;
        //各后继给出的最晚开始上界取最小值，初值为整型最大值
        int BestLatestStart = std::numeric_limits<int>::max();

        for (const Dependency& CurrentDependency
             : TargetProject.GetDependencies()) {
            if (CurrentDependency.GetPredecessor() == Current) {
                HasSuccessor = true;
                std::size_t SuccessorIndex = CurrentDependency.GetSuccessor();
                int Candidate = CurrentDependency.GetPredecessorLatestStart(
                    TargetProject.GetTask(Current),
                    TargetProject.GetTask(SuccessorIndex));
                BestLatestStart = std::min(BestLatestStart, Candidate);
            }
        }

        // 后继推出的最晚开始还要与总工期上界取小，防止越过项目完工时间
        if (HasSuccessor) {
            LatestStarts[Current] = std::min(BestLatestStart,
                                             LatestStarts[Current]);
            LatestFinishes[Current] = LatestStarts[Current]
                + TargetProject.GetTask(Current).GetDuration();
        }
        TargetProject.GetTask(Current).SetSchedule(EarliestStarts[Current],
                                                   EarliestFinishes[Current],
                                                   LatestStarts[Current],
                                                   LatestFinishes[Current]);
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::CollectScheduleResult
//【函数功能】       按拓扑序收集各任务时间参数，写入调度结果，并把关键任务加入关键
//                   路径列表。
//【参数】           TargetProject（输入参数）：已写入调度时间的项目；
//                   TopologicalOrder（输入参数）：任务拓扑序；
//                   Result（输出参数）：待填充任务时间表与关键任务的调度结果。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void CPMScheduler::CollectScheduleResult(
    Project& TargetProject,
    const std::vector<std::size_t>& TopologicalOrder,
    ScheduleResult& Result) const
{
    for (std::size_t Index : TopologicalOrder) {
        const Task& CurrentTask = TargetProject.GetTask(Index);
        TaskScheduleInfo ScheduleInfo(
            CurrentTask.GetES(),
            CurrentTask.GetEF(),
            CurrentTask.GetLS(),
            CurrentTask.GetLF(),
            CurrentTask.GetSlack());
        Result.SetTaskSchedule(Index, ScheduleInfo);
        if (CurrentTask.IsCritical()) {
            Result.AddCriticalTask(Index);
        }
    }
}
