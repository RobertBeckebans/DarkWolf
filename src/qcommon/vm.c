/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vm.c -- DLL-only virtual machine loader (no QVM/interpreter/compiler)

#include "vm_local.h"
#include <stdarg.h>
#include <stdint.h>

vm_t* currentVM = NULL;
vm_t* lastVM = NULL;
int     vm_debugLevel = 0;

#define MAX_VM 3
vm_t vmTable[MAX_VM];

void VM_VmInfo_f(void);
void VM_VmProfile_f(void);

/*
================
VM_VM2C

Historically converted a VM pointer to a native pointer.
For DLL-only builds this is just a raw cast.
================
*/
void* VM_VM2C(vmptr_t p, int length) {
	(void)length;
	return (void*)(uintptr_t)p;
}

void VM_Debug(int level) {
	vm_debugLevel = level;
}

/*
==============
VM_Init
==============
*/
void VM_Init(void) {
	// Keep cvars so existing configs/scripts don't complain.
	// DLL-only build: force native.
	Cvar_Get("vm_cgame", "0", CVAR_ARCHIVE);
	Cvar_Get("vm_game", "0", CVAR_ARCHIVE);
	Cvar_Get("vm_ui", "0", CVAR_ARCHIVE);

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	Com_Memset(vmTable, 0, sizeof(vmTable));
}

/*
===============
VM_ValueToSymbol
===============
*/
const char* VM_ValueToSymbol(vm_t* vm, int value) {
	vmSymbol_t* sym;
	static char text[MAX_TOKEN_CHARS];

	sym = vm->symbols;
	if (!sym) {
		return "NO SYMBOLS";
	}

	while (sym->next && sym->next->symValue <= value) {
		sym = sym->next;
	}

	if (value == sym->symValue) {
		return sym->symName;
	}

	Com_sprintf(text, sizeof(text), "%s+%i", sym->symName, value - sym->symValue);
	return text;
}

/*
===============
VM_ValueToFunctionSymbol
===============
*/
vmSymbol_t* VM_ValueToFunctionSymbol(vm_t* vm, int value) {
	vmSymbol_t* sym;
	static vmSymbol_t nullSym;

	Com_Memset(&nullSym, 0, sizeof(nullSym));

	sym = vm->symbols;
	if (!sym) {
		return &nullSym;
	}

	while (sym->next && sym->next->symValue <= value) {
		sym = sym->next;
	}

	return sym;
}

/*
===============
VM_SymbolToValue
===============
*/
int VM_SymbolToValue(vm_t* vm, const char* symbol) {
	vmSymbol_t* sym;

	for (sym = vm->symbols; sym; sym = sym->next) {
		if (!strcmp(symbol, sym->symName)) {
			return sym->symValue;
		}
	}
	return 0;
}

/*
=====================
VM_SymbolForCompiledPointer
=====================

No compiled/QVM support anymore.
Kept only so anything referencing it still links.
=====================
*/
const char* VM_SymbolForCompiledPointer(vm_t* vm, void* code) {
	(void)vm;
	(void)code;
	return "DLL_ONLY";
}

/*
===============
ParseHex
===============
*/
int ParseHex(const char* text) {
	int value;
	int c;

	value = 0;
	while ((c = *text++) != 0) {
		if (c >= '0' && c <= '9') {
			value = value * 16 + c - '0';
			continue;
		}
		if (c >= 'a' && c <= 'f') {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if (c >= 'A' && c <= 'F') {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

/*
===============
VM_LoadSymbols
===============
*/
void VM_LoadSymbols(vm_t* vm) {
	int         len;
	char* mapfile, * text_p, * token;
	char        name[MAX_QPATH];
	char        symbols[MAX_QPATH];
	vmSymbol_t** prev, * sym;
	int         count;
	int         value;
	int         chars;
	int         segment;

	// don't load symbols if not developer
	if (!com_developer || !com_developer->integer) {
		return;
	}

	COM_StripExtension(vm->name, name);
	Com_sprintf(symbols, sizeof(symbols), "vm/%s.map", name);
	len = FS_ReadFile(symbols, (void**)&mapfile);
	(void)len;

	if (!mapfile) {
		Com_Printf("Couldn't load symbol file: %s\n", symbols);
		return;
	}

	text_p = mapfile;
	prev = &vm->symbols;
	count = 0;

	while (1) {
		token = COM_Parse(&text_p);
		if (!token[0]) {
			break;
		}
		segment = ParseHex(token);
		if (segment) {
			COM_Parse(&text_p);
			COM_Parse(&text_p);
			continue;
		}

		token = COM_Parse(&text_p);
		if (!token[0]) {
			Com_Printf("WARNING: incomplete line at end of file\n");
			break;
		}
		value = ParseHex(token);

		token = COM_Parse(&text_p);
		if (!token[0]) {
			Com_Printf("WARNING: incomplete line at end of file\n");
			break;
		}

		chars = (int)strlen(token);
		sym = (vmSymbol_t*)Hunk_Alloc(sizeof(*sym) + chars, h_high);
		*prev = sym;
		prev = &sym->next;
		sym->next = NULL;
		sym->symValue = value;
		sym->profileCount = 0;
		Q_strncpyz(sym->symName, token, chars + 1);

		count++;
	}

	vm->numSymbols = count;
	Com_Printf("%i symbols parsed from %s\n", count, symbols);
	FS_FreeFile(mapfile);
}

/*
============
VM_DllSyscall

DLLs call this directly.

For DLL-only builds we still forward the address of the first argument
to the engine syscall dispatcher, which matches Quake 3’s classic DLL ABI.
============
*/
intptr_t QDECL VM_DllSyscall(intptr_t arg, ...) {
	intptr_t args[16];
	int i;
	va_list ap;

	if (!currentVM || !currentVM->systemCall) {
		return 0;
	}

	args[0] = arg;

	va_start(ap, arg);
	for (i = 1; i < (int)(sizeof(args) / sizeof(args[0])); i++) {
		args[i] = va_arg(ap, intptr_t);
	}
	va_end(ap);

	return currentVM->systemCall(args);
}

/*
=================
VM_Restart

DLL-only restart: unload and recreate the DLL VM.
=================
*/
vm_t* VM_Restart(vm_t* vm) {
	char name[MAX_QPATH];
	int (*systemCall)(int* parms);

	if (!vm) {
		Com_Error(ERR_FATAL, "VM_Restart: NULL vm");
		return NULL;
	}

	systemCall = vm->systemCall;
	Q_strncpyz(name, vm->name, sizeof(name));

	VM_Free(vm);
	vm = VM_Create(name, systemCall, VMI_NATIVE);

	return vm;
}

/*
================
VM_Create

DLL-only build.
No QVM loading.
No interpreter.
No JIT/compiler.
================
*/
vm_t* VM_Create(const char* module, int (*systemCalls)(int*), vmInterpret_t interpret) {
	vm_t* vm;
	int i;

	(void)interpret;

	if (!module || !module[0] || !systemCalls) {
		Com_Error(ERR_FATAL, "VM_Create: bad parms");
		return NULL;
	}

	// see if we already have the VM
	for (i = 0; i < MAX_VM; i++) {
		if (!Q_stricmp(vmTable[i].name, module)) {
			return &vmTable[i];
		}
	}

	// find a free vm
	for (i = 0; i < MAX_VM; i++) {
		if (!vmTable[i].name[0]) {
			break;
		}
	}

	if (i == MAX_VM) {
		Com_Error(ERR_FATAL, "VM_Create: no free vm_t");
		return NULL;
	}

	vm = &vmTable[i];
	Com_Memset(vm, 0, sizeof(*vm));

	Q_strncpyz(vm->name, module, sizeof(vm->name));
	vm->systemCall = systemCalls;
	vm->compiled = qfalse; // irrelevant now, but keep deterministic
	vm->dataBase = NULL;
	vm->dataMask = 0;
	vm->codeBase = NULL;
	vm->codeLength = 0;
	vm->instructionPointers = NULL;
	vm->instructionPointersLength = 0;
	vm->programStack = 0;
	vm->stackBottom = 0;

	Com_Printf("Loading dll file %s.\n", vm->name);
	vm->dllHandle = Sys_LoadDll(module, &vm->entryPoint, VM_DllSyscall);
	if (!vm->dllHandle) {
		Com_Memset(vm, 0, sizeof(*vm));
		Com_Error(ERR_FATAL, "VM_Create: failed to load DLL module '%s'", module);
		return NULL;
	}

	VM_LoadSymbols(vm);

	Com_Printf("%s loaded as native DLL\n", module);
	return vm;
}

/*
==============
VM_Free
==============
*/
void VM_Free(vm_t* vm) {
	if (!vm) {
		return;
	}

	if (vm->dllHandle) {
		Sys_UnloadDll(vm->dllHandle);
	}

	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear(void) {
	int i;

	for (i = 0; i < MAX_VM; i++) {
		if (vmTable[i].dllHandle) {
			Sys_UnloadDll(vmTable[i].dllHandle);
		}
		Com_Memset(&vmTable[i], 0, sizeof(vm_t));
	}

	currentVM = NULL;
	lastVM = NULL;
}

/*
==============
VM_ArgPtr
==============
*/
void* VM_ArgPtr(int intValue) {
	uintptr_t offset;

	if (!intValue) {
		return NULL;
	}
	if (currentVM == NULL) {
		return NULL;
	}
	if (currentVM->dataBase == NULL) {
		return NULL;
	}

	offset = (uintptr_t)(uint32_t)intValue;
	return (void*)((byte*)currentVM->dataBase + offset);
}

/*
==============
VM_ExplicitArgPtr
==============
*/
void* VM_ExplicitArgPtr(vm_t* vm, int intValue) {
	uintptr_t offset;

	if (!intValue) {
		return NULL;
	}
	if (vm == NULL) {
		return NULL;
	}
	if (vm->dataBase == NULL) {
		return NULL;
	}

	offset = (uintptr_t)(uint32_t)intValue;
	return (void*)((byte*)vm->dataBase + offset);
}

/*
==============
VM_Call

DLL-only dispatch.
No interpreted or compiled fallback remains.
==============
*/
int QDECL VM_Call(vm_t* vm, int callnum, ...) {
	vm_t* oldVM;
	int r;
	int i;
	int args[16];
	va_list ap;

	if (!vm) {
		Com_Error(ERR_FATAL, "VM_Call with NULL vm");
		return 0;
	}

	if (!vm->entryPoint) {
		Com_Error(ERR_FATAL, "VM_Call on VM '%s' without DLL entry point", vm->name);
		return 0;
	}

	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	if (vm_debugLevel) {
		Com_Printf("VM_Call( %s, %i )\n", vm->name, callnum);
	}

	va_start(ap, callnum);
	for (i = 0; i < (int)(sizeof(args) / sizeof(args[0])); i++) {
		args[i] = va_arg(ap, int);
	}
	va_end(ap);

	r = vm->entryPoint(
		callnum,
		args[0], args[1], args[2], args[3],
		args[4], args[5], args[6], args[7],
		args[8], args[9], args[10], args[11],
		args[12], args[13], args[14], args[15]
	);

	currentVM = oldVM;
	return r;
}

//=================================================================

static int QDECL VM_ProfileSort(const void* a, const void* b) {
	vmSymbol_t* sa, * sb;

	sa = *(vmSymbol_t**)a;
	sb = *(vmSymbol_t**)b;

	if (sa->profileCount < sb->profileCount) {
		return -1;
	}
	if (sa->profileCount > sb->profileCount) {
		return 1;
	}
	return 0;
}

/*
==============
VM_VmProfile_f
==============
*/
void VM_VmProfile_f(void) {
	vm_t* vm;
	vmSymbol_t** sorted, * sym;
	int i;
	double total;

	if (!lastVM) {
		return;
	}

	vm = lastVM;

	if (!vm->numSymbols) {
		return;
	}

	sorted = (vmSymbol_t**)Z_Malloc(vm->numSymbols * sizeof(*sorted));
	if (!sorted) {
		return;
	}

	sorted[0] = vm->symbols;
	total = sorted[0] ? sorted[0]->profileCount : 0.0;

	for (i = 1; i < vm->numSymbols; i++) {
		sorted[i] = sorted[i - 1]->next;
		total += sorted[i]->profileCount;
	}

	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);

	for (i = 0; i < vm->numSymbols; i++) {
		int perc = 0;

		sym = sorted[i];
		if (total > 0.0) {
			perc = (int)(100.0 * (double)sym->profileCount / total);
		}

		Com_Printf("%2i%% %9i %s\n", perc, sym->profileCount, sym->symName);
		sym->profileCount = 0;
	}

	Com_Printf("    %9.0f total\n", total);
	Z_Free(sorted);
}

/*
==============
VM_VmInfo_f
==============
*/
void VM_VmInfo_f(void) {
	vm_t* vm;
	int i;

	Com_Printf("Registered virtual machines:\n");
	for (i = 0; i < MAX_VM; i++) {
		vm = &vmTable[i];
		if (!vm->name[0]) {
			continue;
		}

		Com_Printf("%s : ", vm->name);
		if (vm->dllHandle) {
			Com_Printf("native dll\n");
		}
		else {
			Com_Printf("not loaded\n");
		}
	}
}

/*
===============
VM_LogSyscalls

Still useful while debugging native DLL syscall traffic.
===============
*/
void VM_LogSyscalls(int* args) {
	static int callnum;
	static FILE* f;

	if (!f) {
		f = fopen("syscalls.log", "w");
	}
	if (!f || !args) {
		return;
	}

	callnum++;
	fprintf(
		f,
		"%i: %p  %i = %i %i %i %i\n",
		callnum,
		(void*)args,
		args[0], args[1], args[2], args[3], args[4]
	);
	fflush(f);
}

/*
===============================================================
Stubs retained so anything still referencing old VM paths links.
These should never actually be used in DLL-only builds.
===============================================================
*/
int VM_CallCompiled(vm_t* vm, int* args) {
	(void)vm;
	(void)args;
	Com_Error(ERR_FATAL, "VM_CallCompiled called in DLL-only build");
	return 0;
}

int VM_CallInterpreted(vm_t* vm, int* args) {
	(void)vm;
	(void)args;
	Com_Error(ERR_FATAL, "VM_CallInterpreted called in DLL-only build");
	return 0;
}

void VM_Compile(vm_t* vm, vmHeader_t* header) {
	(void)vm;
	(void)header;
	Com_Error(ERR_FATAL, "VM_Compile called in DLL-only build");
}

void VM_PrepareInterpreter(vm_t* vm, vmHeader_t* header) {
	(void)vm;
	(void)header;
	Com_Error(ERR_FATAL, "VM_PrepareInterpreter called in DLL-only build");
}