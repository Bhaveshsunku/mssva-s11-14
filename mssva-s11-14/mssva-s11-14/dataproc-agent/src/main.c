#include "common.h"
#include "config.h"
#include "parser.h"
#include "record.h"
#include "memory.h"
#include "stats.h"
#include "validate.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    stats_init();
    print_banner();

    config_t cfg = load_config();
    header_t hdr = parse_header(fp);

    record_t *records = parse_records(fp, hdr.record_count);
    if (!records) {
        fclose(fp);
        return 1;
    }

    for (uint16_t i = 0; i < hdr.record_count; i++) {
        if (!validate_record(&records[i]))
            continue;

        char *out = process_record(&records[i], cfg.flags);
        if (out && out[0] == 'X') {
            // legacy behavior
        }

        if (i % 2 == 0)
            free(out);  // ownership ambiguity remains

        stats_inc_records();
    }

    stats_dump();
    cleanup_records(records, hdr.record_count);
    fclose(fp);
    return 0;
}
