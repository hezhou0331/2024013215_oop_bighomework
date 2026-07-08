//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ProjectValidator.hpp
//【功能模块和目的】         声明项目校验器类，检查项目依赖图的合法性（无环、连通、有起止任务等）。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#ifndef PROJECT_VALIDATOR_HPP
#define PROJECT_VALIDATOR_HPP

//ValidationResult 校验结果类所属头文件
#include "model/ValidationResult.hpp"

//std::size_t 所属头文件
#include <cstddef>
//std::vector 所属头文件
#include <vector>

// 前置声明：Project 类在 Project.hpp 中定义
class Project;

//-------------------------------------------------------------------------------------------------------------------
//【类名】             ProjectValidator
//【功能】             项目校验器：把项目的任务与依赖抽象为有向图，检查项目是否满足
//                     调度前提，包括任务非空、依赖引用有效、依赖图无环、无孤立任务、
//                     存在起始与结束任务、所有任务均处于起止路径上。
//【接口说明】         Validate 汇总各项检查并返回带错误信息列表的 ValidationResult；
//                     IsDag 判断依赖图是否为有向无环图；GetTopologicalOrder 用 Kahn
//                     算法求拓扑序（有环时结果不含全部任务）；私有辅助函数负责构建
//                     邻接表、逆邻接表与入度表及可达性判断。
//【开发者及日期】     2024013215, 2026-07-05
//【更改记录】         2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
class ProjectValidator {
public:
    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：校验器无内部状态，无需额外初始化
    ProjectValidator();
    // 拷贝构造函数：校验器无内部状态，直接采用默认行为
    ProjectValidator(const ProjectValidator& Source);
    // 拷贝赋值运算符：校验器无内部状态，直接采用默认行为
    ProjectValidator& operator=(const ProjectValidator& Source);
    // 析构函数：校验器不持有资源，无需额外清理
    ~ProjectValidator();

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（均有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 汇总执行全部检查，返回含所有错误信息的校验结果
    ValidationResult Validate(const Project& SourceProject) const;
    // 判断项目依赖图是否为有向无环图（DAG）
    bool IsDag(const Project& SourceProject) const;
    // 用 Kahn 算法求任务的一个拓扑序，有环时结果不含全部任务
    std::vector<std::size_t> GetTopologicalOrder(
        const Project& SourceProject) const;

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有工具函数（均有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 判断是否所有任务都位于某条从起始任务到结束任务的路径上
    bool HasReachableNodes(const Project& SourceProject) const;
    // 按依赖列表构建邻接表：每个任务的直接后继索引集合
    std::vector<std::vector<std::size_t>> BuildAdjacency(
        const Project& SourceProject) const;
    // 按依赖列表构建逆邻接表：每个任务的直接前驱索引集合
    std::vector<std::vector<std::size_t>> BuildReverseAdjacency(
        const Project& SourceProject) const;
    // 按依赖列表统计每个任务的入度（前驱个数）
    std::vector<int> BuildIndegrees(const Project& SourceProject) const;
};

#endif
