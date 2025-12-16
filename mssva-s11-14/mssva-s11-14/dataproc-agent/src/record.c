#include "record.h"
#include "telemetry.h"
#include <stdlib.h>

record_t *parse_records(FILE *fp, uint16_t count) {
    record_t *records = calloc(count, sizeof(record_t));
    if (!records) {
        sec_warn("record_alloc_failed");
        return NULL;
    }

    for (uint16_t i = 0; i < count; i++) {
        if (fread(&records[i].type, 1, 1, fp) != 1)
            break;

        fread(&records[i].length, 2, 1, fp);

        records[i].payload = malloc(records[i].length);
        if (!records[i].payload)
            break;

        fread(records[i].payload, 1, records[i].length, fp);
    }

    return records;
}
