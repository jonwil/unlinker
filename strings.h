#ifndef SCRIPTS_INCLUDE__ENGINE_STRING_H
#define SCRIPTS_INCLUDE__ENGINE_STRING_H



class StringClass {
public:
	StringClass(bool hint_temporary);
	StringClass(int initial_len = 0, bool hint_temporary = false);
	StringClass(const StringClass& string,bool hint_temporary = false);
	StringClass(const char *string,bool hint_temporary = false);
	StringClass(char ch, bool hint_temporary = false);
	StringClass(const wchar_t *string, bool hint_temporary = false);
	~StringClass();
	bool operator ==(const char* rvalue) const;
	bool operator!= (const char* rvalue) const;
	const StringClass& operator=(const char* string);
	const StringClass& operator=(const StringClass& string);
	const StringClass& operator=(char ch);
	const StringClass& operator=(const wchar_t *string);

	const StringClass& operator+=(const char* string);
	const StringClass& operator+=(const StringClass& string);
	const StringClass& operator+=(char ch);

	friend StringClass operator+ (const StringClass &string1, const StringClass &string2);
	friend StringClass operator+ (const StringClass &string1, const char *string2);
	friend StringClass operator+ (const char *string1, const StringClass &string2);

	bool operator < (const char *string) const;
	bool operator <= (const char *string) const;
	bool operator > (const char *string) const;
	bool operator >= (const char *string) const;

	const char & operator[] (int index) const;
	char & operator[] (int index);
	operator const char * (void) const;

	int	Compare (const char *string) const;
	int Compare_No_Case (const char *string) const;

	int	Get_Length (void) const;
	bool Is_Empty (void) const;
	void Erase (int start_index, int char_count);
	int __cdecl Format(const char* format,...);
	int __cdecl Format_Args(const char* format,const va_list& arg_list);
	char *Get_Buffer (int new_length);
	char *Peek_Buffer (void);
	const char * Peek_Buffer (void) const;
	bool Copy_Wide (const wchar_t *source);
	static void Release_Resources();

  /*!
  * Finds any occurances of the search string within this string and replaces them with a specified
  * replacement string.
  *
  * \param[in] search
  *   The substring to be replaced
  * \param[in] replace
  *   The replacement string to insert
  * \param[in] bCaseSensitive
  *   True to perform a case sensitive search, false if case doesn't matter
  * \param[in] maxCount
  *   The maximum number of replacements to perform, or -1 to replace all instances
  *
  * \return
  *   The number of replacements that were made
  */
	int Replace(const char* search, const char* replace, bool bCaseSensitive = true, int maxCount=-1);

	void TruncateLeft(uint truncateLength)
	{
		uint length = Get_Length();
		if (length <= truncateLength)
			Free_String();

		else
		{
			int newLength = length - truncateLength;
			memmove(m_Buffer, m_Buffer + truncateLength, newLength + 1);
			Store_Length(newLength);
		}
	}

	void TruncateRight(uint truncateLength)
	{
		uint length = Get_Length();
		if (length <= truncateLength)
			Free_String();

		else
		{
			int newLength = length - truncateLength;
			m_Buffer[newLength] = '\0';
			Store_Length(newLength);
		}
	}

	void cropTo(int to)
	{
		const int length = Get_Length();
		if (to <= 0)
			Free_String();
		else if (to < length)
		{
			m_Buffer[to] = '\0';
			Store_Length(to);
		}
	}

	void cropFrom(int from)
	{
		const int length = Get_Length();
		if (from >= length)
			Free_String();
		else if (from > 0)
		{
			memmove(m_Buffer, m_Buffer + from, length - from + 1);
			Store_Length(length - from);
		}
	}

	void crop(int from, int to)
	{
		cropTo(to);
		cropFrom(from);
	}

	void TrimLeft()
	{
		char* iter = m_Buffer;
		for (; *iter != '\0' && *iter <= ' '; iter++);

		TruncateLeft((int)(iter - m_Buffer));
	}

	void TrimRight()
	{
		char* iter = m_Buffer + Get_Length() - 1;
		for (; *iter != '\0' && *iter <= ' '; iter--);

		TruncateRight((int)(m_Buffer + Get_Length() - 1 - iter));
	}

	void Trim()
	{
		TrimLeft();
		TrimRight();
	}

	bool StartsWithI(const char* string)
	{
		return _strnicmp(m_Buffer, string, strlen(string)) == 0;
	}

    int IndexOf(char c)
    {
        int length = Get_Length();
        for (int i = 0; i < length; ++i)
        {
            if (m_Buffer[i] == c) return i;
        }
        return -1;
    }

    int LastIndexOf(char c)
    {
        for (int i = Get_Length() - 1; i >= 0; --i)
        {
            if (m_Buffer[i] == c) return i;
        }
        return -1;
    }

	void ToUpper()
	{
		_strupr(m_Buffer);
	}

	void ToLower()
	{
		_strlwr(m_Buffer);
	}

	StringClass AsUpper() const
	{
		StringClass result = *this;
		result.ToUpper();
		return result;
	}

	StringClass AsLower() const
	{
		StringClass result = *this;
		result.ToLower();
		return result;
	}

	static StringClass getFormattedString(const char* format, ...)
	{
		va_list arguments;
		StringClass result;

		va_start(arguments, format);
		result.Format_Args(format, arguments);
		va_end(arguments);

		return result;
	}

private:
	typedef struct _HEADER
	{
		int	allocated_length;
		int	length;
	} HEADER;
	enum
	{
		MAX_TEMP_STRING	= 8,
		MAX_TEMP_LEN		= 256-sizeof(_HEADER),
		MAX_TEMP_BYTES		= (MAX_TEMP_LEN * sizeof (char)) + sizeof (HEADER),
	};
	void Get_String(int length,bool is_temp);
	char* Allocate_Buffer(int len);
	void Resize(int new_len);
	void Uninitialised_Grow(int new_len);
	void Free_String();
	void Store_Length(int length);
	void Store_Allocated_Length(int length);
	HEADER *Get_Header() const;
	int Get_Allocated_Length() const;
	void Set_Buffer_And_Allocated_Length(char *buffer, int length);
	char* m_Buffer;
	static char __declspec(thread) TempStrings[MAX_TEMP_STRING][MAX_TEMP_BYTES];
	static unsigned int __declspec(thread) FreeTempStrings;
	static char *m_EmptyString;
	static char m_NullChar;
};

inline const StringClass &StringClass::operator= (const StringClass &string)
{	
	int len = string.Get_Length();
	Uninitialised_Grow(len+1);
	Store_Length(len);
	memcpy (m_Buffer, string.m_Buffer, (len+1) * sizeof (char));		
	return (*this);
}

inline const StringClass &StringClass::operator= (const char *string)
{
	if (string != 0)
	{
		int len = (int)strlen (string);
		Uninitialised_Grow (len+1);
		Store_Length (len);
		memcpy (m_Buffer, string, (len + 1) * sizeof (char));		
	}
	return (*this);
}

inline const StringClass &StringClass::operator= (const wchar_t *string)
{
	if (string != 0)
	{
		Copy_Wide (string);
	}
	return (*this);
}

inline const StringClass &StringClass::operator= (char ch)
{
	Uninitialised_Grow (2);
	m_Buffer[0] = ch;
	m_Buffer[1] = m_NullChar;
	Store_Length (1);
	return (*this);
}

inline StringClass::StringClass (bool hint_temporary) : m_Buffer (m_EmptyString)
{
	Get_String (MAX_TEMP_LEN, hint_temporary);
	m_Buffer[0]	= m_NullChar;
	return ;
}

inline StringClass::StringClass (int initial_len, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	Get_String (initial_len, hint_temporary);
	m_Buffer[0]	= m_NullChar;
}

inline StringClass::StringClass (char ch, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	Get_String (2, hint_temporary);
	(*this) = ch;
}

inline StringClass::StringClass (const StringClass &string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	if (hint_temporary || (string.Get_Length()>0))
	{
		Get_String (string.Get_Length()+1, hint_temporary);
	}
	(*this) = string;
}

inline StringClass::StringClass (const char *string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	int len=string ? (int)strlen(string) : 0;
	if (hint_temporary || len>0)
	{
		Get_String (len+1, hint_temporary);
	}
	(*this) = string;
}

inline StringClass::StringClass (const wchar_t *string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	int len = string ? (int)wcslen (string) : 0;
	if (hint_temporary || len > 0)
	{
		Get_String (len + 1, hint_temporary);
	}
	(*this) = string;
}

inline StringClass::~StringClass (void)
{
	Free_String ();
}

inline bool StringClass::Is_Empty (void) const
{
	return (m_Buffer[0] == m_NullChar);
}

inline int StringClass::Compare (const char *string) const
{
	return strcmp (m_Buffer, string);
}

inline int StringClass::Compare_No_Case (const char *string) const
{
	return _stricmp (m_Buffer, string);
}

inline const char &StringClass::operator[] (int index) const
{
	return m_Buffer[index];
}

inline char &StringClass::operator[] (int index)
{
	return m_Buffer[index];
}

inline StringClass::operator const char * (void) const
{
	return m_Buffer;
}

inline bool StringClass::operator== (const char *rvalue) const
{
	return (Compare (rvalue) == 0);
}

inline bool StringClass::operator!= (const char *rvalue) const
{
	return (Compare (rvalue) != 0);
}

inline bool StringClass::operator < (const char *string) const
{
	return (strcmp (m_Buffer, string) < 0);
}

inline bool StringClass::operator <= (const char *string) const
{
	return (strcmp (m_Buffer, string) <= 0);
}

inline bool StringClass::operator > (const char *string) const
{
	return (strcmp (m_Buffer, string) > 0);
}

inline bool StringClass::operator >= (const char *string) const
{
	return (strcmp (m_Buffer, string) >= 0);
}

inline void StringClass::Erase (int start_index, int char_count)
{
	int len = Get_Length ();
	if (start_index < len)
	{
		if (start_index + char_count > len)
		{
			char_count = len - start_index;
		}
		memmove (	&m_Buffer[start_index], &m_Buffer[start_index + char_count], (len - (start_index + char_count) + 1) * sizeof (char));
		Store_Length( len - char_count );
	}
}

inline const StringClass &StringClass::operator+= (const char *string)
{
	int cur_len = Get_Length ();
	int src_len = (int)strlen (string);
	int new_len = cur_len + src_len;
	Resize (new_len + 1);
	Store_Length (new_len);
	memcpy (&m_Buffer[cur_len], string, (src_len + 1) * sizeof (char));
	return (*this);
}

inline const StringClass &StringClass::operator+= (char ch)
{
	int cur_len = Get_Length ();
	Resize (cur_len + 2);
	m_Buffer[cur_len] = ch;
	m_Buffer[cur_len + 1] = m_NullChar;
	if (ch != m_NullChar)
	{
		Store_Length (cur_len + 1);
	}
	return (*this);
}

inline char *StringClass::Get_Buffer (int new_length)
{
	Uninitialised_Grow (new_length);
	return m_Buffer;
}

inline char *StringClass::Peek_Buffer (void)
{
	return m_Buffer;
}

inline const char *StringClass::Peek_Buffer (void) const
{
	return m_Buffer;
}

inline const StringClass &StringClass::operator+= (const StringClass &string)
{
	int src_len = string.Get_Length();
	if (src_len > 0)
	{
		int cur_len = Get_Length ();
		int new_len = cur_len + src_len;
		Resize (new_len + 1);
		Store_Length (new_len);
		memcpy (&m_Buffer[cur_len], (const char *)string, (src_len + 1) * sizeof (char));				
	}
	return (*this);
}

inline StringClass operator+ (const StringClass &string1, const StringClass &string2)
{
	StringClass new_string(string1, true);
	new_string += string2;
	return new_string;
}

inline StringClass operator+ (const char *string1, const StringClass &string2)
{
	StringClass new_string(string1, true);
	new_string += string2;
	return new_string;
}

inline StringClass operator+ (const StringClass &string1, const char *string2)
{
	StringClass new_string(string1, true);
	StringClass new_string2(string2, true);
	new_string += new_string2;
	return new_string;
}

inline int StringClass::Get_Allocated_Length (void) const
{
	int allocated_length = 0;
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		allocated_length = header->allocated_length;
	}
	return allocated_length;
}

inline int StringClass::Get_Length (void) const
{
	int length = 0;
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		length = header->length;
		if (length == 0)
		{
			length = (int)strlen (m_Buffer);
			((StringClass *)this)->Store_Length (length);
		}
	}
	return length;
}

inline void StringClass::Set_Buffer_And_Allocated_Length (char *buffer, int length)
{
	Free_String ();
	m_Buffer = buffer;
	if (m_Buffer != m_EmptyString)
	{
		Store_Allocated_Length (length);
		Store_Length (0);		
	}
}

inline char *StringClass::Allocate_Buffer (int length)
{
	char *buffer = new char[(sizeof (char) * length) + sizeof (StringClass::_HEADER)];
	HEADER *header = reinterpret_cast<HEADER *>(buffer);
	header->length = 0;
	header->allocated_length = length;
	return reinterpret_cast<char *>(buffer + sizeof (StringClass::_HEADER));
}

inline StringClass::HEADER *StringClass::Get_Header (void) const
{
	return reinterpret_cast<HEADER *>(((char *)m_Buffer) - sizeof (StringClass::_HEADER));
}

inline void StringClass::Store_Allocated_Length (int allocated_length)
{
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		header->allocated_length = allocated_length;
	}
}

inline void StringClass::Store_Length (int length)
{
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header();
		header->length = length;
	}
}

class WideStringClass {
public:
	WideStringClass (int initial_len = 0, bool hint_temporary = false);
	WideStringClass (const WideStringClass &string,	bool hint_temporary = false);
	WideStringClass (const wchar_t *string, bool hint_temporary = false);
	WideStringClass (wchar_t ch, bool hint_temporary = false);
	WideStringClass (const char *string, bool hint_temporary = false);
	~WideStringClass (void);
	bool operator== (const wchar_t *rvalue) const;
	bool operator!= (const wchar_t *rvalue) const;
	const WideStringClass &operator= (const WideStringClass &string);
	const WideStringClass &operator= (const wchar_t *string);
	const WideStringClass &operator= (wchar_t ch);
	const WideStringClass &operator= (const char *string);

	const WideStringClass &operator+= (const WideStringClass &string);
	const WideStringClass &operator+= (const wchar_t *string);
	const WideStringClass &operator+= (wchar_t ch);

	friend WideStringClass operator+ (const WideStringClass &string1, const WideStringClass &string2);
	friend WideStringClass operator+ (const wchar_t *string1, const WideStringClass &string2);
	friend WideStringClass operator+ (const WideStringClass &string1, const wchar_t *string2);

	bool operator < (const wchar_t *string) const;
	bool operator <= (const wchar_t *string) const;
	bool operator > (const wchar_t *string) const;
	bool operator >= (const wchar_t *string) const;

	wchar_t operator[] (int index) const;
	wchar_t& operator[] (int index);
	operator const wchar_t * (void) const;

	int Compare (const wchar_t *string) const;
	int Compare_No_Case (const wchar_t *string) const;
	int Get_Length (void) const;
	bool Is_Empty (void) const;
	void Erase (int start_index, int char_count);
	int _cdecl Format (const wchar_t *format, ...);
	int _cdecl Format_Args (const wchar_t *format, const va_list & arg_list );
	bool Convert_From (const char *text);
	bool Convert_To (StringClass &string);
	bool Convert_To (StringClass &string) const;
	bool Is_ANSI(void);
	wchar_t *		Get_Buffer (int new_length);
	wchar_t *		Peek_Buffer (void);
	const wchar_t*	Peek_Buffer() const;
	static void	Release_Resources (void);

	void TruncateLeft(uint truncateLength)
	{
		uint length = Get_Length();
		if (length <= truncateLength)
			Free_String();

		else
		{
			int newLength = length - truncateLength;
			memmove(m_Buffer, m_Buffer + truncateLength, (newLength + 1)*2);
			Store_Length(newLength);
		}
	}
	void TruncateRight(uint truncateLength)
	{
		uint length = Get_Length();
		if (length <= truncateLength)
			Free_String();
		else
		{
			int newLength = length - truncateLength;
			m_Buffer[newLength] = L'\0';
			Store_Length(newLength);
		}
	}

	void TrimLeft()
	{
		wchar_t* iter = m_Buffer;
		for (; *iter != L'\0' && *iter <= L' '; iter++);
		TruncateLeft((int)(iter - m_Buffer));
	}

	void TrimRight()
	{
		wchar_t* iter = m_Buffer + Get_Length() - 1;
		for (; *iter != L'\0' && *iter <= L' '; iter--);
		TruncateRight((int)(m_Buffer + Get_Length() - 1 - iter));
	}

	void Trim()
	{
		TrimLeft();
		TrimRight();
	}

	static WideStringClass getFormattedString(const wchar_t* format, ...)
	{
		va_list arguments;
		WideStringClass result;

		va_start(arguments, format);
		result.Format_Args(format, arguments);
		va_end(arguments);

		return result;
	}

	WideStringClass Substring(int start, int length) const;
	void RemoveSubstring(int start, int length);
	void ReplaceSubstring(int start, int length, const WideStringClass& substring);

private:
	typedef struct _HEADER
	{
		int	allocated_length;
		int	length;
	} HEADER;
	enum
	{
		MAX_TEMP_STRING	= 4,
		MAX_TEMP_LEN		= 256,
		MAX_TEMP_BYTES		= (MAX_TEMP_LEN * sizeof (wchar_t)) + sizeof (HEADER),
	};
	void Get_String(int length,bool is_temp);
	wchar_t *		Allocate_Buffer (int length);
	void			Resize (int size);
	void			Uninitialised_Grow (int length);
	void Free_String();
	void Store_Length(int length);
	void Store_Allocated_Length(int length);
	HEADER *Get_Header() const;
	int Get_Allocated_Length() const;
	void Set_Buffer_And_Allocated_Length(wchar_t *buffer, int length);
	wchar_t* m_Buffer;
	static char __declspec(thread) TempStrings[MAX_TEMP_STRING][MAX_TEMP_BYTES];
	static unsigned int __declspec(thread) FreeTempStrings;
	static wchar_t *m_EmptyString;
	static wchar_t m_NullChar;
};

inline WideStringClass::WideStringClass (int initial_len, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	Get_String (initial_len, hint_temporary);
	m_Buffer[0] = m_NullChar;
}

inline WideStringClass::WideStringClass (wchar_t ch, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	Get_String (2, hint_temporary);
	(*this) = ch;
}

inline WideStringClass::WideStringClass (const WideStringClass &string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	if (hint_temporary || (string.Get_Length()>1))
	{
		Get_String(string.Get_Length()+1, hint_temporary);
	}
	(*this) = string;
}

inline WideStringClass::WideStringClass (const wchar_t *string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	int len=string ? (int)wcslen(string) : 0;
	if (hint_temporary || len>0)
	{
		Get_String (len+1, hint_temporary);
	}
	(*this) = string;
}


inline WideStringClass::WideStringClass (const char *string, bool hint_temporary) : m_Buffer (m_EmptyString)
{
	if (hint_temporary || (string && strlen(string)>0))
	{
		Get_String ((int)strlen(string) + 1, hint_temporary);
	}
	(*this) = string;
}

inline WideStringClass::~WideStringClass (void)
{
	Free_String ();
}

inline bool WideStringClass::Is_Empty (void) const
{
	return (m_Buffer[0] == m_NullChar);
}

inline int WideStringClass::Compare (const wchar_t *string) const
{
	if (string)
	{
		return wcscmp (m_Buffer, string);
	}
	return -1;
}

inline int WideStringClass::Compare_No_Case (const wchar_t *string) const
{
	if (string)
	{
		return _wcsicmp (m_Buffer, string);
	}
	return -1;
}

inline wchar_t WideStringClass::operator[] (int index) const
{
	return m_Buffer[index];
}

inline wchar_t& WideStringClass::operator[] (int index)
{
	return m_Buffer[index];
}

inline WideStringClass::operator const wchar_t * (void) const
{
	return m_Buffer;
}

inline bool WideStringClass::operator== (const wchar_t *rvalue) const
{
	return (Compare (rvalue) == 0);
}

inline bool WideStringClass::operator!= (const wchar_t *rvalue) const
{
	return (Compare (rvalue) != 0);
}

inline const WideStringClass & WideStringClass::operator= (const WideStringClass &string)
{	
	return operator= ((const wchar_t *)string);
}

inline bool WideStringClass::operator < (const wchar_t *string) const
{
	if (string)
	{
		return (wcscmp (m_Buffer, string) < 0);
	}
	return false;
}

inline bool WideStringClass::operator <= (const wchar_t *string) const
{
	if (string)
	{
		return (wcscmp (m_Buffer, string) <= 0);
	}
	return false;
}

inline bool WideStringClass::operator > (const wchar_t *string) const
{
	if (string)
	{
		return (wcscmp (m_Buffer, string) > 0);
	}
	return true;
}

inline bool WideStringClass::operator >= (const wchar_t *string) const
{
	if (string)
	{
		return (wcscmp (m_Buffer, string) >= 0);
	}
	return true;
}

inline void WideStringClass::Erase (int start_index, int char_count)
{
	int len = Get_Length ();
	if (start_index < len)
	{
		if (start_index + char_count > len)
		{
			char_count = len - start_index;
		}
		memmove (&m_Buffer[start_index],&m_Buffer[start_index + char_count],(len - (start_index + char_count) + 1) * sizeof (wchar_t));
		Store_Length( (int)wcslen(m_Buffer) );
	}
}

inline const WideStringClass & WideStringClass::operator= (const wchar_t *string)
{
	if (string)
	{
		int len = (int)wcslen (string);
		Uninitialised_Grow (len + 1);
		Store_Length (len);
		memcpy (m_Buffer, string, (len + 1) * sizeof (wchar_t));		
	}
	return (*this);
}

inline const WideStringClass &WideStringClass::operator= (const char *string)
{
	Convert_From(string);
	return (*this);
}

inline const WideStringClass &WideStringClass::operator= (wchar_t ch)
{
	Uninitialised_Grow (2);
	m_Buffer[0] = ch;
	m_Buffer[1] = m_NullChar;
	Store_Length (1);
	return (*this);
}

inline const WideStringClass &WideStringClass::operator+= (const wchar_t *string)
{
	if (string)
	{
		int cur_len = Get_Length ();
		int src_len = (int)wcslen (string);
		int new_len = cur_len + src_len;
		Resize (new_len + 1);
		Store_Length (new_len);
		memcpy (&m_Buffer[cur_len], string, (src_len + 1) * sizeof (wchar_t));
	}
	return (*this);
}

inline const WideStringClass &WideStringClass::operator+= (wchar_t ch)
{
	int cur_len = Get_Length ();
	Resize (cur_len + 2);
	m_Buffer[cur_len] = ch;
	m_Buffer[cur_len + 1] = m_NullChar;
	if (ch != m_NullChar)
	{
		Store_Length (cur_len + 1);
	}
	return (*this);
}

inline wchar_t *WideStringClass::Get_Buffer (int new_length)
{
	Uninitialised_Grow (new_length);
	return m_Buffer;
}

inline wchar_t *WideStringClass::Peek_Buffer (void)
{
	return m_Buffer;
}

inline const wchar_t* WideStringClass::Peek_Buffer() const
{
	return m_Buffer;
}

inline const WideStringClass &WideStringClass::operator+= (const WideStringClass &string)
{
	int src_len = string.Get_Length();
	if (src_len > 0)
	{
		int cur_len = Get_Length ();
		int new_len = cur_len + src_len;
		Resize (new_len + 1);
		Store_Length (new_len);
		memcpy (&m_Buffer[cur_len], (const wchar_t *)string, (src_len + 1) * sizeof (wchar_t));				
	}
	return (*this);
}

inline WideStringClass operator+ (const WideStringClass &string1, const WideStringClass &string2)
{
	WideStringClass new_string(string1, true);
	new_string += string2;
	return new_string;
}

inline WideStringClass operator+ (const wchar_t *string1, const WideStringClass &string2)
{
	WideStringClass new_string(string1, true);
	new_string += string2;
	return new_string;
}

inline WideStringClass operator+ (const WideStringClass &string1, const wchar_t *string2)
{
	WideStringClass new_string(string1, true);
	new_string += string2;
	return new_string;
}

inline int WideStringClass::Get_Allocated_Length (void) const
{
	int allocated_length = 0;
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		allocated_length = header->allocated_length;		
	}
	return allocated_length;
}

inline int WideStringClass::Get_Length (void) const
{
	int length = 0;
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		length = header->length;
		if (length == 0)
		{
			length = (int)wcslen (m_Buffer);
			((WideStringClass *)this)->Store_Length (length);
		}
	}
	return length;
}

inline void WideStringClass::Set_Buffer_And_Allocated_Length (wchar_t *buffer, int length)
{
	Free_String ();
	m_Buffer = buffer;
	if (m_Buffer != m_EmptyString)
	{
		Store_Allocated_Length (length);
		Store_Length (0);		
	}
}

inline wchar_t * WideStringClass::Allocate_Buffer (int length)
{
	char *buffer = new char[(sizeof (wchar_t) * length) + sizeof (WideStringClass::_HEADER)];
	HEADER *header = reinterpret_cast<HEADER *>(buffer);
	header->length = 0;
	header->allocated_length = length;
	return reinterpret_cast<wchar_t *>(buffer + sizeof (WideStringClass::_HEADER));
}

inline WideStringClass::HEADER * WideStringClass::Get_Header (void) const
{
	return reinterpret_cast<HEADER *>(((char *)m_Buffer) - sizeof (WideStringClass::_HEADER));
}

inline void WideStringClass::Store_Allocated_Length (int allocated_length)
{
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		header->allocated_length = allocated_length;
	}
}

inline void WideStringClass::Store_Length (int length)
{
	if (m_Buffer != m_EmptyString)
	{
		HEADER *header = Get_Header ();
		header->length = length;
	}
}

inline bool WideStringClass::Convert_To (StringClass &string)
{
	return (string.Copy_Wide (m_Buffer));
}

inline bool WideStringClass::Convert_To (StringClass &string) const
{
	return (string.Copy_Wide (m_Buffer));
}

struct hash_istring: public std::unary_function<const char*, size_t>
{
    size_t operator()(const char* str) const
    {
        // djb2
        unsigned long hash = 5381;
        while (int c = tolower(*str++)) hash = hash * 33 + c;
        return hash;
    }

    size_t operator()(const StringClass& str) const
    {
        return (*this)(str.Peek_Buffer());
    }
};

struct equals_istring: public std::binary_function<const char*, const char*, bool>
{
    bool operator()(const char* a, const char* b) const
    {
        return _stricmp(a, b) == 0;
    }

    size_t operator()(const StringClass& a, const StringClass& b) const
    {
        return _stricmp(a.Peek_Buffer(), b.Peek_Buffer()) == 0;
    }

    size_t operator()(const StringClass& a, const char* b) const
    {
        return _stricmp(a.Peek_Buffer(), b) == 0;
    }

    size_t operator()(const char* a, const StringClass& b) const
    {
        return _stricmp(a, b.Peek_Buffer()) == 0;
    }
};

struct hash_string : public std::unary_function<const char*, size_t>
{
    size_t operator()(const char* str) const
    {
        // djb2
        unsigned long hash = 5381;
        while (int c = *str++) hash = hash * 33 + c;
        return hash;
    }

    size_t operator()(const StringClass& str) const
    {
        return (*this)(str.Peek_Buffer());
    }
};

struct equals_string : public std::binary_function<const char*, const char*, bool>
{
    bool operator()(const char* a, const char* b) const
    {
        return strcmp(a, b) == 0;
    }

    size_t operator()(const StringClass& a, const StringClass& b) const
    {
        return strcmp(a.Peek_Buffer(), b.Peek_Buffer()) == 0;
    }

    size_t operator()(const StringClass& a, const char* b) const
    {
        return strcmp(a.Peek_Buffer(), b) == 0;
    }

    size_t operator()(const char* a, const StringClass& b) const
    {
        return strcmp(a, b.Peek_Buffer()) == 0;
    }
};

const wchar_t *CharToWideChar(const char *str); //convert a char to a wide char
const char *WideCharToChar(const wchar_t *wcs); //convert a wide char to a char
char *newstr(const char *str); //duplicate a character string
wchar_t *newwcs(const wchar_t *str);  //duplicate a wide character string
char *strtrim(char *); //trim a string
char* strrtrim(char *); //trim trailing whitespace from a string
const char *stristr(const char *str, const char *substr); //like strstr but case insenstive
const wchar_t *wcsistr(const wchar_t *str, const wchar_t *substr); //like strstr but case insenstive and for wchar_t
#endif
