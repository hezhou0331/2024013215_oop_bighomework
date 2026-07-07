//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 CPMScheduler.cpp
//【功能模块和目的】         实现关键路径法（CPM）调度器：校验项目、按拓扑序正推最早时间、
//                           逆推最晚时间，计算总工期、各任务时间参数与关键任务集合。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-05 修复逆推取最晚开始需与总工期上界取小的问题，
//                           并按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
//CPMScheduler 类所属头文件
#include "model/CPMScheduler.hpp"

//Project 模型类所属头文件
#include "model/Project.hpp"
//ProjectValidator 项目校验器类所属头文件
#include "model/ProjectValidator.hpp"

//std::max、std::min 所属头文件
#include <algorithm>
//std::numeric_limits 所属头文件
#include <limits>
//std::logic_error 所属头文件
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::CPMScheduler
//【函数功能】       默认构造 CPM 调度器；调度器无内部状态，无需额外初始化。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler::CPMScheduler() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::CPMScheduler
//【函数功能】       拷贝构造 CPM 调度器；调度器无内部状态，采用默认逐成员拷贝。
//【参数】           Source（输入参数）：被拷贝的调度器对象。
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler::CPMScheduler(const CPMScheduler& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::operator=
//【函数功能】       拷贝赋值 CPM 调度器；调度器无内部状态，采用默认逐成员赋值。
//【参数】           Source（输入参数）：赋值来源的调度器对象。
//【返回值】         返回自身引用，支持连续赋值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
CPMScheduler& CPMScheduler::operator=(const CPMScheduler& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       CPMScheduler::~CPMScheduler
//【函数功能】       析构 CPM 调度器；调度器不持有资源，无需额外清理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
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
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-05 逆推阶段最晚开始与总工期上界取小，修复越过完工时间的问题。
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

    //四组时间参数数组，下标与项目任务表一致
    //入度为 0 的任务（起点）最早开始时间为 0；其他任务初值为 INT_MIN 以允许负 Lag
    std::vector<int> EarliestStarts(TargetProject.GetTaskCount(),
                                    std::numeric_limits<int>::min());
    std::vector<int> EarliestFinishes(TargetProject.GetTaskCount(), 0);
    std::vector<int> LatestStarts(TargetProject.GetTaskCount(), 0);
    std::vector<int> LatestFinishes(TargetProject.GetTaskCount(), 0);

    //统计每个任务的入度，初值为 0 的任务作为项目起点
    std::vector<int> Indegrees(TargetProject.GetTaskCount(), 0);
    for (const Dependency& CurrentDependency
         : TargetProject.GetDependencies()) {
        ++Indegrees[CurrentDependency.GetSuccessor()];
    }
    for (std::size_t Index = 0; Index < TargetProject.GetTaskCount(); ++Index) {
        if (Indegrees[Index] == 0) {
            EarliestStarts[Index] = 0;
        }
    }

    //正推阶段：按拓扑序确定每个任务的最早完成，并把约束沿依赖推向后继
    for (std::size_t Index : Result.GetTopologicalOrder()) {
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

    //总工期为全体任务最早完成时间的最大值
    int ProjectDuration = std::numeric_limits<int>::min();
    for (std::size_t Index = 0; Index < EarliestFinishes.size(); ++Index) {
        ProjectDuration = std::max(ProjectDuration, EarliestFinishes[Index]);
    }
    Result.SetProjectDuration(ProjectDuration);

    //逆推初值：所有任务最晚完成先取总工期，最晚开始随工期折算
    for (std::size_t Index = 0; Index < TargetProject.GetTaskCount();
         ++Index) {
        LatestFinishes[Index] = Result.GetProjectDuration();
        LatestStarts[Index] = LatestFinishes[Index]
            - TargetProject.GetTask(Index).GetDuration();
        TargetProject.GetTask(Index).SetSchedule(EarliestStarts[Index],
                                                 EarliestFinishes[Index],
                                                 LatestStarts[Index],
                                                 LatestFinishes[Index]);
    }

    //逆推阶段：按逆拓扑序由后继依赖收紧每个任务的最晚开始时间
    for (auto iter = Result.GetTopologicalOrder().rbegin();
         iter != Result.GetTopologicalOrder().rend();
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
            TargetProject.GetTask(Current).SetSchedule(
                EarliestStarts[Current],
                EarliestFinishes[Current],
                LatestStarts[Current],
                LatestFinishes[Current]);
        }
    }

    //收集结果：记录各任务时间参数，松弛为零的任务加入关键路径
    for (std::size_t Index : Result.GetTopologicalOrder()) {
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

    return Result;
}
