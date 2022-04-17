#ifndef LWM_COMMAND_H
#define LWM_COMMAND_H

namespace wm
{

    class command
    {
    public:
        command(const char *cmd) : cmd(cmd) {}

        bool exec();

    private:
        const char *cmd;
    };
}

#endif