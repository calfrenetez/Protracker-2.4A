/* Optional native32 regression slice. No audio, GUI, service or shared files.
 * Root supplies an existing, fresh, empty RAM: directory. Refusal preserves
 * evidence: assertions do not retry saves or remove uncertain staging. */
#define PT_TEST_NATIVE_MEMORY_FIXED_MARKERS 1
#include "native_exec_memory.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#define malloc native_allocate
#define free native_release
#include "format_hardening/ledger.h"
#include "mod_project.h"
#include "../src/platform/project_file.h"
#include "../src/platform/mod_file.h"

static struct fh_ledger ledger;
static struct pt_allocator allocator = {&ledger, fh_alloc, fh_free};
static struct pt_document document, saved_document;
static size_t saved_live;
static uint16_t order;
static struct pt_event seed_events[256];
static struct pt_sample seed_sample;
static struct pt_project seed;
static int32_t seed_pcm[16];
static uint8_t ptg[8192], mod[2112], damaged[8192], conversion[2125];
static size_t ptg_bytes;

static void guard_begin(void)
{
    memcpy(&saved_document, &document, sizeof(document));
    saved_live = ledger.live;
    fh_snapshot(&ledger);
}
static void guard_check(void)
{
    assert(ledger.live == saved_live);
    assert(!memcmp(&saved_document, &document, sizeof(document)));
    fh_check_snapshot(&ledger);
}
static void guard_end(void)
{
    guard_check();
    fh_drop_snapshot(&ledger);
}
static void make_inputs(void)
{
    unsigned i;
    memset(&seed, 0, sizeof(seed));
    pt_channels_init(&seed.channels);
    seed.order_count = seed.pattern_count = seed.sample_count = 1;
    seed.bpm = 125; seed.speed = 6;
    seed.orders = &order; seed.events = seed_events; seed.samples = &seed_sample;
    seed_sample.pcm = (struct pt_pcm){seed_pcm,16,16,PT_CLASSIC_RATE,1,24};
    seed_sample.volume = 64;
    for(i = 0; i < 16; ++i)
        seed_pcm[i] = i % 4 == 0 ? -8388608 : i % 4 == 1 ? -1 : i % 4 == 2 ? 0 : 8388607;
    assert(pt_project_encode(&seed, ptg, sizeof(ptg), &ptg_bytes) == PT_PROJECT_OK);
    memset(mod, 0, sizeof(mod));
    for(i = 0; i < 31; ++i) mod[20+i*30+29] = 1;
    mod[43] = 2; mod[45] = 64; mod[950] = 1; mod[951] = 127;
    memcpy(mod+1080, "M.K.", 4);
    mod[2108] = 128; mod[2109] = 255; mod[2110] = 0; mod[2111] = 127;
    pt_document_init(&document, &allocator);
    assert(pt_document_load(&document, ptg, ptg_bytes, 1048576) == PT_PROJECT_OK);
    document.dirty = 1;
    assert(document.project.samples[0].pcm.bits == 24);
}
static void refused_import(const uint8_t *bytes, size_t n, size_t budget)
{
    guard_begin();
    assert(pt_document_load(&document, bytes, n, budget) != PT_PROJECT_OK);
    guard_end();
}
static void malformed_and_allocation(void)
{
    struct fh_ledger trial = {0};
    struct pt_allocator a = {&trial,fh_alloc,fh_free};
    struct pt_document next;
    const uint8_t bad_pp20[12] = {'P','P','2','0',9,9,9,9,0,0,4,33};
    const size_t cuts[] = {0,1,31,1083,2107,2111};
    size_t i, allocations;
    for(i = 0; i < sizeof(cuts)/sizeof(cuts[0]); ++i)
        refused_import(mod, cuts[i], 1048576);
    refused_import(ptg, 31, 1048576);
    refused_import(ptg, ptg_bytes-1, 1048576);
    memcpy(damaged, ptg, ptg_bytes); damaged[ptg_bytes-1] ^= 1;
    refused_import(damaged, ptg_bytes, 1048576);
    refused_import(bad_pp20, sizeof(bad_pp20), 1048576);
    refused_import(mod, sizeof(mod), 0);
    pt_document_init(&next, &a);
    assert(pt_document_load(&next, mod, sizeof(mod), 1048576) == PT_PROJECT_OK);
    allocations = trial.calls;
    assert(allocations && allocations <= 8);
    pt_document_release(&next); assert(!trial.live);
    for(i = 1; i <= allocations; ++i) {
        fh_reset(&ledger, i);
        refused_import(mod, sizeof(mod), 1048576);
        assert(ledger.triggered == 1);
    }
    fh_reset(&ledger, 0);
    fh_reset(&trial, allocations+1);
    pt_document_init(&next, &a);
    assert(pt_document_load(&next, mod, sizeof(mod), 1048576) == PT_PROJECT_OK);
    assert(trial.calls == allocations && !trial.triggered);
    pt_document_release(&next); assert(!trial.live);
    puts("FH32 IMPORT PASS: malformed MOD/PTG/PP20 and each measured allocation refusal preserve full dirty24 master");
}
static void explicit_precision(void)
{
    unsigned bits;
    size_t i, written;
    struct pt_mod_export_report report;
    for(bits = 16; bits <= 24; bits += 8) {
        struct pt_document d;
        struct fh_ledger l = {0};
        struct pt_allocator a = {&l,fh_alloc,fh_free};
        struct pt_document before;
        seed_sample.pcm.bits = (uint8_t)bits;
        for(i = 0; i < 16; ++i) seed_pcm[i] = (i & 1) ? 1 : -1;
        assert(pt_project_encode(&seed, ptg, sizeof(ptg), &ptg_bytes) == PT_PROJECT_OK);
        pt_document_init(&d, &a);
        assert(pt_document_load(&d, ptg, ptg_bytes, 1048576) == PT_PROJECT_OK);
        memcpy(&before, &d, sizeof(d)); fh_snapshot(&l);
        assert(pt_mod_export_analyse(&d.project, &report) == PT_PROJECT_OK);
        assert(report.issues == PT_EXPORT_PRECISION && !report.bytes);
        assert(report.classification == PT_CONVERSION_CONVERTED);
        memset(conversion, 0xa5, sizeof(conversion)); written = 123;
        assert(pt_mod_export_direct(&d.project, conversion, sizeof(conversion), &written) == PT_PROJECT_UNSUPPORTED);
        assert(written == 123);
        for(i = 0; i < sizeof(conversion); ++i) assert(conversion[i] == 0xa5);
        assert(pt_mod_export_round8(&d.project, conversion, sizeof(conversion), &written) == PT_PROJECT_OK);
        assert(written == 2124 && conversion[42] == 0 && conversion[43] == 8 && conversion[2124] == 0xa5);
        assert(pt_mod_export_tpdf8(&d.project, conversion, sizeof(conversion), &written) == PT_PROJECT_OK && written == 2124);
        assert(!memcmp(&before, &d, sizeof(d))); fh_check_snapshot(&l);
        fh_drop_snapshot(&l); pt_document_release(&d); assert(!l.live);
    }
    puts("FH32 PRECISION PASS: direct16/24 refused, explicit round8/TPDF encode preserve full masters");
}
static void word_boundaries(void)
{
    const uint32_t lengths[] = {0,2,3,131070,131072};
    struct pt_project p;
    struct pt_sample sample, before_sample;
    struct pt_mod_export_report report;
    int32_t *values = malloc(131072UL*sizeof(*values));
    uint8_t *out = malloc(133181);
    size_t k, i, written;
    memset(values, 0, 131072UL*sizeof(*values));
    memset(&p, 0, sizeof(p)); memset(&sample, 0, sizeof(sample));
    pt_channels_init(&p.channels); p.order_count = p.pattern_count = p.sample_count = 1;
    p.bpm = 125; p.speed = 6; p.orders = &order; p.events = seed_events; p.samples = &sample;
    sample.pcm = (struct pt_pcm){values,131072,0,PT_CLASSIC_RATE,1,8}; sample.volume = 64;
    for(k = 0; k < sizeof(lengths)/sizeof(lengths[0]); ++k) {
        sample.pcm.frames = lengths[k]; memcpy(&before_sample, &sample, sizeof(sample));
        assert(pt_mod_export_analyse(&p, &report) == PT_PROJECT_OK);
        memset(out, 0xa5, 133181); written = 77;
        if(lengths[k] == 3 || lengths[k] == 131072) {
            assert(report.issues == PT_EXPORT_LIMITS && !report.bytes);
            assert(pt_mod_export_direct(&p, out, 133180, &written) == PT_PROJECT_UNSUPPORTED && written == 77);
            for(i = 0; i < 133181; ++i) assert(out[i] == 0xa5);
        } else {
            uint32_t words = lengths[k]/2;
            assert(!report.issues && report.bytes == 2108UL+lengths[k]);
            assert(pt_mod_export_direct(&p, out, 133180, &written) == PT_PROJECT_OK && written == report.bytes);
            assert(out[42] == (uint8_t)(words>>8) && out[43] == (uint8_t)words && out[written] == 0xa5);
        }
        assert(!memcmp(&sample, &before_sample, sizeof(sample)));
        for(i = 0; i < 131072; ++i) assert(values[i] == 0);
    }
    sample.pcm.frames = 131070; sample.loop = PT_LOOP_FORWARD;
    sample.loop_start = 131066; sample.loop_end = 131070;
    assert(pt_mod_export_direct(&p, out, 133180, &written) == PT_PROJECT_OK);
    assert(out[42] == 255 && out[43] == 255 && out[46] == 255 && out[47] == 253 && out[48] == 0 && out[49] == 2);
    sample.pcm.frames = 8; sample.loop_start = 1; sample.loop_end = 7;
    assert(pt_mod_export_analyse(&p, &report) == PT_PROJECT_OK && report.issues == PT_EXPORT_LOOPS && !report.bytes);
    memset(out, 0xa5, 133181); written = 77;
    assert(pt_mod_export_direct(&p, out, 133180, &written) == PT_PROJECT_UNSUPPORTED && written == 77);
    for(i = 0; i < 133181; ++i) assert(out[i] == 0xa5);
    free(out); free(values);
    puts("FH32 WORDS PASS: zero/one/65535 words, odd/65536-word refusal and exact loop words");
}
struct producer {
    pt_file_sink sink; void *sink_context;
    unsigned calls,fail_call,chunks;
};
static int forward_project(void *context, const uint8_t *data, size_t n)
{
    struct producer *p = context;
    ++p->chunks;
    if(p->calls == p->fail_call && p->chunks == 2) return 0;
    return p->sink(p->sink_context, data, n);
}
static int produce_project(void *context, pt_file_sink sink, void *sink_context)
{
    struct producer *p = context; size_t written = 0;
    ++p->calls; p->chunks = 0; p->sink = sink; p->sink_context = sink_context;
    return pt_project_stream(&document.project, forward_project, p, &written) == PT_PROJECT_OK;
}
static void path_for(char *out, size_t size, const char *dir, const char *leaf)
{
    int n = snprintf(out, size, "%s/%s", dir, leaf);
    assert(n > 0 && (size_t)n < size);
}
static void no_file_or_temp(const char *path)
{
    char temp[900]; unsigned i;
    errno = 0; assert(access(path, F_OK) == -1 && errno == ENOENT);
    for(i = 0; i < 32; ++i) {
        int n = snprintf(temp, sizeof(temp), "%s.pttmp-%lu-%u", path, (unsigned long)getpid(), i);
        assert(n > 0 && (size_t)n < sizeof(temp));
        errno = 0; assert(access(temp, F_OK) == -1 && errno == ENOENT);
    }
}
static void failed_saves(const char *dir)
{
    static const uint8_t sentinel[] = "FH32 EXISTING DESTINATION";
    char path[800]; uint8_t check[sizeof(sentinel)];
    FILE *f; int fd; unsigned policy; size_t n; struct producer producer;
    path_for(path, sizeof(path), dir, "refused.bin"); no_file_or_temp(path);
    guard_begin();
    fh_reset(&ledger, 1);
    assert(pt_project_file_save(path, &document.project, &allocator) == PT_SAVE_MEMORY);
    guard_check(); no_file_or_temp(path);
    for(policy = 1; policy <= 2; ++policy) {
        fh_reset(&ledger, 1);
        assert(pt_mod_file_save(path, &document.project, policy, &allocator) == PT_SAVE_MEMORY);
        guard_check(); no_file_or_temp(path);
    }
    fh_reset(&ledger, 0);
    assert(pt_mod_file_save(path, &document.project, 0, &allocator) == PT_SAVE_INVALID);
    guard_check(); no_file_or_temp(path);
    assert(pt_project_size(&document.project, &n) == PT_PROJECT_OK);
    producer = (struct producer){0}; producer.fail_call = 1;
    assert(pt_file_save_streamed(path, n, produce_project, &producer, &allocator) == PT_SAVE_WRITE && producer.calls == 1);
    guard_check(); no_file_or_temp(path);
    producer = (struct producer){0}; producer.fail_call = 2;
    assert(pt_file_save_streamed(path, n, produce_project, &producer, &allocator) == PT_SAVE_VERIFY && producer.calls == 2);
    guard_check(); no_file_or_temp(path);
    path_for(path, sizeof(path), dir, "existing.bin"); no_file_or_temp(path);
    fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0600); assert(fd >= 0);
    assert(write(fd, sentinel, sizeof(sentinel)) == (long)sizeof(sentinel) && !close(fd));
    for(policy = 0; policy <= 2; ++policy) {
        enum pt_save_result result = policy == 0 ? pt_project_file_save(path, &document.project, &allocator) :
            pt_mod_file_save(path, &document.project, policy, &allocator);
        assert(result == PT_SAVE_PUBLISH); guard_check();
        f = fopen(path, "rb"); assert(f && fread(check, 1, sizeof(check), f) == sizeof(check));
        assert(fgetc(f) == EOF && !ferror(f) && !fclose(f));
        assert(!memcmp(check, sentinel, sizeof(sentinel)));
    }
    assert(!unlink(path)); no_file_or_temp(path);
    guard_end();
    puts("FH32 SAVE PASS: allocation/write/verify/publish refusals preserve full dirty24 master and existing destination");
}
int main(int argc, char **argv)
{
    assert(CHAR_BIT == 8 && sizeof(void *) == 4 && sizeof(size_t) == 4 && sizeof(long) == 4 && sizeof(int) == 4);
    assert(sizeof(uint16_t) == 2 && sizeof(uint32_t) == 4 && sizeof(int32_t) == 4);
    assert(argc == 2 && !strncmp(argv[1], "RAM:", 4) && strlen(argv[1]) > 4 && strlen(argv[1]) < 700);
    assert(!strstr(argv[1], "..") && argv[1][strlen(argv[1])-1] != '/');
    native_memory_start();
    puts("FH32 ABI PASS: 8-bit bytes and 32-bit pointer/size_t/long/int");
    make_inputs(); malformed_and_allocation(); explicit_precision(); word_boundaries(); failed_saves(argv[1]);
    fh_reset(&ledger, 0); pt_document_release(&document); assert(!ledger.live);
    native_memory_finish();
    puts("FH32 COMPLETE: optional native import/save/classic-MOD slice; no audio or GUI");
    return 0;
}
