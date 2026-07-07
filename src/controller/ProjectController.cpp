//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ProjectController.cpp
//【功能模块和目的】         实现项目调度器的控制器单例类：接收界面命令，调用模型层完成
//                           项目、任务、依赖与资源的管理及校验调度，以状态枚举与信息类
//                           回传结果，不做任何文本格式化。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-07 重构为"状态枚举 + 信息类"接口，文本格式化移交界面层。
//-------------------------------------------------------------------------------------------------------------------
#include "controller/ProjectController.hpp"

//CPMScheduler 调度器类所属头文件
#include "model/CPMScheduler.hpp"
//Exporter 基类模板所属头文件
#include "model/Exporter.hpp"
//Importer 基类模板所属头文件
#include "model/Importer.hpp"
//PPMExporter 具体导出器所属头文件
#include "model/PPMExporter.hpp"
//PPMImporter 具体导入器所属头文件
#include "model/PPMImporter.hpp"
//ProjectValidator 校验器类所属头文件
#include "model/ProjectValidator.hpp"

//std 异常类型所属头文件
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetInstance
//【函数功能】       获取控制器的全局唯一实例；首次调用时构造函数内静态对象，之后始终返回同一对象。
//【参数】           无
//【返回值】         ProjectController&，控制器单例的引用。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController& ProjectController::GetInstance()
{
    static ProjectController s_Instance;    //函数内静态对象，首次调用时构造且全程唯一
    return s_Instance;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ProjectController
//【函数功能】       默认构造控制器对象，并向导入/导出器工厂注册 PPM 格式的具体实现；
//                   后续如需支持更多格式，仅需在此追加注册。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 增加 PPM 导入/导出器的工厂注册。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ProjectController()
    : m_Repository(), m_LastError("")
{
    Importer<Project>::Register<PPMImporter>();    //登记 PPM 格式导入器
    Exporter<Project>::Register<PPMExporter>();    //登记 PPM 格式导出器
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::~ProjectController
//【函数功能】       析构控制器对象；成员仓库由其自身析构函数清理，无需额外处理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::~ProjectController() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ResultToText（静态）
//【函数功能】       把返回状态枚举翻译为英文描述文本，供界面层向用户显示。
//【参数】           Result（输入参数）：待翻译的返回状态枚举值。
//【返回值】         std::string，对应的英文描述文本。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::string ProjectController::ResultToText(RES Result)
{
    switch (Result) {
    case RES::SUCCESS :                        //操作成功
        return "Operation completed successfully.";
    case RES::INVALID_ARGUMENT :               //参数被模型层拒绝
        return "Invalid argument rejected by the model.";
    case RES::INDEX_OUT_OF_RANGE :             //索引越界
        return "Index out of range.";
    case RES::SELF_DEPENDENCY :                //自依赖
        return "Self-dependency is not allowed.";
    case RES::CYCLE_DETECTED :                 //将导致循环依赖
        return "Dependency would create a cycle.";
    case RES::FILE_ERROR :                     //文件类型不支持或无法打开
        return "File cannot be opened or its type is not supported.";
    case RES::PARSE_ERROR :                    //文件内容格式非法
        return "File content is malformed.";
    case RES::INVALID_PROJECT :                //项目不合理，无法调度
        return "Cannot schedule an invalid project.";
    default :                                  //其余取值一律视为未知错误
        return "Unknown error.";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::CreateProject
//【函数功能】       按给定名称新建一个空项目并设为当前项目，替换仓库中原有项目。
//【参数】           Name（输入参数）：新项目的名称。
//【返回值】         RES，SUCCESS 表示创建成功；名称非法返回 INVALID_ARGUMENT，
//                   失败详情写入 LastError。
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::TaskInfo ProjectController::BuildTaskInfo(
    const Project& SourceProject, std::size_t Index) const
{
    const Task& CurrentTask = SourceProject.GetTask(Index);
    TaskInfo Info;
    Info.Index = Index;
    Info.Name = CurrentTask.GetName();
    Info.Duration = CurrentTask.GetDuration();
    Info.IsMilestone = (CurrentTask.GetDuration() == 0);
    Info.TotalCost = SourceProject.GetTaskTotalCost(Index);
    Info.EarlyStart = CurrentTask.GetES();
    Info.EarlyFinish = CurrentTask.GetEF();
    Info.LateStart = CurrentTask.GetLS();
    Info.LateFinish = CurrentTask.GetLF();
    Info.SlackDays = CurrentTask.GetSlack();
    Info.Predecessors = CurrentTask.GetPredecessors();
    Info.Successors = CurrentTask.GetSuccessors();
    return Info;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::GetTaskList
//【函数功能】       采集当前项目全部任务的展示信息，按容器索引顺序填入输出列表。
//【参数】           InfoList（输出参数）：填充任务展示信息的列表，原有内容被清空。
//【返回值】         RES，恒为 SUCCESS。
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
    catch (const std::out_of_range& Exception) {        //索引越界
        (void)Exception;                                //const 接口不改写 LastError
        return RES::INDEX_OUT_OF_RANGE;
    }
    catch (const std::exception& Exception) {           //其余未预期异常
        (void)Exception;
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RemoveDependency(std::size_t Predecessor,
                                                            std::size_t Successor)
{
    m_LastError = "";
    try {
        m_Repository.GetCurrentProject().RemoveDependency(Predecessor, Successor);
        return RES::SUCCESS;
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
//【开发者及日期】   2024013215, 2026-07-05
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
        Info.Predecessor = CurrentDependency.GetPredecessor();
        Info.Successor = CurrentDependency.GetSuccessor();
        Info.TypeText = CurrentDependency.GetTypeText();
        Info.LagDays = CurrentDependency.GetLag();
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
//【开发者及日期】   2024013215, 2026-07-05
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
//【开发者及日期】   2024013215, 2026-07-05
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
    catch (const std::logic_error& Exception) {         //里程碑占资源、数量非法等被模型拒绝
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
//【开发者及日期】   2024013215, 2026-07-05
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
        Info.Name = CurrentResource.GetName();
        Info.UnitCost = CurrentResource.GetUnitCost();
        InfoList.push_back(Info);
    }
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ValidateProject
//【函数功能】       调用校验器检查当前项目是否满足调度规则（依赖无环、无孤立悬挂节点、
//                   依赖引用有效），结论与逐条错误信息写入输出参数。
//【参数】           Info（输出参数）：填充校验结论与错误信息。
//【返回值】         RES，恒为 SUCCESS（校验结论在 Info.IsValid 中）。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::ValidateProject(
    ValidationInfo& Info) const
{
    ProjectValidator Validator;    //校验器无内部状态，按需临时创建
    ValidationResult Result = Validator.Validate(
        m_Repository.GetCurrentProject());
    Info.IsValid = Result.IsValid();
    Info.Messages = Result.GetMessages();
    return RES::SUCCESS;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::RunSchedule
//【函数功能】       对当前项目执行关键路径法（CPM）调度计算，把总工期与关键路径写入
//                   输出参数；项目未通过合理性校验时计算被拒绝。
//【参数】           Info（输出参数）：填充项目总工期与关键路径任务索引。
//【返回值】         RES，SUCCESS 表示计算完成；项目不合理返回 INVALID_PROJECT，
//                   失败详情写入 LastError。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::RunSchedule(ScheduleInfo& Info)
{
    m_LastError = "";
    try {
        CPMScheduler Scheduler;    //调度器无内部状态，按需临时创建
        ScheduleResult Result = Scheduler.Calculate(
            m_Repository.GetCurrentProject());
        Info.ProjectDuration = Result.GetProjectDuration();
        Info.CriticalPath = Result.GetCriticalPath();
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
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 改为回传信息类，并更名以明确该操作会触发调度计算。
//-------------------------------------------------------------------------------------------------------------------
ProjectController::RES ProjectController::CollectStatistics(
    StatisticsInfo& Info)
{
    const Project& CurrentProject = m_Repository.GetCurrentProject();
    Info.ProjectName = CurrentProject.GetName();
    Info.TaskCount = CurrentProject.GetTaskCount();
    Info.DependencyCount = CurrentProject.GetDependencyCount();
    Info.ResourceCount = CurrentProject.GetResourceCount();
    Info.TotalCost = CurrentProject.GetProjectTotalCost();
    return RunSchedule(Info.Schedule);    //统计附带一次调度计算，返回其结果状态
}

//-------------------------------------------------------------------------------------------------------------------
//以下为各嵌套信息类（纯数据载体）的构造、拷贝、赋值与析构实现
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::TaskInfo::TaskInfo
//【函数功能】       默认构造任务信息载体，数值成员清零。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::TaskInfo::TaskInfo()
    : Index(0), Name(""), Duration(0), IsMilestone(false), TotalCost(0.0),
      EarlyStart(0), EarlyFinish(0), LateStart(0), LateFinish(0),
      SlackDays(0), Predecessors(), Successors()
{
}

// 拷贝构造函数：逐成员拷贝
ProjectController::TaskInfo::TaskInfo(const TaskInfo& Source) = default;

// 拷贝赋值运算符：逐成员赋值
ProjectController::TaskInfo&
ProjectController::TaskInfo::operator=(const TaskInfo& Source) = default;

// 析构函数：无额外资源需要释放
ProjectController::TaskInfo::~TaskInfo() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::DependencyInfo::DependencyInfo
//【函数功能】       默认构造依赖信息载体，数值成员清零。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::DependencyInfo::DependencyInfo()
    : Predecessor(0), Successor(0), TypeText(""), LagDays(0)
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

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ResourceInfo::ResourceInfo
//【函数功能】       默认构造资源信息载体，数值成员清零。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ResourceInfo::ResourceInfo()
    : Name(""), UnitCost(0.0)
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

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ValidationInfo::ValidationInfo
//【函数功能】       默认构造校验结论载体，结论初始化为不合法。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ValidationInfo::ValidationInfo()
    : IsValid(false), Messages()
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

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::ScheduleInfo::ScheduleInfo
//【函数功能】       默认构造调度结论载体，总工期清零。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::ScheduleInfo::ScheduleInfo()
    : ProjectDuration(0), CriticalPath()
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

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ProjectController::StatisticsInfo::StatisticsInfo
//【函数功能】       默认构造统计信息载体，数值成员清零。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
ProjectController::StatisticsInfo::StatisticsInfo()
    : ProjectName(""), TaskCount(0), DependencyCount(0), ResourceCount(0),
      TotalCost(0.0), Schedule()
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
