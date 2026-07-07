//-------------------------------------------------------------------------------------------------------------------
//【文件名】                 FilePorter.hpp
//【功能模块和目的】         声明并实现文件读写接口基类模板，提供扩展名校验、文件可打开性
//                           检测等与具体模型类型无关的通用工具，供导入器/导出器模板继承。
//【开发者及日期】           2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
#ifndef PROJECT_SCHEDULER_FILE_PORTER_HPP
#define PROJECT_SCHEDULER_FILE_PORTER_HPP

//std::invalid_argument 异常基类所属头文件
#include <stdexcept>
//std::string 所属头文件
#include <string>
//std::ifstream/std::ofstream 所属头文件
#include <fstream>
//std::toupper 所属头文件
#include <cctype>

//-------------------------------------------------------------------------------------------------------------------
//【类名】             FilePorterType（enum class）
//【功能】             标记文件接口的传输方向：IMPORTER 表示导入器，EXPORTER 表示导出器；
//                     作为 FilePorter 的非类型模板参数使用。
//【接口说明】         枚举量 IMPORTER、EXPORTER。
//【开发者及日期】     2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
enum class FilePorterType {
    IMPORTER,    //导入方向：从文件读入模型
    EXPORTER     //导出方向：把模型写入文件
};

//-------------------------------------------------------------------------------------------------------------------
//【类名】             FilePorter
//【功能】             文件读写接口基类模板（模板参数为传输方向）：与具体模型类型和文件
//                     格式无关，可在任何需要按扩展名收发文件的应用中重用。
//【接口说明】         静态 FileTypeTest 校验文件扩展名（不区分大小写，不符抛出异常）；
//                     静态 FileOpenTest 按传输方向测试文件能否打开（失败抛出异常）；
//                     静态 GetExtName 截取文件名中的扩展名；静态 ToUpperCopy 返回大写
//                     副本；内嵌异常类 INVALID_FILE_TYPE、FILE_OPEN_FAIL 用于报告错误；
//                     构造函数受保护，仅允许派生类实例化，禁止拷贝与赋值。
//【开发者及日期】     2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
class FilePorter {
public:
    //-----------------------------------------------------------------------------------------------------------
    //内嵌异常类
    //-----------------------------------------------------------------------------------------------------------
    //【类名】INVALID_FILE_TYPE【功能】报告非法文件扩展名的异常【接口说明】以扩展名构造，
    //what() 返回错误描述【开发者及日期】2024013215, 2026-07-07
    class INVALID_FILE_TYPE : public std::invalid_argument {
    public:
        // 扩展名是必要信息，不应存在默认构造函数
        INVALID_FILE_TYPE() = delete;
        // 带参构造函数：以非法扩展名生成错误描述
        explicit INVALID_FILE_TYPE(const std::string& FileExtName);
        // 拷贝构造函数：异常对象须可拷贝以支持抛出语义
        INVALID_FILE_TYPE(const INVALID_FILE_TYPE& Source);
        // 拷贝赋值运算符
        INVALID_FILE_TYPE& operator=(const INVALID_FILE_TYPE& Source);
        // 析构函数：无额外资源需要释放
        ~INVALID_FILE_TYPE() override;
    };

    //【类名】FILE_OPEN_FAIL【功能】报告文件打开失败的异常【接口说明】以文件名构造，
    //what() 返回错误描述【开发者及日期】2024013215, 2026-07-07
    class FILE_OPEN_FAIL : public std::invalid_argument {
    public:
        // 文件名是必要信息，不应存在默认构造函数
        FILE_OPEN_FAIL() = delete;
        // 带参构造函数：以打开失败的文件名生成错误描述
        explicit FILE_OPEN_FAIL(const std::string& FileName);
        // 拷贝构造函数：异常对象须可拷贝以支持抛出语义
        FILE_OPEN_FAIL(const FILE_OPEN_FAIL& Source);
        // 拷贝赋值运算符
        FILE_OPEN_FAIL& operator=(const FILE_OPEN_FAIL& Source);
        // 析构函数：无额外资源需要释放
        ~FILE_OPEN_FAIL() override;
    };

    //-----------------------------------------------------------------------------------------------------------
    //静态工具成员函数
    //-----------------------------------------------------------------------------------------------------------
    // 校验文件名扩展名与给定扩展名一致（不区分大小写），不一致抛出 INVALID_FILE_TYPE
    static void FileTypeTest(const std::string& FileName,
                             const std::string& FileExtName);
    // 按传输方向测试文件能否打开（导入测可读、导出测可写），失败抛出 FILE_OPEN_FAIL
    static void FileOpenTest(const std::string& FileName);
    // 截取文件名中最后一个 '.' 之后的扩展名，无扩展名返回空字符串
    static std::string GetExtName(const std::string& FileName);
    // 返回字符串的大写副本，用于扩展名的不区分大小写比较
    static std::string ToUpperCopy(const std::string& Text);

    //-----------------------------------------------------------------------------------------------------------
    //公有静态常量数据成员
    //-----------------------------------------------------------------------------------------------------------
    static constexpr FilePorterType PORT_TYPE{DIRECTION};    //本接口的传输方向

protected:
    //-----------------------------------------------------------------------------------------------------------
    //受保护的构造、析构、赋值行为（仅允许派生类实例化）
    //-----------------------------------------------------------------------------------------------------------
    // 默认构造函数：基类无内部状态，仅供派生类触发
    FilePorter();
    // 文件接口为不可复制的工具对象，禁止拷贝构造
    FilePorter(const FilePorter& Source) = delete;
    // 文件接口为不可复制的工具对象，禁止拷贝赋值
    FilePorter& operator=(const FilePorter& Source) = delete;
    // 虚析构函数：作为继承体系的基类，保证派生类析构被正确调用
    virtual ~FilePorter();
};

//-------------------------------------------------------------------------------------------------------------------
//以下为 FilePorter 类模板的实现部分（类模板的声明与实现均须位于头文件中，声明在前、实现在后）
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::INVALID_FILE_TYPE::INVALID_FILE_TYPE
//【函数功能】       以非法扩展名构造异常对象，生成含扩展名的错误描述文本。
//【参数】           FileExtName（输入参数）：非法的文件扩展名。
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::INVALID_FILE_TYPE::INVALID_FILE_TYPE(
    const std::string& FileExtName)
    : std::invalid_argument("'" + FileExtName
                            + "' is an invalid file extension name") {
}

// 拷贝构造函数：采用基类默认逐成员拷贝
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::INVALID_FILE_TYPE::INVALID_FILE_TYPE(
    const INVALID_FILE_TYPE& Source) = default;

// 拷贝赋值运算符：采用基类默认逐成员赋值
template<FilePorterType DIRECTION>
typename FilePorter<DIRECTION>::INVALID_FILE_TYPE&
FilePorter<DIRECTION>::INVALID_FILE_TYPE::operator=(
    const INVALID_FILE_TYPE& Source) = default;

// 析构函数：无额外资源需要释放
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::INVALID_FILE_TYPE::~INVALID_FILE_TYPE() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::FILE_OPEN_FAIL::FILE_OPEN_FAIL
//【函数功能】       以打开失败的文件名构造异常对象，生成含文件名的错误描述文本。
//【参数】           FileName（输入参数）：打开失败的文件路径。
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::FILE_OPEN_FAIL::FILE_OPEN_FAIL(
    const std::string& FileName)
    : std::invalid_argument("'" + FileName + "' open fail") {
}

// 拷贝构造函数：采用基类默认逐成员拷贝
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::FILE_OPEN_FAIL::FILE_OPEN_FAIL(
    const FILE_OPEN_FAIL& Source) = default;

// 拷贝赋值运算符：采用基类默认逐成员赋值
template<FilePorterType DIRECTION>
typename FilePorter<DIRECTION>::FILE_OPEN_FAIL&
FilePorter<DIRECTION>::FILE_OPEN_FAIL::operator=(
    const FILE_OPEN_FAIL& Source) = default;

// 析构函数：无额外资源需要释放
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::FILE_OPEN_FAIL::~FILE_OPEN_FAIL() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::FilePorter
//【函数功能】       默认构造文件接口基类；基类无内部状态，无需额外初始化。
//【参数】           无
//【返回值】         构造函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::FilePorter() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::~FilePorter
//【函数功能】       析构文件接口基类；基类不持有资源，无需额外清理。
//【参数】           无
//【返回值】         析构函数无返回值。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
FilePorter<DIRECTION>::~FilePorter() = default;

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::FileTypeTest
//【函数功能】       校验给定文件名的扩展名与期望扩展名一致（不区分大小写），不一致抛出
//                   INVALID_FILE_TYPE 异常。
//【参数】           FileName（输入参数）：待校验的文件路径；
//                   FileExtName（输入参数）：期望的文件扩展名（不含 '.'）。
//【返回值】         void，无返回值；校验失败以异常报告。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
void FilePorter<DIRECTION>::FileTypeTest(const std::string& FileName,
                                         const std::string& FileExtName)
{
    //扩展名比较不区分大小写，容忍 .PPM 与 .ppm 等写法差异
    if (ToUpperCopy(GetExtName(FileName)) != ToUpperCopy(FileExtName)) {
        throw INVALID_FILE_TYPE(GetExtName(FileName));
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::FileOpenTest
//【函数功能】       按传输方向测试文件能否打开：导入方向测试文件是否存在且可读；导出
//                   方向以追加模式测试文件是否可写（不破坏已有内容）；失败抛出
//                   FILE_OPEN_FAIL 异常。
//【参数】           FileName（输入参数）：待测试的文件路径。
//【返回值】         void，无返回值；打开失败以异常报告。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
void FilePorter<DIRECTION>::FileOpenTest(const std::string& FileName)
{
    if constexpr (DIRECTION == FilePorterType::IMPORTER) {
        //导入方向：文件必须已存在且可读
        std::ifstream TestStream(FileName);
        if (!TestStream.is_open()) {
            throw FILE_OPEN_FAIL(FileName);
        }
    }
    else {
        //导出方向：以追加模式试开，检验目标可写且不破坏已有内容
        std::ofstream TestStream(FileName, std::ios::app);
        if (!TestStream.is_open()) {
            throw FILE_OPEN_FAIL(FileName);
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::GetExtName
//【函数功能】       截取文件名中最后一个 '.' 之后的部分作为扩展名。
//【参数】           FileName（输入参数）：文件路径。
//【返回值】         std::string，文件扩展名；无扩展名或 '.' 位于首位时返回空字符串。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
std::string FilePorter<DIRECTION>::GetExtName(const std::string& FileName)
{
    std::size_t Position = FileName.find_last_of('.');
    //找不到 '.' 或 '.' 在首位均视为无扩展名
    if ((Position == std::string::npos) || (Position == 0)) {
        return std::string("");
    }
    return FileName.substr(Position + 1);
}

//-------------------------------------------------------------------------------------------------------------------
//【函数名称】       FilePorter::ToUpperCopy
//【函数功能】       生成给定字符串的大写副本，用于扩展名的不区分大小写比较。
//【参数】           Text（输入参数）：原始字符串。
//【返回值】         std::string，全部字母转为大写后的副本。
//【开发者及日期】   2024013215, 2026-07-07
//【更改记录】
//-------------------------------------------------------------------------------------------------------------------
template<FilePorterType DIRECTION>
std::string FilePorter<DIRECTION>::ToUpperCopy(const std::string& Text)
{
    std::string Result = Text;
    for (std::size_t Index = 0; Index < Result.size(); ++Index) {
        Result[Index] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(Result[Index])));
    }
    return Result;
}

#endif
