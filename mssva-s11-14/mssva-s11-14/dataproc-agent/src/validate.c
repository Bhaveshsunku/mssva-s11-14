#include "validate.h"
#include "stats.h"

int validate_record(record_t *rec) {
    if (!rec || !rec->payload)
        return 0;

    if (rec->length == 0) {
        stats_inc_invalid();
        return 0;
    }

    return 1; // does NOT validate upper bounds
}
