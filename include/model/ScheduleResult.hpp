//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ScheduleResult.hpp
//【功能模块和目的】         声明 CPM 调度结果类，聚合一次调度得到的总工期、拓扑序、关键路径
//                           与各任务时间参数表，供界面展示与导出使用。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#ifndef SCHEDULE_RESULT_HPP
#define SCHEDULE_RESULT_HPP

//TaskScheduleInfo 任务时间参数类所属头文件
#include "model/TaskScheduleInfo.hpp"

//std::size_t 所属头文件
#include <cstddef>
//std::map 所属头文件
#include <map>
//std::vector 所属头文件
#include <vector>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             ScheduleResult
//【功能】             CPM 调度结果：保存本次调度是否成功、项目总工期、拓扑排序序列、
//                     关键路径任务序列，以及任务下标到其时间参数（TaskScheduleInfo）
//                     的映射。
//【接口说明】         IsSuccessful / GetProjectDuration / GetTopologicalOrder /
//                     GetCriticalPath / GetTaskSchedules 读取各项结果；
//                     MarkSuccessful / SetProjectDuration / SetTopologicalOrder /
//                     AddCriticalTask / SetTaskSchedule 供调度器填充结果。
//【开发者及日期】     2024013215, 2026-07-05
//【更改记录】         2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
class ScheduleResult {
public:
    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：构造未成功、总工期为零的空结果
    ScheduleResult();
    // 拷贝构造函数：复制成功标志、总工期与各序列、时间表
    ScheduleResult(const ScheduleResult& Source);
    // 拷贝赋值运算符：复制成功标志、总工期与各序列、时间表
    ScheduleResult& operator=(const ScheduleResult& Source);
    // 析构函数：容器成员自动释放，无需额外清理
    ~ScheduleResult();

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（均有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 返回本次调度是否成功完成
    bool IsSuccessful() const;
    // 返回项目总工期（天）
    int GetProjectDuration() const;
    // 返回任务的拓扑排序序列
    const std::vector<std::size_t>& GetTopologicalOrder() const;
    // 返回关键路径上的任务下标序列
    const std::vector<std::size_t>& GetCriticalPath() const;
    // 返回任务下标到其时间参数的映射
    const std::map<std::size_t, TaskScheduleInfo>& GetTaskSchedules() const;

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Setter 成员函数（供调度器填充结果）
    //-----------------------------------------------------------------------------------------------------------
    // 标记本次调度为成功
    void MarkSuccessful();
    // 设置项目总工期（天）
    void SetProjectDuration(int Duration);
    // 设置任务的拓扑排序序列
    void SetTopologicalOrder(const std::vector<std::size_t>& Order);
    // 向关键路径追加一个关键任务下标
    void AddCriticalTask(std::size_t TaskIndex);
    // 记录指定任务的时间参数
    void SetTaskSchedule(std::size_t TaskIndex,
                         const TaskScheduleInfo& ScheduleInfo);

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有数据成员
    //-----------------------------------------------------------------------------------------------------------
    bool m_bIsSuccessful;                                    //本次调度是否成功
    int m_iProjectDuration;                                  //项目总工期（天）
    std::vector<std::size_t> m_TopologicalOrder;             //拓扑排序序列
    std::vector<std::size_t> m_CriticalPath;                 //关键路径任务序列
    std::map<std::size_t, TaskScheduleInfo> m_TaskSchedules; //任务下标到时间参数的映射
};

#endif
