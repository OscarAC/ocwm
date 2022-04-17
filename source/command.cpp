#include <command.hpp>
#include <unistd.h>
#include <paths.h>

bool wm::command::exec()
{
    if (fork() == 0) {				
		setsid();
		execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd, NULL);
	}

    return false;
}