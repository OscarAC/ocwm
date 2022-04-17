#include <ocwm.hpp>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <iostream>

using namespace wm;

bool win_manager::connect(const std::string &disp)
{
    ctx.display = XOpenDisplay(disp.empty() ? nullptr : disp.c_str());
    return ctx.display != nullptr;
}

bool win_manager::setup()
{
    // Grab root
    ctx.root = DefaultRootWindow(ctx.display);

    // Init screen
    ctx.screen.idx = DefaultScreen(ctx.display);
    ctx.screen.width = DisplayWidth(ctx.display, ctx.screen.idx);
    ctx.screen.height = DisplayHeight(ctx.display, ctx.screen.idx);

    // Setup events
    XSetWindowAttributes wa;
    wa.event_mask =
        SubstructureRedirectMask | SubstructureNotifyMask |
        ButtonPressMask | PointerMotionMask |
        EnterWindowMask | LeaveWindowMask |
        StructureNotifyMask | PropertyChangeMask;

    XChangeWindowAttributes(ctx.display, ctx.root, CWEventMask | CWCursor, &wa);
    XSelectInput(ctx.display, ctx.root, wa.event_mask);

    // Setup cmd keys
    XGrabKey(ctx.display, XKeysymToKeycode(ctx.display, XK_a),
             ControlMask, ctx.root, true, GrabModeAsync, GrabModeAsync);

    return true;
}

void win_manager::run()
{
    XSync(ctx.display, false);

    for (;;)
    {
        XEvent e;
        XNextEvent(ctx.display, &e);

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


void win_manager::on_map_request(const XMapRequestEvent &e)
{
    std::cout << "OnMapRequest\n";

    XMapWindow(ctx.display, e.window);
}

void win_manager::on_key_press(const XKeyPressedEvent &e)
{
    std::cout << "OnKeyPress\n";
}

int win_manager::on_error(Display *display, XErrorEvent *e)
{
    std::cout << "On Error\n";
    return 0;
}

void win_manager::destroy()
{
    std::cout << "Destroying..\n";

    if (ctx.display)
    {
        std::cout << "display not null\n";
        XCloseDisplay(ctx.display);
        ctx.display = nullptr;
    }
}
