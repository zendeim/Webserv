#define MAIN_FILE

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "core.hpp"
#include "Server.hpp"

int main(int argc, char** argv, char** envp) {
	static Server server = {};	// REVIEW: see if initializing like this touches pages
	(void)argc, (void)argv, (void)envp;

	if (argc < 2)
		server.init("assets/configs/default.conf");
	else if (argc == 2)
		server.init(argv[1]);
	else
		PERR_RETURN(1, "Error: Usage -> ./webserv <config_file>");

	server.run();
	return 0;		// Technically unreachable
}
