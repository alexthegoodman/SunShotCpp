#include <gtk/gtk.h>
#include <nlohmann/json.hpp>

#include <fstream>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}

using json = nlohmann::json;

struct ZoomInfo {
    int startTimestamp;
    int endTimestamp;
    double zoom;
    // animationTension
    // animationFriction
    // animationDuration
};

std::vector<ZoomInfo> mockZoomInfo = {
    {1000, 6000, 0.5},
    {9000, 14000, 0.6}
};

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

double springAnimation(double target, double current, double velocity, double tension, double friction, double direction = 0) {
    // double direction = target - current; // negative if going down, positive if going up
    double springForce = -(current - target);
    double dampingForce = -velocity;
    double force = (springForce * tension) + (dampingForce * friction);

    // printf("Forces: %f %f %f %f %f\n", force, springForce, tension, friction, dampingForce);

    double acceleration = force;
    double newVelocity = velocity + acceleration;
    double displacement = newVelocity;

    // printf("Spring Animation: %f %f %f %f %f\n", acceleration, newVelocity, displacement, current, target);

    if (direction < 0) {
        if (current < target) {
            return 0;
        }
    } else if (direction > 0) {
        if (current > target) {
            return 0;
        }
    }

    return displacement;
}

// based on air friction physics
double frictionalAnimation(double target, double current, double velocity, double friction) {
    double direction = (target - current);
    double newVelocity = direction * std::exp(-friction);
    return newVelocity;
}

static int transform_video (GtkWidget *widget, gpointer data)
{   
    g_print("Opening Mouse Events...\n");

    std::ifstream mouseEventsFile("stubs/project1/mouseEvents.json");
    
    if (!mouseEventsFile.is_open()) {
        g_print("Could not open mouse events file\n");
        return -1;
    }

    json mouseEventsJson;
    mouseEventsFile >> mouseEventsJson;
    
    if (mouseEventsJson.find("mouseEvents") == mouseEventsJson.end()) {
        g_print("Could not find mouse events\n");
        return -1;
    }

    mouseEventsJson = mouseEventsJson["mouseEvents"];

    g_print("Opening Window Data...\n");

    std::ifstream windowDataFile("stubs/project1/windowData.json");
    json windowDataJson;
    windowDataFile >> windowDataJson;

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

    int y = 0;
    double zoom = 1.0;

    g_print("Starting read frames...\n");
    // g_print("Stream Index: %d : %d\n", packet->stream_index, videoStream);
    // Initialize these at the start of the animation (i.e., when frameIndex == zoomStartFrame)
    double currentMultiplier = 1.0;
    double velocity = 0.0;

    int frameIndex = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    double mouseX = 0, mouseY = 0;
    double currentMouseX = decoderCodecCtx->width / 2, currentMouseY = decoderCodecCtx->height / 2;
    double velocityMouseX = 0, velocityMouseY = 0;
    int zoomTop = 0, zoomLeft = 0;
    bool zoomingIn = false;
    bool zoomingIn2 = false;
    static double velocityWidth = 0; // move outside loop and into reset?
    static double velocityHeight = 0;
    bool autoZoom = false;

    // Set the tension and friction to control the behavior of the spring.
    // double tension1 = 0.0005;
    // double friction1 = 0.001;
    double friction1 = 2.5;
    // Intentionally different than values for above springAnimation
    double tension2 = 0.001;
    // double friction2 = 0.2; // for spring
    double friction2 = 2.5; // for frictional
    double tension3 = 0.01;
    // double friction3 = 0.02; // for spring
    double friction3 = 2.5; // for frictional

    double directionX = 0;
    double directionY = 0;

    double smoothZoomTop = 0;
    double smoothZoomLeft = 0;

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

                //  *** Background / Gradient *** //

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

                // *** Inset Video *** //

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

                // *** Zoom *** //
                // auto currentTime = std::chrono::high_resolution_clock::now();
                // auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
                int timeElapsed = frameIndex * 1000 / fpsInt;

                g_print("Time Elapsed: %d\n", timeElapsed);

                // Determine the portion of the background to zoom in on.
                // Start with the entire frame and gradually decrease these dimensions.
                // Use your springAnimation function to animate this process.
                // You will need to determine appropriate values for these variables.
                static double targetWidth = bg_frame->width;
                static double targetHeight = bg_frame->height;

                // Search for the current zoom level
                
                double targetMultiplier = 1.0; // Default value when no zoom effect is active
                for (auto zoomInfoIt = mockZoomInfo.begin(); zoomInfoIt != mockZoomInfo.end(); ++zoomInfoIt) {
                    auto& zoomInfo = *zoomInfoIt;
                    if (zoomInfoIt != mockZoomInfo.end()) {
                        auto &zoomInfo = *zoomInfoIt;
                        if (timeElapsed >= zoomInfo.startTimestamp && timeElapsed < zoomInfo.endTimestamp) {
                        // if (timeElapsed >= zoomInfo.startTimestamp && timeElapsed < zoomInfo.startTimestamp + 1000) {
                            if (zoomingIn == false) {
                                velocity = 0;
                                velocityMouseX = 0;
                                velocityMouseY = 0;
                                velocityWidth = 0;
                                velocityHeight = 0;
                                // tension2 = 0.4; // really small values work better for zoom out
                                // friction2 = 0.8;
                                // currentMouseX = mouseX; // setting to mouseX causes a jump?
                                // currentMouseY = mouseY;
                                zoomingIn = true;
                                g_print("Zooming In\n");
                            }
                            // Rest of the zooming in code...
                            targetMultiplier = zoomInfo.zoom;
                            // break;
                        } else if (timeElapsed >= zoomInfo.endTimestamp && timeElapsed < zoomInfo.endTimestamp + 1000) {
                            if (zoomingIn == true) {
                                velocity = 0;
                                velocityMouseX = 0;
                                velocityMouseY = 0;
                                velocityWidth = 0;
                                velocityHeight = 0;
                                // tension2 = 0.004; // really small values work better for zoom out
                                // friction2 = 0.008;
                                // currentMouseX = mouseX;
                                // currentMouseY = mouseY;
                                zoomingIn = false;
                                g_print("Zooming Out\n");
                            }
                            // Rest of the zooming out code...
                            targetMultiplier = 1.0;
                            // break;
                        }
                        // else {
                        //     zoomingIn = false;
                        //     g_print("Reset Zoom\n");
                        //     // break;
                        // }
                    }
                    //  else {
                    //     zoomingIn = false;
                    // }
                }
                
                // this way targetWidth and targetHeight do no change on each frame
                currentMultiplier = targetMultiplier; 

                // (ex. 1.0 is 100% while 0.8 is ~120%)
                g_print("currentMultiplier %f\n", currentMultiplier);

                targetWidth = bg_frame->width * currentMultiplier;
                targetHeight = bg_frame->height * currentMultiplier;

                static double currentWidth = bg_frame->width;
                static double currentHeight = bg_frame->height;

                // double displacementWidth = springAnimation(targetWidth, currentWidth, velocityWidth, tension2, friction2);
                // double displacementHeight = springAnimation(targetHeight, currentHeight, velocityHeight, tension2, friction2);

                double displacementWidth = frictionalAnimation(targetWidth, currentWidth, velocityWidth, friction2);
                double displacementHeight = frictionalAnimation(targetHeight, currentHeight, velocityHeight, friction2);

                currentWidth += displacementWidth;
                currentHeight += displacementHeight;
                velocityWidth += displacementWidth;
                velocityHeight += displacementHeight;

                // slow down velocity
                // velocityWidth *= 0.1;
                // velocityHeight *= 0.1;

                // clamp (?)
                // currentWidth = currentWidth > 10000 ? 10000 : currentWidth < 1 ? 2 : currentWidth;
                // currentHeight = currentHeight > 10000 ? 10000 : currentHeight < 1 ? 2 : currentHeight;

                g_print("zoomingIn %d\n", zoomingIn);
                if (zoomingIn) {
                    // when zooming in, the targetWidth should be LESS than the currentWidth 
                    // want to prevent currentWidth from being less than targetWidth
                    currentWidth = currentWidth < targetWidth ? targetWidth : currentWidth;
                    currentHeight = currentHeight < targetHeight ? targetHeight : currentHeight;
                } else {
                    currentWidth = currentWidth > targetWidth ? targetWidth : currentWidth;
                    currentHeight = currentHeight > targetHeight ? targetHeight : currentHeight;
                }

                g_print("Dimensions: %f %f %f %f %f %f\n", targetWidth, targetHeight, currentWidth, currentHeight, displacementWidth, displacementHeight);

                velocityWidth = velocityWidth > 10000 ? 10000 : velocityWidth < -10000 ? -10000 : velocityWidth;
                velocityHeight = velocityHeight > 10000 ? 10000 : velocityHeight < -10000 ? -10000 : velocityHeight;

                // Make sure the dimensions are integers and within the frame size.
                int zoomWidth = round(currentWidth);
                zoomWidth = zoomWidth > bg_frame->width ? bg_frame->width : zoomWidth < 1 ? 1 : zoomWidth;
                int zoomHeight = round(currentHeight);
                zoomHeight = zoomHeight > bg_frame->height ? bg_frame->height : zoomHeight < 1 ? 1 : zoomHeight;

                // Calculate the top left position of the zoomed portion.
                // Center the zoomed portion in the frame.
                // int zoomTop = (bg_frame->height - zoomHeight) / 2;
                // int zoomLeft = (bg_frame->width - zoomWidth) / 2;

                // Calculate the top left position of the zoom frame,
                // making sure it's centered on the mouse position and not going out of bounds.
                // int zoomLeft = std::max(0, std::min(static_cast<int>(std::round(mouseX - currentWidth / 2)), static_cast<int>(bg_frame->width - currentWidth)));
                // int zoomTop = std::max(0, std::min(static_cast<int>(std::round(mouseY - currentHeight / 2)), static_cast<int>(bg_frame->height - currentHeight)));

                // animate the mouse positions as well
                if (autoZoom) {
                    for (size_t i = 0; i < mouseEventsJson.size(); ++i) {
                        if (mouseEventsJson[i]["timeStamp"] >= timeElapsed) {
                            // TODO: calculate based on windowDataJson (and surrounding frame?)
                            // smoother mouse movements based on distance of 100px or more
                            int deltaX = abs(mouseX - mouseEventsJson[i]["x"].get<double>());
                            int deltaY = abs(mouseY - mouseEventsJson[i]["y"].get<double>());

                            // printf("Mouse Delta: %f, %d, %d\n", mouseEventsJson[i]["x"].get<double>(), deltaX, deltaY);

                            if (deltaX < 100 && deltaY < 100) {
                                break;
                            }

                            mouseX = mouseEventsJson[i]["x"].get<double>();
                            mouseY = mouseEventsJson[i]["y"].get<double>();

                            // add windowOffset
                            mouseX += windowDataJson["windowX"].get<double>();
                            mouseY += windowDataJson["windowY"].get<double>();

                            // scale mouse positions
                            mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                            mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                            directionX = mouseX - currentMouseX;
                            directionY = mouseY - currentMouseY;
                            break;
                        }
                    }
                } else {
                    for (auto zoomInfoIt = mockZoomInfo.begin(); zoomInfoIt != mockZoomInfo.end(); ++zoomInfoIt) {
                        auto& zoomInfo = *zoomInfoIt;
                        if (zoomInfoIt != mockZoomInfo.end()) {
                            auto &zoomInfo = *zoomInfoIt;
                            if (timeElapsed >= zoomInfo.startTimestamp && timeElapsed < zoomInfo.endTimestamp) {
                            // if (timeElapsed >= zoomInfo.startTimestamp && timeElapsed < zoomInfo.startTimestamp + 1000) {
                                if (zoomingIn2 == false) {
                                    // set mouse coods to the first mouse event after the start timestamp
                                    for (size_t i = 0; i < mouseEventsJson.size(); ++i) {
                                        if (mouseEventsJson[i]["timeStamp"] >= timeElapsed) {
                                            mouseX = mouseEventsJson[i]["x"].get<double>();
                                            mouseY = mouseEventsJson[i]["y"].get<double>();
                                            break;
                                        }
                                    }
                                    zoomingIn2 = true;

                                    // add windowOffset
                                    mouseX += windowDataJson["windowX"].get<double>();
                                    mouseY += windowDataJson["windowY"].get<double>();

                                    // // scale mouse positions
                                    // mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                                    // mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                                    if (!autoZoom) {
                                        currentMouseX = mouseX;
                                        currentMouseY = mouseY;
                                    }

                                    directionX = mouseX - currentMouseX;
                                    directionY = mouseY - currentMouseY;
                                    g_print("Zooming In 2\n");
                                    // break;
                                }
                            } else if (timeElapsed >= zoomInfo.endTimestamp && timeElapsed < zoomInfo.endTimestamp + 1000) {
                                if (zoomingIn2 == true) {
                                    zoomingIn2 = false;

                                    // TODO: make DRY
                                    // add windowOffset
                                    mouseX += windowDataJson["windowX"].get<double>();
                                    mouseY += windowDataJson["windowY"].get<double>();

                                    // // scale mouse positions
                                    // mouseX = mouseX * scaleMultiple + frame->width * 0.1;
                                    // mouseY = mouseY * scaleMultiple + frame->height * 0.1;

                                    directionX = mouseX - currentMouseX;
                                    directionY = mouseY - currentMouseY;
                                    g_print("Zooming Out 2\n");
                                    // break;
                                }
                            }
                            // else {
                            //     zoomingIn2 = false;
                            //     g_print("Reset Zoom 2\n");
                            //     break;
                            // }
                        }
                        //  else {
                        //     zoomingIn = false;
                        // }
                    }
                }
                
                printf("Mouse Positions: %f, %f and %f, %f\n", mouseX, mouseY, currentMouseX, currentMouseY);

                if (autoZoom) {
                    if (mouseX != currentMouseX || mouseY != currentMouseY) {
                        // autoZoom requires animation between mouse positions
                        // printf("Spring Args %f %f %f %f %f\n", mouseX, currentMouseX, velocityMouseX, tension, friction);
                        
                        // double displacementMouseX = springAnimation(mouseX, currentMouseX, velocityMouseX, tension3, friction3, directionX);
                        // double displacementMouseY = springAnimation(mouseY, currentMouseY, velocityMouseY, tension3, friction3, directionY);

                        double displacementMouseX = frictionalAnimation(mouseX, currentMouseX, velocityMouseX, friction3);
                        double displacementMouseY = frictionalAnimation(mouseY, currentMouseY, velocityMouseY, friction3);

                        printf("Displacement Position: %f, %f\n", displacementMouseX, displacementMouseY);

                        // slow down velocity
                        // displacementMouseX *= 0.1;
                        // displacementMouseY *= 0.1;

                        // Update 'current' and 'velocity' variables for the next frame
                        velocityMouseX += displacementMouseX;
                        currentMouseX += displacementMouseX; // or += displacementMouseX?
                        velocityMouseY += displacementMouseY;
                        currentMouseY += displacementMouseY;
                    }
                }

                // clamp max
                double frameWidth = (double) frame->width;
                double frameHeight = (double) frame->height;
                
                // alterative clamp
                currentMouseX = currentMouseX > frameWidth ? frameWidth : currentMouseX < 0 ? 0 : currentMouseX; // or with targetWidth?
                currentMouseY = currentMouseY > frameHeight ? frameHeight : currentMouseY < 0 ? 0 : currentMouseY;

                velocityMouseX = velocityMouseX > frameWidth ? frameWidth : velocityMouseX < -(mouseX) ? -(mouseX) : velocityMouseX;
                velocityMouseY = velocityMouseY > frameHeight ? frameHeight : velocityMouseY < -(mouseY) ? -(mouseY) : velocityMouseY;

                printf("Spring Position: %f, %f\n", currentMouseX, currentMouseY);

                // zoomLeft and zoomTop are position of zooming viewport overtop background
                // then size of zoomed region are with zoomWidth and zoomHeight

                // Center the zoom on the current mouse position
                zoomTop = std::max(0, static_cast<int>(std::min(currentMouseY - currentHeight / 2, static_cast<double>(bg_frame->height) - currentHeight)));
                zoomLeft = std::max(0, static_cast<int>(std::min(currentMouseX - currentWidth / 2, static_cast<double>(bg_frame->width) - currentWidth)));

                // zoomTop = std::max(0, static_cast<int>(std::min(currentMouseY - targetHeight / 2, static_cast<double>(bg_frame->height) - targetHeight)));
                // zoomLeft = std::max(0, static_cast<int>(std::min(currentMouseX - targetWidth / 2, static_cast<double>(bg_frame->width) - targetWidth)));

                // alternative approach
                // double centerY = static_cast<double>(bg_frame->height) / 2.0;
                // double centerX = static_cast<double>(bg_frame->width) / 2.0;
                // zoomTop = std::max(0, static_cast<int>(std::min(currentMouseY - centerY, centerY - targetHeight / 2)));
                // zoomLeft = std::max(0, static_cast<int>(std::min(currentMouseX - centerX, centerX - targetWidth / 2)));

                // round to nearest 10 number
                zoomTop = std::round(zoomTop / 10.0) * 10;
                zoomLeft = std::round(zoomLeft / 10.0) * 10;

                // max clamps
                if (zoomTop + zoomHeight > bg_frame->height) {
                    zoomTop = bg_frame->height - zoomHeight;
                }
                if (zoomLeft + zoomWidth > bg_frame->width) {
                    zoomLeft = bg_frame->width - zoomWidth;
                }

                g_print("Mid Info: %d, %d\n", zoomTop, zoomLeft);

                // clamp zoomTop and zoomLeft
                zoomTop = zoomTop > bg_frame->height ? bg_frame->height : zoomTop;
                zoomLeft = zoomLeft > bg_frame->width ? bg_frame->width : zoomLeft;

                // assure even numbers
                // zoomTop = (zoomTop / 2) * 2;
                // zoomLeft = (zoomLeft / 2) * 2;

                if (frameIndex == 0) {
                    smoothZoomTop = zoomTop;
                    smoothZoomLeft = zoomLeft;
                }

                double smoothingFactor = 0.1; // Adjust this value to change the amount of smoothing (0-1)
                smoothZoomTop = smoothingFactor * zoomTop + (1 - smoothingFactor) * smoothZoomTop;
                smoothZoomLeft = smoothingFactor * zoomLeft + (1 - smoothingFactor) * smoothZoomLeft;

                // round
                smoothZoomTop = std::round(smoothZoomTop);
                smoothZoomLeft = std::round(smoothZoomLeft);

                // assure even numbers
                smoothZoomTop = (static_cast<int>(smoothZoomTop) % 2 == 0) ? smoothZoomTop : smoothZoomTop - 1;
                smoothZoomLeft = (static_cast<int>(smoothZoomLeft) % 2 == 0) ? smoothZoomLeft : smoothZoomLeft - 1;

                // max clamps
                if (smoothZoomTop + zoomHeight > bg_frame->height) {
                    smoothZoomTop = bg_frame->height - zoomHeight;
                }
                if (smoothZoomLeft + zoomWidth > bg_frame->width) {
                    smoothZoomLeft = bg_frame->width - zoomWidth;
                }

                // g_print("Mid Info: %d, %d\n", zoomTop, zoomLeft);

                // clamp zoomTop and zoomLeft
                smoothZoomTop = smoothZoomTop > bg_frame->height ? bg_frame->height : smoothZoomTop;
                smoothZoomLeft = smoothZoomLeft > bg_frame->width ? bg_frame->width : smoothZoomLeft;

                // double sure
                smoothZoomTop = (static_cast<int>(smoothZoomTop) % 2 == 0) ? smoothZoomTop : smoothZoomTop - 1;
                smoothZoomLeft = (static_cast<int>(smoothZoomLeft) % 2 == 0) ? smoothZoomLeft : smoothZoomLeft - 1;

                g_print("Smooth Info: %f, %f\n", smoothZoomTop, smoothZoomLeft);
                
                // Create a new AVFrame for the zoomed portion.
                AVFrame* zoom_frame = av_frame_alloc();
                zoom_frame->format = bg_frame->format;
                zoom_frame->width = bg_frame->width; // Keep the output frame size the same.
                zoom_frame->height = bg_frame->height;
                zoom_frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                av_frame_get_buffer(zoom_frame, 0);

                g_print("Zoom Frame: %d x %d\n", zoomWidth, zoomHeight);
                g_print("Diagnostic Info: %d x %d\n", zoom_frame->width, zoom_frame->height);

                // Scale up the zoomed portion to the output frame size using libswscale.
                struct SwsContext* swsCtxZoom = sws_getContext(
                    zoomWidth, zoomHeight, (enum AVPixelFormat)bg_frame->format,
                    zoom_frame->width, zoom_frame->height, (enum AVPixelFormat)zoom_frame->format,
                    SWS_BILINEAR, NULL, NULL, NULL);

                // Get pointers to the zoomed portion in the background frame.
                uint8_t* zoomData[3];
                // zoomData[0] = &bg_frame->data[0][static_cast<int>(smoothZoomTop * bg_frame->linesize[0] + smoothZoomLeft)];
                // if (static_cast<int>(smoothZoomTop) % 2 == 0 && static_cast<int>(smoothZoomLeft) % 2 == 0) {
                //     zoomData[1] = &bg_frame->data[1][static_cast<int>(smoothZoomTop/2) * bg_frame->linesize[1] + static_cast<int>(smoothZoomLeft/2)];
                //     zoomData[2] = &bg_frame->data[2][static_cast<int>(smoothZoomTop/2) * bg_frame->linesize[2] + static_cast<int>(smoothZoomLeft/2)];
                // } else {
                //     zoomData[1] = NULL;
                //     zoomData[2] = NULL;
                // }

                int smoothZoomTopInt = static_cast<int>(smoothZoomTop);
                int smoothZoomLeftInt = static_cast<int>(smoothZoomLeft);

                zoomData[0] = &bg_frame->data[0][smoothZoomTopInt * bg_frame->linesize[0] + smoothZoomLeftInt];
                if (smoothZoomTopInt % 2 == 0 && smoothZoomLeftInt % 2 == 0) {
                    zoomData[1] = &bg_frame->data[1][(smoothZoomTopInt/2) * bg_frame->linesize[1] + (smoothZoomLeftInt/2)];
                    zoomData[2] = &bg_frame->data[2][(smoothZoomTopInt/2) * bg_frame->linesize[2] + (smoothZoomLeftInt/2)];
                } else {
                    zoomData[1] = NULL;
                    zoomData[2] = NULL;
                }

                // check zoomData
                for (int i = 0; i < 3; i++) {
                    if (zoomData[i] == nullptr) {
                        g_print("zoomData[%d] is null\n", i);
                    }
                }

                // check other sws_scale args
                g_print("sws_scale args %d %d %d %d \n", bg_frame->linesize, zoomHeight, zoom_frame->data, zoom_frame->linesize);
                
                // zoom scale
                sws_scale(swsCtxZoom, (const uint8_t* const*)zoomData, bg_frame->linesize, 0, zoomHeight, zoom_frame->data, zoom_frame->linesize);

                // some cleanup
                sws_freeContext(swsCtxZoom);

                av_frame_free(&bg_frame); // We don't need the original background frame anymore.
                av_frame_free(&frame); // We don't need the original frame anymore.
                av_frame_free(&scaled_frame); // We don't need the scaled frame anymore.

                g_print("Send Frame\n");

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