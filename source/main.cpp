#include <iostream>
#include <X11/Xlib.h>
#include <unistd.h>
#include <wmanager.hpp>

int main()
{
    auto wm = wm::window_manager::instance;

    if(wm.connect(std::string()))
    {
        wm.setup();
        wm.run();
    } else {

        std::cout << "Could not connect\n";
    }

    wm.destroy();

    return 0;
}