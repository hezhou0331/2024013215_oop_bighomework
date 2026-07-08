//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 Project.hpp
//【功能模块和目的】         声明项目模型类，统一管理项目中的任务、依赖关系与资源三类实体。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
#ifndef PROJECT_HPP
#define PROJECT_HPP

//Dependency 依赖关系类所属头文件
#include "model/Dependency.hpp"
//Resource 资源类所属头文件
#include "model/Resource.hpp"
//Task 任务抽象基类所属头文件
#include "model/Task.hpp"

//std::numeric_limits 所属头文件
#include <limits>
//std::unique_ptr 所属头文件
#include <memory>
//std::string 所属头文件
#include <string>
//std::vector 所属头文件
#include <vector>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             Project
//【功能】             项目模型类：以项目名称为标识，统一持有并管理任务列表（多态
//                     Task 指针）、依赖关系列表与资源列表，是调度、校验、导入导出
//                     等功能的核心数据模型。
//【接口说明】         提供任务/资源/依赖的增删改查接口（AddBasicTask、AddResource、
//                     AddDependency 等），提供按索引取元素与取整个容器的只读访问，
//                     提供任务与项目总成本计算、名称与依赖存在性判断，以及依赖变动
//                     后重建任务前驱后继缓存（RebuildTaskRelations）与清空调度结果
//                     （ClearSchedule）的维护接口；非法名称、索引越界等错误以异常报告。
//【开发者及日期】     2024013215, 2026-07-05
//【更改记录】         2026-07-05 按课程 C++ 编码规范 V1.3 修订注释与标识符命名。
//-------------------------------------------------------------------------------------------------------------------
class Project {
public:
    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：以默认名称 "UntitledProject" 创建空项目
    Project();
    // 带参构造函数：以给定名称创建空项目，名称为空时抛出异常
    explicit Project(const std::string& Name);
    // 拷贝构造函数：深拷贝任务列表（逐个 Clone），复制依赖与资源列表
    Project(const Project& Source);
    // 拷贝赋值运算符：按拷贝再交换方式深拷贝整个项目
    Project& operator=(const Project& Source);
    // 移动构造函数：接管源项目的任务、依赖与资源容器
    Project(Project&& Source) noexcept;
    // 移动赋值运算符：接管源项目的任务、依赖与资源容器
    Project& operator=(Project&& Source) noexcept;
    // 析构函数：任务由智能指针管理，无需额外清理
    ~Project();

    //-----------------------------------------------------------------------------------------------------------
    //项目名称的 Getter 与 Setter（Getter 有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 返回项目名称
    const std::string& GetName() const;
    // 设置项目名称，名称为空时抛出异常
    void SetName(const std::string& Name);

    //-----------------------------------------------------------------------------------------------------------
    //任务操作（新增、删除与更新）
    //-----------------------------------------------------------------------------------------------------------
    // 新增普通任务（给定名称与工期），返回其在任务列表中的索引
    std::size_t AddBasicTask(const std::string& Name, int Duration);
    // 新增里程碑任务（工期为 0），返回其在任务列表中的索引
    std::size_t AddMilestoneTask(const std::string& Name);
    // 删除指定索引的任务，并同步删除或平移相关依赖
    void RemoveTask(std::size_t TaskIndex);
    // 更新指定任务的名称与工期，工期 0 转为里程碑任务并保留原有关联
    void UpdateTask(std::size_t TaskIndex,
                    const std::string& Name,
                    int Duration);

    //-----------------------------------------------------------------------------------------------------------
    //资源操作（新增与分配）
    //-----------------------------------------------------------------------------------------------------------
    // 新增资源（给定名称与单位成本），返回其在资源列表中的索引
    std::size_t AddResource(const std::string& Name, double UnitCost);
    // 给指定任务分配指定资源及占用数量
    void AssignResource(std::size_t TaskIndex,
                        std::size_t ResourceIndex,
                        int Quantity);

    //-----------------------------------------------------------------------------------------------------------
    //依赖操作（新增与删除）
    //-----------------------------------------------------------------------------------------------------------
    // 新增依赖关系，非法引用或重复依赖时抛出异常
    void AddDependency(const Dependency& CurrentDependency);
    // 按依赖列表索引删除依赖关系
    void RemoveDependency(std::size_t DependencyIndex);
    // 按前置任务与后置任务索引删除依赖关系
    void RemoveDependency(std::size_t Predecessor, std::size_t Successor);

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（除按索引取任务的可修改版本外，均有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 返回任务数量
    std::size_t GetTaskCount() const;
    // 返回依赖关系数量
    std::size_t GetDependencyCount() const;
    // 返回资源数量
    std::size_t GetResourceCount() const;

    // 按索引返回任务（只读），索引越界时抛出异常
    const Task& GetTask(std::size_t Index) const;
    // 按索引返回任务（可修改），索引越界时抛出异常
    Task& GetTask(std::size_t Index);
    // 按索引返回依赖关系（只读），索引越界时抛出异常
    const Dependency& GetDependency(std::size_t Index) const;
    // 按索引返回资源（只读），索引越界时抛出异常
    const Resource& GetResource(std::size_t Index) const;

    // 返回整个任务列表的只读引用
    const std::vector<std::unique_ptr<Task>>& GetTasks() const;
    // 返回整个依赖关系列表的只读引用
    const std::vector<Dependency>& GetDependencies() const;
    // 返回整个资源列表的只读引用
    const std::vector<Resource>& GetResources() const;

    // 计算指定任务占用资源的总成本
    double GetTaskTotalCost(std::size_t TaskIndex) const;
    // 计算全项目所有任务的资源总成本
    double GetProjectTotalCost() const;

    // 判断任务名是否已存在，可指定一个忽略比较的任务索引（用于改名场景）
    bool HasTaskName(
        const std::string& Name,
        std::size_t IgnoredIndex = std::numeric_limits<std::size_t>::max())
        const;
    // 判断资源名是否已存在
    bool HasResourceName(const std::string& Name) const;
    // 判断指定前置、后置任务之间是否已存在依赖关系
    bool HasDependency(std::size_t Predecessor, std::size_t Successor) const;

    //-----------------------------------------------------------------------------------------------------------
    //一致性维护接口（重建前驱后继缓存、清空调度结果）
    //-----------------------------------------------------------------------------------------------------------
    // 按依赖列表重建各任务的前驱、后继索引缓存
    void RebuildTaskRelations();
    // 清空所有任务的调度结果（ES/EF/LS/LF）
    void ClearSchedule();

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有工具函数（均有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 内部校验：任务索引越界时抛出 std::out_of_range
    void CheckTaskIndex(std::size_t Index) const;
    // 内部校验：资源索引越界时抛出 std::out_of_range
    void CheckResourceIndex(std::size_t Index) const;

    //-----------------------------------------------------------------------------------------------------------
    //私有数据成员
    //-----------------------------------------------------------------------------------------------------------
    std::string m_Name;                          //项目名称，非空
    std::vector<std::unique_ptr<Task>> m_Tasks;  //任务列表，多态持有普通任务与里程碑任务
    std::vector<Dependency> m_Dependencies;      //任务间依赖关系列表
    std::vector<Resource> m_Resources;           //资源列表
};

#endif
