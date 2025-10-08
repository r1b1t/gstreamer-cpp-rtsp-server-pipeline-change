#include "RTSPServer.h"
#include "PipelineFactory.h"
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    RTSPServer server("8554");

    server.addStream("/test2", PipelineFactory::createSwitchablePipeline2(640, 480, 2000), true);
    server.addStream("/test1", PipelineFactory::createSwitchablePipeline(640, 480, 2000), true);

    server.run();

    std::thread([&server]()
                {

    /*Null ile yapsaydık: Yani yayın kesilir, RTSP client bağlantısı kopar,
    GStreamer pipeline sıfırdan başlatılır.*/
    std::this_thread::sleep_for(std::chrono::seconds(10));
    g_print(">>> PAUSE\n");
    server.pauseStream("/test2");
    server.pauseStream("/test");

    g_print(">>> Kaynak değiştiriliyor...\n");
    server.switchSource("/test2", 1);
    server.switchSource("/test1", 1);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    g_print(">>> PLAY\n");
    server.resumeStream("/test2");
    server.resumeStream("/test1"); })
        .detach();

    /* KESİNTİSİZ GEÇİŞ
    // 20 saniye sonra test → kamera geçişi
    std::thread([&server]()
                {
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Kaynak değiştir
    server.switchSource("/test", 1); })
        .detach();
    */

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
}
