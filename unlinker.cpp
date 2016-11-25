#include "General.h"
#include "strings.h"
#include "vector.h"
#include "ini.h"
#include "RawFileClass.h"
#include "udis86.h"
#include "cvinfo.h"
void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);
enum FeatureEnum
{
	SafeSEH = 0x1,
	UsesClr = 0x2,
	ClrSafe = 0x4,
	SupportsWinRT = 0x8,
	Unknown1 = 0x10,
	Unknown2 = 0x80,
	SecurityCheck = 0x100,
	SDL = 0x200,
	ControlFlowGuard = 0x400,
	ControlFlowGuard2 = 0x800,
	GuardEHandler = 0x1000,
	MPXEnabled = 0x2000,
	CompileTargetsKernel = 0x40000000,
	Unknown3 = 0x80000000,
};
enum CompileType
{
	COMPILE_ASM = 0x103,
	COMPILE_C = 0x104,
	COMPILE_CPP = 0x105,
	COMPILE_C_CVTCIL = 0x106,
	COMPILE_CPP_CVTCIL = 0x107,
	COMPILE_C_LTCG = 0x108,
	COMPILE_CPP_LTCG = 0x109,
	COMPILE_MSIL = 0x10A,
	COMPILE_C_PGOINSTRUMENT = 0x10B,
	COMPILE_CPP_PGOINSTRUMENT = 0x10C,
	COMPILE_C_PGOUSE = 0x10D,
	COMPILE_CPP_PGOUSE = 0x10E,
};
struct Symbol;
struct SymbolTableEntry
{
	IMAGE_SYMBOL symbol;
	IMAGE_AUX_SYMBOL aux;
};
struct RelocationEntry 
{
	unsigned long Rva;
	Symbol *Symbol;
	unsigned short Type;
	bool operator== (const RelocationEntry &src) { return false; }
	bool operator!= (const RelocationEntry &src) { return true; }
	RelocationEntry() : Rva(0), Symbol(0), Type(IMAGE_REL_I386_DIR32)
	{
	}
};
struct Symbol
{
	StringClass Name;
	unsigned long Address;
	unsigned long FileAddress;
	unsigned long Size;
	unsigned long CodeSize;
	BYTE Selection;
	unsigned char *Data;
	DynamicVectorClass<RelocationEntry> Relocations;
	int SectionNumber;
	int SectionSymbolNumber;
	int EntrySymbolNumber;
	Symbol() : Address(0), FileAddress(0), Size(0), CodeSize(0), Selection(IMAGE_COMDAT_SELECT_NODUPLICATES), Data(0), SectionNumber(0), SectionSymbolNumber(0), EntrySymbolNumber(0)
	{
	}
	bool operator==(const Symbol &that)
	{
		return Address == that.Address;
	}
	bool operator!=(const Symbol &that)
	{
		return Address != that.Address;
	}
};
DynamicVectorClass<Symbol> CodeSymbols;
DynamicVectorClass<Symbol> RDataSymbols;
DynamicVectorClass<Symbol> DataSymbols;
DynamicVectorClass<Symbol> BSSSymbols;
DynamicVectorClass<Symbol> JumpTableSymbols;
DynamicVectorClass<Symbol> JumpTargetSymbols;
bool IsSymbol(unsigned long address)
{
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		if (address == CodeSymbols[i].Address)
		{
			return true;
		}
	}
	for (int i = 0; i < RDataSymbols.Count(); i++)
	{
		if (address == RDataSymbols[i].Address)
		{
			return true;
		}
	}
	for (int i = 0; i < DataSymbols.Count(); i++)
	{
		if (address == DataSymbols[i].Address)
		{
			return true;
		}
	}
	for (int i = 0; i < BSSSymbols.Count(); i++)
	{
		if (address == BSSSymbols[i].Address)
		{
			return true;
		}
	}
	for (int i = 0; i < JumpTableSymbols.Count(); i++)
	{
		if (address == JumpTableSymbols[i].Address)
		{
			return true;
		}
	}
	return false;
}
Symbol &FindSymbol(unsigned long address)
{
	for (int i = 0;i < CodeSymbols.Count(); i++)
	{
		if (address == CodeSymbols[i].Address)
		{
			return CodeSymbols[i];
		}
	}
	for (int i = 0; i < RDataSymbols.Count(); i++)
	{
		if (address == RDataSymbols[i].Address)
		{
			return RDataSymbols[i];
		}
	}
	for (int i = 0; i < DataSymbols.Count(); i++)
	{
		if (address == DataSymbols[i].Address)
		{
			return DataSymbols[i];
		}
	}
	for (int i = 0; i < BSSSymbols.Count(); i++)
	{
		if (address == BSSSymbols[i].Address)
		{
			return BSSSymbols[i];
		}
	}
	for (int i = 0; i < JumpTableSymbols.Count(); i++)
	{
		if (address == JumpTableSymbols[i].Address)
		{
			return JumpTableSymbols[i];
		}
	}
	UL_UNREACHABLE;
}
Symbol &FindJumpTargetSymbol(unsigned long address)
{
	for (int i = 0; i < JumpTargetSymbols.Count(); i++)
	{
		if (address == JumpTargetSymbols[i].Address)
		{
			return JumpTargetSymbols[i];
		}
	}
	UL_UNREACHABLE;
}
Symbol &FindJumpCodeSymbol(unsigned long address)
{
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		if (address >= CodeSymbols[i].Address && address <= CodeSymbols[i].Address + CodeSymbols[i].Size)
		{
			return CodeSymbols[i];
		}
	}
	UL_UNREACHABLE;
}
void ParseCodeSymbol(Symbol &sym)
{
	ud_t ud_obj;
	ud_init(&ud_obj);
	ud_set_mode(&ud_obj, 32);
	ud_set_input_buffer(&ud_obj, sym.Data, sym.CodeSize);
	ud_set_syntax(&ud_obj, NULL);
	while (ud_disassemble(&ud_obj))
	{
		for (int i = 0; i < 4; i++)
		{
			const ud_operand *operand = ud_insn_opr(&ud_obj, i);
			if (operand)
			{
				if (operand->type == UD_OP_MEM && operand->offset == 32 && IsSymbol(operand->lval.udword))
				{
					unsigned int len = ud_insn_len(&ud_obj);
					const uint8_t *ptr = ud_insn_ptr(&ud_obj);
					Symbol &fsym = FindSymbol(operand->lval.udword);
					uint8_t *buf = (uint8_t *)memmem(ptr, len, &operand->lval.udword, 4);
					buf[0] = 0;
					buf[1] = 0;
					buf[2] = 0;
					buf[3] = 0;
					RelocationEntry r;
					r.Rva = buf - sym.Data;
					r.Symbol = &fsym;
					sym.Relocations.Add(r);
				}
				else if (operand->type == UD_OP_IMM && IsSymbol(operand->lval.udword))
				{
					unsigned int len = ud_insn_len(&ud_obj);
					const uint8_t *ptr = ud_insn_ptr(&ud_obj);
					Symbol &fsym = FindSymbol(operand->lval.udword);
					uint8_t *buf = (uint8_t *)memmem(ptr, len, &operand->lval.udword, 4);
					buf[0] = 0;
					buf[1] = 0;
					buf[2] = 0;
					buf[3] = 0;
					RelocationEntry r;
					r.Rva = buf - sym.Data;
					r.Symbol = &fsym;
					sym.Relocations.Add(r);
				}
				else if (operand->type == UD_OP_JIMM && operand->size == 32)
				{
					unsigned int len = ud_insn_len(&ud_obj);
					const uint8_t *ptr = ud_insn_ptr(&ud_obj);
					unsigned long offset = (unsigned long)ud_insn_off(&ud_obj);
					unsigned long pos = (signed long)sym.Address + (signed long)offset + operand->lval.sdword + (signed long)len;
					if ((pos < sym.Address || pos > sym.Address + sym.CodeSize) && IsSymbol(pos))
					{
						Symbol &fsym = FindSymbol(pos);
						uint8_t *buf = (uint8_t *)memmem(ptr, len, &operand->lval.sdword, 4);
						buf[0] = 0;
						buf[1] = 0;
						buf[2] = 0;
						buf[3] = 0;
						RelocationEntry r;
						r.Rva = buf - sym.Data;
						r.Symbol = &fsym;
						r.Type = IMAGE_REL_I386_REL32;
						sym.Relocations.Add(r);
					}
				}
			}
			else
			{
				break;
			}
		}
	}
}
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: unlinker <input file>\n");
		return 1;
	}
	RawFileClass file(argv[1]);
	if (!file.Is_Available(0))
	{
		return 1;
	}
	INIClass *ini = new INIClass(file);
	StringClass Binary;
	Binary = ini->Get_String(Binary, "General", "Binary");
	StringClass Output;
	Output = ini->Get_String(Output, "General", "Output");
	StringClass OutputFull;
	_fullpath(OutputFull.Get_Buffer(MAX_PATH), Output.Peek_Buffer(), MAX_PATH);
	FILE *f = fopen(Binary, "rb");
	int Language = ini->Get_Int("General", "Language", CV_CFL_CXX);
	int Machine = ini->Get_Int("General", "Machine", CV_CFL_PENTIUMIII);
	int FrontEndMajorVersion = ini->Get_Int("General", "FrontEndMajorVersion", 19);
	int FrontEndMinorVersion = ini->Get_Int("General", "FrontEndMinorVersion", 0);
	int FrontEndBuildVersion = ini->Get_Int("General", "FrontEndBuildVersion", 24215);
	int FrontEndQFEVersion = ini->Get_Int("General", "FrontEndQFEVersion", 1);
	int BackEndMajorVersion = ini->Get_Int("General", "BackEndMajorVersion", 19);
	int BackEndMinorVersion = ini->Get_Int("General", "BackEndMinorVersion", 0);
	int BackEndBuildVersion = ini->Get_Int("General", "BackEndBuildVersion", 24215);
	int BackEndQFEVersion = ini->Get_Int("General", "BackEndQFEVersion", 1);
	StringClass CompilerString;
	CompilerString = ini->Get_String(CompilerString, "General", "CompilerString", "Microsoft (R) Optimizing Compiler");
	int FeatureEnum = ini->Get_Int("General", "FeatureEnum", Unknown1 | Unknown2 | Unknown3 | SafeSEH);
	int CompileType = ini->Get_Int("General", "CompileType", COMPILE_CPP);
	int CompilerIDVersion = ini->Get_Int("General", "CompilerIDVersion", 24215);
	int codecount = ini->Entry_Count("CodeSymbols");
	for (int i = 0; i < codecount; i++)
	{
		Symbol sym;
		const char *codesym = ini->Get_Entry("CodeSymbols", i);
		ini->Get_String(sym.Name, "CodeSymbols", codesym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		sym.FileAddress = ini->Get_Int(sym.Name, "FileAddress", 0);
		sym.Size = ini->Get_Int(sym.Name, "Size", 0);
		sym.CodeSize = ini->Get_Int(sym.Name, "CodeSize", sym.Size);
		sym.Selection = (BYTE)ini->Get_Int(sym.Name, "Selection", IMAGE_COMDAT_SELECT_NODUPLICATES);
		if (sym.Size)
		{
			fseek(f, sym.FileAddress, SEEK_SET);
			sym.Data = new unsigned char[sym.Size];
			fread(sym.Data, 1, sym.Size, f);
		}
		CodeSymbols.Add(sym);
	}
	int rdatacount = ini->Entry_Count("RDataSymbols");
	for (int i = 0; i < rdatacount; i++)
	{
		Symbol sym;
		const char *rdatasym = ini->Get_Entry("RDataSymbols", i);
		ini->Get_String(sym.Name, "RDataSymbols", rdatasym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		sym.FileAddress = ini->Get_Int(sym.Name, "FileAddress", 0);
		sym.Size = ini->Get_Int(sym.Name, "Size", 0);
		sym.Selection = (BYTE)ini->Get_Int(sym.Name, "Selection", IMAGE_COMDAT_SELECT_NODUPLICATES);
		if (sym.Size)
		{
			fseek(f, sym.FileAddress, SEEK_SET);
			sym.Data = new unsigned char[sym.Size];
			fread(sym.Data, 1, sym.Size, f);
		}
		else
		{
			sym.Data = 0;
		}
		RDataSymbols.Add(sym);
	}
	int datacount = ini->Entry_Count("DataSymbols");
	for (int i = 0; i < datacount; i++)
	{
		Symbol sym;
		const char *datasym = ini->Get_Entry("DataSymbols", i);
		ini->Get_String(sym.Name, "DataSymbols", datasym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		sym.FileAddress = ini->Get_Int(sym.Name, "FileAddress", 0);
		sym.Size = ini->Get_Int(sym.Name, "Size", 0);
		sym.Selection = (BYTE)ini->Get_Int(sym.Name, "Selection", 0);
		if (sym.Size)
		{
			fseek(f, sym.FileAddress, SEEK_SET);
			sym.Data = new unsigned char[sym.Size];
			fread(sym.Data, 1, sym.Size, f);
		}
		else
		{
			sym.Data = 0;
		}
		DataSymbols.Add(sym);
	}
	int bsscount = ini->Entry_Count("BSSSymbols");
	for (int i = 0; i < bsscount; i++)
	{
		Symbol sym;
		const char *bsssym = ini->Get_Entry("BSSSymbols", i);
		ini->Get_String(sym.Name, "BSSSymbols", bsssym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		sym.FileAddress = 0;
		sym.Size = ini->Get_Int(sym.Name, "Size", 0);
		sym.Selection = (BYTE)ini->Get_Int(sym.Name, "Selection", 0);
		sym.Data = 0;
		BSSSymbols.Add(sym);
	}
	fclose(f);
	int jumptablecount = ini->Entry_Count("JumpTableSymbols");
	for (int i = 0; i < jumptablecount; i++)
	{
		Symbol sym;
		const char *jumptablesym = ini->Get_Entry("JumpTableSymbols", i);
		ini->Get_String(sym.Name, "JumpTableSymbols", jumptablesym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		sym.Size = ini->Get_Int(sym.Name, "Size", 0);
		JumpTableSymbols.Add(sym);
	}
	int jumptargetcount = ini->Entry_Count("JumpTargetSymbols");
	for (int i = 0; i < jumptargetcount; i++)
	{
		Symbol sym;
		const char *jumptargetsym = ini->Get_Entry("JumpTargetSymbols", i);
		ini->Get_String(sym.Name, "JumpTargetSymbols", jumptargetsym, 0);
		sym.Address = ini->Get_Int(sym.Name, "Address", 0);
		JumpTargetSymbols.Add(sym);
	}
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		if (CodeSymbols[i].Size)
		{
			ParseCodeSymbol(CodeSymbols[i]);
		}
	}
	for (int i = 0; i < JumpTableSymbols.Count(); i++)
	{
		unsigned long JumpTableCount = JumpTableSymbols[i].Size / 4;
		Symbol &JumpTableCodeSym = FindJumpCodeSymbol(JumpTableSymbols[i].Address);
		unsigned long *JumpTableData = (unsigned long *)((JumpTableSymbols[i].Address - JumpTableCodeSym.Address) + JumpTableCodeSym.Data);
		for (unsigned int j = 0; j < JumpTableCount; j++)
		{
			Symbol &JumpTargetSymbol = FindJumpTargetSymbol(JumpTableData[j]);
			JumpTableData[j] = 0;
			RelocationEntry r;
			r.Rva = ((unsigned char *)(&JumpTableData[j])) - JumpTableCodeSym.Data;
			r.Symbol = &JumpTargetSymbol;
			JumpTableCodeSym.Relocations.Add(r);
		}
	}
	int symbolcount = 0;
	int sectioncount = 0;
	int stringtablesize = 0;
	symbolcount++; //@comp.id
	symbolcount++; //@feat.00
	symbolcount++; //.debug$s
	sectioncount++; //.debug$s
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += CodeSymbols[i].Name.Get_Length();
		stringtablesize++;
		if (CodeSymbols[i].Size)
		{
			symbolcount++;
			sectioncount++;
		}
	}
	for (int i = 0; i < JumpTableSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += JumpTableSymbols[i].Name.Get_Length();
		stringtablesize++;
	}
	for (int i = 0; i < JumpTargetSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += JumpTargetSymbols[i].Name.Get_Length();
		stringtablesize++;
	}
	for (int i = 0; i < RDataSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += RDataSymbols[i].Name.Get_Length();
		stringtablesize++;
		if (RDataSymbols[i].Size)
		{
			symbolcount++;
			sectioncount++;
		}
	}
	for (int i = 0; i < DataSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += DataSymbols[i].Name.Get_Length();
		stringtablesize++;
		if (DataSymbols[i].Size)
		{
			symbolcount++;
			sectioncount++;
		}
	}
	for (int i = 0; i < BSSSymbols.Count(); i++)
	{
		symbolcount++;
		stringtablesize += BSSSymbols[i].Name.Get_Length();
		stringtablesize++;
		if (BSSSymbols[i].Size)
		{
			symbolcount++;
			sectioncount++;
		}
	}
	int DebugSize = 0;
	int DebugSSymbolsSize = 0;
	DebugSize += 4; //CV_SIGNATURE_C13
	DebugSize += 4; //DEBUG_S_SYMBOLS
	DebugSize += 4; //size of DebugSSymbols
	DebugSize += sizeof(OBJNAMESYM);
	DebugSSymbolsSize += sizeof(OBJNAMESYM);
	DebugSize += OutputFull.Get_Length();
	DebugSSymbolsSize += OutputFull.Get_Length();
	DebugSize++;
	DebugSSymbolsSize++;
	DebugSize += sizeof(COMPILESYM3);
	DebugSSymbolsSize += sizeof(COMPILESYM3);
	DebugSize += CompilerString.Get_Length();
	DebugSSymbolsSize += CompilerString.Get_Length();
	DebugSize++;
	DebugSSymbolsSize++;
	DebugSize += (4 - (DebugSize % 4)) % 4;
	unsigned char *DebugSSection = new unsigned char[DebugSize];
	memset(DebugSSection, 0, DebugSize);
	unsigned char *DebugSSectionPos = DebugSSection;
	*((unsigned int *)DebugSSectionPos) = CV_SIGNATURE_C13;
	DebugSSectionPos += 4;
	*((unsigned int *)DebugSSectionPos) = DEBUG_S_SYMBOLS;
	DebugSSectionPos += 4;
	*((unsigned int *)DebugSSectionPos) = DebugSSymbolsSize;
	DebugSSectionPos += 4;
	OBJNAMESYM *obj = (OBJNAMESYM *)DebugSSectionPos;
	obj->reclen = (unsigned short)(OutputFull.Get_Length() + 1 + sizeof(OBJNAMESYM) - 2);
	obj->rectyp = S_OBJNAME;
	obj->signature = 0;
	DebugSSectionPos += sizeof(OBJNAMESYM);
	strcpy((char *)DebugSSectionPos, OutputFull.Peek_Buffer());
	DebugSSectionPos += OutputFull.Get_Length();
	DebugSSectionPos++;
	COMPILESYM3 *compile = (COMPILESYM3 *)DebugSSectionPos;
	compile->reclen = (unsigned short)(CompilerString.Get_Length() + 1 + sizeof(COMPILESYM3) - 2);
	compile->rectyp = S_COMPILE3;
	compile->flags.iLanguage = Language;
	compile->flags.fNoDbgInfo = 1;
	compile->machine = (unsigned short)Machine;
	compile->verFEMajor = (unsigned short)FrontEndMajorVersion;
	compile->verFEMinor = (unsigned short)FrontEndMinorVersion;
	compile->verFEBuild = (unsigned short)FrontEndBuildVersion;
	compile->verFEQFE = (unsigned short)FrontEndQFEVersion;
	compile->verMajor = (unsigned short)BackEndMajorVersion;
	compile->verMinor = (unsigned short)BackEndMinorVersion;
	compile->verBuild = (unsigned short)BackEndBuildVersion;
	compile->verQFE = (unsigned short)BackEndQFEVersion;
	DebugSSectionPos += sizeof(COMPILESYM3);
	strcpy((char *)DebugSSectionPos, CompilerString.Peek_Buffer());
	DebugSSectionPos += CompilerString.Get_Length();
	DebugSSectionPos++;
	unsigned int CurrentFilePos = 0;
	CurrentFilePos += sizeof(IMAGE_FILE_HEADER);
	IMAGE_FILE_HEADER FileHeader;
	FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
	FileHeader.NumberOfSections = (WORD)sectioncount;
	FileHeader.TimeDateStamp = (DWORD)time(NULL);
	FileHeader.PointerToSymbolTable = 0;
	FileHeader.NumberOfSymbols = symbolcount + sectioncount;
	FileHeader.SizeOfOptionalHeader = 0;
	FileHeader.Characteristics = 0;
	IMAGE_SECTION_HEADER *sections = new IMAGE_SECTION_HEADER[sectioncount];
	unsigned char **sectiondata = new unsigned char *[sectioncount];
	IMAGE_RELOCATION **sectionrelocations = new IMAGE_RELOCATION *[sectioncount];
	int *sectionrelocsymbols = new int[sectioncount];
	CurrentFilePos += (sizeof(IMAGE_SECTION_HEADER) * sectioncount);
	SymbolTableEntry *symbols = new SymbolTableEntry[symbolcount];
	char *strings = new char[stringtablesize];
	char *strpos = strings;
	int stroffset = 4;
	int cursection = 0;
	int cursymbol = 0;
	int cursymnum = 1;
	memcpy(symbols[cursymbol].symbol.N.ShortName, "@comp.id",8);
	symbols[cursymbol].symbol.Value = (CompileType << 16) | CompilerIDVersion;
	symbols[cursymbol].symbol.SectionNumber = IMAGE_SYM_ABSOLUTE;
	symbols[cursymbol].symbol.Type = 0;
	symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
	symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
	cursymbol++;
	cursymnum++;
	memcpy(symbols[cursymbol].symbol.N.ShortName, "@feat.00", 8);
	symbols[cursymbol].symbol.Value = FeatureEnum;
	symbols[cursymbol].symbol.SectionNumber = IMAGE_SYM_ABSOLUTE;
	symbols[cursymbol].symbol.Type = 0;
	symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
	symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
	cursymbol++;
	cursymnum++;
	memcpy(sections[cursection].Name, ".debug$S", 8);
	sections[cursection].Misc.VirtualSize = 0;
	sections[cursection].VirtualAddress = 0;
	sections[cursection].SizeOfRawData = DebugSize;
	sections[cursection].PointerToRawData = CurrentFilePos;
	sections[cursection].PointerToRelocations = 0;
	sections[cursection].NumberOfRelocations = 0;
	sectionrelocations[cursection] = 0;
	sectionrelocsymbols[cursection] = 0;
	sections[cursection].PointerToLinenumbers = 0;
	sections[cursection].NumberOfLinenumbers = 0;
	sections[cursection].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_1BYTES | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
	sectiondata[cursection] = DebugSSection;
	cursection++;
	CurrentFilePos += DebugSize;
	memcpy(symbols[cursymbol].symbol.N.ShortName, ".debug$S", 8);
	symbols[cursymbol].symbol.Value = 0;
	symbols[cursymbol].symbol.SectionNumber = (SHORT)cursection;
	symbols[cursymbol].symbol.Type = 0;
	symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
	symbols[cursymbol].symbol.NumberOfAuxSymbols = 1;
	symbols[cursymbol].aux.Section.Length = DebugSize;
	symbols[cursymbol].aux.Section.NumberOfRelocations = 0;
	symbols[cursymbol].aux.Section.NumberOfLinenumbers = 0;
	symbols[cursymbol].aux.Section.CheckSum = 0;
	symbols[cursymbol].aux.Section.Number = 0;
	symbols[cursymbol].aux.Section.Selection = 0;
	symbols[cursymbol].aux.Section.bReserved = 0;
	symbols[cursymbol].aux.Section.HighNumber = 0;
	cursymbol++;
	cursymnum++;
	for (int i = 0; i < BSSSymbols.Count(); i++)
	{
		if (BSSSymbols[i].Size)
		{
			memcpy(sections[cursection].Name, ".bss\0\0\0\0", 8);
			sections[cursection].Misc.VirtualSize = 0;
			sections[cursection].VirtualAddress = 0;
			sections[cursection].SizeOfRawData = BSSSymbols[i].Size;
			sections[cursection].PointerToRawData = 0;
			sections[cursection].PointerToRelocations = 0;
			sections[cursection].NumberOfRelocations = 0;
			sectionrelocations[cursection] = 0;
			sectionrelocsymbols[cursection] = 0;
			sections[cursection].PointerToLinenumbers = 0;
			sections[cursection].NumberOfLinenumbers = 0;
			sections[cursection].Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
			sectiondata[cursection] = BSSSymbols[i].Data;
			cursection++;
			BSSSymbols[i].SectionNumber = cursection;
			BSSSymbols[i].SectionSymbolNumber = cursymnum;
			memcpy(symbols[cursymbol].symbol.N.ShortName, ".bss\0\0\0\0", 8);
			symbols[cursymbol].symbol.Value = 0;
			symbols[cursymbol].symbol.SectionNumber = (SHORT)cursection;
			symbols[cursymbol].symbol.Type = 0;
			symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
			symbols[cursymbol].symbol.NumberOfAuxSymbols = 1;
			symbols[cursymbol].aux.Section.Length = BSSSymbols[i].Size;
			symbols[cursymbol].aux.Section.NumberOfRelocations = 0;
			symbols[cursymbol].aux.Section.NumberOfLinenumbers = 0;
			symbols[cursymbol].aux.Section.CheckSum = 0;
			symbols[cursymbol].aux.Section.Number = 0;
			symbols[cursymbol].aux.Section.Selection = BSSSymbols[i].Selection;
			symbols[cursymbol].aux.Section.bReserved = 0;
			symbols[cursymbol].aux.Section.HighNumber = 0;
			cursymbol++;
			cursymnum++;
			cursymnum++;
		}
	}
	for (int i = 0; i < BSSSymbols.Count(); i++)
	{
		BSSSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = 0;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)BSSSymbols[i].SectionNumber;
		symbols[cursymbol].symbol.Type = 0;
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, BSSSymbols[i].Name.Peek_Buffer());
		strpos += BSSSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += BSSSymbols[i].Name.Get_Length();
		stroffset++;
	}
	for (int i = 0; i < DataSymbols.Count(); i++)
	{
		if (DataSymbols[i].Size)
		{
			memcpy(sections[cursection].Name, ".data\0\0\0", 8);
			sections[cursection].Misc.VirtualSize = 0;
			sections[cursection].VirtualAddress = 0;
			sections[cursection].SizeOfRawData = DataSymbols[i].Size;
			sections[cursection].PointerToRawData = CurrentFilePos;
			sections[cursection].PointerToRelocations = 0;
			sections[cursection].NumberOfRelocations = 0;
			sectionrelocations[cursection] = 0;
			sectionrelocsymbols[cursection] = 0;
			sections[cursection].PointerToLinenumbers = 0;
			sections[cursection].NumberOfLinenumbers = 0;
			sections[cursection].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
			sectiondata[cursection] = DataSymbols[i].Data;
			cursection++;
			CurrentFilePos += DataSymbols[i].Size;
			DataSymbols[i].SectionNumber = cursection;
			DataSymbols[i].SectionSymbolNumber = cursymnum;
			memcpy(symbols[cursymbol].symbol.N.ShortName, ".data\0\0\0", 8);
			symbols[cursymbol].symbol.Value = 0;
			symbols[cursymbol].symbol.SectionNumber = (SHORT)cursection;
			symbols[cursymbol].symbol.Type = 0;
			symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
			symbols[cursymbol].symbol.NumberOfAuxSymbols = 1;
			symbols[cursymbol].aux.Section.Length = DataSymbols[i].Size;
			symbols[cursymbol].aux.Section.NumberOfRelocations = 0;
			symbols[cursymbol].aux.Section.NumberOfLinenumbers = 0;
			symbols[cursymbol].aux.Section.CheckSum = CRC_MS(DataSymbols[i].Data, DataSymbols[i].Size, 0);
			symbols[cursymbol].aux.Section.Number = 0;
			symbols[cursymbol].aux.Section.Selection = DataSymbols[i].Selection;
			symbols[cursymbol].aux.Section.bReserved = 0;
			symbols[cursymbol].aux.Section.HighNumber = 0;
			cursymbol++;
			cursymnum++;
			cursymnum++;
		}
	}
	for (int i = 0; i < DataSymbols.Count(); i++)
	{
		DataSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = 0;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)DataSymbols[i].SectionNumber;
		symbols[cursymbol].symbol.Type = 0;
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, DataSymbols[i].Name.Peek_Buffer());
		strpos += DataSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += DataSymbols[i].Name.Get_Length();
		stroffset++;
	}
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		if (CodeSymbols[i].Size)
		{
			memcpy(sections[cursection].Name, ".text$mn", 8);
			sections[cursection].Misc.VirtualSize = 0;
			sections[cursection].VirtualAddress = 0;
			sections[cursection].SizeOfRawData = CodeSymbols[i].Size;
			sections[cursection].PointerToRawData = CurrentFilePos;
			CurrentFilePos += CodeSymbols[i].Size;
			if (CodeSymbols[i].Relocations.Count())
			{
				sections[cursection].PointerToRelocations = CurrentFilePos;
				sections[cursection].NumberOfRelocations = (WORD)CodeSymbols[i].Relocations.Count();
				CurrentFilePos += CodeSymbols[i].Relocations.Count() * sizeof(IMAGE_RELOCATION);
				sectionrelocations[cursection] = new IMAGE_RELOCATION[CodeSymbols[i].Relocations.Count()];
				sectionrelocsymbols[cursection] = i;
				for (int j = 0; j < CodeSymbols[i].Relocations.Count(); j++)
				{
					sectionrelocations[cursection][j].VirtualAddress = CodeSymbols[i].Relocations[j].Rva;
					sectionrelocations[cursection][j].SymbolTableIndex = 0;
					sectionrelocations[cursection][j].Type = CodeSymbols[i].Relocations[j].Type;
				}
			}
			else
			{
				sections[cursection].PointerToRelocations = 0;
				sections[cursection].NumberOfRelocations = 0;
				sectionrelocations[cursection] = 0;
				sectionrelocsymbols[cursection] = 0;
			}
			sections[cursection].PointerToLinenumbers = 0;
			sections[cursection].NumberOfLinenumbers = 0;
			sections[cursection].Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_ALIGN_16BYTES | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
			sectiondata[cursection] = CodeSymbols[i].Data;
			cursection++;
			CodeSymbols[i].SectionNumber = cursection;
			CodeSymbols[i].SectionSymbolNumber = cursymnum;
			memcpy(symbols[cursymbol].symbol.N.ShortName, ".text$mn", 8);
			symbols[cursymbol].symbol.Value = 0;
			symbols[cursymbol].symbol.SectionNumber = (SHORT)cursection;
			symbols[cursymbol].symbol.Type = 0;
			symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
			symbols[cursymbol].symbol.NumberOfAuxSymbols = 1;
			symbols[cursymbol].aux.Section.Length = CodeSymbols[i].Size;
			if (CodeSymbols[i].Relocations.Count())
			{
				symbols[cursymbol].aux.Section.NumberOfRelocations = (WORD)CodeSymbols[i].Relocations.Count();
			}
			else
			{
				symbols[cursymbol].aux.Section.NumberOfRelocations = 0;
			}
			symbols[cursymbol].aux.Section.NumberOfLinenumbers = 0;
			symbols[cursymbol].aux.Section.CheckSum = CRC_MS(CodeSymbols[i].Data,CodeSymbols[i].Size,0);
			symbols[cursymbol].aux.Section.Number = 0;
			symbols[cursymbol].aux.Section.Selection = CodeSymbols[i].Selection;
			symbols[cursymbol].aux.Section.bReserved = 0;
			symbols[cursymbol].aux.Section.HighNumber = 0;
			cursymbol++;
			cursymnum++;
			cursymnum++;
		}
	}
	for (int i = 0; i < CodeSymbols.Count(); i++)
	{
		CodeSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = 0;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)CodeSymbols[i].SectionNumber;
		symbols[cursymbol].symbol.Type = 0x20; //DTYPE_FUNCTION
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, CodeSymbols[i].Name.Peek_Buffer());
		strpos += CodeSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += CodeSymbols[i].Name.Get_Length();
		stroffset++;
	}
	for (int i = 0; i < JumpTableSymbols.Count(); i++)
	{
		Symbol &JumpTableCodeSym = FindJumpCodeSymbol(JumpTableSymbols[i].Address);
		JumpTableSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = JumpTableSymbols[i].Address - JumpTableCodeSym.Address;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)JumpTableCodeSym.SectionNumber;
		symbols[cursymbol].symbol.Type = 0;
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, JumpTableSymbols[i].Name.Peek_Buffer());
		strpos += JumpTableSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += JumpTableSymbols[i].Name.Get_Length();
		stroffset++;
	}
	for (int i = 0; i < JumpTargetSymbols.Count(); i++)
	{
		Symbol &JumpTableCodeSym = FindJumpCodeSymbol(JumpTargetSymbols[i].Address);
		JumpTargetSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = JumpTargetSymbols[i].Address - JumpTableCodeSym.Address;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)JumpTableCodeSym.SectionNumber;
		symbols[cursymbol].symbol.Type = 0;
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_LABEL;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, JumpTargetSymbols[i].Name.Peek_Buffer());
		strpos += JumpTargetSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += JumpTargetSymbols[i].Name.Get_Length();
		stroffset++;
	}
	for (int i = 0; i < RDataSymbols.Count(); i++)
	{
		if (RDataSymbols[i].Size)
		{
			memcpy(sections[cursection].Name, ".rdata\0\0", 8);
			sections[cursection].Misc.VirtualSize = 0;
			sections[cursection].VirtualAddress = 0;
			sections[cursection].SizeOfRawData = RDataSymbols[i].Size;
			sections[cursection].PointerToRawData = CurrentFilePos;
			sections[cursection].PointerToRelocations = 0;
			sections[cursection].NumberOfRelocations = 0;
			sectionrelocations[cursection] = 0;
			sectionrelocsymbols[cursection] = 0;
			sections[cursection].PointerToLinenumbers = 0;
			sections[cursection].NumberOfLinenumbers = 0;
			sections[cursection].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ;
			sectiondata[cursection] = RDataSymbols[i].Data;
			cursection++;
			CurrentFilePos += RDataSymbols[i].Size;
			RDataSymbols[i].SectionNumber = cursection;
			RDataSymbols[i].SectionSymbolNumber = cursymnum;
			memcpy(symbols[cursymbol].symbol.N.ShortName, ".rdata\0\0", 8);
			symbols[cursymbol].symbol.Value = 0;
			symbols[cursymbol].symbol.SectionNumber = (SHORT)cursection;
			symbols[cursymbol].symbol.Type = 0;
			symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
			symbols[cursymbol].symbol.NumberOfAuxSymbols = 1;
			symbols[cursymbol].aux.Section.Length = RDataSymbols[i].Size;
			symbols[cursymbol].aux.Section.NumberOfRelocations = 0;
			symbols[cursymbol].aux.Section.NumberOfLinenumbers = 0;
			symbols[cursymbol].aux.Section.CheckSum = CRC_MS(RDataSymbols[i].Data, RDataSymbols[i].Size, 0);
			symbols[cursymbol].aux.Section.Number = 0;
			symbols[cursymbol].aux.Section.Selection = RDataSymbols[i].Selection;
			symbols[cursymbol].aux.Section.bReserved = 0;
			symbols[cursymbol].aux.Section.HighNumber = 0;
			cursymbol++;
			cursymnum++;
			cursymnum++;
		}
	}
	for (int i = 0; i < RDataSymbols.Count(); i++)
	{
		RDataSymbols[i].EntrySymbolNumber = cursymnum;
		symbols[cursymbol].symbol.N.Name.Short = 0;
		symbols[cursymbol].symbol.N.Name.Long = stroffset;
		symbols[cursymbol].symbol.Value = 0;
		symbols[cursymbol].symbol.SectionNumber = (SHORT)RDataSymbols[i].SectionNumber;
		symbols[cursymbol].symbol.Type = 0;
		symbols[cursymbol].symbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
		symbols[cursymbol].symbol.NumberOfAuxSymbols = 0;
		cursymbol++;
		cursymnum++;
		strcpy(strpos, RDataSymbols[i].Name.Peek_Buffer());
		strpos += RDataSymbols[i].Name.Get_Length();
		strpos++;
		stroffset += RDataSymbols[i].Name.Get_Length();
		stroffset++;
	}
	FileHeader.PointerToSymbolTable = CurrentFilePos;
	for (int i = 0; i < sectioncount; i++)
	{
		if (sectionrelocations[i])
		{
			int index = sectionrelocsymbols[i];
			for (int j = 0; j < CodeSymbols[index].Relocations.Count(); j++)
			{
				sectionrelocations[i][j].SymbolTableIndex = CodeSymbols[index].Relocations[j].Symbol->EntrySymbolNumber;
			}
		}
	}
	FILE *of = fopen(OutputFull, "wb");
	fwrite(&FileHeader, sizeof(FileHeader), 1, of);
	for (int i = 0; i < sectioncount; i++)
	{
		fwrite(&sections[i], sizeof(sections[i]), 1, of);
	}
	for (int i = 0; i < sectioncount; i++)
	{
		if (sectiondata[i])
		{
			fwrite(sectiondata[i], 1, sections[i].SizeOfRawData, of);
			if (sectionrelocations[i])
			{
				int index = sectionrelocsymbols[i];
				fwrite(sectionrelocations[i], sizeof(IMAGE_RELOCATION), CodeSymbols[index].Relocations.Count(), of);
			}
		}
	}
	for (int i = 0; i < symbolcount; i++)
	{
		fwrite(&symbols[i].symbol, sizeof(symbols[i].symbol), 1, of);
		if (symbols[i].symbol.NumberOfAuxSymbols)
		{
			fwrite(&symbols[i].aux.Section, sizeof(symbols[i].aux.Section), 1, of);
		}
	}
	int sz = stringtablesize + 4;
	fwrite(&sz, 4, 1, of);
	fwrite(strings, 1, stringtablesize, of);
	fclose(of);
	delete ini;
	return 0;
}
