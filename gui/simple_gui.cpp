#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <iostream>

// Simple Windows GUI without Qt
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DiskScoutGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        "DiskScoutGUI",
        "DiskScout GUI v2.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        NULL, NULL, hInstance, NULL
    );
    
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Create controls
        CreateWindow("STATIC", "DiskScout GUI v2.0", 
                    WS_VISIBLE | WS_CHILD | SS_CENTER,
                    10, 10, 300, 30, hwnd, NULL, NULL, NULL);
        
        CreateWindow("BUTTON", "Scan C:\\", 
                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    10, 50, 100, 30, hwnd, (HMENU)1, NULL, NULL);
        
        CreateWindow("BUTTON", "Scan D:\\", 
                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    120, 50, 100, 30, hwnd, (HMENU)2, NULL, NULL);
        
        CreateWindow("STATIC", "Results will appear here...", 
                    WS_VISIBLE | WS_CHILD | SS_LEFT,
                    10, 100, 500, 200, hwnd, NULL, NULL, NULL);
        break;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            MessageBox(hwnd, "Scanning C:\\ drive...\n\nThis is a placeholder.\nReal scanning will use your C+Assembly backend.", 
                      "DiskScout", MB_OK | MB_ICONINFORMATION);
        } else if (LOWORD(wParam) == 2) {
            MessageBox(hwnd, "Scanning D:\\ drive...\n\nThis is a placeholder.\nReal scanning will use your C+Assembly backend.", 
                      "DiskScout", MB_OK | MB_ICONINFORMATION);
        }
        break;
        
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Draw a simple treemap placeholder
        RECT rect;
        GetClientRect(hwnd, &rect);
        
        // Draw background
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
        
        // Draw placeholder rectangles
        HBRUSH brush1 = CreateSolidBrush(RGB(255, 0, 0));
        HBRUSH brush2 = CreateSolidBrush(RGB(0, 255, 0));
        HBRUSH brush3 = CreateSolidBrush(RGB(0, 0, 255));
        
        RECT rect1 = {400, 100, 600, 200};
        RECT rect2 = {400, 210, 500, 300};
        RECT rect3 = {510, 210, 700, 300};
        
        FillRect(hdc, &rect1, brush1);
        FillRect(hdc, &rect2, brush2);
        FillRect(hdc, &rect3, brush3);
        
        // Draw labels
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 450, 140, "Users (45.2 GB)", 15);
        TextOut(hdc, 420, 250, "Program Files", 13);
        TextOut(hdc, 530, 250, "Windows", 7);
        
        DeleteObject(brush1);
        DeleteObject(brush2);
        DeleteObject(brush3);
        
        EndPaint(hwnd, &ps);
        break;
    }
    
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}
