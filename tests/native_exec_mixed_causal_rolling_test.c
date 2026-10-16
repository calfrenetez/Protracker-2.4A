/* Native-CPU entry for the unchanged genuine rolling68 software fixture.
 * Only its exact terminal success oracle is mapped to a short capture marker.
 * All original assertions/diagnostics stay live. Clocks/buses remain MODELS;
 * no CIA, interrupts, output, audio, MMIO or physical memory-placement proof. */
/* The four donor suite entries are never invoked by this native entry.
 * Prior internal declarations preserve their original definitions and all
 * genuine rolling68 calls, while allowing unused donor-only code to be omitted.
 * This changes no cases, assertions, compiler warnings or staging limits. */
static int ct_committed_resource_fixture_not_called(void) __attribute__((unused));
static int inherited_causal_pair_suite_not_called(void) __attribute__((unused));
static int cs_typed43_donor_not_called(void) __attribute__((unused));
static int sl_original_producer62_not_called(void) __attribute__((unused));
#define puts pt_rolling_native_fixture_puts
#define PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY pt_rolling_original_68
#include "native_mixed_causal_ram_rolling_song_test.c"
#undef PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY
#undef puts
#include "../src/editor/sampler_establish_inventory_internal.h"
int puts(const char *);
int pt_rolling_native_fixture_puts(const char *text)
{
    assert(text && !strcmp(text, "ROLLING RAM T16-C16-C16-S16 HOST FIXTURE PASS:68 cases;genuine producer-factory-controller-core;both C row reuse after actual quiet-detach-NULL disposal;root-latest actual histories;full20 registry;original16 readers and masters-cache leases;ownSTOP admission closed;root-empty-reused no-arm tombstones and sticky attempted-arm quiet through corrupted profile;cancelled-retired-draining-owner-current refusals;exact960-1920-2880-3840;SOFTWARE_ONLY"));
    return puts("ROLLING RAM NATIVE PASS:68 cases;T16-C16-C16-S16;actual row retirement and original reader pins;960-1920-2880-3840;MODELED CLOCK/BUS ONLY");
}
static void pt_rolling_header_byte_controls(void)
{
    struct pt_sampler original,changed;
    struct field {size_t offset,bytes;} field[]={
#define PT_FIELD(name) {offsetof(struct pt_sampler,name),sizeof(original.name)}
        PT_FIELD(current),PT_FIELD(bytes),PT_FIELD(live_head),PT_FIELD(live_count),
        PT_FIELD(pending_head),PT_FIELD(pending_count)
#undef PT_FIELD
    };
    size_t i,k;unsigned permitted;
    /* Only the raw comparator is called. Reference the private header's full
     * static graph without traversing forged sampler topology. */
    (void)pt_sampler_establish_inventory_current;
    memset(&original,0,sizeof(original));
    assert(pt_establish_inventory_header_equal(&original,&original));
    for(i=0;i<sizeof(original);++i){
        memcpy(&changed,&original,sizeof(changed));((unsigned char *)&changed)[i]=1;
        permitted=0;
        for(k=0;k<sizeof(field)/sizeof(field[0]);++k)
            if(i>=field[k].offset && i<field[k].offset+field[k].bytes)permitted=1;
        assert(pt_establish_inventory_header_equal(&changed,&original)==(int)permitted);
    }
    assert(!pt_establish_inventory_field_byte(0,SIZE_MAX,1));
    assert(!pt_establish_inventory_field_byte(SIZE_MAX,0,SIZE_MAX));
    assert(pt_establish_inventory_field_byte(SIZE_MAX,SIZE_MAX,1));
}
#ifndef PT_PRIVATE_ROLLING_NATIVE_ENTRY
#define PT_PRIVATE_ROLLING_NATIVE_ENTRY main
#endif
int PT_PRIVATE_ROLLING_NATIVE_ENTRY(void)
{
    assert(sizeof(uint64_t)==8 && sizeof(uint32_t)==4);
    pt_rolling_header_byte_controls();
    return pt_rolling_original_68();
}
