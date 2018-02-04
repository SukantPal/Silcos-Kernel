/**
 * File: ElfLinker.cpp
 *
 * Summary:
 * This file contains the implementation for the dynamic linkage & relocation
 * services for modules. It is used by the module-loader to modules as shared
 * libraries in the kernel-space.
 * 
 * Functions:
 *
 * Origin:
 *
 * Copyright (C) 2017 - Shukant Pal
 */
#include <Module/Elf/ElfLinker.hpp>
#include <Module/ModuleRecord.h>
#include <KERNEL.h>

using namespace Module;
using namespace Module::Elf;

void* __dso_handle;

/**
 * Function: ElfLinker::resolveRelocation
 *
 * Summary:
 * This function resolves a 'rel' type relocation.
 *
 * Args:
 * RelEntry *relocEntry - 'Rel' entry descriptor
 * ElfManager &handlerService - Module's elf-manager
 *
 * Author: Shukant Pal
 */
void ElfLinker::resolveRelocation(RelEntry *relocEntry, ElfManager &handlerService)
{
	unsigned long *field = (unsigned long*) (handlerService.baseAddress + relocEntry->Offset);
	unsigned long sindex = ELF32_R_SYM(relocEntry->Info);
	Symbol *symbolReferred = handlerService.dynamicSymbols.entryTable + sindex;
	char *signature = handlerService.dynamicSymbols.nameTable + symbolReferred->Name;
	unsigned long declBase;
	Symbol *declarer = RecordManager::querySymbol(signature, declBase);

	if(*signature == '\0' && ELF32_R_TYPE(relocEntry->Info) == R_386_RELATIVE)
	{
		*field += handlerService.baseAddress;
		return;
	}

	if(declarer != NULL)
	{
		switch(ELF32_R_TYPE(relocEntry->Info))
		{
		case R_386_JMP_SLOT:
		case R_386_GLOB_DAT:
			*field = declarer->Value + declBase;
			break;
		case R_386_32:
			*field += declarer->Value + declBase;
			break;
		case R_386_PC32:
			*field += declarer->Value + declBase - (unsigned long) field;
			break;
		default:
			DbgLine("Error 40A: TODO:: Build code (elf-linkage-reloc-switch");
			while(TRUE){ asm volatile("nop"); }
			break;
		}
	}
	else
	{
		Dbg("__notfound ");
		DbgLine(handlerService.dynamicSymbols.nameTable + symbolReferred->Name);
		while(TRUE){ asm volatile("nop"); }
	}
}

/**
 * Function: ElfLinker::resolveRelocations
 *
 * Summary:
 * This function resolves all the relocations in a 'rel' relocation table.
 *
 * Args:
 * RelTable &relocTable - 'Rel' relocation table
 * ElfManager &handlerService - Elf-manager for the relevant module
 *
 * Author: Shukant Pal
 */
void ElfLinker::resolveRelocations(RelTable &relocTable, ElfManager &handlerService)
{
	unsigned long relIndex = 0;
	unsigned long relCount = relocTable.entryCount;
	RelEntry *relDesc = relocTable.entryTable;

	while(relIndex < relCount)
	{
		ElfLinker::resolveRelocation(relDesc, handlerService);

		++(relIndex);
		++(relDesc);
	}
}