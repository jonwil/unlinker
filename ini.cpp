#include "general.h"
#include "ini.h"
#include "straw.h"
#include "BufferedFileClass.h"
#include "Crc32.h"
int INIClass::Section_Count() const
{
	return SectionIndex->Count();
}

int INIClass::Load(FileClass& file)
{
	FileStraw straw(file);
	if (Filename)
	{
		delete[] Filename;
	}
	Filename = newstr(file.File_Name());
	return Load(straw);
}

int Read_Line(Straw& straw,char* line,int lineSize,bool& isLast)
{
		if (!line || lineSize == 0)
				return 0;
		int i;
		for (i = 0;;)
		{
				char c;
				int getResult = straw.Get(&c, 1);
				if (getResult != 1)
				{
						isLast = true;
						break;
				}
				if (c == '\n')
						break;
				if (c != '\r' && i + 1 < lineSize)
						line[i++] = c;
		}
		line[i] = '\0';
		strtrim(line);
		return (int)strlen(line);
}

int INIClass::Load(Straw& straw)
{
		bool isLastLine = false;
		CacheStraw cacheStraw(4096);
		cacheStraw.Get_From(&straw);
		char line[512];
 
		// Ignore everything above first section (indicated by a line like "[sectionName]")
		while (!isLastLine)
		{
				Read_Line(cacheStraw, line, 512, isLastLine);
				if (isLastLine)
						return false;
				if (line[0] == '[' && strchr(line, ']'))
						break;
		}
 
		if (Section_Count() > 0)
		{
				while (!isLastLine)
				{
						UL_ASSERT(line[0] == '[' && strchr(line, ']')); // at start of section
						line[0] = ' ';
						*strchr(line, ']') = '\0';
						strtrim(line);
						char sectionName[64];
						strcpy(sectionName, line);
						while (!isLastLine)
						{
								int count = Read_Line(cacheStraw, line, 512, isLastLine);
								if (line[0] == '[' && strchr(line, ']'))
										break;
								Strip_Comments(line);
								if (count)
								{
										if (line[0] != ';')
										{
												if (line[0] != '=')
												{
														char* delimiter = strchr(line, '=');
														if (delimiter)
														{
																*delimiter = '\0';
																char* key = line;
																char* value = delimiter + 1;
																strtrim(key);
																if (key[0] != '\0')
																{
																		strtrim(value);
																		if (value[0] == '\0')
																		{
																				continue;
																		}
																		if (!Put_String(sectionName, key, value))
																				return false;
																}
														}
												}
										}
								}
						}
				}
		}
		else
		{
				while (!isLastLine)
				{
						UL_ASSERT(line[0] == '[' && strchr(line, ']')); // at start of section
						line[0] = ' ';
						*strchr(line, ']') = '\0';
						strtrim(line);
						INISection* section = new INISection(newstr(line));
						if (!section)
						{
								Clear(0, 0);
								return false;
						}
						while (!isLastLine)
						{
								int count = Read_Line(cacheStraw, line, 512, isLastLine);
								if (line[0] == '[' && strchr(line, ']'))
										break;
								Strip_Comments(line);
								char* delimiter = strchr(line, '=');
								if (count)
								{
										if (line[0] != ';')
										{
												if (line[0] != '=')
												{
														if (delimiter)
														{
																*delimiter = '\0';
																char* key = line;
																char* value = delimiter + 1;
																strtrim(key);
																if (key[0] != '\0')
																{
																		strtrim(value);
																		if (value[0] == '\0')
																		{
																				continue;
																		}
																		INIEntry* entry = new INIEntry(newstr(key), newstr(value));
																		if (!entry)
																		{
																				delete section;
																				Clear(0, 0);
																				return false;
																		}
																		uint32 crc = CRC_String(entry->Entry, 0);
																		if (section->EntryIndex.Is_Present(crc))
																				DuplicateCRCError(__FUNCTION__, section->Section, line);
																		section->EntryIndex.Add_Index(crc, entry);
																		section->EntryList.Add_Tail(entry);
																}
														}
												}
										}
								}
						}
						if (section->EntryList.Is_Empty())
						{
								delete section;
						}
						else
						{
								uint32 crc = CRC_String(section->Section, 0);
								SectionIndex->Add_Index(crc, section);
								SectionList->Add_Tail(section);
						}
				}
		}
		return true;
}

StringClass &INIClass::Get_String(StringClass& string, const char* section, const char* entry, const char *defaultvalue) const
{
	const char *value = defaultvalue;
	if (!section || !entry)
	{
		string = "";
	}
	INIEntry *Entry = Find_Entry(section,entry);
	if (Entry)
	{
		value = Entry->Value;
	}
	if (value)
	{
		string = value;
	}
	else
	{
		string = "";
	}
	return string;
}
INISection *INIClass::Find_Section(const char* section) const
{
	if (section)
	{
		int crc = CRC_String(section,0);
		if (SectionIndex->Is_Present(crc))
		{
			return (*SectionIndex)[crc];
		}
	}
	return 0;
}
INIEntry *INIClass::Find_Entry(const char* section,const char* entry) const
{
	INISection *Section = Find_Section(section);
	if (Section)
	{
		return Section->Find_Entry(entry);
	}
	return 0;
}
int INIClass::Get_Int(char const *section,char const *entry,int defaultvalue) const
{
	if (section)
	{
		if (entry)
		{
			INIEntry *Entry = Find_Entry(section,entry);
			if (Entry)
			{
				if (Entry->Value)
				{
					int *value;
					const char *pattern;
					if (Entry->Value[0] == '$')
					{
						value = &defaultvalue;
						pattern = "$%x";
					}
					else
					{
						if (tolower(Entry->Value[strlen(Entry->Value) - 1]) != 'h')
						{
							return atoi(Entry->Value);
						}
						value = &defaultvalue;
						pattern = "%xh";
					}
#pragma warning(suppress: 6031) //warning C6031: return value ignored
					sscanf(Entry->Value, pattern, value);
				}
			}
		}
	}
	return defaultvalue;
}
float INIClass::Get_Float(char const *section,char const *entry,float defaultvalue) const
{
	if (section)
	{
		if (entry)
		{
			INIEntry *Entry = Find_Entry(section,entry);
			if (Entry)
			{
				if (Entry->Value)
				{
					float c = defaultvalue;
#pragma warning(suppress: 6031) //warning C6031: return value ignored
					sscanf(Entry->Value, "%f", &c);
					defaultvalue = c;
					if (strchr(Entry->Value, '%'))
					{
						defaultvalue = defaultvalue / 100.0f;
					}
				}
			}
		}
	}
	return defaultvalue;
}
bool INIClass::Get_Bool(char const *section,char const *entry,bool defaultvalue) const
{
	if (section)
	{
		if (entry)
		{
			INIEntry *Entry = Find_Entry(section,entry);
			if (Entry)
			{
				if (Entry->Value)
				{
					switch ( toupper(Entry->Value[0]) )
					{
						case '1':
						case 'T':
						case 'Y':
							return true;
							break;
						case '0':
						case 'F':
						case 'N':
							return false;
							break;
					}
				}
			}
		}
	}
	return defaultvalue;
}
int INIClass::Get_String(char const *section,char const *entry,char const *defaultvalue,char *result,int size) const
{
	if (!result || size <= 1 || !section || !entry)
	{
		return 0;
	}
	INIEntry *Entry = Find_Entry(section,entry);
	const char *value = defaultvalue;
	if (Entry)
	{
		if (Entry->Value)
		{
			value = Entry->Value;
		}
	}
	if (!value)
	{
		result[0] = 0;
		return 0;
	}
	strncpy(result, value, size);
	result[size - 1] = 0;
	strtrim(result);
	return (int)strlen(result);
}
int INIClass::Entry_Count(char const *section) const
{
	INISection *Section = Find_Section(section);
	if (Section)
	{
		return Section->EntryIndex.Count();
	}
	return 0;
}
const char *INIClass::Get_Entry(char const *section,int index) const
{
	int count = index;
	INISection *Section = Find_Section(section);
	if (Section)
	{
		if (Section && index < Section->EntryIndex.Count())
		{
			for (INIEntry *i = Section->EntryList.First();i; i = i->Next())
			{
				if (!i->Is_Valid())
				{
					break;
				}
				if (!count)
				{
					return i->Entry;
				}
				count--;
			}
		}
	}
	return 0;
}
void INIClass::Initialize()
{
	SectionList = new List<INISection *>;
	SectionIndex = new IndexClass<int,INISection *>;
	Filename = newstr("<unknown>");
}
void INIClass::Shutdown()
{
	if (SectionList)
	{
		delete SectionList;
	}
	if (SectionIndex)
	{
		delete SectionIndex;
	}
	if (Filename)
	{
		delete[] Filename;
	}
}
bool INIClass::Clear(char* section,char* entry)
{
	if (section)
	{
		INISection *Section = Find_Section(section);
		if (Section)
		{
			if (entry)
			{
				INIEntry *Entry = Section->Find_Entry(entry);
				if (!Entry)
				{
					return true;
				}
				Section->EntryIndex.Remove_Index(CRC_String(Entry->Entry,0));
				delete Entry;
				return true;
			}
			else
			{
				SectionIndex->Remove_Index(CRC_String(Section->Section,0));
				delete Section;
				return true;
			}
		}
	}
	else
	{
		SectionList->Delete();
		SectionIndex->Clear();
		delete[] Filename;
		Filename = newstr("<unknown>");
	}
	return true;
}
int INIClass::CRC(char *string)
{
	return CRC_String(string,0);
}

INIClass::INIClass()
{
	Filename = 0;
	Initialize();
}

INIClass::INIClass(FileClass &file)
{
	Filename = 0;
	Initialize();
	Load(file);
}

INIClass::~INIClass()
{
	Clear(0,0);
	Shutdown();
}

uint INIClass::Get_Color_UInt(char const *section, char const *entry, uint defaultvalue) const
{
	char buf[256], hex[10];

	sprintf(buf,"%sHex",entry);
	this->Get_String(section, buf, "0xNotValid", hex, 10); 
	if (strcmp(hex, "0xNotValid") != 0) // We've got us a supposedly valid hex value
	{
		uint color;
		int res = sscanf(hex, "%lx", &color);
		if (res == 1) return color; // Yay, we've got a color. Party!
	};

	uint a, r, g, b;
	sprintf(buf,"%sAlpha",entry);
	a = this->Get_Int(section, buf, (defaultvalue >> 24) & 0xFF);
	sprintf(buf,"%sRed",entry);
	r = this->Get_Int(section, buf, (defaultvalue >> 16) & 0xFF);
	sprintf(buf,"%sGreen",entry);
	g = this->Get_Int(section, buf, (defaultvalue >> 8) & 0xFF);
	sprintf(buf,"%sBlue",entry);
	b = this->Get_Int(section, buf, defaultvalue & 0xFF);

	return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
};

void INIClass::Strip_Comments(char* buffer)
{
	if (buffer)
	{
		char *buf = strrchr(buffer,';');
		if (buf)
		{
			buf[0] = 0;
			strtrim(buffer);
		}
	}
}

void INIClass::DuplicateCRCError(const char *function,const char* section,const char* entry)
{
	char OutputString[512];
	_snprintf(OutputString,512,"%s - Duplicate Entry \"%s\" in section \"%s\" (%s)\n",function,entry,section,Filename);
	OutputString[511] = 0;
	OutputDebugStringA(OutputString);
	MessageBoxA(0,OutputString,"Duplicate CRC in INI file.",16);
}

int INIClass::Save(FileClass& file)
{
	FilePipe pipe(&file);
	if (Filename)
	{
		delete[] Filename;
	}
	Filename = newstr(file.File_Name());
	return Save(pipe);
}

int INIClass::Save(Pipe& pipe)
{
	int pos = 0;
	for (INISection *i = SectionList->First();i;i = i->Next())
	{
		if (!i->Is_Valid())
		{
			break;
		}
		int i1 = pipe.Put("[",1) + pos;
		int i2 = pipe.Put(i->Section,(int)strlen(i->Section)) + i1;
		int i3 = pipe.Put("]",1) + i2;
		int i4 = pipe.Put("\n",(int)strlen("\n")) + i3;
		for (INIEntry *j = i->EntryList.First();j;j = j->Next())
		{
			if (!j->Is_Valid())
			{
				break;
			}
			int i5 = pipe.Put(j->Entry,(int)strlen(j->Entry)) + i4;
			int i6 = pipe.Put("=",1) + i5;
			int i7 = pipe.Put(j->Value,(int)strlen(j->Value)) + i6;
			i4 = pipe.Put("\n",(int)strlen("\n")) + i7;
		}
		pos = pipe.Put("\n",(int)strlen("\n")) + i4;
	}
	return pipe.End() + pos;
}

bool INIClass::Put_String(const char* section, const char* entry, const char* string)
{
	if (!section || !entry)
	{
		return false;
	}
	INISection *sec = Find_Section(section);
	if (!sec)
	{
		sec = new INISection(newstr(section));
		SectionList->Add_Tail(sec);
		SectionIndex->Add_Index(CRC_String(sec->Section,0),sec);
	}
	INIEntry *ent = sec->Find_Entry(entry);
	if (ent)
	{
		if (strcmp(ent->Entry,entry))
		{
			DuplicateCRCError("INIClass::Put_String",section,entry);
		}
		SectionIndex->Remove_Index(CRC_String(ent->Entry,0));
		if (ent)
		{
			delete ent;
		}
	}
	if (string && *string)
	{
		ent = new INIEntry(newstr(entry),newstr(string));
		sec->EntryList.Add_Tail(ent);
		sec->EntryIndex.Add_Index(CRC_String(ent->Entry,0),ent);
	}
	return true;
}

bool INIClass::Put_Int(const char* section, const char* entry, int value, int format)
{
	char *form;
	if (format == 1)
	{
		form = "%Xh";
	}
	else
	{
		if (format > 1 && format == 2)
		{
			form = "$%X";
		}
		else
		{
			form = "%d";
		}
	}
	char buf[524];
	sprintf(buf,form,value);
	return Put_String(section,entry,buf);
}

bool INIClass::Put_Bool(const char* section, const char* entry, bool value)
{
	char *str;
	if (value)
	{
		str = "yes";
	}
	else
	{
		str = "no";
	}
	return Put_String(section,entry,str);
}

bool INIClass::Put_Float(const char* section, const char* entry, float value)
{
	char buf[524];
	sprintf(buf,"%f",value);
	return Put_String(section,entry,buf);
}
