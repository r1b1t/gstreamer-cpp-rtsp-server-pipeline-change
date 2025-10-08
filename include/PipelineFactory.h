#ifndef PIPELINE_FACTORY_H
#define PIPELINE_FACTORY_H

#include <string>

class PipelineFactory
{
public:
    // static → bu fonksiyon sınıfa bağlı, nesne oluşturmadan çağırabilirsin.
    static std::string createTestVideo(int width = 640, int height = 480, int bitrate = 2000);
    static std::string createCamera(int width = 1280, int height = 720, int bitrate = 4000);
    static std::string createFileStream(const std::string &filepath);
    static std::string createSwitchablePipeline(int width = 640, int height = 480, int bitrate = 2000);
    static std::string createSwitchablePipeline2(int width = 640, int height = 480, int bitrate = 2000);
};

#endif // PIPELINE_FACTORY_H
