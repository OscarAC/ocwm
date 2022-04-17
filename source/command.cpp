#include <ocwm.hpp>
#include <unistd.h>
#include <paths.h>

bool wm::command_t::exec()
{
    if (fork() == 0) {				
		setsid();
		execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd, NULL);
	}

    return false;
}