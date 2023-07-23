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

    // *** Setup FFmpeg *** //
    AVFormatContext *encoderFormatCtx = NULL;
    avformat_alloc_output_context2(&encoderFormatCtx, NULL, NULL, outputFilename);
    if (!encoderFormatCtx) {
        g_print("Could not create output context\n");
        return;
    }

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

    encoderCodecCtx->bit_rate = 10000; // 10Mbps / 10,000Kbps
    encoderCodecCtx->width = windowWidth;
    encoderCodecCtx->height = windowHeight;
    encoderCodecCtx->time_base = (AVRational){1, 30}; // 30FPS
    encoderCodecCtx->framerate = (AVRational){30, 1}; // 30FPS
    encoderCodecCtx->gop_size = 10;
    encoderCodecCtx->max_b_frames = 1;
    encoderCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    encoderStream->time_base = encoderCodecCtx->time_base;

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
        std::chrono::duration<double>(1.0 / 30.0));

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
            // An error occurred, handle it here
            g_print("An error occurred while capturing\n");
            break;
        }

        // Query the IDXGISurface1 interface.
        IDXGISurface1* desktopSurface = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(IDXGISurface1), (void**)&desktopSurface);

        D3D11_MAPPED_SUBRESOURCE mappedRect; // mapped rect
        hr = d3dDeviceContext->Map(desktopSurface, 0, D3D11_MAP_READ, 0, &mappedRect);

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

                dxgiSurface->Unmap();

                // Now you can use windowBuffer for encoding with FFmpeg...
                AVPixelFormat* captured_pix_fmt = AV_PIX_FMT_BGRA;
                AVPixelFormat* encoder_pix_fmt = AV_PIX_FMT_YUV420P;
                int bytes_per_pixel = 4;

                // Convert the captured window buffer to the proper pixel format expected by the encoder
                struct SwsContext *swsCtx = sws_getContext(windowWidth, windowHeight, captured_pix_fmt,
                    windowWidth, windowHeight, encoder_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
                uint8_t *src_data[4], *dst_data[4];
                int src_linesize[4], dst_linesize[4];
                av_image_alloc(src_data, src_linesize, windowWidth, windowHeight, captured_pix_fmt, 1);
                av_image_alloc(dst_data, dst_linesize, windowWidth, windowHeight, encoder_pix_fmt, 1);
                memcpy(src_data[0], windowBuffer, windowWidth * windowHeight * bytes_per_pixel);
                sws_scale(swsCtx, src_data, src_linesize, 0, windowHeight, dst_data, dst_linesize);

                // Create AVFrame and set its data and linesize
                AVFrame *frame = av_frame_alloc();
                frame->format = encoder_pix_fmt;
                frame->width = windowWidth;
                frame->height = windowHeight;
                frame->data[0] = dst_data[0];
                frame->linesize[0] = dst_linesize[0];

                // Send the frame to the encoder
                if (avcodec_send_frame(encoderCodecCtx, frame) < 0) {
                    fprintf(stderr, "Error sending frame to encoder\n");
                    return;
                }

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
        d3dDeviceContext->Unmap(desktopSurface, 0);

        // fps throttling
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto captureDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

        if (captureDuration < frameDuration) {
            // If we finished capturing faster than our target frame rate,
            // sleep for the remaining time.
            std::this_thread::sleep_for(frameDuration - captureDuration);
        }
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