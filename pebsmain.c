#include "pebs.h"
#include <sys/resource.h>



int main(int argc, char** argv) {

	//printf("errno(%d) strerror(%s):", ESRCH, strerror(ESRCH));

	if (argc != 2) {
		fprintf(stderr, "Incorrect Command\n");
		printf("Usage: ./pebs pid\n");
		assert(argc == 2);
	}

	int tpid = atoi(argv[1]);
	if (tpid <= 0) {
		fprintf(stderr, "Invalid pid[%d]\n", tpid);
		assert(tpid > 0);
	}

	if (getuid()) {
		struct rlimit rlimits;
		if (getrlimit(RLIMIT_NOFILE, &rlimits) == -1) {
			fprintf(stderr, "Error getting resource limits; errno(%d) reason(%s)\n", errno, strerror(errno));
			assert(0);
		}
		rlimits.rlim_cur = rlimits.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rlimits) == -1) {
			fprintf(stderr, "Error changing resource limits; errno(%d) reason(%s)\n", errno, strerror(errno));
			assert(0);
		}
		printf("rlimits updated!\n");
	}/*else {
		printf("rlimits unnecessary to change!\n");
	}*/




	tcount = 0;
	strcpy(procpath, "/proc/");
	strcat(procpath, argv[1]);
	strcat(procpath, "/task");
	printf("The target procpath is %s\n", procpath);

	signal(SIGINT, INThandler);

	pebs_init();

	return 0;
} 
