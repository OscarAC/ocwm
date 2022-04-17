#include <wmanager.hpp>
#include <config.hpp>
#include <string>
#include <singleton.hpp>
#include <iostream>
#include <X11/keysym.h>

using namespace wm;

window_manager window_manager::instance = util::singleton<window_manager>::instance();

bool wm::window_manager::connect(const std::string &disp)
{
    display = XOpenDisplay(disp.empty() ? nullptr : disp.c_str());
    return display != nullptr;
}

bool wm::window_manager::setup()
{
    root = DefaultRootWindow(display);
    init_screen();
    init_cursor();
    init_events();
    init_keys();

    return true;
}

bool wm::window_manager::init_screen()
{
    screen.idx = DefaultScreen(display);
    screen.width = DisplayWidth(display, screen.idx);
    screen.height = DisplayHeight(display, screen.idx);
    return true;
}

bool wm::window_manager::init_cursor()
{
    return true;
}

bool wm::window_manager::init_events()
{
    attributes.event_mask =
        SubstructureRedirectMask | SubstructureNotifyMask |
        ButtonPressMask | PointerMotionMask |
        EnterWindowMask | LeaveWindowMask |
        StructureNotifyMask | PropertyChangeMask;

    XChangeWindowAttributes(display, root, CWEventMask | CWCursor, &attributes);
    XSelectInput(display, root, attributes.event_mask);
    return true;
}

bool wm::window_manager::init_keys()
{
    XGrabKey(display, XKeysymToKeycode(display, XK_a),
             ControlMask, root, true, GrabModeAsync, GrabModeAsync);

    return true;
}

void wm::window_manager::run()
{
    XSync(display, false);

    for (;;)
    {
        XEvent e;
        XNextEvent(display, &e);

        switch (e.type)
        {

        case MapRequest:
            on_map_request(e.xmaprequest);
            break;

        case KeyPress:
            on_key_press(e.xkey);
            break;

        case CreateNotify:
            std::cout << "CreateNotify\n";
            break;

        case DestroyNotify:
            std::cout << "DestroyNotify\n";
            break;

        case ReparentNotify:
            std::cout << "ReparentNotify\n";
            break;

        case MapNotify:
            std::cout << "MapNotify\n";
            break;

        case UnmapNotify:
            std::cout << "UnMapNotify\n";
            break;

        case ConfigureNotify:
            std::cout << "ConfigureNotify\n";
            break;

        case ConfigureRequest:
            std::cout << "ConfigureRequest\n";
            break;

        case ButtonPress:
            std::cout << "ButtonPress\n";
            break;

        case ButtonRelease:
            std::cout << "ButtonRelease\n";
            break;

        case MotionNotify:
            std::cout << "MotionNotify\n";
            break;

        case KeyRelease:
            std::cout << "KeyRelease\n";
            break;

        default:
            std::cout << "Default\n";
            break;
        }
    }
}

void wm::window_manager::on_map_request(const XMapRequestEvent &e)
{
    std::cout << "OnMapRequest\n";

    XMapWindow(display, e.window);
}

void wm::window_manager::on_key_press(const XKeyPressedEvent &e)
{
    std::cout << "OnKeyPress\n";

    keys[0].cmd.exec();
}

int wm::window_manager::on_error(Display *display, XErrorEvent *e)
{
    std::cout << "On Error\n";
    return 0;
}

void wm::window_manager::destroy()
{
    std::cout << "Destroying..\n";

    if (display)
    {
        std::cout << "display not null\n";
        XCloseDisplay(display);
        display = nullptr;
    }
}
