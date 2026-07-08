//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ProjectController.cpp
//【功能模块和目的】         实现项目调度器的控制器单例类
//                           接收界面命令，调用模型层完成
//                           项目、任务、依赖与资源的管理及校验调度
//                           以状态枚举与信息类回传结果，不做任何文本格式化
//【开发者及日期】           刘江宇, 2026-07-05
//【更改记录】               2026-07-07 重构为状态枚举+信息类接口
//                           文本格式化移交界面层
//-------------------------------------------------------------------------------------------------------------------
#include "controller/ProjectController.hpp"
#include "model/CPMScheduler.hpp"
#include "model/Exporter.hpp"
#include "model/Importer.hpp"
#include "model/PPMExporter.hpp"
#include "model/PPMImporter.hpp"
#include "model/ProjectValidator.hpp"
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetInstance
//【函数功能】       获取控制器的全局唯一实例
//                   首次调用时构造函数内静态对象，之后始终返回同一对象
//【参数】           无
//【返回值】         ProjectController&，控制器单例的引用
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ProjectController& ProjectController::GetInstance()
{
    // 函数内静态对象，首次调用时构造且全程唯一
    static ProjectController s_Instance;
    return s_Instance;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ProjectController
//【函数功能】       默认构造控制器对象
//                   向导入/导出器工厂注册PPM格式的具体实现
//                   后续如需支持更多格式，仅需在此追加注册
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 增加PPM导入/导出器的工厂注册
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ProjectController()
    : m_Repository(), m_LastError("")
{
    Importer<Project>::Register<PPMImporter>();    //登记 PPM 格式导入器
    Exporter<Project>::Register<PPMExporter>();    //登记 PPM 格式导出器
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::~ProjectController
//【函数功能】       析构控制器对象
//                   成员仓库由其自身析构函数清理，无需额外处理
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ProjectController::~ProjectController() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetLastError
//【函数功能】       读取最近一次失败操作的详细错误信息
//                   供界面层输出给用户
//【参数】           无
//【返回值】         const std::string&，最近一次失败操作的详细错误信息。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
const std::string& ProjectController::GetLastError() const
{
    return m_LastError;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::CreateProject
//【函数功能】       按给定名称新建一个空项目并设为当前项目，替换仓库中原有项目。
//【参数】           Name（输入参数）：新项目的名称。
//【返回值】         RES，SUCCESS 表示创建成功；名称非法返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::CreateProject(
    const std::string& Name)
{
    m_LastError = "";
    try {
        m_Repository.SetCurrentProject(Project(Name));
        return RES::SUCCESS;
    }
    catch (const std::invalid_argument& Exception) {    //名称为空等非法参数
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ImportProject
//【函数功能】       按文件扩展名从工厂选取已注册导入器，把文件导入为当前项目；扩展名
//                   未注册或文件不可读返回 FILE_ERROR，内容格式非法返回 PARSE_ERROR。
//【参数】           FileName（输入参数）：待导入的项目模型文件路径。
//【返回值】         RES，SUCCESS 表示导入成功；失败详情（含解析错误行号）写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为经导入器工厂按扩展名选取导入器。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::ImportProject(
    const std::string& FileName)
{
    m_LastError = "";
    try {
        //按扩展名取已注册导入器，完成文件到项目对象的转换
        m_Repository.SetCurrentProject(
            Importer<Project>::GetInstanceByFileName(FileName)
                ->LoadFromFile(FileName));
        return RES::SUCCESS;
    }
    catch (const std::invalid_argument& Exception) {    //扩展名未注册或文件不可读
        m_LastError = Exception.what();
        return RES::FILE_ERROR;
    }
    catch (const std::runtime_error& Exception) {       //文件内容格式非法（含行号）
        m_LastError = Exception.what();
        return RES::PARSE_ERROR;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ExportProject
//【函数功能】       按文件扩展名从工厂选取已注册导出器，把当前项目写入指定文件。
//【参数】           FileName（输入参数）：导出目标文件路径。
//【返回值】         RES，SUCCESS 表示导出成功；扩展名未注册或文件不可写返回 FILE_ERROR，
//                   其他异常返回 UNKNOWN_ERROR；失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为经导出器工厂按扩展名选取导出器。
//                   2026-07-07 改为非 const 接口，失败时更新 LastError。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::ExportProject(
    const std::string& FileName)
{
    m_LastError = "";
    try {
        //按扩展名取已注册导出器，完成项目对象到文件的转换
        Exporter<Project>::GetInstanceByFileName(FileName)
            ->SaveToFile(m_Repository.GetCurrentProject(), FileName);
        return RES::SUCCESS;
    }
    catch (const std::invalid_argument& Exception) {    //扩展名未注册或文件不可写
        m_LastError = Exception.what();
        return RES::FILE_ERROR;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::AddTask
//【函数功能】       向当前项目添加任务：工期为 0 时创建里程碑任务，否则创建普通任务。
//【参数】           Name（输入参数）：任务名称；
//                   Duration（输入参数）：任务工期（天），0 表示里程碑。
//【返回值】         RES，SUCCESS 表示添加成功；名称重复或工期非法返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::AddTask(const std::string& Name,
                                                  int Duration)
{
    m_LastError = "";
    try {
        if (Duration == 0) {                            //工期 0 即里程碑任务
            m_Repository.GetCurrentProject().AddMilestoneTask(Name);
        }
        else {                                          //其余按普通任务处理
            m_Repository.GetCurrentProject().AddBasicTask(Name, Duration);
        }
        return RES::SUCCESS;
    }
    catch (const std::invalid_argument& Exception) {    //名称重复或工期非法
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::UpdateTask
//【函数功能】       修改当前项目中指定索引任务的名称与工期；工期在 0 与正数间变化时
//                   任务类型自动转换，索引保持不变。
//【参数】           Index（输入参数）：待修改任务的索引；
//                   Name（输入参数）：任务的新名称；
//                   Duration（输入参数）：任务的新工期（天）。
//【返回值】         RES，SUCCESS 表示修改成功；索引越界返回 INDEX_OUT_OF_RANGE，
//                   参数非法返回 INVALID_ARGUMENT，失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::UpdateTask(std::size_t Index,
                                                     const std::string& Name,
                                                     int Duration)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().UpdateTask(Index, Name, Duration);
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::invalid_argument& Exception) {    //名称重复或工期非法
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::RemoveTask
//【函数功能】       删除当前项目中指定索引的任务，并连带清理与其相关的依赖和资源分配。
//【参数】           Index（输入参数）：待删除任务的索引。
//【返回值】         RES，SUCCESS 表示删除成功；索引越界返回 INDEX_OUT_OF_RANGE，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RemoveTask(std::size_t Index)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().RemoveTask(Index);
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::BuildTaskInfo
//【函数功能】       采集指定任务的展示信息：索引、名称、工期、里程碑标志、总成本、
//                   CPM 时间参数及前驱后继索引表。
//【参数】           SourceProject（输入参数）：被查询的项目；
//                   Index（输入参数）：目标任务的索引。
//【返回值】         TaskInfo，采集到的任务展示信息。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::TaskInfo ProjectController::BuildTaskInfo(
    const Project& SourceProject, std::size_t Index) const
{
    const Task& CurrentTask = SourceProject.GetTask(Index);
    TaskInfo Info;
    Info.SetIndex(Index);
    Info.SetName(CurrentTask.GetName());
    Info.SetDuration(CurrentTask.GetDuration());
    Info.SetMilestone(CurrentTask.GetDuration() == 0);
    Info.SetTotalCost(SourceProject.GetTaskTotalCost(Index));
    Info.SetEarlyStart(CurrentTask.GetES());
    Info.SetEarlyFinish(CurrentTask.GetEF());
    Info.SetLateStart(CurrentTask.GetLS());
    Info.SetLateFinish(CurrentTask.GetLF());
    Info.SetSlackDays(CurrentTask.GetSlack());
    Info.SetPredecessors(CurrentTask.GetPredecessors());
    Info.SetSuccessors(CurrentTask.GetSuccessors());
    return Info;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetTaskList
//【函数功能】       采集当前项目全部任务的展示信息，按容器索引顺序填入输出列表。
//【参数】           InfoList（输出参数）：填充任务展示信息的列表，原有内容被清空。
//【返回值】         RES，恒为 SUCCESS。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类列表。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::GetTaskList(
    TaskInfoList& InfoList) const
{
    const Project& CurrentProject = m_Repository.GetCurrentProject();
    InfoList.clear();
    //按容器索引顺序逐个采集任务信息
    for (std::size_t Index = 0; Index < CurrentProject.GetTaskCount();
         ++Index) {
        InfoList.push_back(BuildTaskInfo(CurrentProject, Index));
    }
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetTaskRelations
//【函数功能】       采集指定任务自身及其全部前驱、后继任务的展示信息，分别填入输出参数。
//【参数】           Index（输入参数）：待查询任务的索引；
//                   QueriedTask（输出参数）：被查询任务自身的展示信息；
//                   Predecessors（输出参数）：前驱任务信息列表，原有内容被清空；
//                   Successors（输出参数）：后继任务信息列表，原有内容被清空。
//【返回值】         RES，SUCCESS 表示查询成功；索引越界返回 INDEX_OUT_OF_RANGE。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类列表。
//                   2026-07-07 增加被查询任务自身信息的输出参数。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::GetTaskRelations(
    std::size_t Index,
    TaskInfo& QueriedTask,
    TaskInfoList& Predecessors,
    TaskInfoList& Successors) const
{
    try {
        const Project& CurrentProject = m_Repository.GetCurrentProject();
        const Task& CurrentTask = CurrentProject.GetTask(Index);
        QueriedTask = BuildTaskInfo(CurrentProject, Index);
        Predecessors.clear();
        Successors.clear();
        //逐个采集前驱任务的展示信息
        for (std::size_t PredecessorIndex : CurrentTask.GetPredecessors()) {
            Predecessors.push_back(
                BuildTaskInfo(CurrentProject, PredecessorIndex));
        }
        //逐个采集后继任务的展示信息
        for (std::size_t SuccessorIndex : CurrentTask.GetSuccessors()) {
            Successors.push_back(
                BuildTaskInfo(CurrentProject, SuccessorIndex));
        }
        return RES::SUCCESS;
    }
    catch (const std::out_of_range&) {                  //索引越界，const 接口不改写 LastError
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::exception&) {                     //其余未预期异常
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::AddDependency
//【函数功能】       向当前项目添加一条依赖：拒绝自依赖，并先在项目副本上试加、经校验器
//                   确认不成环后才提交到当前项目。
//【参数】           Predecessor（输入参数）：前置任务索引；
//                   Successor（输入参数）：后置任务索引；
//                   TypeText（输入参数）：依赖类型文本（FS/SS/FF/SF）；
//                   LagDays（输入参数）：依赖的滞后天数（Lag，可为负）。
//【返回值】         RES，SUCCESS 表示添加成功；自依赖返回 SELF_DEPENDENCY，成环返回
//                   CYCLE_DETECTED，重复依赖或类型非法返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::AddDependency(
    std::size_t Predecessor,
    std::size_t Successor,
    const std::string& TypeText,
    int LagDays)
{
    m_LastError = "";
    try {
        if (Predecessor == Successor) {                 //自依赖直接拒绝
            return RES::SELF_DEPENDENCY;
        }
        Project& TargetProject = m_Repository.GetCurrentProject();
        if ((Predecessor >= TargetProject.GetTaskCount())
            || (Successor >= TargetProject.GetTaskCount())) {
            m_LastError = "Task index is out of range.";
            return RES::INDEX_OUT_OF_RANGE;
        }
        Project CopyProject(TargetProject);             //在副本上试加依赖，校验通过后再提交
        CopyProject.AddDependency(
            Dependency(Predecessor,
                       Successor,
                       Dependency::TypeFromText(TypeText),
                       LagDays));
        ProjectValidator Validator;
        if (!Validator.IsDag(CopyProject)) {            //成环则放弃副本，不影响当前项目
            return RES::CYCLE_DETECTED;
        }
        TargetProject = CopyProject;                    //校验通过，把副本提交为当前项目
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::invalid_argument& Exception) {    //重复依赖、引用非法或类型非法
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::RemoveDependency
//【函数功能】       删除当前项目中指定索引的依赖。
//【参数】           Index（输入参数）：待删除依赖的索引。
//【返回值】         RES，SUCCESS 表示删除成功；索引越界返回 INDEX_OUT_OF_RANGE，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RemoveDependency(std::size_t Index)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().RemoveDependency(Index);
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::RemoveDependency（重载）
//【函数功能】       按前置任务与后置任务索引删除依赖关系；不存在则返回 INVALID_ARGUMENT。
//【参数】           Predecessor（输入参数）：前置任务在容器中的索引；
//                   Successor（输入参数）：后置任务在容器中的索引。
//【返回值】         RES，SUCCESS 表示删除成功；依赖不存在返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RemoveDependency(std::size_t Predecessor,
                                                            std::size_t Successor)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().RemoveDependency(Predecessor, Successor);
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::invalid_argument& Exception) {    //依赖不存在
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetDependencyList
//【函数功能】       采集当前项目全部依赖的展示信息，按容器索引顺序填入输出列表。
//【参数】           InfoList（输出参数）：填充依赖展示信息的列表，原有内容被清空。
//【返回值】         RES，恒为 SUCCESS。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类列表。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::GetDependencyList(
    DependencyInfoList& InfoList) const
{
    const Project& CurrentProject = m_Repository.GetCurrentProject();
    InfoList.clear();
    //按容器索引顺序逐条采集依赖信息
    for (std::size_t Index = 0; Index < CurrentProject.GetDependencyCount();
         ++Index) {
        const Dependency& CurrentDependency
            = CurrentProject.GetDependency(Index);
        DependencyInfo Info;
        Info.SetPredecessor(CurrentDependency.GetPredecessor());
        Info.SetSuccessor(CurrentDependency.GetSuccessor());
        Info.SetTypeText(CurrentDependency.GetTypeText());
        Info.SetLagDays(CurrentDependency.GetLag());
        InfoList.push_back(Info);
    }
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::AddResource
//【函数功能】       向当前项目添加一种资源，名称不可与已有资源重复。
//【参数】           Name（输入参数）：资源名称；
//                   UnitCost（输入参数）：资源的单位时间成本。
//【返回值】         RES，SUCCESS 表示添加成功；名称重复或成本非法返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::AddResource(const std::string& Name,
                                                      double UnitCost)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().AddResource(Name, UnitCost);
        return RES::SUCCESS;
    }
    catch (const std::invalid_argument& Exception) {    //名称重复或单位成本非法
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::AssignResource
//【函数功能】       把指定资源按给定数量分配给当前项目中的指定任务；里程碑任务不可
//                   占用资源。
//【参数】           TaskIndex（输入参数）：目标任务的索引；
//                   ResourceIndex（输入参数）：被分配资源的索引；
//                   Quantity（输入参数）：分配数量（正整数）。
//【返回值】         RES，SUCCESS 表示分配成功；索引越界返回 INDEX_OUT_OF_RANGE，
//                   里程碑占用资源或数量非法返回 INVALID_ARGUMENT，失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为返回状态枚举。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::AssignResource(
    std::size_t TaskIndex,
    std::size_t ResourceIndex,
    int Quantity)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().AssignResource(TaskIndex,
                                                        ResourceIndex,
                                                        Quantity);
        return RES::SUCCESS;
    }
    catch (const std::out_of_range& Exception) {        //索引越界
        m_LastError = Exception.what();
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::invalid_argument& Exception) {    //数量非正等参数校验失败
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::logic_error& Exception) {         //里程碑任务占用资源等业务规则违反
        m_LastError = Exception.what();
        return RES::INVALID_ARGUMENT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetResourceList
//【函数功能】       采集当前项目全部资源的展示信息，按容器索引顺序填入输出列表。
//【参数】           InfoList（输出参数）：填充资源展示信息的列表，原有内容被清空。
//【返回值】         RES，恒为 SUCCESS。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类列表。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::GetResourceList(
    ResourceInfoList& InfoList) const
{
    const Project& CurrentProject = m_Repository.GetCurrentProject();
    InfoList.clear();
    //按容器索引顺序逐个采集资源信息
    for (std::size_t Index = 0; Index < CurrentProject.GetResourceCount();
         ++Index) {
        const Resource& CurrentResource = CurrentProject.GetResource(Index);
        ResourceInfo Info;
        Info.SetName(CurrentResource.GetName());
        Info.SetUnitCost(CurrentResource.GetUnitCost());
        InfoList.push_back(Info);
    }
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ValidateProject
//【函数功能】       调用校验器检查当前项目是否满足调度规则（依赖无环、无孤立悬挂节点、
//                   依赖引用有效），结论与逐条错误信息写入输出参数。
//【参数】           Info（输出参数）：填充校验结论与错误信息。
//【返回值】         RES，恒为 SUCCESS（校验结论在 Info.IsValid() 中）。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::ValidateProject(
    ValidationInfo& Info) const
{
    ProjectValidator Validator;    //校验器无内部状态，按需临时创建
    ValidationResult Result = Validator.Validate(
        m_Repository.GetCurrentProject());
    Info.SetValid(Result.IsValid());
    Info.SetMessages(Result.GetMessages());
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::RunSchedule
//【函数功能】       对当前项目执行关键路径法（CPM）调度计算，把总工期与关键路径写入
//                   输出参数；项目未通过合理性校验时计算被拒绝。
//【参数】           Info（输出参数）：填充项目总工期与关键路径任务索引。
//【返回值】         RES，SUCCESS 表示计算完成；项目不合理返回 INVALID_PROJECT，
//                   失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RunSchedule(ScheduleInfo& Info)
{
    m_LastError = "";
    try {
        CPMScheduler Scheduler;    //调度器无内部状态，按需临时创建
        ScheduleResult Result = Scheduler.Calculate(
            m_Repository.GetCurrentProject());
        Info.SetProjectDuration(Result.GetProjectDuration());
        Info.SetCriticalPath(Result.GetCriticalPath());
        return RES::SUCCESS;
    }
    catch (const std::logic_error& Exception) {         //项目未通过合理性校验
        m_LastError = Exception.what();
        return RES::INVALID_PROJECT;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        m_LastError = Exception.what();
        return RES::UNKNOWN_ERROR;
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::CollectStatistics
//【函数功能】       汇总当前项目统计信息：项目名、任务/依赖/资源总数与总成本，并执行
//                   一次调度计算把总工期与关键路径一并写入输出参数。
//【参数】           Info（输出参数）：填充统计信息与调度结论。
//【返回值】         RES，SUCCESS 表示统计完成；项目不合理无法调度时返回 INVALID_PROJECT
//                   （此时计数与成本字段仍然有效），失败详情写入 LastError。
//【开发者及日期】   刘江宇, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类，并更名以明确该操作会触发调度计算。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::CollectStatistics(
    StatisticsInfo& Info)
{
    const Project& CurrentProject = m_Repository.GetCurrentProject();
    Info.SetProjectName(CurrentProject.GetName());
    Info.SetTaskCount(CurrentProject.GetTaskCount());
    Info.SetDependencyCount(CurrentProject.GetDependencyCount());
    Info.SetResourceCount(CurrentProject.GetResourceCount());
    Info.SetTotalCost(CurrentProject.GetProjectTotalCost());
    ScheduleInfo Schedule;
    RES Result = RunSchedule(Schedule);    //统计附带一次调度计算，返回其结果状态
    Info.SetSchedule(Schedule);
    return Result;
}

//-------------------------------------------------------------------------------------------------------------------
//以下为各嵌套信息类（纯数据载体）的构造、拷贝、赋值与析构实现
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::TaskInfo::TaskInfo
//【函数功能】       默认构造任务信息载体，数值成员清零。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::TaskInfo::TaskInfo()
    : m_uIndex(0), m_Name(""), m_iDuration(0), m_bIsMilestone(false),
      m_rTotalCost(0.0), m_iEarlyStart(0), m_iEarlyFinish(0),
      m_iLateStart(0), m_iLateFinish(0), m_iSlackDays(0),
      m_Predecessors(), m_Successors()
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::TaskInfo::TaskInfo(const TaskInfo& Source) = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::TaskInfo&
ProjectController::TaskInfo::operator=(const TaskInfo& Source) = default;

// 析构函数：无额外资源需要释放
ProjectController::TaskInfo::~TaskInfo() = default;

// 以下为 TaskInfo 的只读访问接口
std::size_t ProjectController::TaskInfo::GetIndex() const
{
    return m_uIndex;
}

const std::string& ProjectController::TaskInfo::GetName() const
{
    return m_Name;
}

int ProjectController::TaskInfo::GetDuration() const
{
    return m_iDuration;
}

bool ProjectController::TaskInfo::IsMilestoneTask() const
{
    return m_bIsMilestone;
}

double ProjectController::TaskInfo::GetTotalCost() const
{
    return m_rTotalCost;
}

int ProjectController::TaskInfo::GetEarlyStart() const
{
    return m_iEarlyStart;
}

int ProjectController::TaskInfo::GetEarlyFinish() const
{
    return m_iEarlyFinish;
}

int ProjectController::TaskInfo::GetLateStart() const
{
    return m_iLateStart;
}

int ProjectController::TaskInfo::GetLateFinish() const
{
    return m_iLateFinish;
}

int ProjectController::TaskInfo::GetSlackDays() const
{
    return m_iSlackDays;
}

const std::vector<std::size_t>&
ProjectController::TaskInfo::GetPredecessors() const
{
    return m_Predecessors;
}

const std::vector<std::size_t>&
ProjectController::TaskInfo::GetSuccessors() const
{
    return m_Successors;
}

// 以下为 TaskInfo 的写入接口，仅控制器层在组装返回数据时使用
void ProjectController::TaskInfo::SetIndex(std::size_t Index)
{
    m_uIndex = Index;
}

void ProjectController::TaskInfo::SetName(const std::string& Name)
{
    m_Name = Name;
}

void ProjectController::TaskInfo::SetDuration(int Duration)
{
    m_iDuration = Duration;
}

void ProjectController::TaskInfo::SetMilestone(bool IsMilestoneValue)
{
    m_bIsMilestone = IsMilestoneValue;
}

void ProjectController::TaskInfo::SetTotalCost(double TotalCost)
{
    m_rTotalCost = TotalCost;
}

void ProjectController::TaskInfo::SetEarlyStart(int EarlyStart)
{
    m_iEarlyStart = EarlyStart;
}

void ProjectController::TaskInfo::SetEarlyFinish(int EarlyFinish)
{
    m_iEarlyFinish = EarlyFinish;
}

void ProjectController::TaskInfo::SetLateStart(int LateStart)
{
    m_iLateStart = LateStart;
}

void ProjectController::TaskInfo::SetLateFinish(int LateFinish)
{
    m_iLateFinish = LateFinish;
}

void ProjectController::TaskInfo::SetSlackDays(int SlackDays)
{
    m_iSlackDays = SlackDays;
}

void ProjectController::TaskInfo::SetPredecessors(
    const std::vector<std::size_t>& Predecessors)
{
    m_Predecessors = Predecessors;
}

void ProjectController::TaskInfo::SetSuccessors(
    const std::vector<std::size_t>& Successors)
{
    m_Successors = Successors;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::DependencyInfo::DependencyInfo
//【函数功能】       默认构造依赖信息载体，数值成员清零。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::DependencyInfo::DependencyInfo()
    : m_uPredecessor(0), m_uSuccessor(0), m_TypeText(""), m_iLagDays(0)
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::DependencyInfo::DependencyInfo(const DependencyInfo& Source)
    = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::DependencyInfo&
ProjectController::DependencyInfo::operator=(const DependencyInfo& Source)
    = default;

// 析构函数：无额外资源需要释放
ProjectController::DependencyInfo::~DependencyInfo() = default;

// 以下为 DependencyInfo 的读写接口
std::size_t ProjectController::DependencyInfo::GetPredecessor() const
{
    return m_uPredecessor;
}

std::size_t ProjectController::DependencyInfo::GetSuccessor() const
{
    return m_uSuccessor;
}

const std::string& ProjectController::DependencyInfo::GetTypeText() const
{
    return m_TypeText;
}

int ProjectController::DependencyInfo::GetLagDays() const
{
    return m_iLagDays;
}

void ProjectController::DependencyInfo::SetPredecessor(std::size_t Predecessor)
{
    m_uPredecessor = Predecessor;
}

void ProjectController::DependencyInfo::SetSuccessor(std::size_t Successor)
{
    m_uSuccessor = Successor;
}

void ProjectController::DependencyInfo::SetTypeText(
    const std::string& TypeText)
{
    m_TypeText = TypeText;
}

void ProjectController::DependencyInfo::SetLagDays(int LagDays)
{
    m_iLagDays = LagDays;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ResourceInfo::ResourceInfo
//【函数功能】       默认构造资源信息载体，数值成员清零。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ResourceInfo::ResourceInfo()
    : m_Name(""), m_rUnitCost(0.0)
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::ResourceInfo::ResourceInfo(const ResourceInfo& Source)
    = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::ResourceInfo&
ProjectController::ResourceInfo::operator=(const ResourceInfo& Source)
    = default;

// 析构函数：无额外资源需要释放
ProjectController::ResourceInfo::~ResourceInfo() = default;

// 以下为 ResourceInfo 的读写接口
const std::string& ProjectController::ResourceInfo::GetName() const
{
    return m_Name;
}

double ProjectController::ResourceInfo::GetUnitCost() const
{
    return m_rUnitCost;
}

void ProjectController::ResourceInfo::SetName(const std::string& Name)
{
    m_Name = Name;
}

void ProjectController::ResourceInfo::SetUnitCost(double UnitCost)
{
    m_rUnitCost = UnitCost;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ValidationInfo::ValidationInfo
//【函数功能】       默认构造校验结论载体，结论初始化为不合法。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ValidationInfo::ValidationInfo()
    : m_bIsValid(false), m_Messages()
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::ValidationInfo::ValidationInfo(const ValidationInfo& Source)
    = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::ValidationInfo&
ProjectController::ValidationInfo::operator=(const ValidationInfo& Source)
    = default;

// 析构函数：无额外资源需要释放
ProjectController::ValidationInfo::~ValidationInfo() = default;

// 以下为 ValidationInfo 的读写接口
bool ProjectController::ValidationInfo::IsValid() const
{
    return m_bIsValid;
}

const std::vector<std::string>&
ProjectController::ValidationInfo::GetMessages() const
{
    return m_Messages;
}

void ProjectController::ValidationInfo::SetValid(bool IsValidValue)
{
    m_bIsValid = IsValidValue;
}

void ProjectController::ValidationInfo::SetMessages(
    const std::vector<std::string>& Messages)
{
    m_Messages = Messages;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ScheduleInfo::ScheduleInfo
//【函数功能】       默认构造调度结论载体，总工期清零。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ScheduleInfo::ScheduleInfo()
    : m_iProjectDuration(0), m_CriticalPath()
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::ScheduleInfo::ScheduleInfo(const ScheduleInfo& Source)
    = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::ScheduleInfo&
ProjectController::ScheduleInfo::operator=(const ScheduleInfo& Source)
    = default;

// 析构函数：无额外资源需要释放
ProjectController::ScheduleInfo::~ScheduleInfo() = default;

// 以下为 ScheduleInfo 的读写接口
int ProjectController::ScheduleInfo::GetProjectDuration() const
{
    return m_iProjectDuration;
}

const std::vector<std::size_t>&
ProjectController::ScheduleInfo::GetCriticalPath() const
{
    return m_CriticalPath;
}

void ProjectController::ScheduleInfo::SetProjectDuration(int ProjectDuration)
{
    m_iProjectDuration = ProjectDuration;
}

void ProjectController::ScheduleInfo::SetCriticalPath(
    const std::vector<std::size_t>& CriticalPath)
{
    m_CriticalPath = CriticalPath;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::StatisticsInfo::StatisticsInfo
//【函数功能】       默认构造统计信息载体，数值成员清零。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
ProjectController::StatisticsInfo::StatisticsInfo()
    : m_ProjectName(""), m_uTaskCount(0), m_uDependencyCount(0),
      m_uResourceCount(0), m_rTotalCost(0.0), m_Schedule()
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::StatisticsInfo::StatisticsInfo(const StatisticsInfo& Source)
    = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::StatisticsInfo&
ProjectController::StatisticsInfo::operator=(const StatisticsInfo& Source)
    = default;

// 析构函数：无额外资源需要释放
ProjectController::StatisticsInfo::~StatisticsInfo() = default;

// 以下为 StatisticsInfo 的读写接口
const std::string& ProjectController::StatisticsInfo::GetProjectName() const
{
    return m_ProjectName;
}

std::size_t ProjectController::StatisticsInfo::GetTaskCount() const
{
    return m_uTaskCount;
}

std::size_t ProjectController::StatisticsInfo::GetDependencyCount() const
{
    return m_uDependencyCount;
}

std::size_t ProjectController::StatisticsInfo::GetResourceCount() const
{
    return m_uResourceCount;
}

double ProjectController::StatisticsInfo::GetTotalCost() const
{
    return m_rTotalCost;
}

const ProjectController::ScheduleInfo&
ProjectController::StatisticsInfo::GetSchedule() const
{
    return m_Schedule;
}

void ProjectController::StatisticsInfo::SetProjectName(
    const std::string& ProjectName)
{
    m_ProjectName = ProjectName;
}

void ProjectController::StatisticsInfo::SetTaskCount(std::size_t TaskCount)
{
    m_uTaskCount = TaskCount;
}

void ProjectController::StatisticsInfo::SetDependencyCount(
    std::size_t DependencyCount)
{
    m_uDependencyCount = DependencyCount;
}

void ProjectController::StatisticsInfo::SetResourceCount(
    std::size_t ResourceCount)
{
    m_uResourceCount = ResourceCount;
}

void ProjectController::StatisticsInfo::SetTotalCost(double TotalCost)
{
    m_rTotalCost = TotalCost;
}

void ProjectController::StatisticsInfo::SetSchedule(const ScheduleInfo& Schedule)
{
    m_Schedule = Schedule;
}
