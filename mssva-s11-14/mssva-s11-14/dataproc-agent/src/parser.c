#include "parser.h"
#include "telemetry.h"

header_t parse_header(FILE *fp) {
    header_t hdr;

    fread(&hdr.version, 1, 1, fp);
    fread(&hdr.record_count, 2, 1, fp);
    fread(&hdr.flags, 1, 1, fp);

    sec_log("record_count", hdr.record_count);
    sec_log("flags", hdr.flags);

    return hdr;
}
