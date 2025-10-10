#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>
#include <vector>
#include <map>

class RTSPServer
{
public:
    explicit RTSPServer(const std::string &service = "8554");
    ~RTSPServer();

    void addStream(const std::string &path, const std::string &pipeline_desc, bool shared = true);
    void switchSource(const std::string &path, int sourceIndex);
    void pauseStream(const std::string &path);
    void resumeStream(const std::string &path);

    void run();

private:
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    /*
    Eklenen tüm RTSP mount path’lerin listesini tutar.
    run() çağrısında bunlar bastırılır.*/
    std::vector<std::string> paths;
    // Her mount path’e karşılık gelen aktif GStreamer pipeline objesini saklar.
    std::map<std::string, GstElement *> activePipelines;

    /*
    friend anahtar kelimesi, bu fonksiyona sınıfın özel üyelerine erişim izni verir.
    Normalde private değişkenlere dışarıdan erişilemez, ama “friend” fonksiyon erişebilir.
    Bu gerekli çünkü on_media_configure() bir C-style callback — GStreamer bunu çağırıyor, C++ sınıfı değil.
    Böylece bu callback içinde:
    self->activePipelines[path] = pipeline;
    gibi işlemleri yapabiliyorsun.*/
    friend void on_media_configure(GstRTSPMediaFactory *factory,
                                   GstRTSPMedia *media,
                                   gpointer user_data);
};

#endif
