//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 ConsoleUI.cpp
//【功能模块和目的】         实现控制台界面类
//                           菜单显示、用户输入读取、命令分发
//                           把控制器回传的状态枚举与信息类格式化为控制台文本
//【开发者及日期】           刘江宇, 2026-07-05
//【修改记录】               2026-07-07 配合控制器接口重构
//                           格式化职责移入本类
//-------------------------------------------------------------------------------------------------------------------
#include "view/ConsoleUI.hpp"
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ConsoleUI
//【函数功能】       默认构造控制台界面对象
//                   界面无内部状态，无需额外初始化
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ConsoleUI::ConsoleUI() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ConsoleUI
//【函数功能】       拷贝构造控制台界面对象
//                   界面无内部状态，采用默认逐成员拷贝
//【参数】           Source（输入参数）：被拷贝的界面对象
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ConsoleUI::ConsoleUI(const ConsoleUI& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::operator=
//【函数功能】       拷贝赋值控制台界面对象
//                   界面无内部状态，采用默认逐成员赋值
//【参数】           Source（输入参数）：赋值来源的界面对象
//【返回值】         返回自身引用，支持连续赋值
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ConsoleUI& ConsoleUI::operator=(const ConsoleUI& Source) = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::~ConsoleUI
//【函数功能】       析构控制台界面对象；界面不持有资源，无需额外清理。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
ConsoleUI::~ConsoleUI() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::Execute
//【函数功能】       运行控制台主循环：反复显示菜单、读取选项并分发处理，直至用户选择退出或输入结束。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::Execute()
{
    bool IsRunning = true;
    //主循环继续标志，选择退出或输入结束时置 false
    while (IsRunning == true) {
        try {
            PrintMenu();                              //先显示菜单再等待用户输入
            int Choice = ReadInt("Choice: ");         //读取用户选择的菜单项编号
            IsRunning = HandleChoice(Choice);         //分发命令，返回 false 表示退出
        }
        catch (const std::runtime_error& Exception) { //输入流结束（EOF）时安全退出主循环
            std::cout << Exception.what() << "\n";
            IsRunning = false;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::HandleChoice
//【函数功能】       按菜单编号把用户命令分发到对应的界面处理函数。
//【参数】           Choice（输入参数）：用户输入的菜单项编号。
//【返回值】         bool，true 表示继续主循环，false 表示用户选择退出。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
bool ConsoleUI::HandleChoice(int Choice) const
{
    bool IsContinuing = true;                         //返回值：是否继续主循环

    switch (Choice) {
    case 0 :                                          //退出程序
        IsContinuing = false;
        break;
    case 1 :                                          //新建空项目
        CreateProject();
        break;
    case 2 :                                          //导入项目模型文件
        ImportProject();
        break;
    case 3 :                                          //导出项目模型文件
        ExportProject();
        break;
    case 4 :                                          //添加任务
        AddTask();
        break;
    case 5 :                                          //列出全部任务
        ListTasks();
        break;
    case 6 :                                          //修改任务
        UpdateTask();
        break;
    case 7 :                                          //删除任务
        RemoveTask();
        break;
    case 8 :                                          //列出指定任务的前驱与后继
        ListTaskRelations();
        break;
    case 9 :                                          //添加依赖
        AddDependency();
        break;
    case 10 :                                         //列出全部依赖
        ListDependencies();
        break;
    case 11 :                                         //删除依赖（按索引）
        RemoveDependency();
        break;
    case 12 :                                         //删除依赖（按任务对）
        RemoveDepByTaskPair();
        break;
    case 13 :                                         //添加资源
        AddResource();
        break;
    case 14 :                                         //列出全部资源
        ListResources();
        break;
    case 15 :                                         //为任务分配资源
        AssignResource();
        break;
    case 16 :                                         //验证项目合理性
        ValidateProject();
        break;
    case 17 :                                         //执行关键路径调度计算
        RunSchedule();
        break;
    case 18 :                                         //显示统计信息
        ShowStatistics();
        break;
    default :                                         //其余编号一律视为未知命令
        std::cout << "Unknown command.\n";
        break;
    }
    return IsContinuing;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ResultToText
//【函数功能】       把控制器状态枚举翻译为界面层显示文本，避免控制器承担输出格式职责。
//【参数】           Result（输入参数）：待翻译的控制器状态枚举值。
//【返回值】         std::string，对应的英文提示文本。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
std::string ConsoleUI::ResultToText(ProjectController::RES Result) const
{
    switch (Result) {
    case ProjectController::RES::SUCCESS :              //操作成功
        return "Operation completed successfully.";
    case ProjectController::RES::INVALID_ARGUMENT :     //参数被模型层拒绝
        return "Invalid argument rejected by the model.";
    case ProjectController::RES::INDEX_OUT_OF_RANGE :   //索引越界
        return "Index out of range.";
    case ProjectController::RES::SELF_DEPENDENCY :      //自依赖
        return "Self-dependency is not allowed.";
    case ProjectController::RES::CYCLE_DETECTED :       //将导致循环依赖
        return "Dependency would create a cycle.";
    case ProjectController::RES::FILE_ERROR :           //文件类型不支持或无法打开
        return "File cannot be opened or its type is not supported.";
    case ProjectController::RES::PARSE_ERROR :          //文件内容格式非法
        return "File content is malformed.";
    case ProjectController::RES::INVALID_PROJECT :      //项目不合理，无法调度
        return "Cannot schedule an invalid project.";
    default :                                           //其余取值一律视为未知错误
        return "Unknown error.";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::PrintFailure
//【函数功能】       输出失败状态对应的描述文本；对修改类操作追加控制器记录的失败详情。
//【参数】           Result（输入参数）：控制器返回的失败状态枚举；
//                   HasDetail（输入参数）：true 表示该操作会写错误详情，追加详情输出。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::PrintFailure(ProjectController::RES Result,
                             bool HasDetail) const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::cout << ResultToText(Result) << "\n";
    //修改类操作的失败详情由控制器提供（如解析错误行号、模型拒绝原因）
    if ((HasDetail == true) && (Controller.GetLastError().empty() == false)) {
        std::cout << "Detail: " << Controller.GetLastError() << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::PrintSchedule
//【函数功能】       输出一次调度计算的结论：项目总工期与关键路径任务索引。
//【参数】           Info（输入参数）：控制器回传的调度结论。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::PrintSchedule(const ProjectController::ScheduleInfo& Info) const
{
    std::cout << "Project duration: " << Info.GetProjectDuration() << "\n";
    std::cout << "Critical path:";
    for (std::size_t Index : Info.GetCriticalPath()) {
        std::cout << " " << Index;                    //同行列出关键路径上的任务索引
    }
    std::cout << "\n";
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::CreateProject
//【函数功能】       读取项目名称并请求控制器新建空项目，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::CreateProject() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::string Name = ReadText("Project name: ");    //读取新项目名称
    ProjectController::RES Result = Controller.CreateProject(Name);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Project created.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ImportProject
//【函数功能】       读取文件名并请求控制器导入项目模型，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ImportProject() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::string FileName = ReadText("PPM file: ");    //读取待导入的模型文件名
    ProjectController::RES Result = Controller.ImportProject(FileName);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Project imported.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ExportProject
//【函数功能】       读取文件名并请求控制器把当前项目导出到文件，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ExportProject() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::string FileName = ReadText("Output PPM file: "); //读取导出目标文件名
    ProjectController::RES Result = Controller.ExportProject(FileName);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Project exported.\n";
    }
    else {
        PrintFailure(Result, true);                   //导出失败时输出具体文件错误
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::AddTask
//【函数功能】       依次读取任务名称与工期并请求控制器添加任务，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::AddTask() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::string Name = ReadText("Task name: ");       //先读任务名称
    int Duration = ReadInt("Duration: ");             //再读工期，0 表示里程碑
    ProjectController::RES Result = Controller.AddTask(Name, Duration);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Task added.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::UpdateTask
//【函数功能】       依次读取任务索引、新名称与新工期并请求控制器修改任务，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::UpdateTask() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t TaskIndex = GetPositiveIndex("Task index: ");
    std::string NewName = ReadText("New name: ");     //修改后的任务名称
    int NewDuration = ReadInt("New duration: ");      //修改后的工期，0 将转为里程碑
    ProjectController::RES Result = Controller.UpdateTask(TaskIndex,
                                                          NewName,
                                                          NewDuration);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Task updated.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::RemoveTask
//【函数功能】       读取任务索引并请求控制器删除该任务（连带其依赖与资源分配），输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::RemoveTask() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t TaskIndex = GetPositiveIndex("Task index: ");
    ProjectController::RES Result = Controller.RemoveTask(TaskIndex);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Task removed.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ListTasks
//【函数功能】       向控制器索取全部任务信息，逐个格式化输出名称、工期、成本、CPM 时间
//                   参数及前驱后继索引。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ListTasks() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::TaskInfoList InfoList;
    Controller.GetTaskList(InfoList);                 //采集全部任务展示信息
    std::cout << std::fixed << std::setprecision(2);  //成本统一保留两位小数显示
    std::cout << "Tasks:\n";
    //逐个任务输出基本信息、时间参数及前驱后继索引
    for (const ProjectController::TaskInfo& Info : InfoList) {
        std::cout << "[" << Info.GetIndex() << "] " << Info.GetName()
                  << " Duration=" << Info.GetDuration()
                  << " Cost=" << Info.GetTotalCost()
                  << " ES=" << Info.GetEarlyStart()
                  << " EF=" << Info.GetEarlyFinish()
                  << " LS=" << Info.GetLateStart()
                  << " LF=" << Info.GetLateFinish()
                  << " Slack=" << Info.GetSlackDays()
                  << "\n  Predecessors:";
        for (std::size_t PredecessorIndex : Info.GetPredecessors()) {
            std::cout << " " << PredecessorIndex;     //同行列出全部前驱任务索引
        }
        std::cout << "\n  Successors:";
        for (std::size_t SuccessorIndex : Info.GetSuccessors()) {
            std::cout << " " << SuccessorIndex;       //同行列出全部后继任务索引
        }
        std::cout << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ListTaskRelations
//【函数功能】       读取任务索引，向控制器索取其前驱与后继任务信息并格式化输出。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ListTaskRelations() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t TaskIndex = GetPositiveIndex("Task index: ");
    ProjectController::TaskInfo QueriedTask;
    ProjectController::TaskInfoList Predecessors;
    ProjectController::TaskInfoList Successors;
    ProjectController::RES Result = Controller.GetTaskRelations(TaskIndex,
                                                                QueriedTask,
                                                                Predecessors,
                                                                Successors);
    if (Result != ProjectController::RES::SUCCESS) {
        PrintFailure(Result, false);                  //const 查询接口不写错误详情
        return;
    }

    //先输出被查询任务自身的索引、名称与工期
    std::cout << "Task " << QueriedTask.GetIndex() << " ("
              << QueriedTask.GetName() << ") - "
              << QueriedTask.GetDuration() << " days\n";
    std::cout << "Predecessors:\n";
    if (Predecessors.empty()) {                       //无前驱时显式提示 None
        std::cout << "  None\n";
    }
    //逐个输出前驱任务的索引、名称与工期
    for (const ProjectController::TaskInfo& Info : Predecessors) {
        std::cout << "  - Index: " << Info.GetIndex()
                  << ", Name: " << Info.GetName()
                  << ", Duration: " << Info.GetDuration() << "\n";
    }
    std::cout << "Successors:\n";
    if (Successors.empty()) {                         //无后继时显式提示 None
        std::cout << "  None\n";
    }
    //逐个输出后继任务的索引、名称与工期
    for (const ProjectController::TaskInfo& Info : Successors) {
        std::cout << "  - Index: " << Info.GetIndex()
                  << ", Name: " << Info.GetName()
                  << ", Duration: " << Info.GetDuration() << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::AddDependency
//【函数功能】       依次读取前置索引、后置索引、依赖类型与 Lag 并请求控制器添加依赖，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::AddDependency() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t Predecessor = GetPositiveIndex("Predecessor index: ");
    std::size_t Successor = GetPositiveIndex("Successor index: ");
    std::string TypeText = ReadText("Type(FS/SS/FF/SF): "); //依赖类型文本
    int LagDays = ReadInt("Lag: ");                   //滞后（正）或提前（负）天数
    ProjectController::RES Result = Controller.AddDependency(Predecessor,
                                                             Successor,
                                                             TypeText,
                                                             LagDays);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Dependency added.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::RemoveDependency
//【函数功能】       读取依赖索引并请求控制器删除该依赖，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::RemoveDependency() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t DependencyIndex = GetPositiveIndex("Dependency index: ");
    ProjectController::RES Result
        = Controller.RemoveDependency(DependencyIndex);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Dependency removed.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::RemoveDepByTaskPair
//【函数功能】       读取前置与后置任务索引，请求控制器删除该任务对之间的依赖关系。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::RemoveDepByTaskPair() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t Predecessor = GetPositiveIndex("Predecessor task index: ");
    std::size_t Successor = GetPositiveIndex("Successor task index: ");
    ProjectController::RES Result
        = Controller.RemoveDependency(Predecessor, Successor);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Dependency removed.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ListDependencies
//【函数功能】       向控制器索取全部依赖信息，逐条格式化输出前后置索引、类型与 Lag。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ListDependencies() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::DependencyInfoList InfoList;
    Controller.GetDependencyList(InfoList);           //采集全部依赖展示信息
    std::cout << "Dependencies:\n";
    //逐条依赖输出序号、前后置索引、类型文本与 Lag 天数
    for (std::size_t Index = 0; Index < InfoList.size(); ++Index) {
        const ProjectController::DependencyInfo& Info = InfoList[Index];
        std::cout << "[" << Index << "] "
                  << Info.GetPredecessor() << " -> "
                  << Info.GetSuccessor() << " "
                  << Info.GetTypeText() << " Lag="
                  << Info.GetLagDays() << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::AddResource
//【函数功能】       依次读取资源名称与单位成本并请求控制器添加资源，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::AddResource() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::string Name = ReadText("Resource name: ");   //先读资源名称
    double UnitCost = ReadDouble("Unit cost: ");      //再读单位时间成本
    ProjectController::RES Result = Controller.AddResource(Name, UnitCost);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Resource added.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ListResources
//【函数功能】       向控制器索取全部资源信息，逐个格式化输出索引、名称与单位成本。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ListResources() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::ResourceInfoList InfoList;
    Controller.GetResourceList(InfoList);             //采集全部资源展示信息
    std::cout << std::fixed << std::setprecision(2);  //单位成本统一保留两位小数显示
    std::cout << "Resources:\n";
    //逐个资源输出索引、名称与单位成本
    for (std::size_t Index = 0; Index < InfoList.size(); ++Index) {
        const ProjectController::ResourceInfo& Info = InfoList[Index];
        std::cout << "[" << Index << "] " << Info.GetName()
                  << " UnitCost=" << Info.GetUnitCost() << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::AssignResource
//【函数功能】       依次读取任务索引、资源索引与数量并请求控制器建立资源分配，输出操作结果。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::AssignResource() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    std::size_t TaskIndex = GetPositiveIndex("Task index: ");
    std::size_t ResourceIndex = GetPositiveIndex("Resource index: ");
    int Quantity = ReadInt("Quantity: ");             //占用该资源的数量
    ProjectController::RES Result = Controller.AssignResource(TaskIndex,
                                                              ResourceIndex,
                                                              Quantity);
    if (Result == ProjectController::RES::SUCCESS) {
        std::cout << "Resource assigned.\n";
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ValidateProject
//【函数功能】       请求控制器校验当前项目合理性，格式化输出结论与逐条错误信息。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ValidateProject() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::ValidationInfo Info;
    Controller.ValidateProject(Info);                 //采集校验结论
    if (Info.IsValid() == true) {
        std::cout << "Project is valid.\n";
        return;
    }
    std::cout << "Project is invalid:\n";
    //逐条列出校验器给出的错误信息
    for (const std::string& Message : Info.GetMessages()) {
        std::cout << "- " << Message << "\n";
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::RunSchedule
//【函数功能】       请求控制器执行 CPM 调度计算，格式化输出总工期与关键路径。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::RunSchedule() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::ScheduleInfo Info;
    ProjectController::RES Result = Controller.RunSchedule(Info);
    if (Result == ProjectController::RES::SUCCESS) {
        PrintSchedule(Info);
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ShowStatistics
//【函数功能】       请求控制器汇总项目统计信息，格式化输出项目名、三类元素总数、总成本
//                   及调度结论。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 格式化逻辑由控制器迁移至此。
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::ShowStatistics() const
{
    ProjectController& Controller = ProjectController::GetInstance();
    ProjectController::StatisticsInfo Info;
    ProjectController::RES Result = Controller.CollectStatistics(Info);
    std::cout << std::fixed << std::setprecision(2);  //总成本统一保留两位小数显示
    std::cout << "Project: " << Info.GetProjectName() << "\n"
              << "Tasks: " << Info.GetTaskCount() << "\n"
              << "Dependencies: " << Info.GetDependencyCount() << "\n"
              << "Resources: " << Info.GetResourceCount() << "\n"
              << "Total cost: " << Info.GetTotalCost() << "\n";
    if (Result == ProjectController::RES::SUCCESS) {  //统计末尾附带调度结论
        PrintSchedule(Info.GetSchedule());
    }
    else {
        PrintFailure(Result, true);                   //修改类操作，附带失败详情
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::PrintMenu
//【函数功能】       在控制台输出全部菜单项及其编号。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
void ConsoleUI::PrintMenu() const
{
    std::cout << "\nProject Scheduler\n"
              << "1. Create project\n"
              << "2. Import PPM\n"
              << "3. Export PPM\n"
              << "4. Add task\n"
              << "5. List tasks\n"
              << "6. Update task\n"
              << "7. Remove task\n"
              << "8. List task relations\n"
              << "9. Add dependency\n"
              << "10. List dependencies\n"
              << "11. Remove dependency (by index)\n"
              << "12. Remove dependency (by task pair)\n"
              << "13. Add resource\n"
              << "14. List resources\n"
              << "15. Assign resource\n"
              << "16. Validate project\n"
              << "17. Run CPM schedule\n"
              << "18. Show statistics\n"
              << "0. Exit\n";
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ReadInt
//【函数功能】       显示提示并从标准输入读取一个整数；输入非法时清空缓冲重新提示，输入结束时抛出异常。
//【参数】           Prompt（输入参数）：显示给用户的提示文本。
//【返回值】         int，用户输入的整数值。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
int ConsoleUI::ReadInt(const std::string& Prompt) const
{
    int Value = 0;                                    //读取结果，读取失败时保持提示循环
    std::cout << Prompt << std::flush;
    while (!(std::cin >> Value)) {
        if (std::cin.eof()) {                         //输入流已结束，无法继续交互
            throw std::runtime_error("Input ended.");
        }
        std::cin.clear();                             //清除失败状态并丢弃本行残余输入
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << Prompt << std::flush;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return Value;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ReadDouble
//【函数功能】       显示提示并从标准输入读取一个浮点数；输入非法时清空缓冲重新提示，输入结束时抛出异常。
//【参数】           Prompt（输入参数）：显示给用户的提示文本。
//【返回值】         double，用户输入的浮点数值。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
double ConsoleUI::ReadDouble(const std::string& Prompt) const
{
    double Value = 0.0;                               //读取结果，读取失败时保持提示循环
    std::cout << Prompt << std::flush;
    while (!(std::cin >> Value)) {
        if (std::cin.eof()) {                         //输入流已结束，无法继续交互
            throw std::runtime_error("Input ended.");
        }
        std::cin.clear();                             //清除失败状态并丢弃本行残余输入
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << Prompt << std::flush;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return Value;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::ReadText
//【函数功能】       显示提示并从标准输入读取一整行文本，输入结束时抛出异常。
//【参数】           Prompt（输入参数）：显示给用户的提示文本。
//【返回值】         std::string，用户输入的整行文本。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
std::string ConsoleUI::ReadText(const std::string& Prompt) const
{
    std::string Text;                                 //读取结果
    std::cout << Prompt << std::flush;
    if (!std::getline(std::cin, Text)) {              //输入流已结束，无法继续交互
        throw std::runtime_error("Input ended.");
    }
    return Text;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       ConsoleUI::GetPositiveIndex
//【函数功能】       读取一个非负整数作为索引，负数输入时提示错误并重新提示。
//【参数】           Prompt（输入参数）：显示给用户的提示文本。
//【返回值】         std::size_t，用户输入的非负整数值；输入结束时抛出异常。
//【开发者及日期】   刘江宇, 2026-07-07
//-------------------------------------------------------------------------------------------------------------------
std::size_t ConsoleUI::GetPositiveIndex(const std::string& Prompt) const
{
    int Value = 0;                                    //读取的整数值
    while (true) {
        Value = ReadInt(Prompt);                      //读取一个整数
        if (Value >= 0) {                             //非负数合法
            return static_cast<std::size_t>(Value);
        }
        //负数提示错误并重试
        std::cout << "Index must be non-negative. Please try again.\n";
    }
}
