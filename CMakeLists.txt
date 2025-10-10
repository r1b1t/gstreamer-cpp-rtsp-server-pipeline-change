# -------------------------------------------------------------
#  Minimum CMake sürümü ve proje tanımı
# -------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)
project(LiveCamGStreamer)

# -------------------------------------------------------------
#  C++ standardı
# -------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
#  GStreamer paketlerini bul
# -------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-rtsp-server-1.0)

# -------------------------------------------------------------
#  Include dizinlerini ekle
# -------------------------------------------------------------
include_directories(
    ${GST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# -------------------------------------------------------------
#  Tüm src klasöründeki cpp dosyalarını bul
# -------------------------------------------------------------
file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# -------------------------------------------------------------
#  main.cpp hariç diğer cpp dosyalarını ayır
# -------------------------------------------------------------
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/main.cpp)

# -------------------------------------------------------------
#  Kütüphane (statik) oluştur
#  Diğer tüm .cpp dosyaları bu kütüphanede derlenir
# -------------------------------------------------------------
add_library(livecam_modules STATIC ${SRC_FILES})

# -------------------------------------------------------------
#  Uygulama: sadece main.cpp’den oluşur
# -------------------------------------------------------------
add_executable(livecamgstreamer main.cpp)

#  GStreamer ve kendi modül kütüphaneni linkle
target_link_libraries(livecamgstreamer
    livecam_modules          # kendi kodların (src/*.cpp)
    ${GST_LIBRARIES}         # GStreamer kütüphaneleri
)

# -------------------------------------------------------------
#  (İsteğe bağlı) .exe çıktısını bin klasörüne al
# -------------------------------------------------------------
set_target_properties(livecamgstreamer
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
