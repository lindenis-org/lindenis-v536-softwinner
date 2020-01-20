#include "window/shutdown_window.h"
#include "widgets/graphic_view.h"
#include "resource/resource_manager.h"

using namespace std;

IMPLEMENT_DYNCRT_CLASS(ShutDownWindow)

int ShutDownWindow::HandleMessage(HWND hwnd, int message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
        case MSG_PAINT:
            return HELP_ME_OUT;
        default:
            return SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
    }
    SystemWindow::HandleMessage(hwnd, message, wparam, lparam);
    return HELP_ME_OUT;
}

void ShutDownWindow::GetCreateParams(CommonCreateParams& params)
{
    params.style = WS_NONE;
    params.exstyle = WS_EX_NONE | WS_EX_TOPMOST;
    params.class_name = " ";
    params.alias      = GetClassName();
}

ShutDownWindow::ShutDownWindow(IComponent *parent)
    : SystemWindow(parent)
{
    wname = "ShutDownWindow";
    Load();

    string bkgnd_bmp = R::get()->GetImagePath("shutdown");
    SetWindowBackImage(bkgnd_bmp.c_str());

    //GraphicView::LoadImage(GetControl("gv_shutdown"), "shutdown");
}

ShutDownWindow::~ShutDownWindow()
{
}

string ShutDownWindow::GetResourceName()
{
    return string(GetClassName());
}
