#define _GNU_SOURCE
#include "pebs.h"

// DEFINITION
uint32_t tcount = 0;
char procpath[1024];
DIR *dir;
struct dirent *entity;
char tidname[1024];
uint32_t tid;

__u64 event1, event2, event3;

struct perf_event_mmap_page *perf_page[MAXCOUNT][NPBUFTYPES];
int pfd[MAXCOUNT][NPBUFTYPES];

char filename[64];
FILE *fp; // for writingthe profiling result file
pthread_t scan_thread;

//

long _perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);

//struct perf_event_mmap_page* perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t index);
void perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t index);

void *pebs_scan_thread(void*);
	

void pebs_init(void) {
	printf("pebs_init: starts!\n");

	snprintf(filename, sizeof(filename), "profiling_%lu",
		(unsigned long)time(NULL));

	fp = fopen(filename, "w");
	if (!fp) {
		fprintf(stderr, "open file error!\n");
		assert(fp != NULL);
	}

	for (int i=0;i<MAXCOUNT;++i) {
		for (int j=0;j<NPBUFTYPES;++j) {
			perf_page[i][j] = NULL;
			pfd[i][j] = -1;
		}
	}

	int ret = pfm_initialize();
	if (ret != PFM_SUCCESS) {
		fprintf(stderr, "Cannot initialize library: %s\n", pfm_strerror(ret));
		assert(ret == PFM_SUCCESS);
	}

	struct perf_event_attr attr;
	memset(&attr, 0, sizeof(attr));
	ret = pfm_get_perf_event_encoding("MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM", PFM_PLMH, &attr, NULL, NULL);
	if (ret != PFM_SUCCESS) {
		fprintf(stderr, "Cannot get encoding %s\n", pfm_strerror(ret));
		assert(ret == PFM_SUCCESS);
	}
	event1 = attr.config;

	ret = pfm_get_perf_event_encoding("MEM_LOAD_RETIRED.LOCAL_PMM", PFM_PLMH, &attr, NULL, NULL);
	if (ret != PFM_SUCCESS) {
		fprintf(stderr, "Cannot get encoding %s\n", pfm_strerror(ret));
		assert(ret == PFM_SUCCESS);
	}
	event2 = attr.config;

	ret = pfm_get_perf_event_encoding("MEM_INST_RETIRED.ALL_STORES", PFM_PLMH, &attr, NULL, NULL);
	if (ret != PFM_SUCCESS) {
		fprintf(stderr, "Cannot get encoding %s\n", pfm_strerror(ret));
		assert(ret == PFM_SUCCESS);
	}
	event3 = attr.config;

	printf("Events Numbers are: %x, %x, %x\n", event1, event2, event3);

	int r = pthread_create(&scan_thread, NULL, pebs_scan_thread, NULL);
	if (r) {
		fprintf(stderr, "pthread_create error!\n");
		assert(r == 0);
	}
	printf("pebs_init finished!\n");

	// Now this process should join_wait for its child thread
	void *ret_thread;
	int join_ret = pthread_join(scan_thread, &ret_thread);
	if (join_ret) {
		fprintf(stderr, "pthread_join error!\n");
		assert(join_ret == 0);
	}
	if (ret_thread != PTHREAD_CANCELED) {
		fprintf(stderr, "pthread_cancle fails!\n");
		assert(ret_thread == PTHREAD_CANCELED);
	}

	printf("Profiling shutdowned!\n");
}


long _perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	int ret;

	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
	
	return ret;
}

//struct perf_event_mmap_page* perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t tid, uint32_t index) {
void perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t tid, uint32_t index) {
	struct perf_event_attr attr;
	memset(&attr, 0, sizeof(struct perf_event_attr));
	attr.type = PERF_TYPE_RAW;
	attr.size = sizeof(struct perf_event_attr);
	attr.config = config;
	attr.config1 = config1;
	attr.sample_period = SAMPLE_PERIOD;
	attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_PHYS_ADDR;
	attr.disabled = 0;
	attr.exclude_kernel = 1;
	attr.exclude_hv = 1;
	attr.exclude_callchain_kernel = 1;
	attr.exclude_callchain_user = 1;
	attr.precise_ip = 1;

// int syscall(SYS_perf_event_open, struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags);
	pfd[index][type] = _perf_event_open(&attr, tid, -1, -1, 0);
	if (pfd[index][type] == -1) {

		if (errno == ESRCH) {
			// No such process
			return;
		}

		fprintf(stderr, "_perf_event_open error tid%u index%u type%d openingleader(%llx) errno(%d) reason(%s)\n", 
			tid, index, type, attr.config, errno, strerror(errno));
		assert(pfd[index][type] != -1);
	}
	//printf("success tid%u index%u type%d\n", tid, index, type);

	size_t mmap_size = sysconf(_SC_PAGESIZE) * PERF_PAGES;
	struct perf_event_mmap_page *p = 
		reinterpret_cast<struct perf_event_mmap_page *>(mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, pfd[index][type], 0));
	
	if (p == MAP_FAILED) {
		fprintf(stderr, "mmap for perf_event_mmap_page error\n");
		assert(p != MAP_FAILED);
	}

	perf_page[index][type] = p;
}

void *pebs_scan_thread(void *) {

	__label__ LABEL_CLEAN;

	int cancel_state = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (cancel_state) {
		fprintf(stderr, "thread cancel_state error\n");
		assert(cancel_state == 0);
	}

	int cancel_type = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (cancel_type) {
		fprintf(stderr, "thread cancel_type error\n");
		assert(cancel_type == 0);
	}
	
	while (true) {
		
		tcount = 0;
		dir = opendir(procpath);
		if (!dir) {
			fprintf(stderr, "opendir error\n");
			assert(dir != NULL);
		}
		
		// setup for perf_page[tindex][NPBUFTYPES]
		for (entity=readdir(dir); entity!=NULL; 
			entity=readdir(dir)) {
			if (entity->d_name[0] != '.') {
				strcpy(tidname, entity->d_name);
				if (atoi(tidname) > 0) {
					tid = uint32_t(atoi(tidname));
					perf_setup(event1, 0, DRAMREAD, tid, tcount);
					if (pfd[tcount][DRAMREAD] == -1)
						goto LABEL_CLEAN;

					perf_setup(event2, 0, NVMREAD, tid, tcount);
					if (pfd[tcount][NVMREAD] == -1)
						goto LABEL_CLEAN;

					perf_setup(event3, 0, WRITE, tid, tcount);
					if (pfd[tcount][WRITE] == -1)
						goto LABEL_CLEAN;

					++tcount;
				}else {
					fprintf(stderr, "invalid tidname[%s]\n", 
						tidname); assert(0);
				}
			} // end of if (entity->d_name[0] != '.')
		} // end of loop entity=readdir(dir)

		// repeat times sampling
		for (uint32_t times=0;times<10000;++times) {

		for(uint32_t i=0;i<tcount;++i) {
			for (uint32_t j=0;j<NPBUFTYPES;++j) {
				struct perf_event_mmap_page *p = perf_page[i][j];
				char *pbuf = (char *)p + p->data_offset;
				__sync_synchronize();
				
				if (p->data_head == p->data_tail) {
					continue;
				}

				struct perf_event_header *ph = 
					reinterpret_cast<struct perf_event_header*>(pbuf + (p->data_tail % p->data_size));
				struct perf_sample *ps;

				switch (ph->type) {
					case PERF_RECORD_SAMPLE:
						ps = (struct perf_sample*)ph;
						assert(ps != NULL);
						if (ps->addr != 0) {
							//printf("tid=%u %p\n", ps->tid, (void *)ps->addr);
							/*
							fprintf(fp, "pid=%u tid=%u weight=%llu addr=%p phys_addr=%p\n",
								ps->pid, ps->tid, ps->weight, (void *)ps->addr, (void *)ps->phys_addr);
							*/
							fprintf(fp, "%llu\n", ps->addr >> 12);

						}/*else {
							printf("Zero Page\n");
						}*/
						break;

					case PERF_RECORD_THROTTLE:
					case PERF_RECORD_UNTHROTTLE:
						break;
					default:
						fprintf(stderr, "Unknown type %u\n", ph->type);
						break;

				}

				p->data_tail += ph->size;
	
			} // end of loop NPBUFTYPES
		} // end of loop tcount

		//sleep(1);
		
		} // end of repeated sampling times

		LABEL_CLEAN:
		// finish 1 round sampling, clean the garbage
		for (uint32_t i=0;i<tcount;++i) {
			for (uint32_t j=0;j<NPBUFTYPES;++j) {
				if (perf_page[i][j]) {
					munmap(perf_page[i][j], sysconf(_SC_PAGESIZE)*PERF_PAGES);
					perf_page[i][j] = NULL;
				}
				if (pfd[i][j] != -1) {
					ioctl(pfd[i][j], PERF_EVENT_IOC_DISABLE, 0);
					close(pfd[i][j]);
					pfd[i][j] = -1;
				}
			}
		} // end of garbage clean
		closedir(dir);
		//sleep(1);
	}

	return NULL;
}

void INThandler(int sig) {
	signal(sig, SIG_IGN);
	printf("OUCH! Ctrl-C is pressed, we will quit!\n");

	int ret_cancel = pthread_cancel(scan_thread);
	if (ret_cancel) {
		fprintf(stderr, "pthread_cancel error!\n");
		assert(ret_cancel == 0);
	}

	fclose(fp);
	printf("Shutdowned!\n");
}



