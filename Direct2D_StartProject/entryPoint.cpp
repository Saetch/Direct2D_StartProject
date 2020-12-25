#include "entryPoint.h"
#include <windows.h>
#include <d2d1.h>
#include <stdio.h>
#pragma comment(lib, "d2d1")

#include "BaseWindow.h"

int icall = 0;

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class MainWindow : public BaseWindow<MainWindow>
{
    ID2D1Factory* pFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    ID2D1SolidColorBrush* pBrush;


    ID2D1Bitmap* buffer;
    ID2D1BitmapRenderTarget* pBufferTarget;
    ID2D1SolidColorBrush* pBufferBrush;
    D2D1_ELLIPSE            ellipse;
    ID2D1SolidColorBrush* pStroke;
    void    CalculateLayout();
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
    void    OnPaint();
    void    Resize();

public:

    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
    {
    }

    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

// Recalculate drawing layout when the size of the window changes.

void MainWindow::CalculateLayout()
{
    if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize(); //GetSize returns in dip (device independent pixels), which is the correct measurement for all layout options
        const float x = size.width / 2;
        const float y = size.height / 2;
        const float radius = min(x, y);
        ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
    }
}

HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
        
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);
        if (SUCCEEDED(hr)) {
            hr = pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(100.0f, 100.0f), &pBufferTarget);

        }

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0, 1.0f);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            if (SUCCEEDED(hr)) {
                hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f), &pStroke);
                if (SUCCEEDED(hr))
                {
                    hr = pBufferTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 0.5f, 0.5f), &pBufferBrush);
                    
                    if (SUCCEEDED(hr)) {
                        hr = pBufferTarget->GetBitmap(&buffer);
                        if (SUCCEEDED(hr)) {

                            CalculateLayout();

                        }
                    }

                    
                }
            }
            
        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void MainWindow::DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth)
{
    pRenderTarget->SetTransform(
        D2D1::Matrix3x2F::Rotation(fAngle, ellipse.point)
    );

    // endPoint defines one end of the hand.
    D2D_POINT_2F endPoint = D2D1::Point2F(
        ellipse.point.x,
        ellipse.point.y - (ellipse.radiusY * fHandLength)
    );


    // Draw a line from the center of the ellipse to endPoint.
    pRenderTarget->DrawLine(
        ellipse.point, endPoint, pStroke, fStrokeWidth);
}

void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(111, 0.0f));
        pRenderTarget->FillEllipse(ellipse, pBrush);
        pRenderTarget->DrawEllipse(ellipse, pStroke);
        pBufferTarget->BeginDraw();
        pBufferTarget->Clear();
        if (icall < 20) {
            pBufferTarget->DrawLine(D2D1::Point2F(10.0f, 10.0f), D2D1::Point2F(150.0f, 150.0f), pBufferBrush, 20.0f);
            icall++;
        }
        else {
            pBufferTarget->DrawLine(D2D1::Point2F(10.0f, 10.0f), D2D1::Point2F(150.0f, 150.0f), pBufferBrush, 5.0f);

        }
        pBufferTarget->EndDraw();
        pRenderTarget->DrawRectangle(D2D1::RectF(0.0f, 0.0f, 160.0f, 150.0f), pBrush);
        pRenderTarget->DrawBitmap(buffer, D2D1::RectF(0.0f, 0.0f, 150.0f, 150.0f),1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,D2D1::RectF(0.0f, 0.0f, 200.0f, 200.0f));
        // Draw hands
        SYSTEMTIME time;
        GetLocalTime(&time);

        // 60 minutes = 30 degrees, 1 minute = 0.5 degree
        const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
        const float fMinuteAngle = (360.0f / 60) * (time.wMinute);

        DrawClockHand(0.6f, fHourAngle, 6);
        DrawClockHand(0.85f, fMinuteAngle, 4);

        // Restore the identity transformation.
        pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        printf_s("%d\n",EndPaint(m_hwnd, &ps));
    }
}

void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        CalculateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

int main(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Circle", WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    ShowWindow(win.Window(), nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;



    case WM_SIZE:
        Resize();
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}