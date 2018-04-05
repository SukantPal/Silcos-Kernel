///
/// @file IOAPIC.hpp
/// -------------------------------------------------------------------
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>
///
/// Copyright (C) 2017 - Shukant Pal
///

#ifndef HAL_IOAPIC_HPP__
#define HAL_IOAPIC_HPP__

#include <Object.hpp>
#include <ACPI/MADT.h>
#include <Executable/IRQHandler.hpp>
#include <Synch/Spinlock.h>
#include <Utils/ArrayList.hpp>
#include <Utils/LinkedList.h>
#include <TYPE.h>

namespace HAL
{

///
/// @class IOAPIC
///
/// Provides multi-processor interrupt management and incorporates
/// symmtric interrupt distribution across all CPUs in the system. The
/// interrupt steering and vector information is provided per input-signal,
/// using an indirect-register accessing scheme.
///
/// The IOAPIC provides 24 input-signal redirection entries. When a device
/// asserts a specific input-signal, the IOAPIC uses the corresponding
/// redirection entry to format an interrupt message.
///
/// For details of the IOAPIC refer to the Intel preliminary document.
///
/// @version 1.0
/// @since Silcos 3.02
/// @author Shukant Pal
///
class IOAPIC final : public Lockable, public LinkedListNode
{
public:
	enum DeliveryMode
	{
		Fixed		= 0b000,
		LowestPriority	= 0b001,
		SMI		= 0b010,
		INIT		= 0b101,
		ExtINT		= 0b111
	};

	struct RedirectionEntry
	{
		U64 vector	: 8;/// interrupt vector for this input
		U64 delvMode	: 3;/// sets how LAPICs react to this interrupt
		U64 destMode	: 1;/// physical/logical destination mode
		U64 delvStatus	: 1;/// 0=IDLE and 1=Send-Pending
		U64 pinPolarity	: 1;/// 0=High-active and 1=Low-active
		U64 remoteIRR	: 1;/// Set (level-trigger), when LAPICs accept
		U64 triggerMode	: 1;/// 1=Level-sensitive and 0=Edge-sensitive
		U64 mask	: 1;/// on setting this, signal is masked
		U64 reserved	: 39;
		U64 destination : 8;/// Id for the interrupt's destination
	};

	struct InputSignal : public Executable::IRQ
	{
		IOAPIC *intrRouter;
	};

	unsigned char id(){ return (apicID); }
	unsigned char version(){ return (hardwareVersion); }
	unsigned char redirectionEntries(){ return (redirEntries); }
	unsigned char arbitrationId(){ return (arbId); }
	unsigned long intrBase(){ return (globalSystemInterruptBase); }
	unsigned long intrCount(){ return (redirEntries); }

	RedirectionEntry getRedirEnt(unsigned char inputSignal);
	void setRedirEnt(unsigned char inputSignal, RedirectionEntry *data);

	static void registerIOAPIC(MADTEntryIOAPIC *ioaEnt);
	static void mapAllRoutesUniformly();

	static IOAPIC *getIterable()
	{
		return ((IOAPIC*) systemIOAPICs.get(0));
	}
private:
	unsigned char apicID;
	unsigned char hardwareVersion;
	unsigned char redirEntries;
	unsigned char arbId;
	unsigned long physRegs;
	unsigned long virtAddr;
	unsigned long globalSystemInterruptBase;

	inline U32 read()
	{
		return (*(U32 volatile*)(virtAddr + 0x10));
	}

	inline U32 read(U8 regOff)
	{
		*(U32 volatile*) virtAddr = regOff;
		return (*(U32 volatile*)(virtAddr + 0x10));
	}

	inline void write(U32 val)
	{
		*(U32 volatile *)(virtAddr + 0x10) = val;
	}

	inline void write(U8 regOff, U32 val)
	{
		*(U32 volatile*) virtAddr = regOff;
		*(U32 volatile*)(virtAddr + 0x10) = val;
	}

#define IOAPICID	0x00
#define IOAPICVER	0x01
#define IOAPICARB	0x02
#define IOREDTBL(n)	0x10 + 2*n

	static ArrayList systemIOAPICInputs;
	static ArrayList systemIOAPICs;
	static unsigned long routesUnderReset;
	friend class IRQ;

	IOAPIC(unsigned long physicalBase, unsigned long intrBase);
};

}// namespace HAL

#endif/* HAL/IOAPIC.hpp */
