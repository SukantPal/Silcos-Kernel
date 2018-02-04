
/**
 * File: Main.c
 *
 * Summary:
 * This file contains the initialization code for the IA32 platform. Internal kernel
 * subsystems are setup before waking all other CPUs and preparing SMP in the system.
 *
 * CPUID support is implemented for getting all model-specific information on the
 * system. This allows for the flexibility of the software to bend against changes
 * in the models of a CPU in the architecture.
 *
 * Copyright (C) 2017 - Shukant Pal
 */
#define NAMESPACE_MAIN
#define NAMESPACE_MEMORY_MANAGER

#include <ACPI/RSDP.h>
#include <ACPI/RSDT.h>
#include <IA32/HAL.h>
#include <IA32/APBoot.h>
#include <HAL/ADM.h>
#include <HAL/CPUID.h>
#include <HAL/Processor.h>
#include <Executable/RunqueueBalancer.hpp>
#include <Executable/Thread.h>
#include <Memory/Pager.h>
#include <Memory/KFrameManager.h>
#include <Memory/KMemoryManager.h>
#include <Memory/KMemorySpace.h>
#include <Memory/KObjectManager.h>
#include <Module/ModuleLoader.h>
#include <Module/MSI.h>
#include <Util/Memory.h>
#include <Synch/Spinlock.h>
#include <Debugging.h>
#include <Multiboot2.h>
#include <TYPE.h>
#include <KERNEL.h>
#include <Module/Elf/ELF.h>

using namespace HAL;
using namespace Executable;

extern void SetupTick();
import_asm void BSPGrantPermit();

const char *cpuIdNotSupportedError = "Error: 0xAAAE1: Platform does not support CPUID.";/* CPUID test returns zero value, indicating failure */

void ImmatureHang(const char *dbgString){
	DbgLine(dbgString);
	while(TRUE){ asm("hlt"); }
}

extern "C" void stupid();

extern "C" void __cxa_pure_virtual()
{
	DbgLine("compiler err: __cxa_pure_virtual() called, c++ virtual function problem!");
}
/**
 * Function: ValidateSupport()
 *
 * Summary: This function validates the required features to run the kernel, by checking for
 * CPUID support and then searching for the list features.
 *
 * @Version 1
 * @Since Circuit 2.03
 */
void ValidateSupport(){
	if(TestCPUID() != 0) {
		// TODO: Implement CPUID feature searching through ADM subset.
	} else
		ImmatureHang(cpuIdNotSupportedError);
}

/*
 * For debugging, print sizes of sections & global data structures that are
 * important for kernel size.
 */
void printStatic()
{
#ifdef DEBUG
	Dbg("Kernel Code: "); DbgInt((U32) &KernelCodeEnd - (U32) &KernelCodeStart);
	Dbg("\nKernel Data: "); DbgInt((U32) &KernelDataEnd - (U32) &KernelDataStart);
	Dbg("\nKernel BSS: "); DbgInt((U32) &KernelBSSEnd - (U32) &KernelBSSStart);
	Dbg("\nKernel PDat: "); DbgInt((U32) &KernelPDatEnd - (U32) &KernelPDatStart);
	DbgLine("");
#endif
}

extern "C" void ArchMain(); // hal -> Startup.cpp (ia32)

/**
 * Function: Main()
 *
 * Args:
 * bootInfo - virtual address of the multiboot header
 * magicNo - the identification number given by the loader
 *
 * Returns: void
 *
 * @See Init.c, InitRuntime.asm
 * @Version 21
 * @Since Circuit 1.02
 */
export_asm void Main(
		U32 bootInfo,
		U32 magicNo
){
	DbgLine("Reporting Load: @(com.silcos.circuit.2030)\t--- Silcos Kernel 2.05! ---");
	printStatic();

	if(magicNo != MULTIBOOT2_BOOTLOADER_MAGIC){
		DbgLine("Error : Multiboot-compliant bootloader not found!");
		DbgLine("Please install a multiboot-compliant bootloader, e.g. GRUB2");
		asm volatile("hlt;");
	}

	SetupKFrameManager();
	SetupKMemoryManager();
	obSetupAllocator();
	SetupPrimitiveObjects();

	MdSetupLoader();
	KernelElf::loadBootModules();

	ArchMain();// HAL is responsible for further init
}