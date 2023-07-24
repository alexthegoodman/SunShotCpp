#include <gtk/gtk.h>

#include <dxgi1_2.h>
#include <d3d11.h>

#include <chrono>
#include <thread>
#include <atomic>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}

#include "shared.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

std::atomic<bool> continueCapturing(false);

void SetupDesktopDuplication(HWND sharedHwnd) {
    const char* outputFilename = "C:\\Users\\alext\\projects\\common\\SunShotCpp\\stubs\\project1\\basicCapture.mp4";

    // *** Setup DirectX *** //
    IDXGIDevice* dxgiDevice;
    IDXGIAdapter* dxgiAdapter;
    IDXGIOutput* dxgiOutput;
    IDXGIOutput1* dxgiOutput1;
    IDXGIOutputDuplication* dxgiOutputDuplication;

    // Create D3D device
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dDeviceContext;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dDeviceContext);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to create D3D device\n");
        return;
    }

    // Get DXGI device
    hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI device\n");
        return;
    }

    // Get DXGI adapter
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI adapter\n");
        return;
    }

    // Get DXGI output
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI output\n");
        return;
    }

    // Get DXGI output1
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI output1\n");
        return;
    }

    // Get output duplication
    hr = dxgiOutput1->DuplicateOutput(dxgiDevice, &dxgiOutputDuplication);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get output duplication\n");
        return;
    }

    // *** window recording setup *** //
    g_mutex_lock(&shared_mutex);
    HWND hWnd = sharedHwnd;
    g_mutex_unlock(&shared_mutex);

    g_print("HWND: %p\n", hWnd);

    // capture window
    // Get window rectangle
    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);

    // Create buffer for window image data
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    windowWidth = (windowWidth % 2 == 0) ? windowWidth : windowWidth - 1;
    windowHeight = (windowHeight % 2 == 0) ? windowHeight : windowHeight - 1;

    int fpsInt = 15;
    double fpsDouble = 15.0;

    // *** Setup FFmpeg *** //
    AVFormatContext *encoderFormatCtx = NULL;
    avformat_alloc_output_context2(&encoderFormatCtx, NULL, NULL, outputFilename);
    if (!encoderFormatCtx) {
        g_print("Could not create output context\n");
        return;
    }

    encoderFormatCtx->oformat = av_guess_format("mp4", NULL, NULL);

    // Setup the codec
    AVStream *encoderStream = avformat_new_stream(encoderFormatCtx, NULL);
    if (!encoderStream) {
        g_print("Failed to allocate output stream\n");
        return;
    }

    const AVCodec* encoderCodec = avcodec_find_encoder_by_name("libx264");
    if (!encoderCodec) {
        g_print("Could not find encoder\n");
        return;
    }

    AVCodecContext *encoderCodecCtx = avcodec_alloc_context3(encoderCodec);
    if (!encoderCodecCtx) {
        g_print("Could not create video codec context\n");
        return;
    }

    g_print("Setting up codec context...\n");
    g_print("Width: %d Height: %d\n", windowWidth, windowHeight);

    encoderCodecCtx->bit_rate = 5000 * 1000; // 5,000 Kbps = 5,000,000 bps
    encoderCodecCtx->width = windowWidth;
    encoderCodecCtx->height = windowHeight;
    encoderCodecCtx->time_base = (AVRational){1, fpsInt}; // 30FPS
    encoderCodecCtx->framerate = (AVRational){fpsInt, 1}; // 30FPS
    encoderCodecCtx->gop_size = 15; // distance between keyframes, larger GOP size will reducce file size and quality
    encoderCodecCtx->max_b_frames = 3; // better for motion, better for file size, increasing encode / decode time, can be 0-16
    encoderCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // g_print("Encoder time_base %d %d %d\n", encoderCodecCtx->time_base, encoderCodecCtx->time_base.num, encoderCodecCtx->time_base.den);

    encoderStream->time_base = encoderCodecCtx->time_base;
    // encoderStream->r_frame_rate = encoderCodecCtx->framerate;
    // encoderStream->avg_frame_rate = encoderCodecCtx->framerate;

    // Step 3: Open the codec and add the stream info to the format context
    if (avcodec_open2(encoderCodecCtx, NULL, NULL) < 0) {
        g_print("Failed to open encoder for stream\n");
        return;
    }

    if (avcodec_parameters_from_context(encoderStream->codecpar, encoderCodecCtx) < 0) {
        g_print("Failed to copy encoder parameters to output stream\n");
        return;
    }

    // Open output file
    g_print("Open output file...\n");
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&encoderFormatCtx->pb, outputFilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            g_print("Could not open output file %s\n", outputFilename);
            return;
        }
    }

    // Write file header
    if (avformat_write_header(encoderFormatCtx, NULL) < 0) {
        g_print("Error occurred when opening output file\n");
        return;
    }

    // Determine the duration of each frame at 30 FPS.
    auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double>(1.0 / fpsDouble));
    int frameIndex = 0;
    
    while (continueCapturing) {
        // Time point when we start capturing this frame.
        auto frameStart = std::chrono::high_resolution_clock::now();

        // Capture and process the frame...
        IDXGIResource* desktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;

        // Acquire the next frame.
        hr = dxgiOutputDuplication->AcquireNextFrame(1000, &frameInfo, &desktopResource); // Timeout after 1 second

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            // No new frame within the timeout period, try again
            continue;
        }
        else if (FAILED(hr)) {
            g_print("An error occurred while capturing\n");
            break;
        }

        // Query the IDXGISurface1 interface.
        IDXGISurface1* desktopSurface = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(IDXGISurface1), (void**)&desktopSurface);

        if (FAILED(hr)) {
            g_print("An error occurred while querying the IDXGISurface1 interface\n");
        }
        
        ID3D11Texture2D* pTexture = nullptr;
        // Convert the IDXGISurface1 to ID3D11Texture2D
        hr = desktopSurface->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pTexture));

        if (FAILED(hr)) {
            g_print("An error occurred while querying the ID3D11Texture2D interface\n");
        }

        // copy the texture to read-allowed space
        D3D11_TEXTURE2D_DESC desc;
        pTexture->GetDesc(&desc);

        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;  // We don't need to bind this texture to the pipeline.
        desc.MiscFlags = 0;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        ID3D11Texture2D* cpuTexture = nullptr;
        hr = d3dDevice->CreateTexture2D(&desc, nullptr, &cpuTexture);
        if (FAILED(hr)) {
            g_print("An error occurred while creating texture\n");
        }

        // Now we have a texture that we can map. Copy the data from the original texture.
        d3dDeviceContext->CopyResource(cpuTexture, pTexture);

        D3D11_MAPPED_SUBRESOURCE mappedRect; // mapped rect
        hr = d3dDeviceContext->Map(cpuTexture, 0, D3D11_MAP_READ, 0, &mappedRect);

        if (FAILED(hr)) {
            g_print("Failed to map desktop surface image\n");
        }

        if (SUCCEEDED(hr)) {
            // Now that you have the IDXGISurface1, you can extract the bitmap data,
            // convert it to the format that ffmpeg expects and feed it to your encoder.

            g_print("Captured a frame\n");

            if (hWnd != NULL) {
                // capture a window
                BYTE* windowBuffer = new BYTE[windowWidth * windowHeight * 4];  // assuming BGRA format
                
                // Copy image data row by row
                for (int y = 0; y < windowHeight; y++) {
                    // BYTE* srcRow = mappedRect.pBits + (windowRect.top + y) * mappedRect.Pitch;
                    BYTE* srcRow = static_cast<BYTE*>(mappedRect.pData) + (windowRect.top + y) * mappedRect.RowPitch;
                    BYTE* destRow = windowBuffer + y * windowWidth * 4;
                    memcpy(destRow, srcRow + windowRect.left * 4, windowWidth * 4);
                }

                desktopSurface->Unmap();

                // check that windowBuffer has valid data
                if (windowBuffer == nullptr) {
                    g_print("windowBuffer is null\n");
                    return;
                }

                // Now you can use windowBuffer for encoding with FFmpeg...
                AVPixelFormat captured_pix_fmt = AV_PIX_FMT_BGRA;
                AVPixelFormat encoder_pix_fmt = AV_PIX_FMT_YUV420P;
                int bytes_per_pixel = 4;

                // Convert the captured window buffer to the proper pixel format expected by the encoder
                struct SwsContext *swsCtx = sws_getContext(windowWidth, windowHeight, captured_pix_fmt,
                    windowWidth, windowHeight, encoder_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
                uint8_t *src_data[4], *dst_data[4];
                int src_linesize[4], dst_linesize[4];
                int ret = av_image_alloc(src_data, src_linesize, windowWidth, windowHeight, captured_pix_fmt, 1);

                if (ret < 0) {
                    g_print("Error allocating image 1\n");
                }

                ret = av_image_alloc(dst_data, dst_linesize, windowWidth, windowHeight, encoder_pix_fmt, 1);

                if (ret < 0) {
                    g_print("Error allocating image 2\n");
                }

                memcpy(src_data[0], windowBuffer, windowWidth * windowHeight * bytes_per_pixel);
                sws_scale(swsCtx, src_data, src_linesize, 0, windowHeight, dst_data, dst_linesize);

                // // check dst_data
                // for (int i = 0; i < 4; i++) {
                //     if (dst_data[i] == nullptr) {
                //         // [3] should by null for YUV
                //         g_print("dst_data[%d] is null\n", i);
                //     }
                // }

                // // check dst_linesize
                // for (int i = 0; i < 4; i++) {
                //     if (dst_linesize[i] == 0) {
                //         // [3] should by null for YUV here also
                //         g_print("dst_linesize[%d] is 0\n", i);
                //     }
                //     else {
                //         g_print("dst_linesize[%d] = %d\n", i, dst_linesize[i]);
                //     }
                // }

                // g_print("Allocating frame...\n");

                // Create AVFrame and set its data and linesize
                AVFrame *frame = av_frame_alloc();
                frame->format = encoder_pix_fmt;
                frame->width = windowWidth;
                frame->height = windowHeight;
                frame->data[0] = dst_data[0]; // Y plane
                frame->data[1] = dst_data[1]; // U plane
                frame->data[2] = dst_data[2]; // V plane
                frame->linesize[0] = dst_linesize[0]; // Y plane
                frame->linesize[1] = dst_linesize[1]; // U plane
                frame->linesize[2] = dst_linesize[2]; // V plane
                frame->pts = av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base);
                frame->time_base = encoderCodecCtx->time_base;

                g_print("Sending frame... %d\n", frameIndex);
                // g_print("Check pts and timebase %d %d\n", frame->pts, av_rescale_q(frameIndex, encoderCodecCtx->time_base, encoderStream->time_base));

                // Send the frame to the encoder
                ret = avcodec_send_frame(encoderCodecCtx, frame);
                if (ret < 0) {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
                    g_print("Error sending frame to encoder: %s\n", errbuf);
                    return;
                }

                //g_print("Writing frame...\n");

                // Receive the encoded packets from the encoder
                AVPacket *packet = av_packet_alloc();
                while (avcodec_receive_packet(encoderCodecCtx, packet) == 0) {
                    // Write the encoded packet to the output file
                    if (av_write_frame(encoderFormatCtx, packet) < 0) {
                        fprintf(stderr, "Error writing packet to output file\n");
                        return;
                    }
                    av_packet_unref(packet);
                }

                // Clean up
                av_frame_free(&frame);
                av_packet_free(&packet);
                av_freep(&src_data[0]);
                av_freep(&dst_data[0]);
                sws_freeContext(swsCtx);
                
                delete[] windowBuffer;
            }
            // else if (captureScreen != NULL) {
            //     // capture an entire screen
            // }
        }
        
        // clean up
        desktopSurface->Release();
        desktopResource->Release();
        dxgiOutputDuplication->ReleaseFrame();
        d3dDeviceContext->Unmap(cpuTexture, 0);
        cpuTexture->Release();

        // fps throttling
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto captureDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

        frameIndex++;

        if (captureDuration < frameDuration) {
            // If we finished capturing faster than our target frame rate,
            // sleep for the remaining time.
            std::this_thread::sleep_for(frameDuration - captureDuration);
        }
    }

    // encoderStream->duration = frameIndex * frameDuration.count();

    // Flush the encoder
    avcodec_send_frame(encoderCodecCtx, NULL);
    AVPacket *packet = av_packet_alloc();
    while (avcodec_receive_packet(encoderCodecCtx, packet) == 0) {
        // Write the encoded packet to the output file
        if (av_write_frame(encoderFormatCtx, packet) < 0) {
            fprintf(stderr, "Error writing packet to output file\n");
            return;
        }
        av_packet_unref(packet);
    }

    // Write file trailer and close the file
    g_print("Write trailer...\n");
    av_write_trailer(encoderFormatCtx);
    if (!(encoderFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&encoderFormatCtx->pb);
    }

    // *** Cleanup *** //
    avcodec_free_context(&encoderCodecCtx);
    avformat_free_context(encoderFormatCtx);

    dxgiOutputDuplication->Release();
    dxgiOutput1->Release();
    dxgiOutput->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();
    d3dDeviceContext->Release();
    d3dDevice->Release();

    // return 0;
}

void* start_recorder (void* args)
{
    continueCapturing = true;
    // get selectedWindow from args
    HWND selectedWindow = reinterpret_cast<HWND>(args);
    g_print("Selected window: %p\n", selectedWindow);

    SetupDesktopDuplication(selectedWindow);

    return nullptr;
}

static int stop_recorder (GtkWidget *widget, gpointer data)
{
    continueCapturing = false;

    return 0;
}