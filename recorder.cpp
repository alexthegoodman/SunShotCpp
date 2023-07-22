#include <gtk/gtk.h>

#include <dxgi1_2.h>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

IDXGISurface1* SetupDesktopDuplication() {
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
        return nullptr;
    }

    // Get DXGI device
    hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI device\n");
        return nullptr;
    }

    // Get DXGI adapter
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI adapter\n");
        return nullptr;
    }

    // Get DXGI output
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI output\n");
        return nullptr;
    }

    // Get DXGI output1
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get DXGI output1\n");
        return nullptr;
    }

    // Get output duplication
    hr = dxgiOutput1->DuplicateOutput(dxgiDevice, &dxgiOutputDuplication);

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get output duplication\n");
        return nullptr;
    }

    // Get next frame
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;
    hr = dxgiOutputDuplication->AcquireNextFrame(0, &frameInfo, &desktopResource);

    IDXGISurface1* desktopSurface = nullptr;
    if (SUCCEEDED(hr)) {
        hr = desktopResource->QueryInterface(__uuidof(IDXGISurface1), (void**)&desktopSurface);

        if (SUCCEEDED(hr)) {
            // Do something with desktopSurface...

            // Clean up when you're done with it.
            desktopSurface->Release();
        }

        // Make sure to release the resource when you're done with it.
        desktopResource->Release();
    }

    if (FAILED(hr)) {
        // Error handling
        g_print("Failed to get next frame\n");
        return nullptr;
    }

    return desktopSurface;
}

static int print_surface_info (GtkWidget *widget, gpointer data)
{
    IDXGISurface1* desktopSurface = SetupDesktopDuplication();

    if (desktopSurface != nullptr) {
        // You now have a DirectX surface that contains the desktop image
        // You can use this surface with any DirectX API to manipulate the image or copy it to a different surface
        // If you want to capture the desktop at a certain frame rate, you need to call AcquireNextFrame on a loop
        g_print("Got desktop surface\n");
    }

    return 0;
}