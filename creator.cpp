#include <gtk/gtk.h>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
}

static void print_version_info(GtkWidget *widget, gpointer data) {
    const char* info1 = av_version_info();
    const int info2 = avformat_version();
    const char* info3 = avformat_configuration();
    g_print ("Hello Info\n");
    g_print(info1);
    g_print("%d", info2);
    g_print(info3);
}

static void print_file_info (GtkWidget *widget,
                         gpointer   data)
{
    // Set input url
    const char* inputUrl = "C:\\Users\\alext\\projects\\common\\TestCMinGW\\stubs\\project1\\sample-mp4-file-small.mp4";

    // Initialize FFmpeg
    avformat_network_init();

    // Open the file
    AVFormatContext* pFormatCtx = nullptr;
    if (avformat_open_input(&pFormatCtx, inputUrl, nullptr, nullptr) != 0) {
        g_print("Could not open file\n");
        return;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        g_print("Could not find stream information\n");
        return;
    }

    // Print some information about the file
    g_print("Format: %s\n", pFormatCtx->iformat->name);
    g_print("Duration: %ld seconds\n", pFormatCtx->duration / AV_TIME_BASE);
    g_print("Number of streams: %d\n", pFormatCtx->nb_streams);
    g_print("Number of programs: %d\n", pFormatCtx->nb_programs);
    g_print("Number of frames: %ld\n", pFormatCtx->streams[0]->nb_frames);

    // Close the file
    avformat_close_input(&pFormatCtx);

}