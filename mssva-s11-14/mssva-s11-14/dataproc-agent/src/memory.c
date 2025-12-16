#include "memory.h"
#include "telemetry.h"
#include <stdlib.h>
#include <string.h>
#include <record.h>
#include <common.h>

char *process_record(record_t *rec, uint8_t flags) {
    char *buf = malloc(rec->length + 1);
    if (!buf)
        return NULL;

    if (flags & FAST_MODE) {
        memcpy(buf, rec->payload, rec->length + 1); // off-by-one
    } else {
        memcpy(buf, rec->payload, rec->length);
        buf[rec->length] = '\0';
    }
    return buf;
}

void cleanup_records(record_t *records, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        free(records[i].payload);
    }
    free(records);
}
