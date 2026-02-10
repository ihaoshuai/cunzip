#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>


enum class CompressMethod : uint16_t
{
    Deflate = 8,
    None = 0,
    UnKnown = 100,
};

struct LocalFile
{
    bool isEncrypted;
    CompressMethod compressMethod;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    char *fileName;
    std::streampos start;
public:
    ~LocalFile();
};

/*  Zip文件结构
    [local file header 1]
    [encryption header 1]
    [file data 1]
    [data descriptor 1]
    .
    .
    .
    [local file header n]
    [encryption header n]
    [file data n]
    [data descriptor n]
    [archive decryption header]
    [archive extra data record]
    [central directory header 1]
    .
    .
    .
    [central directory header n]
    [zip64 end of central directory record]
    [zip64 end of central directory locator]
    [end of central directory record]
*/
class Zip
{
public:
    std::vector<LocalFile*> files;
    bool open(const char *filePath);
    bool unzip(const char *targetPath);
    ~Zip();
private:
    std::ifstream _fileStream;
    bool _resolve();
};

std::ostream& operator<<(std::ostream& os, const LocalFile& localFile);
