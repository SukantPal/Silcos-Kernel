/** 
 * Copyright (C) 2017 - Shukant Pal
 */

#define NAMESPACE_KFRAME_MANAGER

#include <Memory/KFrameManager.h>
#include <Memory/ZoneManager.h>
#include <KERNEL.h>

using namespace Memory;
using namespace Memory::Internal;

#define ZONE_ALLOCABLE 10
#define ZONE_RESERVE_ONLY 11
#define ZONE_BARRIER_ONLY 12
#define ZONE_OVERLOAD 101
#define ZONE_LOADED 102

void ZoneAllocator::resetAllocator(
		BuddyBlock *entryTable,
		struct ZonePreference *prefTable,
		unsigned long prefCount,
		struct Zone *zoneTable,
		unsigned long zoneCount
){
	this->descriptorTable = entryTable;
	this->prefTable = prefTable;
	this->prefCount = prefCount;
	this->zoneTable = zoneTable;
	this->zoneCount = zoneCount;
}

void ZoneAllocator::resetStatistics()
{
	struct Zone *zone = zoneTable;
	for(unsigned long index = 0; index < zoneCount; index++){
		zone->memoryAllocated = 0;
		++(zone);
	}
}

/**
 * Enum: ZoneState
 * Attributes: file-only, local
 *
 * Summary:
 * This enum contains constants which indicate the status of a zone for
 * a particular allocation w.r.t the required memory-size.
 *
 * Removals:
 * ZONE_OVERLOADED 101 - Present to indicated required-mem> total mem-size for zone
 *
 * Version: 1.1
 * Since: Circuit 2.03,++
 * Author: Shukant Pal
 */
enum ZoneState
{
	ALLOCABLE = 10,			// Zone is allocable for general-use memory
	RESERVE_OVERLAP = 11,	// Zone is allocable for ATOMIC operations only
	BARRIER_OVERLAP = 12,	// Zone is allocable for emergency operations only
	LOW_ON_MEMORY = 1001	// Zone is low-only-memory for required allocation
};

typedef ULONG ZNALLOC; /* Internal type - Used for testing if zone is allocable from */

/**
 * Function: getStatus
 * Attributes: C-style, internal
 *
 * Summary:
 * This function will calculate a status (constant) for a particular allocation
 * case with a zone w.r.t the required memory. See ZoneState for more info
 * regarding the values returned.
 *
 * Args:
 * unsigned long requiredMemory - Memory required in this allocation-case
 * struct Zone *searchZone - Zone in which the allocation (can) occur
 *
 * Version: 1.1.9
 * Since: Circuit 2.03,++
 * Author: Shukant Pal
 */
static enum ZoneState getStatus(
		unsigned long requiredMemory,
		struct Zone *searchZone
){
	unsigned long generalMemoryAvail = searchZone->memorySize - searchZone->memoryAllocated;
	if(requiredMemory > searchZone->memorySize - searchZone->memoryAllocated){
		return (LOW_ON_MEMORY);
	} else {
		// Memory excluding reserved amount
		generalMemoryAvail -= searchZone->memoryReserved;

		if(requiredMemory <= generalMemoryAvail){
			return (ALLOCABLE);
		} else {
			// Memory including reserved amount (minus emergency amount)
			generalMemoryAvail += (7 * searchZone->memoryReserved) >> 3;

			if(requiredMemory <= generalMemoryAvail)
				return (RESERVE_OVERLAP);// Only allow for ATOMIC allocations
			else
				return (BARRIER_OVERLAP);// Only allow for emergency operations
		}
	}
}

#define ZONE_ALLOCATE 0xF1
#define ZONE_SWITCH 0xF2
#define ZONE_FAILURE 0xFF
typedef ULONG ZNACTION;

/**
 * Enum: AllocationAction
 * Attributes: file-only, local
 *
 * Summary:
 * This enum contains constants for evaluating what to do for a
 * specific allocation case, based on the zone-state.
 *
 * Version: 1.01
 * Since: Circuit 2.03,++
 * Author: Shukant Pal
 */
enum AllocationAction
{
	ALLOCATE = 0x100A1,	// Allocate directly now
	GOTO_NEXT = 0x100A2,// Try another zone
	RET_FAIL = 0x100FF	// Return failure (for only first zone, other
						// -wise same as GOTO_NEXT)
};

/**
 * Function: getAction
 * Attributes: static
 *
 * Summary:
 * This function is a zone-allocator helper function for getting a corresponding
 * action for a zone based on its state w.r.t the allocation flags.
 *
 * Args:
 * enum ZoneState allocState - State of the allocating zone
 * ZoneControl allocFlags - Control-flags for the zone-based allocation
 *
 * Version: 1.1
 * Since: Circuit 2.03,++
 * Author: Shukant Pal
 */
static enum AllocationAction getAction(
		enum ZoneState allocState,
		ZNFLG allocFlags
){
	switch(allocState){

	case ALLOCABLE:
		return (ALLOCATE);

	case RESERVE_OVERLAP:
		if(FLAG_SET(allocState, ATOMIC) || FLAG_SET(allocState, NO_FAILURE))
			return (ALLOCATE);
		break;

	case BARRIER_OVERLAP:
		if(FLAG_SET(allocFlags, NO_FAILURE))
			return (ALLOCATE);
		break;
	}

	if(FLAG_SET(allocFlags, ZONE_REQUIRED))
		return (RET_FAIL);
	else
		return (GOTO_NEXT);
}

/**
 * Function: ZoneAllocator::getZone
 *
 * Summary:
 * This function is used for choosing a zone during allocation directly by querying
 * zone states & getting the corresponding actions. If a zone gives a ALLOCATE action,
 * then it is returned.
 *
 * Args:
 * unsigned long blockOrder - Order of block being requested for allocation
 * unsigned long basePref - Minimal preference-index of the allocation
 * ZoneControl allocFlags - Flags for allocating the block
 * struct Zone *zonePref - Preferred zone of allocation
 *
 * Version: 1.1
 * Since: Circuit 2.03,++
 * Author: Shukant Pal
 */
struct Zone *ZoneAllocator::getZone(
		unsigned long blockOrder,
		unsigned long basePref,
		ZNFLG allocFlags,
		struct Zone *zonePref
){
	enum ZoneState testState;
	enum AllocationAction testAction;
	struct Zone *trialZone = zonePref;
	struct Zone *trialZero = zonePref;// Head of circular-list

	unsigned long testPref = zonePref->preferenceIndex;
	while(testPref >= basePref){
		do {
			SpinLock(&trialZone->controlLock);

			testState =  getStatus(blockOrder, trialZone);
			testAction = getAction(testState, allocFlags);
			switch(testAction)
			{
			case ALLOCATE:
				return (trialZone);
			case RET_FAIL:
				return (NULL);
			}

			SpinUnlock(&trialZone->controlLock);
			trialZone = trialZone->nextZone;
		} while(trialZone != trialZero);

		--(testPref);
		trialZone = (struct Zone *) (prefTable[testPref].ZoneList.ClnMain);
		trialZero = trialZone;
	}

	return (NULL);// Rarely used
}

/**
 * Function: ZoneAllocator::allocateBlock
 *
 * Summary:
 * This function is used for allocating a block, trying to cover maximum
 * number of zones as possible. It will allocate a block in the process -
 *
 * 1. Preferred Zone-Check - It will firstly try allocating directly from
 * the zone preferred most.
 *
 * 2. Uniform Zone-Distribution Search - As all zones in a specific preference
 * are arranged in a circular list, it will try allocating from all zones starting
 * from after preferred zone.
 *
 * 3. Lower Preference Allocations - It will do the circular-search in lower zone
 * preferences till prefBase.
 *
 * Version: 1.1.1
 * Since: Circuit 2.03++
 * Author: Shukant Pal
 */
struct BuddyBlock *ZoneAllocator::allocateBlock(
		unsigned long blockOrder,
		unsigned long basePref,
		struct Zone *zonePref,
		ZNFLG allocFlags
){
	struct Zone *allocatingZone = getZone(blockOrder, basePref, allocFlags, zonePref);
	if(allocatingZone == NULL){
		return (NULL);
	} else {
		// Try allocating from its cache, for order(0) allocations
		// TODO: Implement the ZCache extension for alloc-internal

		struct BuddyBlock *blockRequired =
				allocatingZone->memoryAllocator.allocateBlock(blockOrder);
		allocatingZone->memoryAllocated += SIZEOF_ORDER(blockOrder);

		SpinUnlock(&allocatingZone->controlLock);// getZone gives the zone in a locked state
		return (blockRequired);
	}
}


/*
decl_c
BDINFO *ZnAllocateBlock(ULONG bOrder, ULONG znBasePref, ZNINFO *znInfo, ZNFLG znFlags, ZNSYS *znSys){
	ZNINFO *znAllocator = ZnChoose(bOrder, znBasePref, znFlags, znInfo, znSys);

	if(znAllocator == NULL)
		return (NULL);
	else {
		if(!FLAG_SET(znFlags, ZONE_NO_CACHE) && znSys->ZnCacheRefill != 0 && bOrder == 0) {
			SpinUnlock(&znAllocator->Lock);
			ULONG chStatus;
			LINODE *chData = ChDataAllocate(&znAllocator->Cache, &chStatus);

			if(FLAG_SET((ULONG) chStatus, CH_POPULATE)) {
				SpinLock(&znAllocator->Lock);
				chStatus &= ~1;
				CHREG *chReg = (CHREG *) chStatus;
				ULONG chDataSize = znAllocator->MmManager.DescriptorSize;

				BDINFO *chPopulater = BdAllocateBlock(znSys->ZnCacheRefill, &znAllocator->MmManager);
				if(chPopulater != NULL) {
					for(ULONG dOffset=0; dOffset<(1 << znSys->ZnCacheRefill); dOffset++) {
						chPopulater->UpperOrder = 0;
						chPopulater->LowerOrder = 0;
						PushHead((LINODE*) chPopulater, &chReg->DList);
						chPopulater = (BDINFO *) ((ULONG) chPopulater + chDataSize);
					}
				}

				SpinUnlock(&znAllocator->Lock);
			}

			if(chData != NULL) {
				return (BDINFO*) (chData);
			} else {
				return (BDINFO*) (ChDataAllocate(&znAllocator->Cache, &chStatus));
			}
		}

		znAllocator->MmAllocated += (1 << bOrder);
		BDINFO *allocatedBlock = BdAllocateBlock(bOrder, &znAllocator->MmManager);
		SpinUnlock(&znAllocator->Lock);
		return (allocatedBlock);
	}
}*/

/**
 * Function: ZoneAllocator::freeBlock
 *
 * Summary:
 * This function is used for freeing a unused block, which was allocated by THIS
 * allocator. No searching is done for any zone.
 *
 * Version: 1.1.0
 * Since: Circuit 2.03++
 * Author: Shukant Pal
 */
Void ZoneAllocator::freeBlock(
		struct BuddyBlock *blockGiven
){
	struct Zone *owner = zoneTable + blockGiven->ZnOffset;
	owner->memoryAllocated -= SIZEOF_ORDER(blockGiven->Order);
	owner->memoryAllocator.freeBlock(blockGiven);
}

/**
 * Function: ZoneAllocator::configureZones
 * Attributes: static
 *
 * Summary:
 * This function is used for setting up zone-based allocator configurations. It
 * will take a array of zones & setup (call constructor) their buddy-systems.
 *
 * Note that after calling this, the client MUST manually configure the zones
 * memory boundaries.
 *
 * Version: 1.0
 * Since: Circuit 2.03++
 * Author: Shukant Pal
 */
void ZoneAllocator::configureZones(
		unsigned long entrySize,
		unsigned long highestOrder,
		unsigned short *listInfo,
		struct LinkedList *listArray,
		struct Zone *zoneTable,
		unsigned long count
){
	struct Zone *zone = zoneTable;
	class BuddyAllocator *buddySys = &zone->memoryAllocator;

	unsigned long liCount = BDSYS_VECTORS(highestOrder);
	unsigned long liSize = liCount + 1;

	for(unsigned long zoneIndex = 0; zoneIndex < count; zoneIndex++){
		(void) new (buddySys) BuddyAllocator(entrySize, NULL, highestOrder, listInfo, listArray);
		listInfo += liSize;
		listArray += liCount;
		++(zone);
		buddySys = &zone->memoryAllocator;
	}
}

void ZoneAllocator::configurePreference(
		struct Zone *zoneArray,
		struct ZonePreference *pref,
		UINT count
){
	UINT index = 0;
	while(index < count){
		ClnInsert((struct CircularListNode *) zoneArray, CLN_LAST, &pref->ZoneList);
		++(index);
		++(zoneArray);
	}
}

/**
 * Function: ZoneAllocator::configureZoneMappings
 */
static void ZoneAllocator::configureZoneMappings(
		struct Zone *zoneArray,
		unsigned long count
){
	UBYTE *blockPtr;
	unsigned long blockIndex;
	unsigned long blockCount;
	unsigned long blockEntrySz = zoneArray->memoryAllocator.getEntrySize();
	for(unsigned long index = 0; index < count; index++){
		blockCount = zoneArray->memorySize;
		blockPtr = (UBYTE *) zoneArray->memoryAllocator.getEntryTable();
		for(blockIndex = 0; blockIndex < blockCount; blockIndex++){
			((struct BuddyBlock *) blockPtr)->ZnOffset = index;
			blockPtr += blockEntrySz;
		}

		++(zoneArray);
	}
}

/*
decl_c
VOID ZnFreeBlock(BDINFO *bdInfo, ZNSYS *znSys){
	ZNINFO *znInfo = znSys->ZnSet + bdInfo->ZnOffset;
	znInfo->MmAllocated -= (1 << bdInfo->UpperOrder);
	BdFreeBlock(bdInfo, &znInfo->MmManager);
}
*/
/*
decl_c
BDINFO *ZnExchangeBlock(BDINFO *bdInfo, ULONG *status, ULONG znBasePref, ULONG znFlags, ZNSYS *znSys){
	ZNINFO *zone = &znSys->ZnSet[bdInfo->ZnOffset];
	SpinLock(&zone->Lock);
	BDINFO *ebInfo = BdExchangeBlock(bdInfo, &(zone->MmManager), status); // @Prefix eb - Exchanged Block
	SpinUnlock(&zone->Lock);
	if(ebInfo == NULL) {
		return (ZnAllocateBlock(bdInfo->LowerOrder + 1, znBasePref, zone, znFlags, znSys));
	} else if((ULONG) ebInfo == BD_ERR_FREE) {
		return (NULL);
	} else {
		return (ebInfo);
	}
}*/