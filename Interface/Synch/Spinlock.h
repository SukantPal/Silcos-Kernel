/**
 * Copyright (C) 2017 - Shukant Pal
 */

#ifndef SYNCH_SPIN_LOCK_H
#define SYNCH_SPIN_LOCK_H

#include <Types.h>

typedef volatile UINT Spinlock;
typedef volatile UINT SPIN_LOCK;

import_asm void SpinLock(volatile SPIN_LOCK *);
import_asm void SpinUnlock(volatile SPIN_LOCK *);

#endif /* Synch/Spinlock.h */