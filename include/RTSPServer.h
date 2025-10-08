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
    std::vector<std::string> paths;
    std::map<std::string, GstElement *> activePipelines;

    // 👇 Bu satır tam olarak burada olmalı (private: kısmının *içinde*)
    friend void on_media_configure(GstRTSPMediaFactory *factory,
                                   GstRTSPMedia *media,
                                   gpointer user_data);
};

#endif
