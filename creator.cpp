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
    AVFormatContext* decoderFormatCtx = nullptr;
    if (avformat_open_input(&decoderFormatCtx, inputUrl, nullptr, nullptr) != 0) {
        g_print("Could not open file\n");
        return;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(decoderFormatCtx, nullptr) < 0) {
        g_print("Could not find stream information\n");
        return;
    }

    // Print some information about the file
    g_print("Format: %s\n", decoderFormatCtx->iformat->name);
    g_print("Duration: %ld seconds\n", decoderFormatCtx->duration / AV_TIME_BASE);
    g_print("Number of streams: %d\n", decoderFormatCtx->nb_streams);
    g_print("Number of programs: %d\n", decoderFormatCtx->nb_programs);
    g_print("Number of frames: %ld\n", decoderFormatCtx->streams[0]->nb_frames);

    // Close the file
    avformat_close_input(&decoderFormatCtx);

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
    const char* filename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\originalCapture.mp4";
    const char* outputFilename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\output.mp4";

    AVFormatContext* decoderFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&decoderFormatCtx, filename, NULL, NULL) != 0) {
        g_print("Could not open file\n");
        return -1;
    }

    g_print("Finding Video Info...\n");

    if (avformat_find_stream_info(decoderFormatCtx, NULL) < 0) {
        g_print("Could not find stream information\n");
        return -1;
    }

    g_print("Finding Video Stream... Num Streams: %d Num Frames: %d \n", decoderFormatCtx->nb_streams, decoderFormatCtx->streams[0]->nb_frames);

    int videoStream = -1;
    AVCodecParameters *codecpar = NULL;
    for (int i = 0; i < decoderFormatCtx->nb_streams; i++) {
        g_print("Stream %d type: %d\n", i, decoderFormatCtx->streams[i]->codecpar->codec_id);
        if (decoderFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            g_print("Found video stream\n");
            videoStream = i;
            codecpar = decoderFormatCtx->streams[i]->codecpar;
            break;
        }
    }

    if (videoStream == -1) {
        g_print("Could not find video stream\n");
        return -1;
    }

    // AVCodecContext* decoderCodecCtx = decoderFormatCtx->streams[videoStream]->codec;

    g_print("Allocate Context...\n");

    AVCodecContext *decoderCodecCtx = avcodec_alloc_context3(NULL);
    if (!decoderCodecCtx) {
        g_print("Could not allocate video codec context\n");
    }

    // decoderCodecCtx->codec_id = codecpar->codec_id;
    // decoderCodecCtx->width = codecpar->width;
    // decoderCodecCtx->height = codecpar->height;

    if (avcodec_parameters_to_context(decoderCodecCtx, codecpar) < 0) {
        // Handle error
        g_print("Could not assign parameters to context\n");
    }

    g_print("Find Decoder... %d\n", decoderCodecCtx->codec_id);

    // const AVCodec* decoderCodec = avcodec_find_decoder(decoderCodecCtx->codec_id);
    // if (decoderCodec == NULL) {
    //     g_print("Unsupported codec\n");
    //     return -1;
    // }
    const AVCodec* decoderCodec = avcodec_find_decoder_by_name("h264");
    if (decoderCodec == NULL) {
        g_print("Unsupported codec\n");
        return -1;
    }

    g_print("Found Codec... %s\n", decoderCodec->name);

    if (avcodec_open2(decoderCodecCtx, decoderCodec, NULL) < 0) {
        g_print("Could not open codec\n");
        return -1;
    }

    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        g_print("Could not allocate frame\n");
        return -1;
    }

    g_print("Prep SwS Context... %d x %d\n ", decoderCodecCtx->width, decoderCodecCtx->height);

    printf("Source pixel format: %s\n", av_get_pix_fmt_name(decoderCodecCtx->pix_fmt));
    printf("Destination pixel format: %s\n", av_get_pix_fmt_name(AV_PIX_FMT_RGB24));

    uint8_t* buffer;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, decoderCodecCtx->width, decoderCodecCtx->height, 1);
    buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, decoderCodecCtx->width, decoderCodecCtx->height, 1);

    g_print("Get SwS Context...\n");

    struct SwsContext* sws_ctx = sws_getContext(
        decoderCodecCtx->width,
        decoderCodecCtx->height,
        decoderCodecCtx->pix_fmt,
        decoderCodecCtx->width,
        decoderCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    // FILE *outfile = fopen(outputFilename, "wb");
    // if (!outfile) {
    //     fprintf(stderr, "Could not open output file\n");
    //     return -1;
    // }

    // *** prep enncoding ***
    // Create output context
    AVFormatContext *encoderFormatCtx = NULL;
    avformat_alloc_output_context2(&encoderFormatCtx, NULL, NULL, outputFilename);
    if (!encoderFormatCtx) {
        g_print("Could not create output context\n");
        return -1;
    }

    // Setup the codec
    AVStream *encoderStream = avformat_new_stream(encoderFormatCtx, NULL);
    if (!encoderStream) {
        g_print("Failed to allocate output stream\n");
        return -1;
    }

    const AVCodec* encoderCodec = avcodec_find_encoder_by_name("libx264");
    if (!encoderCodec) {
        g_print("Could not find encoder\n");
        return -1;
    }

    AVCodecContext *encoderCodecCtx = avcodec_alloc_context3(encoderCodec);
    if (!encoderCodecCtx) {
        g_print("Could not create video codec context\n");
        return -1;
    }

    g_print("Setting up codec context...\n");
    g_print("Bit Rate: %d\n", decoderCodecCtx->bit_rate);

    encoderCodecCtx->bit_rate = decoderCodecCtx->bit_rate; 
    encoderCodecCtx->width = decoderCodecCtx->width;
    encoderCodecCtx->height = decoderCodecCtx->height;
    encoderCodecCtx->time_base = (AVRational){1, 30}; // 30FPS
    encoderCodecCtx->framerate = (AVRational){30, 1}; // 30FPS
    encoderCodecCtx->gop_size = 10;
    encoderCodecCtx->max_b_frames = 1;
    encoderCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    encoderStream->time_base = encoderCodecCtx->time_base;

    // Step 3: Open the codec and add the stream info to the format context
    if (avcodec_open2(encoderCodecCtx, NULL, NULL) < 0) {
        g_print("Failed to open encoder for stream\n");
        return -1;
    }

    if (avcodec_parameters_from_context(encoderStream->codecpar, encoderCodecCtx) < 0) {
        g_print("Failed to copy encoder parameters to output stream\n");
        return -1;
    }

    // Open output file
    g_print("Open output file...\n");
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&encoderFormatCtx->pb, outputFilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            g_print("Could not open output file %s\n", outputFilename);
            return ret;
        }
    }

    // Write file header
    if (avformat_write_header(encoderFormatCtx, NULL) < 0) {
        g_print("Error occurred when opening output file\n");
        return -1;
    }

    // int frameFinished;
    // AVPacket packet;
    // AVPacket* packet = av_packet_alloc();
    // if (!packet) {
    //     // handle error, e.g. out of memory
    //     g_print("Could not allocate packet\n");
    //     return -1;
    // }

    // AVPacket* packet2 = av_packet_alloc();
    // if (!packet2) {
    //     // handle error, e.g. out of memory
    //     g_print("Could not allocate packet2\n");
    //     return -1;
    // }

    int y = 0;
    double zoom = 1.0;

    g_print("Starting read frames...\n");
    // g_print("Stream Index: %d : %d\n", packet->stream_index, videoStream);

    while (1) {
        printf("Read Frame\n");
        AVPacket* inputPacket = av_packet_alloc();
        if (av_read_frame(decoderFormatCtx, inputPacket) < 0) {
            // Break the loop if we've read all packets
            av_packet_free(&inputPacket);
            break;
        }
        if (inputPacket->stream_index == videoStream) {
            // printf("Send Packet\n");
            if (avcodec_send_packet(decoderCodecCtx, inputPacket) < 0) {
                fprintf(stderr, "Error sending packet for decoding\n");
                av_packet_free(&inputPacket);
                return -1;
            }
            while (1) {
                // printf("Receive Frame\n");
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(decoderCodecCtx, frame) < 0) {
                    // Break the loop if no more frames
                    av_frame_free(&frame);
                    break;
                }
                if (avcodec_send_frame(encoderCodecCtx, frame) < 0) {
                    fprintf(stderr, "Error sending frame for encoding\n");
                    av_frame_free(&frame);
                    av_packet_free(&inputPacket);
                    return -1;
                }
                while (1) {
                    // printf("Receive Packet\n");
                    AVPacket* outputPacket = av_packet_alloc();
                    if (avcodec_receive_packet(encoderCodecCtx, outputPacket) < 0) {
                        // Break the loop if no more packets
                        av_packet_free(&outputPacket);
                        break;
                    }
                    // printf("Write Packet\n");
                    if (av_write_frame(encoderFormatCtx, outputPacket) < 0) {
                        fprintf(stderr, "Error writing packet\n");
                        av_frame_free(&frame);
                        av_packet_free(&inputPacket);
                        av_packet_free(&outputPacket);
                        return -1;
                    }
                    av_packet_free(&outputPacket);
                }
                av_frame_free(&frame);
            }
        }
        av_packet_free(&inputPacket);
    }

    // // flush the decoder
    // avcodec_send_packet(decoderCodecCtx, NULL);
    // while (avcodec_receive_frame(decoderCodecCtx, pFrame) == 0) {
    //     avcodec_send_frame(encoderCodecCtx, pFrame);
    //     av_frame_unref(pFrame);
    // }

    // // flush the encoder
    // avcodec_send_frame(encoderCodecCtx, NULL);
    // while (avcodec_receive_packet(encoderCodecCtx, packet) == 0) {
    //     av_interleaved_write_frame(encoderFormatCtx, packet);
    //     av_packet_unref(packet);
    // }

    // TODO: write final frame (unnecessary?)

    // AVFrame* pFrameFinal = av_frame_alloc();
    // if (!pFrameFinal) {
    //     g_print("Could not allocate video frame\n");
    //     return -1;
    // }

    // Assuming that pFrameFinal is the frame you want to write
    // g_print("Write final frame (?)\n");
    // AVPacket pkt = {0};
    // av_init_packet(&pkt);

    // if (avcodec_send_frame(encoderCodecCtx, pFrameFinal) == 0) {
    //     int ret = avcodec_receive_packet(encoderCodecCtx, &pkt);
    //     if (ret < 0) {
    //         g_print("Error during encoding\n");
    //     } else {
    //         static int counter = 0;
    //         // Prepare packet for writing
    //         pkt.stream_index = encoderStream->index;
    //         av_packet_rescale_ts(&pkt, encoderCodecCtx->time_base, encoderStream->time_base);
    //         pkt.pos = -1;

    //         // Write packet to output file
    //         if (av_interleaved_write_frame(encoderFormatCtx, &pkt) < 0) {
    //             g_print("Error while writing video frame\n");
    //             return -1;
    //         }
    //         av_packet_unref(&pkt);
    //         counter++;
    //     }
    // }

    // Write file trailer and close the file
    g_print("Write trailer...\n");
    av_write_trailer(encoderFormatCtx);
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&encoderFormatCtx->pb);
    }

    g_print("Clean Up\n");

    // fclose(outfile);

    // Cleanup encode
    avcodec_free_context(&encoderCodecCtx);
    avformat_free_context(encoderFormatCtx);
    // av_packet_free(&packet);

    // cleanup decode
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    // av_frame_free(&pFrameFinal);
    avcodec_close(decoderCodecCtx);
    avformat_close_input(&decoderFormatCtx);
    // avcodec_free_context(&decoderCodecCtx); ?

    return 0;
}