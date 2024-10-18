#include "GUI.h"
#include <iostream> // For std::cout
#include <queue>
#include <mutex>
#include <string>

// Constructor implementation
GUI::GUI(HINSTANCE hInstance, int nCmdShow)
    : hInstance(hInstance), nCmdShow(nCmdShow), hwnd(nullptr), running(true) {
}

// Destructor implementation
GUI::~GUI() {
    if (hwnd) {
        DestroyWindow(hwnd);
    }
}

// Initialize method implementation
bool GUI::Initialize() {
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"MyWindowClass"; // Use wide string (L"")
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        std::cerr << "Failed to register window class." << std::endl;
        return false;
    }

    // Create the window
    hwnd = CreateWindowEx(
        0,
        L"MyWindowClass",        // Use wide string (L"")
        L"Chat Server",          // Use wide string (L"")
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return true;
}

// Window procedure callback implementation
LRESULT CALLBACK GUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Implementation for IsRunning
bool GUI::IsRunning() const {
    return running;
}

// Implementation for Update
void GUI::Update() {
    // Simulate some updates
}

// Implementation for HasNewMessage
bool GUI::HasNewMessage() {
    std::lock_guard<std::mutex> lock(guiMutex);
    return !messages.empty();
}

// Implementation for GetNewMessage
std::string GUI::GetNewMessage() {
    std::lock_guard<std::mutex> lock(guiMutex);
    if (!messages.empty()) {
        std::string message = messages.front();
        messages.pop();
        return message;
    }
    return "";
}

// Implementation for DisplayMessageFromServer
void GUI::DisplayMessageFromServer(const std::string& message) {
    std::cout << "Server: " << message << std::endl;
}

// Run the message loop
void GUI::RunMessageLoop() {
    MSG msg;
    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Update(); // Call update method to handle GUI updates
    }
}
