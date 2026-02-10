#include "spdlog/spdlog.h"
#include "zip.h"

int main()
{
    

    Zip zipFile;
    if (!zipFile.open("E:/resource/comic/碧蓝之海/[Kmoe][GRANDBLUE碧藍之海]卷01.epub"))
        spdlog::error("fail to open file");
    else
    {
        std::cout << "file size: " << zipFile.files.size() << std::endl;
        // for(LocalFile *file : zipFile.files) {
        //     std::cout << *file << std::endl;
        // }
        if(zipFile.unzip("out"))
            spdlog::info("unzip file succeed");
        else
            spdlog::error("unzip file fail");
    }

    std::cin.get();
}