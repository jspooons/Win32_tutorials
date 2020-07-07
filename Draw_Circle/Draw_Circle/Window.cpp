#include <windows.h>
#include <d2d1.h>
#include <windowsx.h>
#pragma comment(lib, "d2d1")
#pragma warning(disable: 4996)

#include "basewin.h"
#include "resource.h"

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

// Mouse coordinates are given in physical pixels, but Direct2D
// expects device-independent pixels (DIPs). To handle high-DPI
// settings correctly, you must translate the pixel coordinate
// into DIPs. Following code shows a helper class that converts
// pixels into DIPs
class DPIScale
{
    static float scaleX;
    static float scaleY;

public:
    static void Initialize(ID2D1Factory* pFactory)
    {
        FLOAT dpiX, dpiY;
        pFactory->GetDesktopDpi(&dpiX, &dpiY);
        scaleX = dpiX / 96.0f;
        scaleY = dpiY / 96.0f;
    }

    template <typename T>
    static D2D1_POINT_2F PixelsToDips(T x, T y)
    {
        return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
    }
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;


class MouseTrackEvents
{
    bool m_bMouseTracking;

public:
    MouseTrackEvents() : m_bMouseTracking(false)
    {
    }

    void OnMouseMove(HWND hwnd)
    {
        if (!m_bMouseTracking)
        {
            // Enable mouse tracking.
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(tme);
            tme.hwndTrack = hwnd;
            tme.dwFlags = TME_HOVER | TME_LEAVE;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            m_bMouseTracking = true;
        }
    }
    void Reset(HWND hwnd)
    {
        m_bMouseTracking = false;
    }
};

class MainWindow : public BaseWindow<MainWindow>
{
    
    ID2D1Factory                *pFactory;
    ID2D1HwndRenderTarget       *pRenderTarget; // interface representing the render target
    ID2D1SolidColorBrush        *pBrush; // interface respresenting the brush
    
    D2D1_ELLIPSE                ellipse;
    D2D1_POINT_2F               ptMouse; // remove for previous example
    MouseTrackEvents            mouseTrack;

    void    CalculateLayout();
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    Resize();

    // message handlers
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();
    void    OnMouseMove(int pixelX, int pixelY, DWORD flags);

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL), 
        ellipse(D2D1::Ellipse(D2D1::Point2F(), 0, 0)), 
        ptMouse(D2D1::Point2F())
    {
    }

    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

// Recalculate drawing layout when the size of the window changes.

// as the window grows or shrinks, you will typically need
// to recalculate the position of the objects that you draw
// in the circle program, the radius and center point 
// must be updated
void MainWindow::CalculateLayout()
{
    /*if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize(); // returns the size of the render target in DIPs (not pixels)
        // which is the appropriate unit for calculating layout
        // But remember that drawing is performed in DIPs, not pixels.
        const float x = size.width / 2;
        const float y = size.height / 2;
        const float radius = min(x, y);
        ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
    }*/ //not needed for drawing circles on screen example 
}

// following code creates the 'render target' and 'brush' resources:
HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        // create a render target for a window
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(), // specifies options that are common to any type of render target
            D2D1::HwndRenderTargetProperties(m_hwnd, size), // specifies the handle to the window and the size of the render target, in pixels
            &pRenderTarget); // receives an ID2D!HwndRenderTarget pointer

        if (SUCCEEDED(hr))
        {
            // color given to color
            //const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            //const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0, 1.0f);
            D2D1_COLOR_F clr;
            clr.r = 1;
            clr.g = 0;
            clr.b = 1;
            clr.a = 1; //alpha, opaque

            // f - foregound color
            // b - background color
            // Can do colour blending with the following formula
            // color = af * Cf + (1 - af) * Cb

            // create the brush
            hr = pRenderTarget->CreateSolidColorBrush(clr, &pBrush);

            if (SUCCEEDED(hr))
            {
                CalculateLayout();
            }
        }
    }
    return hr;
}

// if device lost, render target becomes invalid
// Direct2D signals a lost device by returning the error code
// D2DERR_RECREATE_TARGET from the EndDraw method
// if this error code received, you must recreate the render
// target and all device-dependent resources
// to discard a resource, simply release the interface
// for that resource
void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

// code that draws the circle
void MainWindow::OnPaint()
{
    // ID2D1RenderTarget interface is used for all drawing operations

    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        // signals the start of drawing
        pRenderTarget->BeginDraw();

        // fills the enite render target with a solid color
        // color given as a D2D1_COLOR_F structure
        // you can use the D2D1::ColorF class to init the
        // structure
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
        
        // draws a filled ellipse
        pRenderTarget->FillEllipse(ellipse, pBrush);

        // signals the completion of drawing for this frame
        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
        //   If an error occurs during the execution of any 
        // of these methods, the error is signaled through 
        // the return value of the EndDraw method
    }
}

// To get the DPI setting, call the ID2D1Factory::GetDesktopDpi method
// DPI is returned as two floating-point values

// if the size of the window changes you must resize the render
// target to match. In most cases, you will also need to update
// the layout and repaint the window
// following code shows these steps
void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc); // gets new size of client area, in physical pixels (not DIPs)

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        // ID2D1HwndRenderTarget::Resize method updates the size of the render target
        // also specified in pixels.
        pRenderTarget->Resize(size);
        CalculateLayout();

        // forces a repaint by adding the entire client area to 
        // the windows update region
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Circle", WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    ShowWindow(win.Window(), nCmdShow);

    // accelerator table works by translating key strokes into WM_COMMAND messages
    // wParam parameter of WM_COMMAND contains the numeric identifier
    // of the command.
    //
    // For example, using the table shown previously, the key stroke 
    // CTRL+M is translated into a WM_COMMAND message with 
    // the value ID_TOGGLE_MODE
    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
    
    // Run the message loop.
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // call added to translateAccelerator function, examining each message window
        //##
        if (!TranslateAccelerator(win.Window(), hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    SetCapture(m_hwnd); // begin capturing mouse

    // Store the position of the mouse click in the ptMouse 
    // variable. This position defines the upper left corner 
    // of the bounding box for the ellipse.
    ellipse.point = ptMouse = DPIScale::PixelsToDips(pixelX, pixelY);
    
    // reset ellipse structure
    ellipse.radiusX = ellipse.radiusY = 1.0f;

    //force window to be repainted
    InvalidateRect(m_hwnd, NULL, FALSE);
}


void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
    // check whether left mouse button is down
    if (flags & MK_LBUTTON)
    {
        // if it is, recalculate the ellipse and repaint the window
        // {

        // in Direct2D, an ellipse is defined by the center point
        // and x- and y- radii. We want to draw an ellipse that
        // fits te bounding box defined by the mouse-down
        // point [ptMouse] (think of creating a textbox in powerpoint, want
        // to keep the ellipse defined to those edges) and the
        // current cursor position [x,y], so a bit of arithmetic
        // is needed to find the width, height, and position of
        // the ellipse
        const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);

        const float width = (dips.x - ptMouse.x) / 2;
        const float height = (dips.y - ptMouse.y) / 2;
        const float x1 = ptMouse.x + width;
        const float y1 = ptMouse.y + height;

        ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);

        InvalidateRect(m_hwnd, NULL, FALSE);
        // }
    }
}


// for the left-button-up message, simply call ReleaseCapture
// to release the mouse capture
void MainWindow::OnLButtonUp()
{
    ReleaseCapture();
}


LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        // create Direct2D factory object (call D2D1CreateFactory func)
        // factory creates other objects
        // first arg. is a flag that specifies creation options
        // second arg. is a pointer to the ID2D1Factory interface

        // WM_CREATE is a good place to create the factory
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        DPIScale::Initialize(pFactory);
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

        // because the render target is a window, drawing is
        // done in response to WM_PAINT messages
    case WM_PAINT:
        OnPaint();
        return 0;

        // Other messages not shown...

    case WM_SIZE:
        Resize();
        return 0;


    // use the GET_X_LPARAM and GET_Y_LPARAM macros to get
    // the pixel coords
    // Call DPIScale::PixelsToDipsX and DPIScale::PixelsToDipsY
    // to convert pixels to DIPs

    case WM_LBUTTONDOWN:
        // if your UI supports dragging of UI elements, there
        // is one other function that you should call in your
        // mouse-down message handler: DragDetect
        // returns TRUE if the user initiates a mouse
        // gesture that should be interpreted as dragging
        //The following code shows how to use this function.
        
        //POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        //if (DragDetect(m_hwnd, pt))
        //{
        //    // Start dragging.
        //}

        // Windows defines a drag threshold of a few pixels
        // 

        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        
        if (GetKeyState(VK_MENU) & 0x8000)
        {
            // ALT key is down and so is left button
        }
        
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp();
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        
        //EXTENSION (this + BELOW STUFF)
        mouseTrack.OnMouseMove(m_hwnd);  // Start tracking.
        // TODO: Handle the mouse-move message.

        return 0;


    case WM_MOUSELEAVE:
        // TODO: Handle the mouse-leave message.
        mouseTrack.Reset(m_hwnd);

        return 0;

    case WM_MOUSEHOVER:
        // TODO: Handle the mouse-hover message.
        mouseTrack.Reset(m_hwnd);

        return 0;

    case WM_MOUSEWHEEL:
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        // +'ive (rotate forward, away from user)
        // -'ive (rotate backward, toward user)

        return 0;
    

    // two other mouse messages are disabled by default,
    // WM_MOUSEHOVER and WM_MOUSELEAVE  .
    // To enable these messages, call the TrackMouseEvent func

    // your window might be flooded with tracking messages. 
    // For example, if the mouse is hovering, the system 
    // would continue to generate a stream of WM_MOUSEHOVER 
    // messages while the mouse is stationary. You don't 
    // actually want another WM_MOUSEHOVER message until the 
    // mouse moves to another spot and hovers again.
    // see the helper class below

        /*
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_DRAW_MODE:
            SetMode(DrawMode);
            break;

        case ID_SELECT_MODE:
            SetMode(SelectMode);
            break;

        case ID_TOGGLE_MODE:
            if (mode == DrawMode)
            {
                SetMode(SelectMode);
            }
            else
            {
                SetMode(DrawMode);
            }
            break;
        }
        return 0;*/



    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

// The Direct2D Render Loop

/*
1   -   Create device-independent resources
2      
        a)  Check if a valid render target exists
            If not, create the render target and device-
            dependent resources

        b)  Call ID2D1RenderTarget::BeginDraw
        c)  Issue drawing commands
        d)  Call ID2D1RenderTarget::EndDraw
        e)  If EndDraw returns D2DERR_RECREATE_TARGET, discard
            the render target and device-dependent resources

3   -   Repeat step 2 whenever you need to update or redraw 
        the scene

If the render target is a window, step 2 occurs whenever the 
window receives a WM_PAINT message.

The loop shown here handles device loss by discarding the 
device-dependent resources and re-creating them at the start 
of the next loop (step 2a).
*/

