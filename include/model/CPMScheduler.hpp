//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 CPMScheduler.hpp
//【功能模块和目的】         声明关键路径法（CPM）调度器类，对项目做正推/逆推计算，得到各任务
//                           的最早/最晚开始与完成时间、总工期和关键路径。
//【开发者及日期】           刘江宇, 2026-07-05
//【修改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#ifndef CPM_SCHEDULER_HPP
#define CPM_SCHEDULER_HPP
#include "model/ScheduleResult.hpp"
#include <cstddef>
#include <vector>

// 前置声明：Project 类在 Project.hpp 中定义
class Project;

//-------------------------------------------------------------------------------------------------------------------
//【类名】             CPMScheduler
//【功能】             关键路径法调度器：先校验项目合法性，再按拓扑序正推最早时间、
//                     逆推最晚时间，计算总工期、各任务时间参数与关键任务集合，
//                     调度器本身无内部状态。
//【接口说明】         Calculate 对给定项目执行完整 CPM 计算，把时间参数写回项目中的
//                     各任务，并以 ScheduleResult 返回总工期、拓扑序、任务时间表与
//                     关键路径；项目不合法时抛出异常。构造、拷贝、赋值与析构均为
//                     平凡操作。
//【开发者及日期】     刘江宇, 2026-07-05
//【修改记录】         2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
class CPMScheduler {
public:
    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：调度器无内部状态，无需初始化
    CPMScheduler();
    // 拷贝构造函数：调度器无内部状态，采用默认行为
    CPMScheduler(const CPMScheduler& Source);
    // 拷贝赋值运算符：调度器无内部状态，采用默认行为
    CPMScheduler& operator=(const CPMScheduler& Source);
    // 析构函数：无资源需要释放
    ~CPMScheduler();

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 对项目执行 CPM 计算，写回各任务时间参数并返回调度结果
    ScheduleResult Calculate(Project& TargetProject) const;

private:
    //-----------------------------------------------------------------------------------------------------------
    //内部计算步骤
    //-----------------------------------------------------------------------------------------------------------
    // 初始化最早开始时间数组，入度为 0 的任务 ES 取 0
    void InitializeEarliestStarts(const Project& SourceProject,
                                  std::vector<int>& EarliestStarts) const;
    // 按拓扑序执行正推，计算 ES/EF 并传播依赖约束
    void RunForwardPass(Project& TargetProject,
                        const std::vector<std::size_t>& TopologicalOrder,
                        std::vector<int>& EarliestStarts,
                        std::vector<int>& EarliestFinishes) const;
    // 根据最早完成时间设置项目总工期
    void SetDurationFromEF(
        const std::vector<int>& EarliestFinishes,
        ScheduleResult& Result) const;
    // 用项目总工期初始化全部任务 LS/LF
    void InitializeLatestTimes(Project& TargetProject,
                               const std::vector<int>& EarliestStarts,
                               const std::vector<int>& EarliestFinishes,
                               std::vector<int>& LatestStarts,
                               std::vector<int>& LatestFinishes,
                               int ProjectDuration) const;
    // 按逆拓扑序执行逆推，收紧各任务 LS/LF
    void RunBackwardPass(Project& TargetProject,
                         const std::vector<std::size_t>& TopologicalOrder,
                         const std::vector<int>& EarliestStarts,
                         const std::vector<int>& EarliestFinishes,
                         std::vector<int>& LatestStarts,
                         std::vector<int>& LatestFinishes) const;
    // 把任务时间表与关键任务写入结果对象
    void CollectScheduleResult(Project& TargetProject,
                               const std::vector<std::size_t>& TopologicalOrder,
                               ScheduleResult& Result) const;
};

#endif
