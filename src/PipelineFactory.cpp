#include "PipelineFactory.h"

std::string PipelineFactory::createTestVideo(int width, int height, int bitrate)
{
    return "( videotestsrc is-live=true "
           "! video/x-raw,framerate=30/1,width=" +
           std::to_string(width) + ",height=" + std::to_string(height) +
           " ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=" + std::to_string(bitrate) + " key-int-max=30 "
                                                                                                     "! rtph264pay name=pay0 pt=96 config-interval=1 )";
}

std::string PipelineFactory::createCamera(int width, int height, int bitrate)
{
    return "( ksvideosrc "
           "! video/x-raw,format=YUY2,"
           "width=" +
           std::to_string(width) +
           ",height=" + std::to_string(height) +
           ",framerate=30/1 "
           "! videoconvert "
           "! x264enc tune=zerolatency speed-preset=ultrafast "
           "bitrate=" +
           std::to_string(bitrate) + " key-int-max=30 "
                                     "! rtph264pay name=pay0 pt=96 config-interval=1 )";
}

std::string PipelineFactory::createFileStream(const std::string &filepath)
{
    return "( filesrc location=" + filepath +
           " ! decodebin "
           " ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=30 "
           " ! rtph264pay name=pay0 pt=96 config-interval=1 )";
}

/*
input-selector, GStreamer’daki özel bir elementtir.
Birden fazla giriş (sink pad) alır, sadece bir tanesini aktif olarak seçer ve bunu çıkışa gönderir.

Yani input-selector = “birden fazla kaynaktan sadece birini seçip devam ettir”
*/
std::string PipelineFactory::createSwitchablePipeline(int width, int height, int bitrate)
{
    return "( input-selector name=sel "
           // 1️⃣ statik test (Renkli SMPTE bar)
           "videotestsrc is-live=true pattern=snow ! "
           // ile formatlanıyor ve sıraya (queue) alınarak input-selector’a gönderiliyor.
           "video/x-raw,framerate=30/1,width=" +
           std::to_string(width) +
           ",height=" + std::to_string(height) +
           " ! queue ! sel.sink_0 "

           // 2️⃣ hareketli top (pingpong)
           "videotestsrc is-live=true pattern=gradient ! "
           // ile formatlanıyor ve sıraya (queue) alınarak input-selector’a gönderiliyor.
           "video/x-raw,framerate=30/1,width=" +
           std::to_string(width) +
           ",height=" + std::to_string(height) +
           " ! queue ! sel.sink_1 "

           // çıkış (encoder + RTP)
           /*
           videoconvert: Giriş formatını x264 encoder’ın anlayacağı biçime çevirir.
           x264enc: H.264 formatında video sıkıştırma (encode) yapar.
           tune=zerolatency: Canlı yayın için düşük gecikme ayarı
           speed-preset=ultrafast: CPU’yu az yoran hızlı sıkıştırma
           bitrate: Fonksiyon parametresiyle belirleniyor
           key-int-max=30: Maksimum keyframe aralığı 1 saniye (30 fps’te)
           rtph264pay: RTP (Real-Time Protocol) paketleyici, yani ağ üzerinden yayın yapılabilir hale getiriyor.*/
           "sel. ! videoconvert ! x264enc tune=zerolatency speed-preset=ultrafast "
           "bitrate=" +
           std::to_string(bitrate) +
           " key-int-max=30 ! rtph264pay name=pay0 pt=96 config-interval=1 )";
}

std::string PipelineFactory::createSwitchablePipeline2(int width, int height, int bitrate)
{
    return "( ksvideosrc device-index=0 ! video/x-raw,width=" +
           std::to_string(width) + ",height=" + std::to_string(height) +
           ",framerate=30/1 ! tee name=t "

           // 1 Orijinal renk, hızlı ama kaliteli
           /*videobalance: renkleri değiştirir
           hue=0.5: ton dengesini değiştirir
           saturation=1.8: renk doygunluğunu artırır
           brightness=0.1: parlaklığı biraz artırır*/
           "t. ! queue ! videoconvert ! x264enc tune=zerolatency speed-preset=veryfast "
           "bitrate=" +
           std::to_string(bitrate) +
           " key-int-max=30 bframes=0 qp-min=10 qp-max=35 "
           "! queue ! sel.sink_0 "

           // 2 RENKLİ + düşük kalite (belirgin fark)
           "t. ! queue ! videoconvert ! videobalance hue=0.5 saturation=1.8 brightness=0.1 ! "
           "x264enc tune=zerolatency speed-preset=ultrafast "
           "bitrate=" +
           std::to_string(bitrate / 2) +
           " key-int-max=15 bframes=0 qp-min=25 qp-max=50 "
           "! queue ! sel.sink_1 "

           // Selector çıkışı (RTSP payloader)
           "input-selector name=sel ! queue ! rtph264pay name=pay0 pt=96 config-interval=1 )";
}
