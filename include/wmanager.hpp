#ifndef LWM_H
#define LWM_H

#include <X11/Xlib.h>
#include <string>
#include <singleton.hpp>

namespace wm
{
    struct screen_t
    {
        int idx;
        int width;
        int height;
    };


    class window_manager
    {

    public:
        static window_manager instance;

    public:
        bool connect(const std::string &disp);
        bool setup();
        void run();

        void destroy();

        ~window_manager() { destroy(); }

    private:
        window_manager() noexcept {}        

        bool init_screen();
        bool init_cursor();
        bool init_events();
        bool init_keys();

        static int on_error(Display * display, XErrorEvent * e);
        static int on_wm_detected(Display * display, XErrorEvent * e);

        
        void on_map_request(const XMapRequestEvent& e);
        void on_key_press(const XKeyPressedEvent& e);


    private:                
        Display *display;
        screen_t screen;
        Window root;
        XSetWindowAttributes attributes;

    private:
        friend class util::singleton<window_manager>;
    };
}

#endif