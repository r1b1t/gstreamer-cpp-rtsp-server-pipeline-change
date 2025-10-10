#include "RTSPServer.h"
#include <iostream>
#include <gst/app/gstappsrc.h>

RTSPServer::RTSPServer(const std::string &service)
{
    server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, service.c_str());
    mounts = gst_rtsp_server_get_mount_points(server);
}

RTSPServer::~RTSPServer()
{
    if (mounts)
        g_object_unref(mounts);
    if (server)
        g_object_unref(server);
}
/*Bu on_media_configure() fonksiyonu, RTSP sunucusu her yeni pipeline (yani akış oturumu) oluşturduğunda otomatik olarak çağrılıyor.
RTSP sunucusu yeni bir istemci bağlandığında oluşturduğu GStreamer pipeline nesnesini yakalayıp kaydetmek.
Bu sayede switchSource("/test", 1) diyerek o pipeline’ın içindeki input-selector elementine ulaşıp,
anlık kaynak değişimi yapabiliyoruz
*/
void on_media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
{
    // 'user_data' parametresi GStreamer tarafından callback'e gönderilir (void* tipindedir).
    // Biz bu verinin RTSPServer nesnesi olduğunu biliyoruz, o yüzden kendi tipimize çeviriyoruz.
    // Böylece callback içinden sınıfın üyelerine (ör. activePipelines) erişebiliriz.
    auto *self = static_cast<RTSPServer *>(user_data);

    /*
    - Callback fonksiyonuna, `addStream()` sırasında `this` (RTSPServer nesnesi) gönderilmişti.
    - Burada onu geri alıyoruz.
    - Böylece bu fonksiyon `RTSPServer` sınıfının içindeki verilere (`activePipelines`) erişebiliyor.*/
    GstElement *pipeline = gst_rtsp_media_get_element(media);

    if (pipeline)
    {
        // Bu media, factory'nin mount path'ine aittir.
        // Media objesinin mount path’ini almak yerine factory’ye ait path’i biliyoruz.
        // O yüzden callback'e bağlarken user_data olarak path’i geçiriyoruz.
        /*
        factory nesnesinden daha önce addStream() içinde eklediğimiz "mount-path" bilgisini alıyoruz.
        Bu bilgi RTSP path’tir, örneğin:
        "/test"  veya  "/camera"
        Yani bu pipeline hangi yayına ait, onu öğreniyoruz.*/
        const char *streamPath = static_cast<const char *>(g_object_get_data(G_OBJECT(factory), "mount-path"));
        if (streamPath)
        {
            self->activePipelines[streamPath] = pipeline;
            g_print("Media configured for path: %s\n", streamPath);
        }

        /*
        RTSP server yeni bir pipeline oluşturduğunda bu kod çalışır.
        Oluşturulan pipeline'ı alıp hangi RTSP yoluna (ör. "/test") aitse o path ile kaydediyoruz.
        Böylece switchSource() doğru pipeline'ı bulup kaynak değişimi yapabilir.*/
    }
}

void RTSPServer::addStream(const std::string &path, const std::string &pipeline_desc, bool shared)
{
    /*Yeni bir **RTSP medya fabrikası** oluşturuyoruz.
    Bu, her istemci (örneğin VLC, ffplay) bağlandığında GStreamer pipeline’ını başlatır.
    RTSP Server’ın “yayın kaynağı üreticisi”dir.*/
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, pipeline_desc.c_str());
    /* Eğer `shared = true` ise:
    Tüm istemciler **aynı pipeline’ı** paylaşır.
    Yani 1 kişi bağlansa da 5 kişi bağlansa da, tek bir pipeline çalışır.

    `shared = false` olsaydı, her istemci bağlandığında yeni bir pipeline başlardı (fazla CPU harcar).
    Bu projede biz **tek pipeline paylaşıyoruz** çünkü `input-selector` ile kaynak değiştireceğiz.
    */
    gst_rtsp_media_factory_set_shared(factory, shared);

    // Her factory'ye kendi RTSP yolunu (/test, /camera vs.) iliştiriyoruz.
    // Böylece 'media-configure' callback'inde hangi yayının pipeline'ı oluşturulduğunu bilebiliyoruz.
    g_object_set_data(G_OBJECT(factory), "mount-path", g_strdup(path.c_str()));

    // Media configure callback
    g_signal_connect(factory, "media-configure", (GCallback)on_media_configure, this);

    gst_rtsp_mount_points_add_factory(mounts, path.c_str(), factory);
    paths.push_back(path);
}

void RTSPServer::switchSource(const std::string &path, int sourceIndex)
{
    /*
    activePipelines haritasında her RTSP mount path’e (/test, /camera vb.) karşılık gelen GStreamer pipeline saklanıyor.
    Yani hangi yayına ait pipeline olduğunu bu satır buluyor.*/
    auto it = activePipelines.find(path);
    if (it == activePipelines.end())
    {
        g_printerr("Pipeline bulunamadı. (İstemci bağlı mı?)\n");
        return;
    }

    /*Bu satırda it aslında bir iterator — yani std::map (C++ haritası) içinde dolaşan bir imleç.
    Haritamız şu şekilde tanımlanmıştı:
    Bu harita şunu tutuyor:
    Key (anahtar) → RTSP path (örneğin "/test" veya "/camera")
    Value (değer) → o path’e ait GStreamer pipeline nesnesi (GstElement* türünde)
    */
    GstElement *pipeline = it->second;
    /*o pipeline’ın içindeki input-selector elementine erişiyorsun.
    Yani “bu yayın pipeline’ında name=sel olan elemanı getir” diyorsun.*/
    GstElement *selector = gst_bin_get_by_name(GST_BIN(pipeline), "sel");

    if (!selector)
    {
        g_printerr("input-selector bulunamadı.\n");
        return;
    }

    /*input-selector’ın giriş pad’leri sink_0, sink_1, ... şeklindedir.
    sourceIndex kaçıncı kaynağa geçeceğini belirliyor.*/
    gchar *padname = g_strdup_printf("sink_%d", sourceIndex);
    /*selector = input-selector elemanının kendisi.
    Bu çağrı, input-selector üzerinde adı padname olan giriş pad’ini (ör. sink_1) döndürür*/
    GstPad *pad = gst_element_get_static_pad(selector, padname);

    if (pad)
    {
        /*input-selector bir GObject; üzerinde active-pad diye bir property var.
        Bu property’i istediğin pad ile set edince, o giriş “yayına verilen” giriş olur.
        Değişim anlıktır: bir sonraki buffer’dan itibaren yeni kaynak akmaya başlar.*/
        g_object_set(selector, "active-pad", pad, NULL);
        g_print("Kaynak değiştirildi: %s aktif\n", padname);
        gst_object_unref(pad);
    }
    else
    {
        g_printerr("Pad bulunamadı: %s\n", padname);
    }

    g_free(padname);
    gst_object_unref(selector);
}

void RTSPServer::pauseStream(const std::string &path)
{
    auto it = activePipelines.find(path);
    if (it == activePipelines.end())
    {
        g_printerr("Pipeline bulunamadı.\n");
        return;
    }

    GstElement *pipeline = it->second;
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    g_print("Yayın duraklatıldı: %s\n", path.c_str());
}

void RTSPServer::resumeStream(const std::string &path)
{
    auto it = activePipelines.find(path);
    if (it == activePipelines.end())
    {
        g_printerr("Pipeline bulunamadı.\n");
        return;
    }

    GstElement *pipeline = it->second;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Yayın devam ettirildi: %s\n", path.c_str());
}

void RTSPServer::run()
{
    gst_rtsp_server_attach(server, NULL);

    const gchar *port = gst_rtsp_server_get_service(server);
    for (const auto &path : paths)
    {
        g_print("  rtsp://127.0.0.1:%s%s\n", port, path.c_str());
    }
}