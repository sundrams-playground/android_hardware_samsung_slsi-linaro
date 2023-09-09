#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <log/log.h>

#include <hardware/memtrack.h>

#include "memtrack_exynos.h"

/* Following includes added for directory parsing. */
#include <sys/types.h>
#include <dirent.h>

/* Some general defines. */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

struct memtrack_record sgpu_record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

int sgpu_memtrack_get_memory(pid_t pid, int __unused type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
    size_t allocated_records = min(*num_records, ARRAY_SIZE(sgpu_record_templates));
    FILE *fp;
    char line[1024] = {0}, mem_type[16] = {0};
    int r, cur_pid;
    size_t mem_size = 0;

    *num_records = ARRAY_SIZE(sgpu_record_templates);

    /* fastpath to return the necessary number of records */
    if (allocated_records == 0)
        return 0;

    memcpy(records, sgpu_record_templates,
           sizeof(struct memtrack_record) * allocated_records);

    fp = fopen("/sys/kernel/gpu/mem_info", "r");

    if (fp == NULL)
	return 0;

    while(1) {
	if (fgets(line, sizeof(line), fp) == NULL)
	    break;

	r = sscanf(line, "%s", mem_type);
	if (!strcmp(mem_type, "pid:")) {
	    r = sscanf(line, "%*s %d %zu\n", &cur_pid, &mem_size);
	    if (!r) {
		fclose(fp);
	        return r;
	    }

	    if(cur_pid != pid)
		continue;
	    else
		break;
	}
	mem_size = 0;
	break;
    }

    if (fp != NULL)
	    fclose(fp);

    if (allocated_records > 0)
        records[0].size_in_bytes = 0;
    if (allocated_records > 1)
        records[1].size_in_bytes = mem_size;

    return 0;
}
