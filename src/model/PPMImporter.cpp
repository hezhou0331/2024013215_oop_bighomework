//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 PPMImporter.cpp
//【功能模块和目的】         实现 PPM 格式导入器：逐行解析 PPM 文本文件，校验区块顺序与
//                           字段合法性，把文件内容构建为内存中的 Project 对象。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-07 基类重构为 Importer<Project> 模板，改为实现流解析接口。
//                           2026-07-07 按行类型拆分解析函数，并增加区块顺序与连续性校验。
//-------------------------------------------------------------------------------------------------------------------
//PPMImporter 类所属头文件
#include "model/PPMImporter.hpp"

//Dependency 依赖关系类所属头文件
#include "model/Dependency.hpp"
//Project 模型类所属头文件
#include "model/Project.hpp"

//std::isspace 所属头文件
#include <cctype>
//std::ifstream 所属头文件
#include <fstream>
//std::map 所属头文件
#include <map>
//std::istringstream 所属头文件
#include <sstream>
//std::runtime_error、std::invalid_argument 所属头文件
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::PPMImporter
//【函数功能】       默认构造 PPM 导入器，向基类登记本导入器支持的扩展名 "PPM"。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 基类重构为 Importer<Project> 模板，构造时登记扩展名。
//-------------------------------------------------------------------------------------------------------------------
PPMImporter::PPMImporter()
    : Importer<Project>("PPM")
{
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::~PPMImporter
//【函数功能】       析构 PPM 导入器；导入器不持有资源，无需额外清理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
PPMImporter::~PPMImporter() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::LoadFromStream
//【函数功能】       从已打开的 PPM 文件流逐行解析：跳过注释与空行，校验区块顺序与连续
//                   性，按行首标识分发到各解析函数，最后重建任务前驱后继缓存。文件打开
//                   与扩展名校验由基类 LoadFromFile 统一完成。
//【参数】           Stream（输入参数）：已打开的 PPM 文件输入流。
//【返回值】         Project，解析得到的项目模型对象；格式非法时抛出含行号的
//                   std::runtime_error。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】       2026-07-07 拆分出各行解析函数并增加区块顺序校验，控制本函数长度。
//-------------------------------------------------------------------------------------------------------------------
Project PPMImporter::LoadFromStream(std::ifstream& Stream) const
{
    Project NewProject;                               //解析结果，逐行填充
    std::map<int, std::size_t> TaskIdToIndex;         //文件任务 ID 到容器索引的映射
    std::map<int, std::size_t> ResourceIdToIndex;     //文件资源 ID 到容器索引的映射
    std::string Line;                                 //当前读到的一行原始文本
    int LineNumber = 0;                               //当前行号，用于错误定位
    std::string SeenGroups = "";                      //已出现过的区块组标识集合
    char LastGroup = ' ';                             //上一内容行的区块组标识

    while (std::getline(Stream, Line)) {
        ++LineNumber;
        Line = Trim(Line);                            //去除行首行尾空白
        if (Line.empty() || (Line[0] == '#')) {       //空行与注释行直接跳过
            continue;
        }

        std::istringstream LineStream(Line);
        std::string Kind;                             //行首标识字母（P/T/M/R/D/A）
        LineStream >> Kind;

        try {
            if ((Kind != "P") && (Kind != "T") && (Kind != "M")
                && (Kind != "R") && (Kind != "D") && (Kind != "A")) {
                //无法识别的行首标识
                throw std::invalid_argument("Unknown PPM line type.");
            }
            char GroupChar = GroupOf(Kind);           //归并后的区块组标识
            CheckLineOrder(GroupChar, SeenGroups, LastGroup);

            if (Kind == "P") {                        //项目名称行
                ParseProjectLine(LineStream, NewProject);
            }
            else if ((Kind == "T") || (Kind == "M")) { //普通任务行或里程碑任务行
                ParseTaskLine(Kind, LineStream, NewProject, TaskIdToIndex);
            }
            else if (Kind == "R") {                   //资源行
                ParseResourceLine(LineStream, NewProject, ResourceIdToIndex);
            }
            else if (Kind == "D") {                   //依赖关系行
                ParseDependencyLine(LineStream, NewProject, TaskIdToIndex);
            }
            else {                                    //资源分配行
                ParseAllocationLine(LineStream, NewProject,
                                    TaskIdToIndex, ResourceIdToIndex);
            }

            if (SeenGroups.find(GroupChar) == std::string::npos) {
                SeenGroups += GroupChar;              //登记该区块组已出现
            }
            LastGroup = GroupChar;
        }
        catch (const std::exception& Exception) {     //统一补充行号后重新抛出
            throw std::runtime_error(
                "PPM parse error at line " + std::to_string(LineNumber)
                + ": " + Exception.what());
        }
    }

    if (SeenGroups.find('P') == std::string::npos) {  //P 行有且仅有一行，不可缺失
        throw std::runtime_error(
            "PPM parse error: project line is missing.");
    }

    NewProject.RebuildTaskRelations();                //按依赖表重建各任务的前驱后继缓存
    return NewProject;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::GroupOf（静态）
//【函数功能】       把行首标识归并为区块组标识：T 与 M 同属任务块，组标识为 'T'；
//                   其余标识的组即其自身首字母。
//【参数】           Kind（输入参数）：行首标识字符串（P/T/M/R/D/A 之一）。
//【返回值】         char，行所属的区块组标识。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
char PPMImporter::GroupOf(const std::string& Kind)
{
    if (Kind == "M") {                                //里程碑行与普通任务行同属任务块
        return 'T';
    }
    return Kind[0];
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::CheckLineOrder（静态）
//【函数功能】       校验 PPM 区块顺序与连续性约束：P 行唯一且为首个内容行；任务块必须
//                   出现在依赖块之前；分配块必须位于文件末尾（其后不得再有其他块）；
//                   同一区块的行必须连续出现。违反任一约束抛出异常。
//【参数】           GroupChar（输入参数）：当前行的区块组标识；
//                   SeenGroups（输入参数）：此前已出现过的区块组标识集合；
//                   LastGroup（输入参数）：上一内容行的区块组标识。
//【返回值】         void，无返回值；校验失败以 std::invalid_argument 异常报告。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::CheckLineOrder(char GroupChar,
                                 const std::string& SeenGroups,
                                 char LastGroup)
{
    bool HasSeenSelf = (SeenGroups.find(GroupChar) != std::string::npos);

    if (GroupChar == 'P') {                           //P 行：唯一且为首个内容行
        if (HasSeenSelf == true) {
            throw std::invalid_argument("Duplicate project line.");
        }
        if (SeenGroups.empty() == false) {
            throw std::invalid_argument(
                "Project line must be the first content line.");
        }
        return;
    }
    if (SeenGroups.find('P') == std::string::npos) {  //其余各行必须出现在 P 行之后
        throw std::invalid_argument(
            "Project line must appear before other lines.");
    }
    if ((GroupChar != 'A')
        && (SeenGroups.find('A') != std::string::npos)) {
        //分配块居末，其后不得再有其他类型的内容行
        throw std::invalid_argument(
            "Allocation lines must be the last block.");
    }
    if ((GroupChar == 'T')
        && (SeenGroups.find('D') != std::string::npos)) {
        //任务块必须出现在依赖块之前
        throw std::invalid_argument(
            "Task lines must appear before dependency lines.");
    }
    if ((HasSeenSelf == true) && (LastGroup != GroupChar)) {
        //同一区块被其他区块打断，违反连续性
        throw std::invalid_argument("PPM blocks must be contiguous.");
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::ParseProjectLine（静态）
//【函数功能】       解析 P 行：读取项目名称并写入项目对象；名称缺失抛出异常。
//【参数】           LineStream（输入参数）：定位到名称字段前的行内容流；
//                   TargetProject（输出参数）：被填充项目名称的项目对象。
//【返回值】         void，无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::ParseProjectLine(std::istringstream& LineStream,
                                   Project& TargetProject)
{
    std::string Name;
    if (!(LineStream >> Name)) {                      //名称字段缺失
        throw std::invalid_argument("Project name missing.");
    }
    std::string ExtraField;
    if (LineStream >> ExtraField) {                   //检查是否有多余字段
        throw std::invalid_argument("Unexpected extra field in project line.");
    }
    TargetProject.SetName(Name);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::ParseTaskLine（静态）
//【函数功能】       解析 T/M 行：读取任务 ID 与名称；T 行必须带正整数工期，M 行工期可
//                   省略、写出时必须为 0；ID 不可重复；工期 0 一律创建里程碑任务，否则
//                   创建普通任务，并登记文件 ID 到容器索引的映射。
//【参数】           Kind（输入参数）：行首标识（"T" 或 "M"）；
//                   LineStream（输入参数）：定位到 ID 字段前的行内容流；
//                   TargetProject（输出参数）：被添加任务的项目对象；
//                   TaskIdToIndex（输入/输出参数）：文件任务 ID 到容器索引的映射表。
//【返回值】         void，无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::ParseTaskLine(const std::string& Kind,
                                std::istringstream& LineStream,
                                Project& TargetProject,
                                std::map<int, std::size_t>& TaskIdToIndex)
{
    int TaskID = 0;
    std::string Name;
    if (!(LineStream >> TaskID >> Name)) {            //ID 或名称字段缺失/非法
        throw std::invalid_argument("Invalid task line.");
    }
    if (TaskID < 0) {                                 //任务 ID 必须为非负整数
        throw std::invalid_argument("Task ID must be non-negative.");
    }
    int Duration = 0;                                 //里程碑行允许省略工期，默认 0
    if (Kind == "T") {                                //普通任务行必须带工期字段
        if (!(LineStream >> Duration)) {
            throw std::invalid_argument("Task duration missing.");
        }
    }
    else if ((LineStream >> Duration) && (Duration != 0)) {
        //里程碑行若写了工期，则必须为 0
        throw std::invalid_argument("Milestone duration must be 0.");
    }
    std::string ExtraField;
    if (LineStream >> ExtraField) {                   //检查是否有多余字段
        throw std::invalid_argument("Unexpected extra field in task line.");
    }
    if (TaskIdToIndex.count(TaskID) != 0) {           //任务 ID 不允许重复
        throw std::invalid_argument("Duplicate task ID.");
    }
    std::size_t Index = 0;                            //新任务在容器中的索引
    if ((Kind == "M") || (Duration == 0)) {           //工期为 0 一律按里程碑创建
        Index = TargetProject.AddMilestoneTask(Name);
    }
    else {
        Index = TargetProject.AddBasicTask(Name, Duration);
    }
    TaskIdToIndex[TaskID] = Index;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::ParseResourceLine（静态）
//【函数功能】       解析 R 行：读取资源 ID、名称与单位成本；字段缺失或 ID 重复抛出
//                   异常；创建资源并登记文件 ID 到容器索引的映射。
//【参数】           LineStream（输入参数）：定位到 ID 字段前的行内容流；
//                   TargetProject（输出参数）：被添加资源的项目对象；
//                   ResourceIdToIndex（输入/输出参数）：文件资源 ID 到容器索引的映射表。
//【返回值】         void，无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::ParseResourceLine(
    std::istringstream& LineStream,
    Project& TargetProject,
    std::map<int, std::size_t>& ResourceIdToIndex)
{
    int ResourceID = 0;
    std::string Name;
    double UnitCost = 0.0;
    if (!(LineStream >> ResourceID >> Name >> UnitCost)) {
        //字段缺失或单位成本非法
        throw std::invalid_argument("Invalid resource line.");
    }
    if (ResourceID < 0) {                             //资源 ID 必须为非负整数
        throw std::invalid_argument("Resource ID must be non-negative.");
    }
    std::string ExtraField;
    if (LineStream >> ExtraField) {                   //检查是否有多余字段
        throw std::invalid_argument("Unexpected extra field in resource line.");
    }
    if (ResourceIdToIndex.count(ResourceID) != 0) {   //资源 ID 不允许重复
        throw std::invalid_argument("Duplicate resource ID.");
    }
    ResourceIdToIndex[ResourceID]
        = TargetProject.AddResource(Name, UnitCost);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::ParseDependencyLine（静态）
//【函数功能】       解析 D 行：读取前后置任务 ID、依赖类型与 Lag；字段缺失或引用不存在
//                   的任务 ID 抛出异常；向项目添加依赖。
//【参数】           LineStream（输入参数）：定位到前置 ID 字段前的行内容流；
//                   TargetProject（输出参数）：被添加依赖的项目对象；
//                   TaskIdToIndex（输入参数）：文件任务 ID 到容器索引的映射表。
//【返回值】         void，无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::ParseDependencyLine(
    std::istringstream& LineStream,
    Project& TargetProject,
    const std::map<int, std::size_t>& TaskIdToIndex)
{
    int PredecessorID = 0;
    int SuccessorID = 0;
    std::string TypeText;
    int LagDays = 0;
    if (!(LineStream >> PredecessorID >> SuccessorID >> TypeText
                     >> LagDays)) {                   //四个字段缺一不可
        throw std::invalid_argument("Invalid dependency line.");
    }
    if ((PredecessorID < 0) || (SuccessorID < 0)) {   //任务 ID 必须为非负整数
        throw std::invalid_argument("Dependency task ID must be non-negative.");
    }
    std::string ExtraField;
    if (LineStream >> ExtraField) {                   //检查是否有多余字段
        throw std::invalid_argument("Unexpected extra field in dependency line.");
    }
    if ((TaskIdToIndex.count(PredecessorID) == 0)
        || (TaskIdToIndex.count(SuccessorID) == 0)) {
        //依赖引用了不存在的任务 ID
        throw std::invalid_argument("Dependency task ID missing.");
    }
    TargetProject.AddDependency(
        Dependency(TaskIdToIndex.at(PredecessorID),
                   TaskIdToIndex.at(SuccessorID),
                   Dependency::TypeFromText(TypeText),
                   LagDays));
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::ParseAllocationLine（静态）
//【函数功能】       解析 A 行：读取任务 ID、资源 ID 与数量；字段缺失、数量非正或引用
//                   不存在的 ID 抛出异常；建立任务对资源的分配关系。
//【参数】           LineStream（输入参数）：定位到任务 ID 字段前的行内容流；
//                   TargetProject（输出参数）：被建立资源分配的项目对象；
//                   TaskIdToIndex（输入参数）：文件任务 ID 到容器索引的映射表；
//                   ResourceIdToIndex（输入参数）：文件资源 ID 到容器索引的映射表。
//【返回值】         void，无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
void PPMImporter::ParseAllocationLine(
    std::istringstream& LineStream,
    Project& TargetProject,
    const std::map<int, std::size_t>& TaskIdToIndex,
    const std::map<int, std::size_t>& ResourceIdToIndex)
{
    int TaskID = 0;
    int ResourceID = 0;
    int Quantity = 0;
    if (!(LineStream >> TaskID >> ResourceID >> Quantity)) {
        //字段缺失或数量非法
        throw std::invalid_argument("Invalid allocation line.");
    }
    if ((TaskID < 0) || (ResourceID < 0)) {           //任务与资源 ID 必须为非负整数
        throw std::invalid_argument("Allocation ID must be non-negative.");
    }
    if (Quantity <= 0) {                              //占用数量必须为正整数
        throw std::invalid_argument("Quantity must be positive.");
    }
    std::string ExtraField;
    if (LineStream >> ExtraField) {                   //检查是否有多余字段
        throw std::invalid_argument("Unexpected extra field in allocation line.");
    }
    if ((TaskIdToIndex.count(TaskID) == 0)
        || (ResourceIdToIndex.count(ResourceID) == 0)) {
        //分配引用了不存在的任务或资源 ID
        throw std::invalid_argument("Allocation ID missing.");
    }
    TargetProject.AssignResource(TaskIdToIndex.at(TaskID),
                                 ResourceIdToIndex.at(ResourceID),
                                 Quantity);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMImporter::Trim（静态）
//【函数功能】       去除字符串首尾的空白字符，用于容忍 PPM 行首空白与行尾空白。
//【参数】           Text（输入参数）：待处理的原始字符串。
//【返回值】         std::string，去除首尾空白后的字符串。
//【开发者及日期】   2024013215, 2026-07-05
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
std::string PPMImporter::Trim(const std::string& Text)
{
    std::size_t Head = 0;                             //首个非空白字符的位置
    while ((Head < Text.size())
           && (std::isspace(static_cast<unsigned char>(Text[Head])) != 0)) {
        ++Head;
    }

    std::size_t Tail = Text.size();                   //末个非空白字符之后的位置
    while ((Tail > Head)
           && (std::isspace(static_cast<unsigned char>(Text[Tail - 1])) != 0)) {
        --Tail;
    }

    return Text.substr(Head, Tail - Head);
}
