#include <gtk/gtk.h>

#include <dxgi1_2.h>
#include <d3d11.h>

#include <chrono>
#include <thread>
#include <atomic>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

std::atomic<bool> continueCapturing(false);

void SetupDesktopDuplication() {
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

        if (SUCCEEDED(hr)) {
            // Now that you have the IDXGISurface1, you can extract the bitmap data,
            // convert it to the format that ffmpeg expects and feed it to your encoder.

            g_print("Captured a frame\n");

            // Remember to release the IDXGISurface1 when you're done with it.
            desktopSurface->Release();
        }

        // Release the IDXGIResource.
        desktopResource->Release();

        // Release the frame when you're done with it.
        dxgiOutputDuplication->ReleaseFrame();

        // Time point after capturing the frame.
        auto frameEnd = std::chrono::high_resolution_clock::now();

        // Duration of the capture process.
        auto captureDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

        if (captureDuration < frameDuration) {
            // If we finished capturing faster than our target frame rate,
            // sleep for the remaining time.
            std::this_thread::sleep_for(frameDuration - captureDuration);
        }
    }

    // return 0;
}

void* start_recorder (void* args)
{
    continueCapturing = true;
    SetupDesktopDuplication();

    return nullptr;
}

static int stop_recorder (GtkWidget *widget, gpointer data)
{
    continueCapturing = false;

    return 0;
}