#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include <queue>
#include <string>
#include <mutex>

class GUI {
public:
    GUI(HINSTANCE hInstance, int nCmdShow);
    ~GUI();
    bool Initialize();
    void RunMessageLoop();
    bool IsRunning() const;
    void Update();
    bool HasNewMessage();
    std::string GetNewMessage();
    void DisplayMessageFromServer(const std::string& message);

private:
    HINSTANCE hInstance;
    int nCmdShow;
    HWND hwnd;
    bool running;
    std::queue<std::string> messages;
    std::mutex guiMutex;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // GUI_H
