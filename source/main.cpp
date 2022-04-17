#include <iostream>
#include <ocwm.hpp>

using namespace wm;

int main()
{
    win_manager wm;

    if(!wm.connect(std::string()))
    {
        std::cout << "Could not connect\n";
        return 1;
    }

    wm.setup();
    wm.run();
    wm.destroy();
    return 0;
}