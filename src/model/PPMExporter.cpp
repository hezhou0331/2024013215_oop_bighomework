//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 PPMExporter.cpp
//【功能模块和目的】         实现 PPM 格式导出器，把内存中的 Project 对象按 PPM 文本格式写入文件。
//【开发者及日期】           刘江宇, 2026-07-05
//【修改记录】               2026-07-05 里程碑行改为不输出工期字段，并按编码规范 V1.3 重写注释。
//-------------------------------------------------------------------------------------------------------------------
#include "model/PPMExporter.hpp"

#include "model/BasicTask.hpp"
#include "model/MilestoneTask.hpp"
#include "model/Project.hpp"

#include <fstream>
#include <stdexcept>

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMExporter::PPMExporter
//【函数功能】       默认构造 PPM 导出器，向基类登记本导出器支持的扩展名 "PPM"。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 基类重构为 Exporter<Project> 模板，构造时登记扩展名。
//-------------------------------------------------------------------------------------------------------------------
PPMExporter::PPMExporter()
    : Exporter<Project>("PPM")
{
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMExporter::~PPMExporter
//【函数功能】       析构 PPM 导出器；导出器不持有资源，无需额外清理。
//【参数】           无
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//-------------------------------------------------------------------------------------------------------------------
PPMExporter::~PPMExporter() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       PPMExporter::SaveToStream
//【函数功能】       按 PPM 格式把项目模型写入已打开的文件流：先写项目名，再依次写任务
//                   块、资源块、依赖块与分配块，块间以空行分隔。文件打开与扩展名校验
//                   由基类 SaveToFile 统一完成。
//【参数】           Stream（输出参数）：已打开的导出目标文件输出流；
//                   SourceProject（输入参数）：待导出的项目模型。
//【返回值】         无。
//【开发者及日期】   刘江宇, 2026-07-05
//【修改记录】       2026-07-07 由 Export(Project, FileName) 重构为基类模板的流写出接口。
//-------------------------------------------------------------------------------------------------------------------
void PPMExporter::SaveToStream(std::ofstream& Stream,
                               const Project& SourceProject) const
{
    Stream << "# Exported by Project Scheduler\n";
    Stream << "P " << SourceProject.GetName() << "\n\n"; //P 行有且仅有一行，位于最顶部

    //任务块：普通任务输出 "T ID 名称 工期"，里程碑输出 "M ID 名称"（无工期字段）
    for (std::size_t Index = 0; Index < SourceProject.GetTaskCount();
         ++Index) {
        const Task& CurrentTask = SourceProject.GetTask(Index);
        if (CurrentTask.GetDuration() == 0) {         //工期 0 即里程碑任务
            Stream << "M " << Index << " " << CurrentTask.GetName() << "\n";
        }
        else {
            Stream << "T " << Index << " " << CurrentTask.GetName() << " "
                   << CurrentTask.GetDuration() << "\n";
        }
    }

    Stream << "\n";
    //资源块："R ID 名称 单位成本"
    for (std::size_t Index = 0; Index < SourceProject.GetResourceCount();
         ++Index) {
        const Resource& CurrentResource = SourceProject.GetResource(Index);
        Stream << "R " << Index << " " << CurrentResource.GetName() << " "
               << CurrentResource.GetUnitCost() << "\n";
    }

    Stream << "\n";
    //依赖块："D 前置ID 后置ID 类型 Lag"
    for (const Dependency& CurrentDependency
         : SourceProject.GetDependencies()) {
        Stream << "D " << CurrentDependency.GetPredecessor() << " "
               << CurrentDependency.GetSuccessor() << " "
               << CurrentDependency.GetTypeText() << " "
               << CurrentDependency.GetLag() << "\n";
    }

    Stream << "\n";
    //分配块："A 任务ID 资源ID 数量"，位于文件末尾区域
    for (std::size_t TaskIndex = 0; TaskIndex < SourceProject.GetTaskCount();
         ++TaskIndex) {
        const Task& CurrentTask = SourceProject.GetTask(TaskIndex);
        for (const ResourceAllocation& Allocation
             : CurrentTask.GetResources()) {
            Stream << "A " << TaskIndex << " "
                   << Allocation.GetResourceIndex() << " "
                   << Allocation.GetQuantity() << "\n";
        }
    }
}
