//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 Exporter.hpp
//【功能模块和目的】         声明并实现导出器基类模板：按文件扩展名注册/查找具体导出器，
//                           把任意模型类型 T 的对象写入指定文件，与具体文件格式解耦。
//【开发者及日期】           2024013215, 2026-07-07
//【更改记录】               2026-07-07 由非模板抽象基类重构为类模板，增加扩展名注册工厂。
//-------------------------------------------------------------------------------------------------------------------
#ifndef EXPORTER_HPP
#define EXPORTER_HPP

//FilePorter 基类模板所属头文件
#include "model/FilePorter.hpp"

//std::shared_ptr、std::make_shared 所属头文件
#include <memory>
//std::string 所属头文件
#include <string>
//std::ofstream 所属头文件
#include <fstream>
//std::vector 所属头文件
#include <vector>
//std::is_base_of 所属头文件
#include <type_traits>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             Exporter
//【功能】             导出器基类模板（模板参数 T 为源模型类型）：定义"模型对象到文件"
//                     的统一导出流程与按扩展名注册/查找具体导出器的工厂机制，可脱离本
//                     作业用于任何需要多格式文件导出的应用。
//【接口说明】         SaveToFile 校验扩展名与文件可写性后调用派生类 SaveToStream 把 T 对象
//                     写入文件流；SaveToStream 为纯虚函数，由具体格式派生类实现；静态
//                     Register<DERIVED> 注册一种具体导出器（扩展名不可重复）；静态
//                     GetInstanceByFileName/GetInstanceByExtName 按文件名/扩展名查找已注册
//                     导出器实例指针，未注册抛出 INVALID_FILE_TYPE；构造函数受保护，禁止
//                     拷贝与赋值；GetSupportedExtName 只读返回所支持扩展名。
//【开发者及日期】     2024013215, 2026-07-07
//【更改记录】         2026-07-07 由非模板抽象基类重构为类模板，增加扩展名注册工厂。
//-------------------------------------------------------------------------------------------------------------------
template<class T>
class Exporter : public FilePorter<FilePorterType::EXPORTER> {
public:
    //-----------------------------------------------------------------------------------------------------------
    //公有的析构、被删除的拷贝构造与赋值行为
    //-----------------------------------------------------------------------------------------------------------
    // 导出器以注册实例形式共享使用，禁止拷贝构造
    Exporter(const Exporter& Source) = delete;
    // 导出器以注册实例形式共享使用，禁止拷贝赋值
    Exporter& operator=(const Exporter& Source) = delete;
    // 虚析构函数：作为继承体系的基类，保证派生类析构被正确调用
    virtual ~Exporter();

    //-----------------------------------------------------------------------------------------------------------
    //非静态 Getter 成员函数（均有后置 const，不改变导出器自身状态）
    //-----------------------------------------------------------------------------------------------------------
    // 校验扩展名与文件可写性后，把 T 类型对象写入指定文件
    void SaveToFile(const T& SourceObject, const std::string& FileName) const;
    // 把 T 类型对象写入已打开的文件流，由具体格式派生类实现
    virtual void SaveToStream(std::ofstream& Stream,
                              const T& SourceObject) const = 0;
    // 返回本导出器支持的文件扩展名（不含 '.'）
    const std::string& GetSupportedExtName() const;

    //-----------------------------------------------------------------------------------------------------------
    //静态工厂成员函数
    //-----------------------------------------------------------------------------------------------------------
    // 注册一种具体导出器（DERIVED 须为 Exporter<T> 的派生类，扩展名不可与已注册者重复）
    template<class DERIVED>
    static void Register();
    // 按文件名中的扩展名查找已注册导出器实例指针，未注册抛出 INVALID_FILE_TYPE
    static std::shared_ptr<Exporter<T>> GetInstanceByFileName(
        const std::string& FileName);
    // 按扩展名（不区分大小写）查找已注册导出器实例指针，未注册抛出 INVALID_FILE_TYPE
    static std::shared_ptr<Exporter<T>> GetInstanceByExtName(
        const std::string& FileExtName);

protected:
    //-----------------------------------------------------------------------------------------------------------
    //受保护的构造行为（仅允许派生类实例化）
    //-----------------------------------------------------------------------------------------------------------
    // 带参构造函数：指定本导出器支持的文件扩展名，扩展名不可为空
    explicit Exporter(const std::string& FileExtName);

private:
    //-----------------------------------------------------------------------------------------------------------
    //私有数据成员
    //-----------------------------------------------------------------------------------------------------------
    std::string m_ExtName;    //本导出器支持的文件扩展名（不含 '.'）

    //所有已注册导出器实例指针的列表（静态工厂的登记表）
    static std::vector<std::shared_ptr<Exporter<T>>> m_pExporters;
};

//-------------------------------------------------------------------------------------------------------------------
//以下为 Exporter 类模板的实现部分（类模板的声明与实现均须位于头文件中，声明在前、实现在后）
//-------------------------------------------------------------------------------------------------------------------

//所有已注册导出器实例指针的列表（类模板静态数据成员定义）
template<class T>
std::vector<std::shared_ptr<Exporter<T>>> Exporter<T>::m_pExporters{};

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::Exporter
//【函数功能】       以支持的文件扩展名构造导出器基类部分；扩展名为空抛出异常。
//【参数】           FileExtName（输入参数）：本导出器支持的文件扩展名（不含 '.'）。
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
Exporter<T>::Exporter(const std::string& FileExtName)
    : FilePorter<FilePorterType::EXPORTER>(), m_ExtName(FileExtName)
{
    //扩展名是查找导出器的唯一依据，不允许为空
    if (FileExtName == "") {
        throw INVALID_FILE_TYPE("EMPTY");
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::~Exporter
//【函数功能】       析构导出器基类部分；不持有动态资源，无需额外清理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
Exporter<T>::~Exporter() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::GetSupportedExtName
//【函数功能】       读取本导出器支持的文件扩展名。
//【参数】           无
//【返回值】         const std::string&，本导出器支持的文件扩展名（不含 '.'）。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
const std::string& Exporter<T>::GetSupportedExtName() const
{
    return m_ExtName;
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::SaveToFile
//【函数功能】       完成统一导出流程：校验文件扩展名、测试文件可写性、打开文件流，再
//                   调用派生类实现的 SaveToStream 把模型对象写入文件。
//【参数】           SourceObject（输入参数）：待导出的模型对象；
//                   FileName（输入参数）：导出目标文件路径。
//【返回值】         void，无返回值；扩展名不符或文件不可写时抛出异常。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
void Exporter<T>::SaveToFile(const T& SourceObject,
                             const std::string& FileName) const
{
    FileTypeTest(FileName, m_ExtName);    //扩展名不符抛出 INVALID_FILE_TYPE
    FileOpenTest(FileName);               //文件不可写抛出 FILE_OPEN_FAIL
    std::ofstream Stream(FileName);
    SaveToStream(Stream, SourceObject);   //具体格式化交给派生类实现
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::Register（静态）
//【函数功能】       注册一种具体导出器：实例化 DERIVED 对象并登记到导出器列表；DERIVED
//                   必须是 Exporter<T> 的派生类，且扩展名不可与已注册导出器重复。
//【参数】           无（模板参数 DERIVED 指定被注册的具体导出器类型）。
//【返回值】         void，无返回值；扩展名重复时抛出 INVALID_FILE_TYPE。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
template<class DERIVED>
void Exporter<T>::Register()
{
    //编译期断言：只允许注册 Exporter<T> 的派生类
    static_assert(std::is_base_of<Exporter<T>, DERIVED>::value,
                  "DERIVED must inherit from Exporter<T>");
    std::shared_ptr<Exporter<T>> pDerivedExporter
        = std::make_shared<DERIVED>();
    //扩展名查重（不区分大小写），同一扩展名只允许一个导出器
    for (const std::shared_ptr<Exporter<T>>& pExporter : m_pExporters) {
        if (ToUpperCopy(pExporter->GetSupportedExtName())
            == ToUpperCopy(pDerivedExporter->GetSupportedExtName())) {
            throw INVALID_FILE_TYPE(pDerivedExporter->GetSupportedExtName());
        }
    }
    m_pExporters.push_back(pDerivedExporter);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::GetInstanceByFileName（静态）
//【函数功能】       截取文件名中的扩展名，查找并返回对应的已注册导出器实例指针。
//【参数】           FileName（输入参数）：导出目标文件路径。
//【返回值】         std::shared_ptr<Exporter<T>>，对应导出器实例指针；未注册抛出
//                   INVALID_FILE_TYPE。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
std::shared_ptr<Exporter<T>> Exporter<T>::GetInstanceByFileName(
    const std::string& FileName)
{
    return GetInstanceByExtName(GetExtName(FileName));
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       Exporter::GetInstanceByExtName（静态）
//【函数功能】       按扩展名（不区分大小写）在已注册导出器列表中查找并返回实例指针。
//【参数】           FileExtName（输入参数）：文件扩展名（不含 '.'）。
//【返回值】         std::shared_ptr<Exporter<T>>，对应导出器实例指针；未注册抛出
//                   INVALID_FILE_TYPE。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<class T>
std::shared_ptr<Exporter<T>> Exporter<T>::GetInstanceByExtName(
    const std::string& FileExtName)
{
    //遍历登记表，扩展名比较不区分大小写
    for (const std::shared_ptr<Exporter<T>>& pExporter : m_pExporters) {
        if (ToUpperCopy(pExporter->GetSupportedExtName())
            == ToUpperCopy(FileExtName)) {
            return pExporter;
        }
    }
    throw INVALID_FILE_TYPE(FileExtName);
}

#endif
