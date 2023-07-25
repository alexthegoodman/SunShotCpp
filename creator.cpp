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
    const char* filename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\basicCapture.mp4";
    const char* outputFilename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\output.mp4";
    int fpsInt = 15;

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

    g_print("Allocate Context...\n");

    AVCodecContext *decoderCodecCtx = avcodec_alloc_context3(NULL);
    if (!decoderCodecCtx) {
        g_print("Could not allocate video codec context\n");
    }

    if (avcodec_parameters_to_context(decoderCodecCtx, codecpar) < 0) {
        // Handle error
        g_print("Could not assign parameters to context\n");
    }

    g_print("Find Decoder... %d\n", decoderCodecCtx->codec_id);

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
    encoderCodecCtx->time_base = (AVRational){1, fpsInt}; // 30FPS
    encoderCodecCtx->framerate = (AVRational){fpsInt, 1}; // 30FPS
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
    // Initialize these at the start of the animation (i.e., when frameIndex == zoomStartFrame)
    double currentMultiplier = 1.0;
    double velocity = 0.0;

    // Set the tension and friction to control the behavior of the spring.
    double tension = 0.0001;
    double friction = 0.0002;

    int frameIndex = 0;

    while (1) {
        printf("Read Frame %d\n", frameIndex);
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

                // *** Frame Presentation Transformation *** //

                // Create a new AVFrame for the background.
                AVFrame* bg_frame = av_frame_alloc();
                bg_frame->format = AV_PIX_FMT_YUV420P; // Your desired format.
                bg_frame->width = encoderCodecCtx->width;
                bg_frame->height = encoderCodecCtx->height;
                // bg_frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                av_frame_get_buffer(bg_frame, 0);

                // Fill the frame with your desired color.
                // Note: The exact color values will depend on your pixel format.
                // For YUV420P, you might fill with (0, 128, 128) for black, for example.
                // or (130, 190, 45) for light blue
                for (int y = 0; y < bg_frame->height; ++y) {
                    for (int x = 0; x < bg_frame->width; ++x) {
                        // Fill Y plane.
                        bg_frame->data[0][y * bg_frame->linesize[0] + x] = 130; // Y
                        // Fill U and V planes.
                        if (y % 2 == 0 && x % 2 == 0) {
                            bg_frame->data[1][(y/2) * bg_frame->linesize[1] + (x/2)] = 190; // U
                            bg_frame->data[2][(y/2) * bg_frame->linesize[2] + (x/2)] = 45; // V
                        }
                    }
                }

                // Scale down the frame using libswscale.
                double scaleMultiple = 0.75;

                struct SwsContext* swsCtx = sws_getContext(
                    frame->width, frame->height, (enum AVPixelFormat)frame->format,
                    frame->width * scaleMultiple, frame->height * scaleMultiple, (enum AVPixelFormat)frame->format,
                    SWS_BILINEAR, NULL, NULL, NULL);

                AVFrame* scaled_frame = av_frame_alloc();
                scaled_frame->format = frame->format;
                scaled_frame->width = frame->width * scaleMultiple;
                scaled_frame->height = frame->height * scaleMultiple;
                av_frame_get_buffer(scaled_frame, 0);

                sws_scale(swsCtx, (const uint8_t* const*)frame->data, frame->linesize, 0, frame->height, scaled_frame->data, scaled_frame->linesize);
                sws_freeContext(swsCtx);

                // Insert your scaled frame into the background frame.
                int offsetX = (bg_frame->width - scaled_frame->width) / 2; // Center the video.
                int offsetY = (bg_frame->height - scaled_frame->height) / 2;
                for (int y = 0; y < scaled_frame->height; ++y) {
                    for (int x = 0; x < scaled_frame->width; ++x) {
                        // Copy Y plane.
                        bg_frame->data[0][(y+offsetY) * bg_frame->linesize[0] + (x+offsetX)] = scaled_frame->data[0][y * scaled_frame->linesize[0] + x];
                        // Copy U and V planes.
                        if (y % 2 == 0 && x % 2 == 0) {
                            bg_frame->data[1][((y+offsetY)/2) * bg_frame->linesize[1] + ((x+offsetX)/2)] = scaled_frame->data[1][(y/2) * scaled_frame->linesize[1] + (x/2)];
                            bg_frame->data[2][((y+offsetY)/2) * bg_frame->linesize[2] + ((x+offsetX)/2)] = scaled_frame->data[2][(y/2) * scaled_frame->linesize[2] + (x/2)];
                        }
                    }
                }

                // Determine the portion of the background to zoom in on.
                // Start with the entire frame and gradually decrease these dimensions.
                // Use your springAnimation function to animate this process.
                // You will need to determine appropriate values for these variables.
                static double targetWidth = bg_frame->width;
                static double targetHeight = bg_frame->height;

                int zoomStartFrame = 0;
                int zoomEndFrame = 60; // 30 frames = 1 second at 30 fps

                if (frameIndex >= zoomStartFrame && frameIndex <= zoomEndFrame) {
                    double targetMultiplier = 0.5; // the zoom level we want to end up at
                    double displacement = springAnimation(targetMultiplier, currentMultiplier, velocity, tension, friction);
                    
                    // Update 'current' variables for the next frame
                    velocity += displacement;
                    currentMultiplier += velocity;

                    // make sure the multiplier is within the desired range (?)
                    currentMultiplier = currentMultiplier > 1.0 ? 1.0 : currentMultiplier < targetMultiplier ? targetMultiplier : currentMultiplier;

                    g_print("currentMultiplier %f\n", currentMultiplier);

                    targetWidth = bg_frame->width * currentMultiplier;
                    targetHeight = bg_frame->height * currentMultiplier;
                } else if (frameIndex > zoomEndFrame) {
                    // The zoom effect has ended.
                    // Keep the target dimensions at the desired zoom level.
                    targetWidth = bg_frame->width;
                    targetHeight = bg_frame->height;
                }

                static double currentWidth = bg_frame->width;
                static double currentHeight = bg_frame->height;
                static double velocityWidth = 0;
                static double velocityHeight = 0;

                // Intentionally different than values for above springAnimation
                double tension = 0.8;
                double friction = 0.9;

                double displacementWidth = springAnimation(targetWidth, currentWidth, velocityWidth, tension, friction);
                double displacementHeight = springAnimation(targetHeight, currentHeight, velocityHeight, tension, friction);
                currentWidth += displacementWidth;
                currentHeight += displacementHeight;
                velocityWidth += displacementWidth;
                velocityHeight += displacementHeight;

                // Make sure the dimensions are integers and within the frame size.
                int zoomWidth = round(currentWidth);
                zoomWidth = zoomWidth > bg_frame->width ? bg_frame->width : zoomWidth < 1 ? 1 : zoomWidth;
                int zoomHeight = round(currentHeight);
                zoomHeight = zoomHeight > bg_frame->height ? bg_frame->height : zoomHeight < 1 ? 1 : zoomHeight;

                // Calculate the top left position of the zoomed portion.
                // Center the zoomed portion in the frame.
                // int zoomTop = (bg_frame->height - zoomHeight) / 2;
                // int zoomLeft = (bg_frame->width - zoomWidth) / 2;

                int mouseX = bg_frame->width / 2, mouseY = 500;

                // Calculate the top left position of the zoom frame,
                // making sure it's centered on the mouse position and not going out of bounds.
                int zoomLeft = std::max(0, std::min(static_cast<int>(std::round(mouseX - currentWidth / 2)), static_cast<int>(bg_frame->width - currentWidth)));
                int zoomTop = std::max(0, std::min(static_cast<int>(std::round(mouseY - currentHeight / 2)), static_cast<int>(bg_frame->height - currentHeight)));

                // assure even numbers
                zoomTop = (zoomTop / 2) * 2;
                zoomLeft = (zoomLeft / 2) * 2;

                // Create a new AVFrame for the zoomed portion.
                AVFrame* zoom_frame = av_frame_alloc();
                zoom_frame->format = bg_frame->format;
                zoom_frame->width = bg_frame->width; // Keep the output frame size the same.
                zoom_frame->height = bg_frame->height;
                zoom_frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                av_frame_get_buffer(zoom_frame, 0);

                // g_print("Zoom Frame: %d x %d\n", zoomWidth, zoomHeight);
                // g_print("Diagnostic Info: %d x %d\n", zoom_frame->width, zoom_frame->height);
                // g_print("More Info: %d, %d\n", zoomTop, zoomLeft);

                // Scale up the zoomed portion to the output frame size using libswscale.
                struct SwsContext* swsCtxZoom = sws_getContext(
                    zoomWidth, zoomHeight, (enum AVPixelFormat)bg_frame->format,
                    zoom_frame->width, zoom_frame->height, (enum AVPixelFormat)zoom_frame->format,
                    SWS_BILINEAR, NULL, NULL, NULL);

                // Get pointers to the zoomed portion in the background frame.
                uint8_t* zoomData[3];
                zoomData[0] = &bg_frame->data[0][zoomTop * bg_frame->linesize[0] + zoomLeft];
                if (zoomTop % 2 == 0 && zoomLeft % 2 == 0) {
                    zoomData[1] = &bg_frame->data[1][(zoomTop/2) * bg_frame->linesize[1] + (zoomLeft/2)];
                    zoomData[2] = &bg_frame->data[2][(zoomTop/2) * bg_frame->linesize[2] + (zoomLeft/2)];
                } else {
                    zoomData[1] = NULL;
                    zoomData[2] = NULL;
                }

                sws_scale(swsCtxZoom, (const uint8_t* const*)zoomData, bg_frame->linesize, 0, zoomHeight, zoom_frame->data, zoom_frame->linesize);

                // check zoomData
                // for (int i = 0; i < 3; i++) {
                //     if (zoomData[i] == nullptr) {
                //         g_print("zoomData[%d] is null\n", i);
                //     }
                // }

                sws_freeContext(swsCtxZoom);

                av_frame_free(&bg_frame); // We don't need the original background frame anymore.
                av_frame_free(&frame); // We don't need the original frame anymore.
                av_frame_free(&scaled_frame); // We don't need the scaled frame anymore.

                if (avcodec_send_frame(encoderCodecCtx, zoom_frame) < 0) {
                    fprintf(stderr, "Error sending frame for encoding\n");
                    av_frame_free(&zoom_frame);
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
                        av_frame_free(&bg_frame);
                        av_packet_free(&inputPacket);
                        av_packet_free(&outputPacket);
                        return -1;
                    }
                    av_packet_free(&outputPacket);
                }
                av_frame_free(&bg_frame);
            }
        }
        av_packet_free(&inputPacket);
        frameIndex++;
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
    // av_free(buffer);
    // av_frame_free(&pFrameRGB);
    // av_frame_free(&pFrame);
    // av_frame_free(&pFrameFinal);
    avcodec_close(decoderCodecCtx);
    avformat_close_input(&decoderFormatCtx);
    // avcodec_free_context(&decoderCodecCtx); ?

    return 0;
}