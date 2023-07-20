#include <gtk/gtk.h>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
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
    const char* inputUrl = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\sample-mp4-file-small.mp4";

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

#define GRADIENT_SPEED 2
#define ZOOM_FACTOR 2.0

double springAnimation(double target, double current, double velocity, double tension, double friction) {
    double springForce = -(current - target);
    double dampingForce = -velocity;
    double force = springForce * tension + dampingForce * friction;

    double acceleration = force;
    double newVelocity = velocity + acceleration;
    double displacement = newVelocity;

    return displacement;
}

static int transform_video (GtkWidget *widget, gpointer data)
{   
    g_print("Starting Transform...\n");

    // Initialize FFmpeg
    avformat_network_init();

    // *** decode video ***
    const char* filename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\sample-mp4-file-small.mp4";
    const char* outputFilename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\output.mp4";

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        g_print("Could not open file\n");
        return -1;
    }

    g_print("Finding Video Info...\n");

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        g_print("Could not find stream information\n");
        return -1;
    }

    g_print("Finding Video Stream... %d \n", pFormatCtx->nb_streams);

    int videoStream = -1;
    AVCodecParameters *codecpar = NULL;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        g_print("Stream %d type: %d\n", i, pFormatCtx->streams[i]->codecpar->codec_id);
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            g_print("Found video stream\n");
            videoStream = i;
            codecpar = pFormatCtx->streams[i]->codecpar;
            break;
        }
    }

    if (videoStream == -1) {
        g_print("Could not find video stream\n");
        return -1;
    }

    // AVCodecContext* pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    g_print("Allocate Context...\n");

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (!pCodecCtx) {
        g_print("Could not allocate video codec context\n");
    }

    // pCodecCtx->codec_id = codecpar->codec_id;
    // pCodecCtx->width = codecpar->width;
    // pCodecCtx->height = codecpar->height;

    if (avcodec_parameters_to_context(pCodecCtx, codecpar) < 0) {
        // Handle error
        g_print("Could not assign parameters to context\n");
    }

    g_print("Find Decoder... %d\n", pCodecCtx->codec_id);

    // const AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    // if (pCodec == NULL) {
    //     g_print("Unsupported codec\n");
    //     return -1;
    // }
    const AVCodec* pCodec = avcodec_find_decoder_by_name("h264");
    if (pCodec == NULL) {
        g_print("Unsupported codec\n");
        return -1;
    }

    g_print("Found Codec... %s\n", pCodec->name);

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        g_print("Could not open codec\n");
        return -1;
    }

    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        g_print("Could not allocate frame\n");
        return -1;
    }

    g_print("Prep SwS Context... %d x %d\n ", pCodecCtx->width, pCodecCtx->height);

    printf("Source pixel format: %s\n", av_get_pix_fmt_name(pCodecCtx->pix_fmt));
    printf("Destination pixel format: %s\n", av_get_pix_fmt_name(AV_PIX_FMT_RGB24));

    uint8_t* buffer;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

    g_print("Get SwS Context...\n");

    struct SwsContext* sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    // int frameFinished;
    // AVPacket packet;
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        // handle error, e.g. out of memory
        g_print("Could not allocate packet\n");
        return -1;
    }

    int y = 0;
    double zoom = 1.0;

    g_print("Starting read frames...\n");
    g_print("Stream Index: %d : %d\n", packet->stream_index, videoStream);

    if(packet->stream_index == videoStream) {
        g_print("Index match...\n");

        int response = avcodec_send_packet(pCodecCtx, packet);

        if(response < 0) {
            // logging error
            g_print("Error decoding frame\n");
            return -1;
        }

        g_print("Start frame loop...\n");

        while(response >= 0) {
            response = avcodec_receive_frame(pCodecCtx, pFrame);

            g_print("Frame Received: %d %d %d\n", response, AVERROR(EAGAIN), AVERROR_EOF);

            if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                // those two cases are OK, it means that there is no frame available for now
                break;
            } else if(response < 0) {
                // logging error
                break;
            }

            if(response >= 0) {
                // We have a decoded frame here
                // Do whatever you need to do with the frame.
                // You can also get frameFinished information from the response code
                bool frameFinished = (response >= 0);

                g_print("Frame Finished: %d\n", frameFinished);

                if (frameFinished) {
                    sws_scale(
                        sws_ctx,
                        (uint8_t const* const*)pFrame->data,
                        pFrame->linesize,
                        0,
                        pCodecCtx->height,
                        pFrameRGB->data,
                        pFrameRGB->linesize
                    );
                    
                    for (int y = 0; y < pCodecCtx->height; y++) {
                        for (int x = 0; x < pCodecCtx->width; x++) {
                            int gradient = (y + frameFinished * GRADIENT_SPEED) % 256;
                            pFrameRGB->data[0][y * pFrameRGB->linesize[0] + x] = gradient;
                            pFrameRGB->data[1][y * pFrameRGB->linesize[1] + x] = gradient;
                            pFrameRGB->data[2][y * pFrameRGB->linesize[2] + x] = gradient;
                        }
                    }

                    // Zoom animation
                    if (frameFinished % 100 == 0) {
                        zoom += springAnimation(ZOOM_FACTOR, zoom, 0.1, 0.1, 0.1);
                    }
                }
            }
        }
    }

    AVFrame* pFrameFinal = av_frame_alloc();
    if (!pFrameFinal) {
        g_print("Could not allocate video frame\n");
        return -1;
    }

    g_print("Starting Encode...\n");

    // *** encode video ***
    // Step 1: Create output context
    AVFormatContext *outFormatCtx = NULL;
    avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outputFilename);
    if (!outFormatCtx) {
        g_print("Could not create output context\n");
        return -1;
    }

    // Step 2: Setup the codec
    AVStream *outStream = avformat_new_stream(outFormatCtx, NULL);
    if (!outStream) {
        g_print("Failed to allocate output stream\n");
        return -1;
    }

    const AVCodec* outCodec = avcodec_find_encoder_by_name("libx264");
    if (!outCodec) {
        g_print("Could not find encoder\n");
        return -1;
    }

    AVCodecContext *codecCtx = avcodec_alloc_context3(outCodec);
    if (!codecCtx) {
        g_print("Could not create video codec context\n");
        return -1;
    }

    g_print("Setting up codec context...\n");
    g_print("Bit Rate: %d\n", pCodecCtx->bit_rate);
    // g_print("Width: %d\n", pFrameFinal->width);
    // g_print("Height: %d\n", pFrameFinal->height);

    codecCtx->bit_rate = pCodecCtx->bit_rate; 
    // codecCtx->width = pFrameFinal->width;
    // codecCtx->height = pFrameFinal->height;
    codecCtx->width = pCodecCtx->width;
    codecCtx->height = pCodecCtx->height;
    codecCtx->time_base = (AVRational){1, 25}; // Assume 25FPS for the example
    codecCtx->framerate = (AVRational){25, 1}; // Assume 25FPS for the example
    codecCtx->gop_size = 10;
    codecCtx->max_b_frames = 1;
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    outStream->time_base = codecCtx->time_base;

    // Step 3: Open the codec and add the stream info to the format context
    if (avcodec_open2(codecCtx, NULL, NULL) < 0) {
        g_print("Failed to open encoder for stream\n");
        return -1;
    }

    if (avcodec_parameters_from_context(outStream->codecpar, codecCtx) < 0) {
        g_print("Failed to copy encoder parameters to output stream\n");
        return -1;
    }

    // Step 4: Open output file
    g_print("Open output file...\n");
    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&outFormatCtx->pb, outputFilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            g_print("Could not open output file %s\n", outputFilename);
            return ret;
        }
    }

    // Step 5: Write file header
    if (avformat_write_header(outFormatCtx, NULL) < 0) {
        g_print("Error occurred when opening output file\n");
        return -1;
    }

    // Step 6: Process each frame (you might loop this part over your frames)
    // Assuming that pFrameFinal is the frame you want to write
    g_print("Write final frame (?)\n");
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    if (avcodec_send_frame(codecCtx, pFrameFinal) == 0) {
        int ret = avcodec_receive_packet(codecCtx, &pkt);
        if (ret < 0) {
            g_print("Error during encoding\n");
        } else {
            static int counter = 0;
            // Prepare packet for writing
            pkt.stream_index = outStream->index;
            av_packet_rescale_ts(&pkt, codecCtx->time_base, outStream->time_base);
            pkt.pos = -1;

            // Write packet to output file
            if (av_interleaved_write_frame(outFormatCtx, &pkt) < 0) {
                g_print("Error while writing video frame\n");
                return -1;
            }
            av_packet_unref(&pkt);
            counter++;
        }
    }

    // Step 7: Write file trailer and close the file
    g_print("Write trailer...\n");
    av_write_trailer(outFormatCtx);
    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFormatCtx->pb);
    }

    g_print("Clean Up\n");

    // Cleanup encode
    avcodec_free_context(&codecCtx);
    avformat_free_context(outFormatCtx);
    av_packet_free(&packet);

    // cleanup decode
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameFinal);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    // avcodec_free_context(&pCodecCtx); ?

    return 0;
}