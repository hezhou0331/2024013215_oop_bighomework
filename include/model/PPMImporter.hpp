//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 PPMImporter.hpp
//【功能模块和目的】         声明 PPM 格式导入器，把 PPM 文本文件解析为内存中的 Project 对象。
//【开发者及日期】           2024013215, 2026-07-05
//【更改记录】               2026-07-07 基类重构为 Importer<Project> 模板，改为实现流解析接口。
//                           2026-07-07 按行类型拆分解析函数，并增加区块顺序与连续性校验。
//-------------------------------------------------------------------------------------------------------------------
#ifndef PROJECT_SCHEDULER_PPM_IMPORTER_HPP
#define PROJECT_SCHEDULER_PPM_IMPORTER_HPP

//Importer 基类模板所属头文件
#include "model/Importer.hpp"
//Project 模型类所属头文件
#include "model/Project.hpp"

//std::ifstream 所属头文件
#include <fstream>
//std::map 所属头文件
#include <map>
//std::istringstream 所属头文件
#include <sstream>
//std::string 所属头文件
#include <string>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             PPMImporter
//【功能】             PPM 格式导入器：Importer<Project> 的具体派生类，逐行解析 PPM 文本
//                     文件中的 P/T/M/R/D/A 各类行，校验区块顺序（P 行唯一且最先、任务块
//                     先于依赖与分配块、资源块先于分配块、各块连续、分配块居末），构建
//                     包含任务、资源、依赖与分配的项目模型；扩展名固定为 "PPM"。
//【接口说明】         默认构造函数向基类登记扩展名 "PPM"；LoadFromStream 从已打开的文件流
//                     解析出 Project 对象，格式非法时抛出 std::runtime_error（含行号）；
//                     文件打开与扩展名校验由基类 LoadFromFile 统一完成；各 Parse* 与
//                     CheckLineOrder 为内部工具函数；导入器无内部状态，禁止拷贝构造与
//                     拷贝赋值。
//【开发者及日期】     2024013215, 2026-07-05
//【更改记录】         2026-07-07 基类重构为 Importer<Project> 模板，改为实现流解析接口。
//                     2026-07-07 按行类型拆分解析函数，并增加区块顺序与连续性校验。
//-------------------------------------------------------------------------------------------------------------------
class PPMImporter : public Importer<Project> {
public:
    //-----------------------------------------------------------------------------------------------------------
    //必要的构造、析构、赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：向基类登记本导入器支持的扩展名 "PPM"
    PPMImporter();
    // 导入器以注册实例形式共享使用，禁止拷贝构造
    PPMImporter(const PPMImporter& Source) = delete;
    // 导入器以注册实例形式共享使用，禁止拷贝赋值
    PPMImporter& operator=(const PPMImporter& Source) = delete;
    // 析构函数：导入器不持有资源，无需额外清理
    ~PPMImporter() override;

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（有后置 const）
    //-----------------------------------------------------------------------------------------------------------
    // 从已打开的 PPM 文件流逐行解析，返回构建好的项目模型
    Project LoadFromStream(std::ifstream& Stream) const override;

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有静态工具函数（逐行解析与格式校验）
    //-----------------------------------------------------------------------------------------------------------
    // 去除字符串首尾空白，用于容忍 PPM 行首行尾空白
    static std::string Trim(const std::string& Text);
    // 把行首标识归并为区块组标识：T 与 M 同属任务块 'T'，其余保持自身
    static char GroupOf(const std::string& Kind);
    // 校验区块顺序与连续性（P 唯一且最先、任务块先于依赖块、分配块居末、各块连续）
    static void CheckLineOrder(char GroupChar,
                               const std::string& SeenGroups,
                               char LastGroup);
    // 解析 P 行：读取项目名称并写入项目
    static void ParseProjectLine(std::istringstream& LineStream,
                                 Project& TargetProject);
    // 解析 T/M 行：读取任务 ID、名称与工期，创建普通任务或里程碑并登记 ID 映射
    static void ParseTaskLine(const std::string& Kind,
                              std::istringstream& LineStream,
                              Project& TargetProject,
                              std::map<int, std::size_t>& TaskIdToIndex);
    // 解析 R 行：读取资源 ID、名称与单位成本，创建资源并登记 ID 映射
    static void ParseResourceLine(std::istringstream& LineStream,
                                  Project& TargetProject,
                                  std::map<int, std::size_t>& ResourceIdToIndex);
    // 解析 D 行：读取前后置任务 ID、类型与 Lag，创建依赖
    static void ParseDependencyLine(
        std::istringstream& LineStream,
        Project& TargetProject,
        const std::map<int, std::size_t>& TaskIdToIndex);
    // 解析 A 行：读取任务 ID、资源 ID 与数量，建立资源分配
    static void ParseAllocationLine(
        std::istringstream& LineStream,
        Project& TargetProject,
        const std::map<int, std::size_t>& TaskIdToIndex,
        const std::map<int, std::size_t>& ResourceIdToIndex);
};

#endif
