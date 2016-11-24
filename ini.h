#pragma once
#include "vector.h"
#include "strings.h"
#include "Crc32.h"
class FileClass;
class Straw;
class Pipe;
struct INIEntry : public Node<INIEntry *>
{
public:
	char* Entry; // 000C
	char* Value; // 0010
	~INIEntry()
	{
		delete[] Entry;
		Entry = 0;
		delete[] Value;
		Value = 0;
	}
	INIEntry(char *entry, char *value) : Entry(entry), Value(value)
	{
	}
	int Index_ID()
	{
		return CRC_String(Entry,0);
	}
}; // 0014
struct INISection : public Node<INISection *>
{
public:
	char* Section; // 000C
	List<INIEntry *> EntryList; // 0010
	IndexClass<int,INIEntry *> EntryIndex; // 002C
	~INISection()
	{
		delete[] Section;
		EntryList.Delete();
	}
	INIEntry *Find_Entry(const char *entry)
	{
		if (entry)
		{
			int crc = CRC_String(entry,0);
			if (EntryIndex.Is_Present(crc))
			{
				return EntryIndex[crc];
			}
		}
		return 0;
	}
	INISection(char *section) : Section(section)
	{
	}
	int Index_ID()
	{
		return CRC_String(Section,0);
	}
}; // 0040
class INIClass {
	List<INISection *> *SectionList;
	IndexClass<int,INISection *> *SectionIndex;
	char *Filename;
public:
	static void Strip_Comments(char* buffer);
	static int CRC(char *string);
	void DuplicateCRCError(const char *function,const char* message,const char* entry);
	INIClass();
	INIClass(FileClass &file);
	void Initialize();
	void Shutdown();
	bool Clear(char* section,char* entry);
	int Get_Int(char const *section,char const *entry,int defaultvalue) const;
	uint Get_Color_UInt(char const *section, char const *entry, uint defaultvalue) const;
	float Get_Float(char const *section,char const *entry,float defaultvalue) const;
	bool Get_Bool(char const *section,char const *entry,bool defaultvalue) const;
	int Get_String(char const *section,char const *entry,char const *defaultvalue,char *result,int size) const;
	StringClass &Get_String(StringClass &str, const char* section, const char* entry, const char* defaultvalue = 0) const;
	bool Put_String(const char* section, const char* entry, const char* string);
	bool Put_Int(const char* section, const char* entry, int value, int format);
	bool Put_Bool(const char* section, const char* entry, bool value);
	bool Put_Float(const char* section, const char* entry, float value);
	int Entry_Count(char const *section) const;
	const char *Get_Entry(char const *section,int index) const;
	INIEntry *Find_Entry(const char* section,const char* entry) const;
	INISection *Find_Section(const char* section) const;
	virtual ~INIClass();
	int Load(Straw& ffile);
	int Load(FileClass& file);
	int Save(FileClass& file);
	int Save(Pipe& pipe);
	int Section_Count() const;
	bool Is_Present(const char *section,const char *entry) const
	{
		if (entry)
		{
			return Find_Entry(section,entry) != 0;
		}
		else
		{
			return Find_Section(section) != 0;
		}
	}
	bool Section_Present(const char *section) const
	{
		return Find_Section(section) != 0;
	}
	List<INISection *> &Get_Section_List()
	{
		return *SectionList;
	}
	IndexClass<int,INISection *>&Get_Section_Index()
	{
		return *SectionIndex;
	}
};
