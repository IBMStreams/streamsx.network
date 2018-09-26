/*********************************************************************
 * Copyright (C) 2018 International Business Machines Corporation
 * All Rights Reserved
 ********************************************************************/

#ifndef INSTRUMENTATION_H_
#define INSTRUMENTATION_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <x86intrin.h>




#define INST_ENABLE

#define INST_BUCKET_SIZE (60UL * 2194707391UL)
#define INST_BUCKET_COUNT 10
#define INST_SCRATCH_SIZE 4096
#define INST_METRIC_COUNT 2
#ifdef INST_ENABLE
#define INST_TS(d) d = __rdtsc()
#define INST_UPDATE_METRIC(v, m, d) instAddToMetric(&v[v##_index].metrics[m], (d))
#define INST_SET_BUCKET_START(v, d) v##_startTime) = d
#define INST_BUCKETS_DEFINE(v)                    \
    InstBucket v[INST_BUCKET_COUNT];              \
    size_t v##_index;                             \
    uint64_t v##_startTime

#define INST_BUCKETS_CLEAR(v)                                           \
    memset(v, 0, sizeof(InstBucket) * INST_BUCKET_COUNT);               \
    {                                                                   \
        size_t inst_i;                                                  \
        for(inst_i = 0; inst_i < INST_BUCKET_COUNT; ++inst_i) {         \
            size_t inst_j;                                              \
            for(inst_j = 0; inst_j < INST_METRIC_COUNT; ++inst_j) {     \
                v[inst_i].metrics[inst_j].min = UINT64_MAX;             \
            }                                                           \
        }                                                               \
    }                                                                   \
    v##_startTime = 0;                                                  \
    v##_index = 0

#define INST_BUCKETS_HANDLE(v, d, t, i, q)           \
    q = false;                                       \
    if(d - v##_startTime >= INST_BUCKET_SIZE) {      \
        v[v##_index].duration = d - v##_startTime;   \
        ++v##_index;                                 \
        if(v##_index == INST_BUCKET_COUNT) {         \
            instDumpMetricsBuckets(v, t, i);         \
            INST_BUCKETS_CLEAR(v);                   \
            INST_TS(d);                              \
            q = true;                                \
        }                                            \
        v##_startTime = d;                           \
    }



#else
#define INST_TS(d)
#define INST_UPDATE_METRIC(v, m, d)
#define INST_SET_BUCKET_START(v, d)
#define INST_BUCKETS_DEFINE(v)
#define INST_BUCKETS_CLEAR(v)
#define INST_BUCKETS_HANDLE(v, d, t, i, q) q = false

#endif

struct InstMetric {
    uint64_t count;
    uint64_t total;
    uint64_t squareTotal;
    uint64_t min;
    uint64_t max;
};

struct InstBucket {
    uint64_t duration;
    struct InstMetric metrics[INST_METRIC_COUNT];
};

inline __attribute__((always_inline))
void instAddToMetric(struct InstMetric *m, uint64_t datum) {
    ++m->count;
    m->total += datum;
    m->squareTotal += datum * datum;
    if(datum < m->min) {
        m->min = datum;
    }
    if(datum > m->max) {
        m->max = datum;
    }
}

inline __attribute__((always_inline))
void instDumpMetricsBuckets(struct InstBucket *b, const char * tag, size_t index) {
    char scratch[INST_SCRATCH_SIZE];
    size_t i = 0;
    size_t j = 0;

    struct timespec ts_real;
    clock_gettime(CLOCK_REALTIME, &ts_real);

    for(i = 0; i < INST_BUCKET_COUNT; ++i) {
        // Display the data for this bucket/thread to stdout for now
        size_t offset = 0;
        offset += snprintf(scratch + offset, INST_SCRATCH_SIZE - offset, "%s: METRICS %lu %lu.%09lu-%lu %lu",
                           tag, (uint64_t)index, ts_real.tv_sec, ts_real.tv_nsec, (uint64_t)INST_BUCKET_COUNT - i - 1, b[i].duration);
        for(j = 0; j < INST_METRIC_COUNT; ++j) {
            offset += snprintf(scratch + offset, INST_SCRATCH_SIZE - offset, " : %lu %lu %lu %lu %lu",
                               b[i].metrics[j].count, b[i].metrics[j].total, b[i].metrics[j].squareTotal, b[i].metrics[j].min, b[i].metrics[j].max);
        }
        puts(scratch);
    }
}









#endif
