//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ProjectController.hpp
//【功能模块和目的】         声明 MVC 控制器单例类：向界面层提供项目管理全部业务接口，
//                           以状态枚举报告结果、以信息类传递数据，不做任何文本格式化。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-07 重构为"状态枚举 + 信息类"接口，文本格式化移交界面层。
//-------------------------------------------------------------------------------------------------------------------
#ifndef PROJECT_SCHEDULER_PROJECT_CONTROLLER_HPP
#define PROJECT_SCHEDULER_PROJECT_CONTROLLER_HPP

//ProjectRepository 项目仓库类所属头文件
#include "model/ProjectRepository.hpp"

//std::size_t 所属头文件
#include <cstddef>
//std::string 所属头文件
#include <string>
//std::vector 所属头文件
#include <vector>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             ProjectController
//【功能】             项目调度器的业务流程/控制器类（MVC 中的 C），单例模式：接收界面层
//                     命令，调用模型层完成项目、任务、依赖、资源的管理与校验调度，以
//                     状态枚举 RES 报告执行结果，以嵌套信息类回传数据；不含任何输入
//                     输出与文本格式化，格式化由界面层完成。
//【接口说明】         静态 GetInstance 获取唯一实例；CreateProject/ImportProject/
//                     ExportProject 管理项目整体；AddTask/UpdateTask/RemoveTask/
//                     GetTaskList/GetTaskRelations 管理任务；AddDependency/
//                     RemoveDependency/GetDependencyList 管理依赖；AddResource/
//                     AssignResource/GetResourceList 管理资源；ValidateProject 校验、
//                     RunSchedule 执行 CPM 计算、CollectStatistics 汇总统计；静态
//                     ResultToText 把状态枚举翻译为描述文本；LastError 为最近一次修改类
//                     操作的失败详情（只读常引用，const 查询接口不改写它）。
//【开发者及日期】     2024013215, 2026-07-05
//【更改记录】         2026-07-07 重构为"状态枚举 + 信息类"接口，文本格式化移交界面层。
//-------------------------------------------------------------------------------------------------------------------
class ProjectController {
public:
    //-----------------------------------------------------------------------------------------------------------
    //与界面层交换数据的枚举与嵌套信息类（纯数据载体，成员无读写规则，故为公有）
    //-----------------------------------------------------------------------------------------------------------

    //【类名】RES（enum class）【功能】控制器全部公开业务接口的返回状态
    //【接口说明】SUCCESS 表示成功，其余枚举量对应各类失败原因
    //【开发者及日期】2024013215, 2026-07-07
    enum class RES : int {
        SUCCESS            = 0,    //操作成功
        INVALID_ARGUMENT   = 1,    //参数被模型层拒绝（重名、非法工期/成本/数量等）
        INDEX_OUT_OF_RANGE = 2,    //任务/依赖/资源索引越界
        SELF_DEPENDENCY    = 3,    //前置与后置为同一任务
        CYCLE_DETECTED     = 4,    //添加依赖将导致循环依赖
        FILE_ERROR         = 5,    //文件类型不支持或无法打开
        PARSE_ERROR        = 6,    //文件内容格式非法
        INVALID_PROJECT    = 7,    //项目未通过合理性校验，无法调度
        UNKNOWN_ERROR      = 8     //其他未预期的错误
    };

    //【类名】TaskInfo【功能】单个任务的展示信息（纯数据载体）
    //【接口说明】含索引、名称、工期、里程碑标志、总成本、CPM 时间参数与前驱后继索引表
    //【开发者及日期】2024013215, 2026-07-07
    class TaskInfo {
    public:
        // 默认构造函数：数值成员清零
        TaskInfo();
        // 拷贝构造函数：逐成员拷贝
        TaskInfo(const TaskInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        TaskInfo& operator=(const TaskInfo& Source);
        // 析构函数：无额外资源需要释放
        ~TaskInfo();

        std::size_t Index;                        //任务在容器中的索引
        std::string Name;                         //任务名称
        int Duration;                             //工期（天），里程碑为 0
        bool IsMilestone;                         //是否为里程碑任务
        double TotalCost;                         //任务占用资源的总成本
        int EarlyStart;                           //最早开始时间 ES
        int EarlyFinish;                          //最早完成时间 EF
        int LateStart;                            //最晚开始时间 LS
        int LateFinish;                           //最晚完成时间 LF
        int SlackDays;                            //松弛（可推迟）天数
        std::vector<std::size_t> Predecessors;    //全部前驱任务索引
        std::vector<std::size_t> Successors;      //全部后继任务索引
    };

    //【类名】DependencyInfo【功能】单条依赖的展示信息（纯数据载体）
    //【接口说明】含前置/后置任务索引、类型文本与 Lag 天数
    //【开发者及日期】2024013215, 2026-07-07
    class DependencyInfo {
    public:
        // 默认构造函数：数值成员清零
        DependencyInfo();
        // 拷贝构造函数：逐成员拷贝
        DependencyInfo(const DependencyInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        DependencyInfo& operator=(const DependencyInfo& Source);
        // 析构函数：无额外资源需要释放
        ~DependencyInfo();

        std::size_t Predecessor;                  //前置任务索引
        std::size_t Successor;                    //后置任务索引
        std::string TypeText;                     //依赖类型文本（FS/SS/FF/SF）
        int LagDays;                              //滞后（正）或提前（负）天数
    };

    //【类名】ResourceInfo【功能】单种资源的展示信息（纯数据载体）
    //【接口说明】含资源名称与单位时间成本
    //【开发者及日期】2024013215, 2026-07-07
    class ResourceInfo {
    public:
        // 默认构造函数：数值成员清零
        ResourceInfo();
        // 拷贝构造函数：逐成员拷贝
        ResourceInfo(const ResourceInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        ResourceInfo& operator=(const ResourceInfo& Source);
        // 析构函数：无额外资源需要释放
        ~ResourceInfo();

        std::string Name;                         //资源名称
        double UnitCost;                          //单位时间成本
    };

    //【类名】ValidationInfo【功能】项目合理性校验结论（纯数据载体）
    //【接口说明】IsValid 为总体结论，Messages 为逐条错误描述
    //【开发者及日期】2024013215, 2026-07-07
    class ValidationInfo {
    public:
        // 默认构造函数：结论初始化为不合法
        ValidationInfo();
        // 拷贝构造函数：逐成员拷贝
        ValidationInfo(const ValidationInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        ValidationInfo& operator=(const ValidationInfo& Source);
        // 析构函数：无额外资源需要释放
        ~ValidationInfo();

        bool IsValid;                             //项目是否满足全部合理性条件
        std::vector<std::string> Messages;        //不合法时的逐条错误描述
    };

    //【类名】ScheduleInfo【功能】CPM 调度计算结论（纯数据载体）
    //【接口说明】含项目总工期与按拓扑序排列的关键路径任务索引表
    //【开发者及日期】2024013215, 2026-07-07
    class ScheduleInfo {
    public:
        // 默认构造函数：总工期清零
        ScheduleInfo();
        // 拷贝构造函数：逐成员拷贝
        ScheduleInfo(const ScheduleInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        ScheduleInfo& operator=(const ScheduleInfo& Source);
        // 析构函数：无额外资源需要释放
        ~ScheduleInfo();

        int ProjectDuration;                      //项目总工期（天）
        std::vector<std::size_t> CriticalPath;    //关键路径任务索引（按拓扑序）
    };

    //【类名】StatisticsInfo【功能】项目统计信息汇总（纯数据载体）
    //【接口说明】含项目名、三类元素总数、总成本及一次调度计算的结论
    //【开发者及日期】2024013215, 2026-07-07
    class StatisticsInfo {
    public:
        // 默认构造函数：数值成员清零
        StatisticsInfo();
        // 拷贝构造函数：逐成员拷贝
        StatisticsInfo(const StatisticsInfo& Source);
        // 拷贝赋值运算符：逐成员赋值
        StatisticsInfo& operator=(const StatisticsInfo& Source);
        // 析构函数：无额外资源需要释放
        ~StatisticsInfo();

        std::string ProjectName;                  //项目名称
        std::size_t TaskCount;                    //任务总数
        std::size_t DependencyCount;              //依赖总数
        std::size_t ResourceCount;                //资源总数
        double TotalCost;                         //项目总成本
        ScheduleInfo Schedule;                    //附带的调度计算结论
    };

    //任务信息列表类型
    using TaskInfoList = std::vector<TaskInfo>;
    //依赖信息列表类型
    using DependencyInfoList = std::vector<DependencyInfo>;
    //资源信息列表类型
    using ResourceInfoList = std::vector<ResourceInfo>;

    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为（单例模式）
    //-----------------------------------------------------------------------------------------------------------
    // 单例模式，禁止拷贝构造
    ProjectController(const ProjectController& Source) = delete;
    // 单例模式，禁止拷贝赋值
    ProjectController& operator=(const ProjectController& Source) = delete;
    // 析构函数：成员仓库由其自身析构函数清理
    ~ProjectController();

    //-----------------------------------------------------------------------------------------------------------
    //静态成员函数
    //-----------------------------------------------------------------------------------------------------------
    // 获取控制器的全局唯一实例
    static ProjectController& GetInstance();
    // 把返回状态枚举翻译为英文描述文本，供界面显示
    static std::string ResultToText(RES Result);

    //-----------------------------------------------------------------------------------------------------------
    //项目整体操作（修改类接口，失败详情写入 LastError）
    //-----------------------------------------------------------------------------------------------------------
    // 按名称新建空项目并设为当前项目
    RES CreateProject(const std::string& Name);
    // 按扩展名选择已注册导入器，把文件导入为当前项目
    RES ImportProject(const std::string& FileName);
    // 按扩展名选择已注册导出器，把当前项目写入文件
    RES ExportProject(const std::string& FileName) const;

    //-----------------------------------------------------------------------------------------------------------
    //任务操作
    //-----------------------------------------------------------------------------------------------------------
    // 添加任务：工期 0 自动创建里程碑，否则创建普通任务
    RES AddTask(const std::string& Name, int Duration);
    // 修改指定索引任务的名称与工期（工期 0 与正数间自动转换任务类型，索引不变）
    RES UpdateTask(std::size_t Index, const std::string& Name, int Duration);
    // 删除指定索引的任务，连带清理相关依赖与资源分配
    RES RemoveTask(std::size_t Index);
    // 收集全部任务的展示信息（输出参数 InfoList）
    RES GetTaskList(TaskInfoList& InfoList) const;
    // 收集指定任务自身及其全部前驱、后继任务的展示信息（输出参数 QueriedTask/Predecessors/Successors）
    RES GetTaskRelations(std::size_t Index,
                         TaskInfo& QueriedTask,
                         TaskInfoList& Predecessors,
                         TaskInfoList& Successors) const;

    //-----------------------------------------------------------------------------------------------------------
    //依赖操作
    //-----------------------------------------------------------------------------------------------------------
    // 添加依赖：拒绝自依赖与重复依赖，先在副本上试加并确认不成环后才提交
    RES AddDependency(std::size_t Predecessor,
                      std::size_t Successor,
                      const std::string& TypeText,
                      int LagDays);
    // 删除指定索引的依赖
    RES RemoveDependency(std::size_t Index);
    // 收集全部依赖的展示信息（输出参数 InfoList）
    RES GetDependencyList(DependencyInfoList& InfoList) const;

    //-----------------------------------------------------------------------------------------------------------
    //资源操作
    //-----------------------------------------------------------------------------------------------------------
    // 添加资源（名称不可重复）
    RES AddResource(const std::string& Name, double UnitCost);
    // 把指定资源按给定数量分配给指定任务（里程碑不可占用资源）
    RES AssignResource(std::size_t TaskIndex,
                       std::size_t ResourceIndex,
                       int Quantity);
    // 收集全部资源的展示信息（输出参数 InfoList）
    RES GetResourceList(ResourceInfoList& InfoList) const;

    //-----------------------------------------------------------------------------------------------------------
    //校验与调度
    //-----------------------------------------------------------------------------------------------------------
    // 校验当前项目合理性，结论写入输出参数 Info
    RES ValidateProject(ValidationInfo& Info) const;
    // 对当前项目执行 CPM 调度计算，结论写入输出参数 Info
    RES RunSchedule(ScheduleInfo& Info);
    // 汇总当前项目统计信息（含一次调度计算），结论写入输出参数 Info
    RES CollectStatistics(StatisticsInfo& Info);

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有的构造行为与内部工具（单例模式）
    //-----------------------------------------------------------------------------------------------------------
    // 单例模式，私有默认构造函数；构造时注册 PPM 导入器与导出器
    ProjectController();
    // 采集指定任务的展示信息，供列表与关系查询共用
    TaskInfo BuildTaskInfo(const Project& SourceProject,
                           std::size_t Index) const;

    //-----------------------------------------------------------------------------------------------------------
    //私有数据成员
    //-----------------------------------------------------------------------------------------------------------
    ProjectRepository m_Repository;    //持有当前项目的仓库（MVC 中 C 对 M 的唯一入口）
    std::string m_LastError;           //最近一次修改类操作的失败详情文本

public:
    //-----------------------------------------------------------------------------------------------------------
    //公有常引用数据成员（只读访问，无写规则）
    //-----------------------------------------------------------------------------------------------------------
    const std::string& LastError{m_LastError};    //最近一次修改类操作的失败详情（只读）
};

#endif
