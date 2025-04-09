#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "os.h"
#include "debug.h"
#include "box64context.h"
#include "box64cpu.h"
#include "emu/x64emu_private.h"
#include "x64emu.h"
#include "box64stack.h"
#include "callback.h"
#include "emu/x64run_private.h"
#include "x64trace.h"
#include "dynablock.h"
#include "dynablock_private.h"
#include "dynarec_private.h"
#include "alternate.h"

#include "dynarec_native.h"
#include "dynarec_arch.h"
#include "native_lock.h"

#include "custommem.h"
#include "khash.h"
#include "rbtree.h"

uint32_t X31_hash_code(void* addr, int len)
{
    if(!len) return 0;
    #ifdef ARCH_CRC
    ARCH_CRC(addr, len);
    #endif
    uint8_t* p = (uint8_t*)addr;
    int32_t h = *p;
    for (--len, ++p; len; --len, ++p) h = (h << 5) - h + (int32_t)*p;
    return (uint32_t)h;
}

dynablock_t* InvalidDynablock(dynablock_t* db, int need_lock)
{
    if(db) {
        if(db->gone)
            return NULL; // already in the process of deletion!
        dynarec_log(LOG_DEBUG, "InvalidDynablock(%p), db->block=%p x64=%p:%p already gone=%d\n", db, db->block, db->x64_addr, db->x64_addr+db->x64_size-1, db->gone);
        // remove jumptable without waiting
        setJumpTableDefault64(db->x64_addr);
        if(need_lock)
            mutex_lock(&my_context->mutex_dyndump);
        db->done = 0;
        db->gone = 1;
        uintptr_t db_size = db->x64_size;
        #ifdef ARCH_NOP
        if(db->callret_size) {
            // mark all callrets to UDF
            for(int i=0; i<db->callret_size; ++i)
                *(uint32_t*)(db->block+db->callrets[i].offs) = ARCH_NOP;
            ClearCache(db->block, db->size);
        }
        #endif
        if(db_size && my_context) {
            uint32_t n = rb_dec(my_context->db_sizes, db_size, db_size+1);
            if(!n && (db_size >= my_context->max_db_size)) {
                my_context->max_db_size = rb_get_righter(my_context->db_sizes);
                dynarec_log(LOG_INFO, "BOX64 Dynarec: lower max_db=%d\n", my_context->max_db_size);
            }
        }
        if(need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
    }
    return db;
}

void FreeInvalidDynablock(dynablock_t* db, int need_lock)
{
    if(db) {
        if(!db->gone)
            return; // already in the process of deletion!
        dynarec_log(LOG_DEBUG, "FreeInvalidDynablock(%p), db->block=%p x64=%p:%p already gone=%d\n", db, db->block, db->x64_addr, db->x64_addr+db->x64_size-1, db->gone);
        if(need_lock)
            mutex_lock(&my_context->mutex_dyndump);
        FreeDynarecMap((uintptr_t)db->actual_block);
        customFree(db);
        if(need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
    }
}

void FreeDynablock(dynablock_t* db, int need_lock)
{
    if(db) {
        if(db->gone)
            return; // already in the process of deletion!
        dynarec_log(LOG_DEBUG, "FreeDynablock(%p), db->block=%p x64=%p:%p already gone=%d\n", db, db->block, db->x64_addr, db->x64_addr+db->x64_size-1, db->gone);
        // remove jumptable without waiting
        setJumpTableDefault64(db->x64_addr);
        if(need_lock)
            mutex_lock(&my_context->mutex_dyndump);
        dynarec_log(LOG_DEBUG, " -- FreeDyrecMap(%p, %d)\n", db->actual_block, db->size);
        db->done = 0;
        db->gone = 1;
        uintptr_t db_size = db->x64_size;
        if(db_size && my_context) {
            uint32_t n = rb_dec(my_context->db_sizes, db_size, db_size+1);
            if(!n && (db_size >= my_context->max_db_size)) {
                my_context->max_db_size = rb_get_righter(my_context->db_sizes);
                dynarec_log(LOG_INFO, "BOX64 Dynarec: lower max_db=%d\n", my_context->max_db_size);
            }
        }
        if(db->previous)
            FreeInvalidDynablock(db->previous, 0);
        FreeDynarecMap((uintptr_t)db->actual_block);
        customFree(db);
        if(need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
    }
}



void MarkDynablock(dynablock_t* db)
{
    if(db) {
        dynarec_log(LOG_DEBUG, "MarkDynablock %p %p-%p\n", db, db->x64_addr, db->x64_addr+db->x64_size-1);
        if(!setJumpTableIfRef64(db->x64_addr, db->jmpnext, db->block)) {
            dynablock_t* old = db;
            db = getDB((uintptr_t)old->x64_addr);
            if(!old->gone && db!=old) {
                printf_log(LOG_INFO, "Warning, couldn't mark block as dirty for %p, block=%p, current_block=%p\n", old->x64_addr, old, db);
                // the block is lost, need to invalidate it...
                old->gone = 1;
                old->done = 0;
                if(!db || db->previous)
                    FreeInvalidDynablock(old, 1);
                else
                    db->previous = old;
            }
        } 
        #ifdef ARCH_NOP
        else if(db->callret_size) {
            // mark all callrets to UDF
            for(int i=0; i<db->callret_size; ++i)
                *(uint32_t*)(db->block+db->callrets[i].offs) = ARCH_UDF;
        }
        #endif
    }
}

static int IntervalIntersects(uintptr_t start1, uintptr_t end1, uintptr_t start2, uintptr_t end2)
{
    if(start1 > end2 || start2 > end1)
        return 0;
    return 1;
}

void MarkRangeDynablock(dynablock_t* db, uintptr_t addr, uintptr_t size)
{
    // Mark will try to find *any* blocks that intersect the range to mark
    if(!db)
        return;
    dynarec_log(LOG_DEBUG, "MarkRangeDynablock %p-%p .. startdb=%p, sizedb=%p\n", (void*)addr, (void*)addr+size-1, (void*)db->x64_addr, (void*)db->x64_size);
    if(IntervalIntersects((uintptr_t)db->x64_addr, (uintptr_t)db->x64_addr+db->x64_size-1, addr, addr+size+1))
        MarkDynablock(db);
}

int FreeRangeDynablock(dynablock_t* db, uintptr_t addr, uintptr_t size)
{
    if(!db)
        return 1;

    int need_lock = my_context?1:0;
    if(IntervalIntersects((uintptr_t)db->x64_addr, (uintptr_t)db->x64_addr+db->x64_size-1, addr, addr+size+1)) {
        FreeDynablock(db, need_lock);
        return 0;
    }
    return 1;
}

dynablock_t *AddNewDynablock(uintptr_t addr)
{
    dynablock_t* block;
    // create and add new block
    dynarec_log(LOG_VERBOSE, "Ask for DynaRec Block creation @%p\n", (void*)addr);
    block = (dynablock_t*)customCalloc(1, sizeof(dynablock_t));
    return block;
}

NEW_JUMPBUFF(dynarec_jmpbuf);

void cancelFillBlock()
{
    LongJmp(GET_JUMPBUFF(dynarec_jmpbuf), 1);
}

/* 
    return NULL if block is not found / cannot be created. 
    Don't create if create==0
*/
static dynablock_t* internalDBGetBlock(x64emu_t* emu, uintptr_t addr, uintptr_t filladdr, int create, int need_lock, int is32bits)
{
    if(hasAlternate((void*)addr))
        return NULL;
    const uint32_t req_prot = (box64_pagesize==4096)?(PROT_EXEC|PROT_READ):PROT_READ;
    dynablock_t* block = getDB(addr);
    if(block || !create) {
        if(block && getNeedTest(addr) && (getProtection(addr)&req_prot)!=req_prot)
            block = NULL;
        return block;
    }

    if(need_lock) {
        if(BOX64ENV(dynarec_wait)) {
            mutex_lock(&my_context->mutex_dyndump);
        } else {
            if(mutex_trylock(&my_context->mutex_dyndump))   // FillBlock not available for now
                return NULL;
        }
        block = getDB(addr);    // just in case
        if(block) {
            if(block && getNeedTest(addr) && (getProtection_fast(addr)&req_prot)!=req_prot)
                block = NULL;
            mutex_unlock(&my_context->mutex_dyndump);
            return block;
        }
    }
    if((getProtection_fast(addr)&req_prot)!=req_prot) {// cannot be run, get out of the Dynarec
        if(need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
        return NULL;
    }
    block = AddNewDynablock(addr);

    // fill the block
    block->x64_addr = (void*)addr;
    if (SigSetJmp(GET_JUMPBUFF(dynarec_jmpbuf), 1)) {
        printf_log(LOG_INFO, "FillBlock at %p triggered a segfault, canceling\n", (void*)addr);
        FreeDynablock(block, 0);
        if(need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
        return NULL;
    }
    void* ret = FillBlock64(block, filladdr, (addr==filladdr)?0:1, is32bits, MAX_INSTS);
    if(!ret) {
        dynarec_log(LOG_DEBUG, "Fillblock of block %p for %p returned an error\n", block, (void*)addr);
        customFree(block);
        block = NULL;
    }
    // check size
    if(block) {
        // fill-in jumptable
        if(!addJumpTableIfDefault64(block->x64_addr, block->dirty?block->jmpnext:block->block)) {
            FreeDynablock(block, 0);
            block = getDB(addr);
            MarkDynablock(block);   // just in case...
        } else {
            if(block->x64_size) {
                if(block->x64_size>my_context->max_db_size) {
                    my_context->max_db_size = block->x64_size;
                    dynarec_log(LOG_INFO, "BOX64 Dynarec: higher max_db=%d\n", my_context->max_db_size);
                }
                block->done = 1;    // don't validate the block if the size is null, but keep the block
                rb_inc(my_context->db_sizes, block->x64_size, block->x64_size+1);
            }
        }
    }
    if(need_lock)
        mutex_unlock(&my_context->mutex_dyndump);

    dynarec_log(LOG_DEBUG, "%04d| --- DynaRec Block created @%p:%p (%p, 0x%x bytes)\n", GetTID(), (void*)addr, (void*)(addr+((block)?block->x64_size:1)-1), (block)?block->block:0, (block)?block->size:0);

    return block;
}

dynablock_t* DBGetBlock(x64emu_t* emu, uintptr_t addr, int create, int is32bits)
{
    if(isInHotPage(addr))
        return NULL;
    dynablock_t *db = internalDBGetBlock(emu, addr, addr, create, 1, is32bits);
    if(db && db->done && db->block && getNeedTest(addr)) {
        if (db->always_test) SchedYield(); // just calm down...
        uint32_t hash = X31_hash_code(db->x64_addr, db->x64_size);
        int need_lock = mutex_trylock(&my_context->mutex_dyndump);
        if(hash!=db->hash) {
            db->done = 0;   // invalidating the block
            dynarec_log(LOG_DEBUG, "Invalidating block %p from %p:%p (hash:%X/%X, always_test:%d) for %p\n", db, db->x64_addr, db->x64_addr+db->x64_size-1, hash, db->hash, db->always_test,(void*)addr);
            // Free db, it's now invalid!
            dynablock_t* old = InvalidDynablock(db, need_lock);
            // start again... (will create a new block)
            db = internalDBGetBlock(emu, addr, addr, create, need_lock, is32bits);
            if(db) {
                if(db->previous)
                    FreeInvalidDynablock(db->previous, need_lock);
                db->previous = old;
            } else
                FreeInvalidDynablock(old, need_lock);
        } else {
            dynarec_log(LOG_DEBUG, "Validating block %p from %p:%p (hash:%X, always_test:%d) for %p\n", db, db->x64_addr, db->x64_addr+db->x64_size-1, db->hash, db->always_test, (void*)addr);
            if(db->always_test)
                protectDB((uintptr_t)db->x64_addr, db->x64_size);
            else {
                #ifdef ARCH_NOP
                if(db->callret_size) {
                    // mark all callrets to UDF
                    for(int i=0; i<db->callret_size; ++i)
                        *(uint32_t*)(db->block+db->callrets[i].offs) = ARCH_NOP;
                    ClearCache(db->block, db->size);
                }
                #endif
                protectDBJumpTable((uintptr_t)db->x64_addr, db->x64_size, db->block, db->jmpnext);
            }
        }
        if(!need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
    } 
    if(!db || !db->block || !db->done)
        emu->test.test = 0;
    return db;
}

dynablock_t* DBAlternateBlock(x64emu_t* emu, uintptr_t addr, uintptr_t filladdr, int is32bits)
{
    dynarec_log(LOG_DEBUG, "Creating AlternateBlock at %p for %p%s\n", (void*)addr, (void*)filladdr, is32bits?" 32bits":"");
    int create = 1;
    dynablock_t *db = internalDBGetBlock(emu, addr, filladdr, create, 1, is32bits);
    if(db && db->done && db->block && getNeedTest(filladdr)) {
        if (db->always_test) SchedYield(); // just calm down...
        int need_lock = mutex_trylock(&my_context->mutex_dyndump);
        uint32_t hash = X31_hash_code(db->x64_addr, db->x64_size);
        if(hash!=db->hash) {
            db->done = 0;   // invalidating the block
            dynarec_log(LOG_DEBUG, "Invalidating alt block %p from %p:%p (hash:%X/%X) for %p\n", db, db->x64_addr, db->x64_addr+db->x64_size, hash, db->hash, (void*)addr);
            // Free db, it's now invalid!
            dynablock_t* old = InvalidDynablock(db, need_lock);
            // start again... (will create a new block)
            db = internalDBGetBlock(emu, addr, filladdr, create, need_lock, is32bits);
            if(db) {
                if(db->previous)
                    FreeInvalidDynablock(db->previous, need_lock);
                db->previous = old;
            } else
                FreeInvalidDynablock(old, need_lock);
        } else {
            if(db->always_test)
                protectDB((uintptr_t)db->x64_addr, db->x64_size);
            else {
                #ifdef ARCH_NOP
                if(db->callret_size) {
                    // mark all callrets to UDF
                    for(int i=0; i<db->callret_size; ++i)
                        *(uint32_t*)(db->block+db->callrets[i].offs) = ARCH_NOP;
                    ClearCache(db->block, db->size);
                }
                #endif
                protectDBJumpTable((uintptr_t)db->x64_addr, db->x64_size, db->block, db->jmpnext);
            }
        }
        if(!need_lock)
            mutex_unlock(&my_context->mutex_dyndump);
    } 
    if(!db || !db->block || !db->done)
        emu->test.test = 0;
    return db;
}
