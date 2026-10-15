/* Reuse genuine immutable master pins and independently obtained live leases.
 * Only ordinary descriptive input copies change; no owner/cache state repair. */
#define PT_DELIVERY4_EXTERNAL_MAIN
#include "paula_delivery4_genuine_owner_test.c"
static void cache_source_pairs(void)
{
    struct d4_fixture x,before;struct pt_paula_delivery4_action out,saved;
    struct pt_paula_segment_source dependency[3];struct pt_paula_future_packet packet;
    enum pt_paula_delivery4_snapshot_result result;unsigned sample,cache,cases=0;
    for(sample=0;sample<3;++sample)for(cache=0;cache<3;++cache){
        d4_setup(&x);memcpy(&before,&x,sizeof(before));
        memcpy(dependency,x.dependency,sizeof(dependency));memcpy(&packet,&x.packet,sizeof(packet));
        /* All three leases really exist, have the same current cache version
         * and extent, and remain pinned. Equal length/version is insufficient
         * to pair another sample's converted bytes with this genuine master. */
        assert(dependency[sample].cache_bytes==dependency[cache].cache_bytes);
        dependency[sample].lease=dependency[cache].lease;
        dependency[sample].cache_data=dependency[cache].cache_data;
        dependency[sample].cache_bytes=dependency[cache].cache_bytes;
        if(sample==0)memcpy(packet.geometry.source,dependency,sizeof(*dependency));
        memset(&out,0x59,sizeof(out));memcpy(&saved,&out,sizeof(saved));
        result=d4_call(&x,&packet,x.domain,dependency,x.f.pin,
            &x.f.config.grid,x.f.config.frame,0,&out);
        if(sample==cache){
            assert(result==PT_PAULA_DELIVERY4_SNAPSHOT_OK);
            assert(!memcmp(&out.packet,&packet,sizeof(packet))&&out.domain==x.domain);
        }else{
            if(result!=PT_PAULA_DELIVERY4_SNAPSHOT_STALE)
                fprintf(stderr,"cache pairing refusal missing: master=%u cache=%u result=%u\n",
                    sample,cache,(unsigned)result);
            assert(result==PT_PAULA_DELIVERY4_SNAPSHOT_STALE);
            assert(!memcmp(&out,&saved,sizeof(out)));
        }
        d4_audit_equal(&x,&before);d4_finish(&x,cases&1U);++cases;
    }
    assert(cases==9);
}
int main(void)
{
    assert(pt_delivery4_fixture_main()==0);cache_source_pairs();
    puts("delivery4 cache binding: 9 cases; 3 genuine controls, 6 wrong-source live leases refused");
    return 0;
}
