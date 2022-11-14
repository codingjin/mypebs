#ifndef PEBS_H
#define PEBS_H

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <cstring>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <err.h>
#include <signal.h>
#include <dirent.h>
#include <cerrno>

#define PERF_PAGES (1 + (1 << 6))
#define SAMPLE_PERIOD 80

#define MAXCOUNT 1024

//extern struct perf_sample;
//extern enum pbuftype;
struct perf_sample {
	struct perf_event_header header;
	__u64 ip; /* if PERF_SAMPLE_IP */
	__u32 pid, tid; /* if PERF_SAMPLE_TID */
	__u64 addr; /* if PERF_SAMPLE_ADDR */
	__u64 weight; /* if PERF_SAMPLE_WEIGHT */
	__u64 phys_addr; /* if PERF_SAMPLE_PHYS_ADDR */
};

enum pbuftype {
	DRAMREAD = 0,
	NVMREAD = 1,
	WRITE = 2,
	NPBUFTYPES
};
extern void pebs_init(void);
extern long _perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);

//struct perf_event_mmap_page* perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t index);
extern void perf_setup(__u64 config, __u64 config1, __u64 type, uint32_t index);
extern void *pebs_scan_thread(void*);
extern void INThandler(int);

extern uint32_t tcount;
extern char procpath[MAXCOUNT];
extern DIR *dir;
extern struct dirent *entity;
extern char tidname[MAXCOUNT];
extern uint32_t tid;

extern __u64 event1, event2, event3;

extern struct perf_event_mmap_page *perf_page[MAXCOUNT][NPBUFTYPES];
extern int pfd[MAXCOUNT][NPBUFTYPES];

extern char filename[64];
extern FILE *fp; // for writingthe profiling result file
extern pthread_t scan_thread;

#endif
