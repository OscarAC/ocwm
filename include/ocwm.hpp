#ifndef OCWM_H
#define OCWM_H

#include <string>
#include <X11/Xlib.h>

namespace wm
{
    using window_t = Window;
    using display_t = Display *;

    struct screen_t
    {
        int idx;
        int width;
        int height;
    };

    struct context_t
    {
        display_t display;
        window_t root;
        screen_t screen;
    };

    struct command_t
    {
        const char* cmd;

        bool exec();
    };

    struct key_t
    {
        command_t cmd;
    };

    class win_manager
    {
    public:
        bool connect(const std::string &disp);
        bool setup();
        void run();
        void destroy();

        static int on_error(Display *display, XErrorEvent *e);
        static int on_wm_detected(Display *display, XErrorEvent *e);

        void on_map_request(const XMapRequestEvent &e);
        void on_key_press(const XKeyPressedEvent &e);

    private:
        context_t ctx;
    };
};

#endif