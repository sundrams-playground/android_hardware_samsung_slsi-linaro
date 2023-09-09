/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "Memtrack.h"

#include <sys/types.h>
#include <dirent.h>

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

struct dmabuf_trace_memory {
    __u32 version;
    __u32 pid;
    __u32 count;
    __u32 type;
    __u32 *flags;
    __u32 *size_in_bytes;
    __u32 reserved[2];
};

#define DMABUF_TRACE_BASE   't'
#define DMABUF_TRACE_IOCTL_GET_MEMORY    _IOWR(DMABUF_TRACE_BASE, 0, struct dmabuf_trace_memory)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define min(x, y) ((x) < (y) ? (x) : (y))

static uint32_t available_flags[] = {
    MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_SHARED_PSS | MemtrackRecord::FLAG_SYSTEM | MemtrackRecord::FLAG_NONSECURE,
    MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_SHARED_PSS | MemtrackRecord::FLAG_DEDICATED | MemtrackRecord::FLAG_NONSECURE,
    MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_SHARED_PSS | MemtrackRecord::FLAG_SYSTEM | MemtrackRecord::FLAG_SECURE,
    MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_SHARED_PSS | MemtrackRecord::FLAG_DEDICATED | MemtrackRecord::FLAG_SECURE,
};

static uint32_t sgpu_available_flags[] = {
    MemtrackRecord::FLAG_SMAPS_ACCOUNTED | MemtrackRecord::FLAG_PRIVATE | MemtrackRecord::FLAG_NONSECURE,
    MemtrackRecord::FLAG_SMAPS_UNACCOUNTED | MemtrackRecord::FLAG_PRIVATE | MemtrackRecord::FLAG_NONSECURE,
};

const char DMABUF_TRACE_PATH[] = "/dev/dmabuf_trace";
static void getDmaBufMem(pid_t pid, std::vector<MemtrackRecord>* _aidl_return) {
    struct dmabuf_trace_memory data;
    unsigned int count = ARRAY_SIZE(available_flags);

    struct LocalSweeper {
        int fd;
        uint32_t *flags;
        uint32_t *size_in_bytes;

        LocalSweeper() : fd(0), flags(nullptr), size_in_bytes(nullptr) { }
        ~LocalSweeper() {
            if (fd)
                close(fd);

            if (flags)
                free(flags);

            if (size_in_bytes)
                free(size_in_bytes);
        }
    } sweeper;

    int fd = open(DMABUF_TRACE_PATH, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return;

    sweeper.fd = fd;

    data.flags = (uint32_t *)calloc(count, sizeof(int32_t));
    if (!data.flags)
        return;
    sweeper.flags = data.flags;

    data.size_in_bytes = (uint32_t *)calloc(count, sizeof(int32_t));
    if (!data.size_in_bytes)
        return;
    sweeper.size_in_bytes = data.size_in_bytes;

    data.count = count;
    data.type = (uint32_t)MemtrackType::GRAPHICS;
    data.pid = pid;
    for (size_t i = 0; i < count; i++)
        data.flags[i] = available_flags[i];

    int ret = ioctl(fd, DMABUF_TRACE_IOCTL_GET_MEMORY, &data);
    if (ret < 0)
        return;

    for (size_t i = 0; i < count; i++) {
        MemtrackRecord record = {
            .flags = static_cast<int32_t>(data.flags[i]),
            .sizeInBytes = static_cast<long>(data.size_in_bytes[i]),
        };
        _aidl_return->emplace_back(record);
    }
}

static void getGpuMem(pid_t pid, std::vector<MemtrackRecord>* _aidl_return) {
    size_t allocated_records = ARRAY_SIZE(sgpu_available_flags);
    FILE *fp;
    char line[1024] = {0, }, mem_type[16] = {0, };
    int r, cur_pid;
    size_t mem_size = 0;

    /* fastpath to return the necessary number of
     * * records */
    if (allocated_records == 0)
	return;

    fp = fopen("/sys/kernel/gpu/mem_info", "r");

    if (fp == NULL)
	return;

    while(1) {
	if (fgets(line, sizeof(line), fp) == NULL)
	    break;

	r = sscanf(line, "%s", mem_type);
	if (!strcmp(mem_type, "pid:")) {
	    r = sscanf(line, "%*s %d %zu\n", &cur_pid, &mem_size);
	    if (!r) {
		    fclose(fp);
		    return;
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

    if (allocated_records > 0) {
	MemtrackRecord record = {
	    .flags = static_cast<int32_t>(sgpu_available_flags[0]),
	    .sizeInBytes = static_cast<long>(0),
	};
	_aidl_return->emplace_back(record);
    }
    if (allocated_records > 1) {
	MemtrackRecord record = {
	    .flags = static_cast<int32_t>(sgpu_available_flags[1]),
	    .sizeInBytes = static_cast<long>(mem_size),
	};
	_aidl_return->emplace_back(record);
    }
}

ndk::ScopedAStatus Memtrack::getMemory(int pid, MemtrackType type,
                                       std::vector<MemtrackRecord>* _aidl_return) {
    struct stat st;

    if (pid < 0) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
    if (type != MemtrackType::OTHER && type != MemtrackType::GL && type != MemtrackType::GRAPHICS &&
        type != MemtrackType::MULTIMEDIA && type != MemtrackType::CAMERA) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    _aidl_return->clear();

    switch (type) {
	case MemtrackType::GL:
	    if (!stat("/sys/kernel/gpu", &st) && S_ISDIR(st.st_mode))
		getGpuMem(pid, _aidl_return);
	    break;
	case MemtrackType::GRAPHICS:
	    getDmaBufMem(pid, _aidl_return);
	    break;
	default:
	    break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Memtrack::getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) {
    DeviceInfo dev_info = {.id = 0, .name = "exynos"};
    FILE *fp;
    char gpu_model[32] = {0, }, dev_type[32] = {0, }, *r = 0;
    int i = 0;

    fp = fopen("/sys/kernel/gpu/gpu_model", "r");
    if (fp != NULL) {
	r = fgets(gpu_model, sizeof(gpu_model), fp);
	while (gpu_model[i]) {
		if (gpu_model[i] == '_') {
			dev_type[i] = '\0';
			break;
		}
		dev_type[i] = gpu_model[i];
		i++;
	}
	fclose(fp);
    }

    _aidl_return->clear();

    if (r && !strcmp(dev_type, "AMDGPU"))
	dev_info = {.id = 0, .name = gpu_model};

    _aidl_return->emplace_back(dev_info);
    return ndk::ScopedAStatus::ok();
}

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
