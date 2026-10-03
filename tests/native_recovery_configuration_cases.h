/* Included after the native controller implementation. No SDK/assert headers
 * are re-included: preserve the fixture's RC20 resource-cleanup assertion.
 * GetVar/time remain fixture substitutions; Lock/Examine/NameFromLock are real
 * DOS operations on the existing owned RAM workspace and source file. */
static void native_recovery_configuration_owner_refusal(struct pt_native_recovery *r)
{
    struct pt_native_recovery before;struct pt_native_recovery_configuration c;
    assert(r->store.opened && r->store.owned);
    memcpy(&before,r,sizeof(before));assert(pt_native_recovery_get_configuration(r,&c));
    assert(!pt_native_recovery_apply_configuration(r,&c));
    assert(!memcmp(r,&before,sizeof(before)));
    c.enabled=0;c.directory[0]=0;c.media=PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;
    assert(!pt_native_recovery_apply_configuration(r,&c));
    assert(!memcmp(r,&before,sizeof(before)));
    assert(!pt_native_recovery_configure(r,&r->allocator));
    assert(!memcmp(r,&before,sizeof(before)));
}
static void native_recovery_configuration_cases(const char *directory,const char *source,
    const struct pt_project *project,const struct pt_allocator *allocator)
{
    struct pt_native_recovery r={0},before,restored;
    struct pt_native_recovery_configuration c,draft,out,image;
    struct pt_recovery_candidate found,found_image;
    char canonical[PT_NATIVE_RECOVERY_ROOT_SIZE],missing[PT_NATIVE_RECOVERY_ROOT_SIZE];
    BPTR lock;struct FileInfoBlock info __attribute__((aligned(4)));size_t i;int examined,canonicalized;
    /* Valid self-aliasing allocator input; no caller allocator use is needed. */
    r.allocator=*allocator;assert(pt_native_recovery_configure(&r,&r.allocator));
    assert(r.allocator.context==allocator->context && r.allocator.allocate==allocator->allocate && r.allocator.release==allocator->release);
    assert(pt_native_recovery_get_configuration(&r,&c));
    lock=Lock((STRPTR)directory,ACCESS_READ);assert(lock);
    examined=Examine(lock,&info) && info.fib_DirEntryType>0;
    canonicalized=examined && NameFromLock(lock,(STRPTR)canonical,sizeof(canonical));UnLock(lock);
    assert(examined && canonicalized);
    assert(!strcmp(c.directory,canonical) && c.enabled && c.media==PT_NATIVE_RECOVERY_MEDIA_FIXED && c.interval_seconds==30);
    assert(pt_native_recovery_bind(&r,source) && r.bound && !r.store.opened);
    r.project=project;r.schedule.armed=1;r.schedule.since=20;r.schedule.observed=41;
    r.schedule.have_snapshot=1;r.schedule.snapshot_revision=13;r.schedule.snapshot_saved_revision=6;
    memcpy(&before,&r,sizeof(before));
    assert(pt_native_recovery_get_configuration(&r,&draft));draft.enabled=0;draft.interval_seconds=45;
    /* Abandoning this copied draft is cancellation; no owner publication. */
    assert(!memcmp(&r,&before,sizeof(r)));
    assert(strlen(c.directory)+1<sizeof(c.directory));c.directory[strlen(c.directory)+1]='x';
    memcpy(&image,&c,sizeof(image));
    assert(pt_native_recovery_apply_configuration(&r,&c));
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&c,&image,sizeof(c)));
    assert(pt_native_recovery_get_configuration(&r,&c));
    {
        void *aliases[]={&r,r.root,&r.schedule,&r.store,(unsigned char *)&r+sizeof(r)-1};
        for(i=0;i<sizeof(aliases)/sizeof(*aliases);++i) {
            assert(!pt_native_recovery_get_configuration(&r,aliases[i]));
            assert(!pt_native_recovery_apply_configuration(&r,aliases[i]));
            assert(!memcmp(&r,&before,sizeof(r)));
        }
    }
    for(i=0;i<4;++i) {
        struct pt_native_recovery held;
        memcpy(&r,&before,sizeof(r));
        if(i==0)r.schedule.busy=1;
        if(i==1)r.store.busy=1;
        if(i==2)r.store.opened=1;
        if(i==3)r.store.owned=1;
        memcpy(&held,&r,sizeof(held));
        assert(!pt_native_recovery_apply_configuration(&r,&c));
        assert(!memcmp(&r,&held,sizeof(r)));
        draft=c;draft.enabled=0;assert(!pt_native_recovery_apply_configuration(&r,&draft));
        assert(!memcmp(&r,&held,sizeof(r)));
        if(i<2) {
            memset(&out,0x5a,sizeof(out));memcpy(&image,&out,sizeof(image));
            assert(!pt_native_recovery_get_configuration(&r,&out));
            assert(!memcmp(&out,&image,sizeof(out)) && !memcmp(&r,&held,sizeof(r)));
        }
    }
    memcpy(&r,&before,sizeof(r));
    /* Invalid policy/text are complete before-image refusals. */
    for(i=0;i<4;++i) {
        draft=c;
        if(i==0)draft.interval_seconds=29;
        if(i==1)draft.enabled=2;
        if(i==2)memset(draft.directory,'x',sizeof(draft.directory));
        if(i==3) {draft.media=PT_NATIVE_RECOVERY_MEDIA_REMOVABLE;draft.allow_removable=0;}
        memcpy(&image,&draft,sizeof(image));assert(!pt_native_recovery_apply_configuration(&r,&draft));
        assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&draft,&image,sizeof(draft)));
    }
    /* The absent path is only a name under this already owned workspace. No
     * entry is created, adopted or deleted. Release an unexpected Lock first. */
    strcpy(missing,canonical);assert(AddPart((STRPTR)missing,(STRPTR)"typed-config-missing",sizeof(missing)));
    lock=Lock((STRPTR)missing,ACCESS_READ);if(lock)UnLock(lock);assert(!lock);
    draft=c;strcpy(draft.directory,missing);memcpy(&image,&draft,sizeof(image));
    assert(!pt_native_recovery_apply_configuration(&r,&draft));
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&draft,&image,sizeof(draft)));
    assert(strlen(source)<sizeof(draft.directory));draft=c;strcpy(draft.directory,source);
    memcpy(&image,&draft,sizeof(image));assert(!pt_native_recovery_apply_configuration(&r,&draft));
    assert(!memcmp(&r,&before,sizeof(r)) && !memcmp(&draft,&image,sizeof(draft)));
    /* Actual source-file Lock succeeds, but Examine rejects non-directory. */
    lock=Lock((STRPTR)source,ACCESS_READ);assert(lock);
    examined=Examine(lock,&info) && info.fib_DirEntryType<0;UnLock(lock);assert(examined);
    draft=c;draft.interval_seconds=45;draft.enabled=0;
    assert(pt_native_recovery_apply_configuration(&r,&draft));
    memcpy(&restored,&r,sizeof(restored));memcpy(&restored.schedule,&before.schedule,sizeof(restored.schedule));
    memcpy(restored.root,before.root,sizeof(restored.root));restored.configured=before.configured;restored.removable=before.removable;
    assert(!memcmp(&restored,&before,sizeof(restored)) && r.bound && r.project==project && !r.schedule.policy.enabled);
    assert(pt_native_recovery_get_configuration(&r,&out) && !out.enabled && out.interval_seconds==45 && !strcmp(out.directory,canonical));
    memcpy(&before,&r,sizeof(before));draft=out;draft.enabled=1;draft.interval_seconds=30;
    assert(pt_native_recovery_apply_configuration(&r,&draft));
    assert(r.schedule.policy.enabled && !r.schedule.armed && !r.schedule.have_snapshot && !r.schedule.since);
    memcpy(&restored,&r,sizeof(restored));memcpy(&restored.schedule,&before.schedule,sizeof(restored.schedule));
    assert(!memcmp(&restored,&before,sizeof(restored)));
    assert(pt_native_recovery_get_configuration(&r,&draft));draft.enabled=0;draft.directory[0]=0;draft.media=PT_NATIVE_RECOVERY_MEDIA_UNKNOWN;
    assert(pt_native_recovery_apply_configuration(&r,&draft) && r.bound && r.project==project && !r.root[0]);
    memset(&found,0x5a,sizeof(found));memcpy(&found_image,&found,sizeof(found_image));
    assert(pt_native_recovery_find(&r,&found)==PT_RECOVERY_NONE && !memcmp(&found,&found_image,sizeof(found)));
    assert(pt_native_recovery_finish(&r,0) && !r.configured && !r.bound && !r.store.opened);
    puts("NATIVE RECOVERY CONTROLLER PASS: real DOS canonical directory, missing-path/non-directory refusal, exact no-op/cancel/disabled/busy/owned/alias preservation and allocator self-alias");
}
