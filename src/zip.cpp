
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include "zip.h"
#include <zlib.h>
#include <cstring>

const uint16_t _ZIP_SUPPORT_MAX_VERSION = 63;
const uint32_t _LOCAL_FILE_HEADER_SIGNATURE = 0x04034b50;
const uint32_t _DATA_DESCRIPTOR_SIGNATURE = 0x08074b50;

CompressMethod _toCompressMethod(uint16_t value)
{
    switch (value)
    {
    case 8:
        return CompressMethod::Deflate;
    case 0:
        return CompressMethod::None;

    default:
        return CompressMethod::UnKnown;
    }
}

bool Zip::open(const char *filePath)
{
    std::filesystem::path path(filePath); 
    _fileStream.open(path, std::ios::binary);
    if (!_fileStream.is_open())
        return false;

    // 解析zip文件流
    return _resolve();
}

/* local file header结构
    local file header signature     4 bytes  (0x04034b50)
    version needed to extract       2 bytes
    general purpose bit flag        2 bytes
    compression method              2 bytes
    last mod file time              2 bytes
    last mod file date              2 bytes
    crc-32                          4 bytes
    compressed size                 4 bytes
    uncompressed size               4 bytes
    file name length                2 bytes
    extra field length              2 bytes
    file name (variable size)
    extra field (variable size)
*/
bool Zip::_resolve()
{
    char buf[30];
    char *index;

    while (true)
    {
        _fileStream.read(buf, 30);
        if (_fileStream.eof() && _fileStream.fail())
            break;
        index = buf;

        // 不是LocalFile
        if (*(uint32_t*)index != _LOCAL_FILE_HEADER_SIGNATURE)
            return true;
        index += 4;

        // 支持的最小版本
        if (*(uint16_t *)index > _ZIP_SUPPORT_MAX_VERSION)
            return false;
        index += 2;

        // 构造LocalFile并加入files
        LocalFile* file = new LocalFile();
        // 通用位 Bit0代表加密, Bit3代表是否有数据描述
        file->isEncrypted = *(uint16_t *)index & 0x01;
        bool hasDataDescriptor = *(uint16_t *)index & 0x08;
        index += 2;

        file->compressMethod = _toCompressMethod(*(uint16_t *)index);
        index += 2;


        file->lastModTime = *(uint16_t *)index;
        index += 2;


        file->lastModDate = *(uint16_t *)index;
        index += 2;

        // 跳过crc32
        index += 4;

        file->compressedSize = *(uint32_t *)index;
        index += 4;

        file->uncompressedSize = *(uint32_t *)index;
        index += 4;

        // 读取文件名长度, 文件名不会以\0结尾
        file->fileName = new char[*(uint16_t *)index + 1];
        _fileStream.read(file->fileName, *(uint16_t *)index);
        file->fileName[*(uint16_t *)index] = 0;
        index += 2;

        // 额外信息
        _fileStream.seekg(*(uint16_t *)index, std::ios::cur);
        index += 2;

        file->start = _fileStream.tellg();
        files.push_back(file);

        _fileStream.seekg(file->compressedSize, std::ios::cur);

        if (hasDataDescriptor)
        {
            _fileStream.read(buf, 4);
            if (*(uint32_t *)buf == _DATA_DESCRIPTOR_SIGNATURE)
                _fileStream.seekg(12, std::ios::cur);
            else
                _fileStream.seekg(8, std::ios::cur);
        }
    }

    return true;
}

bool _decompress(std::vector<char>& source, std::ofstream& outFile, int windowsBit = -15) {
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    zs.next_in = (Bytef*)source.data();
    zs.avail_in = source.size();

    if(inflateInit2(&zs, windowsBit) != Z_OK)
        return false;

    const int CHUNK = 65536; // 64KB 缓冲区
    std::vector<char> out_buffer(CHUNK);
    int ret;

    do {
        zs.next_out = (Bytef*)out_buffer.data();
        zs.avail_out = CHUNK;

        ret = inflate(&zs, Z_NO_FLUSH);

        if (ret == Z_OK || ret == Z_STREAM_END) {
            // 计算解压了多少数据：总容量 - 剩余容量
            std::streamsize bytes_decompressed = CHUNK - zs.avail_out;
            // 直接写入文件
            outFile.write(out_buffer.data(), bytes_decompressed);
        }else {
            break;
        }
    } while (ret != Z_STREAM_END);

    return inflateEnd(&zs) == Z_OK;

}


bool Zip::unzip(const char* targetPath) {

    for(LocalFile* file : files) {
        std::filesystem::path path(targetPath);
        path.append(file->fileName);
        if(path.has_parent_path()){
            auto parentPath = path.parent_path();
            if(!std::filesystem::exists(parentPath))
                std::filesystem::create_directories(parentPath);
        }  
        std::ofstream outFile(path, std::ios::binary);
        if(!outFile.is_open())
            return false;

        bool ret = false;
        std::vector<char> compressData(file->compressedSize);
        _fileStream.seekg(file->start, std::ios::beg);
        _fileStream.read(compressData.data(), file->compressedSize);

        switch (file->compressMethod)
        {
        case CompressMethod::None:
            outFile.write(compressData.data(), file->uncompressedSize);
            ret = true;
            break;
        case CompressMethod::Deflate:
            ret = _decompress(compressData, outFile);
            break;
        default:
            break;
        }     

        outFile.close();
        if(!ret) return false;
    }
    return true;
}


Zip::~Zip()
{
    if (_fileStream.is_open())
        _fileStream.close();
    for(LocalFile *file : files) {
        delete file;
    }
}

LocalFile::~LocalFile()
{
    if(fileName)
        delete[] fileName;
}

std::ostream &operator<<(std::ostream &os, const CompressMethod &compressMethod)
{
    switch (compressMethod)
    {
    case CompressMethod::Deflate:
        os << "Deflate";
        break;
    case CompressMethod::None:
        os << "No Compress";
        break;
    default:
        os << "Unknown";
        break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const LocalFile &localFile)
{
    os << "LocalFile { "
        << "fileName: \"" << localFile.fileName << "\" "
        << "compressMethod: \"" << localFile.compressMethod << "\" "
        << " }";
    return os;
}


