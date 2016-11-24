#pragma once
#include "RawFileClass.h"
class Straw {
private:
	Straw* ChainTo;
	Straw* ChainFrom;
public:
	Straw();
	virtual ~Straw();
	virtual void Get_From(Straw* straw);
	virtual int Get(void* source,int slen);
};
class Buffer {
private:
	void* BufferPtr;
	long Size;
	bool IsAllocated;
public:
	Buffer(void* buffer,long size);
	Buffer(long size) : BufferPtr(0), Size(size), IsAllocated(false)
	{
		if (size > 0)
		{
			BufferPtr = new char[size];
			IsAllocated = true;
		}
	}
	void *Get_Buffer()
	{
		return BufferPtr;
	}
	long Get_Size()
	{
		return Size;
	}
	~Buffer();
};

class BufferStraw : public Straw  {
private:
	Buffer BufferPtr;
	int Index;
public:
	BufferStraw(void* buffer, int size);
	~BufferStraw();
	int Get(void* source,int slen);
};

class FileClass;
class FileStraw : public Straw {
private:
	FileClass* File;
	bool HasOpened;
public:
	FileStraw(FileClass&);
	~FileStraw();
	int Get(void* source,int slen);
};

class CacheStraw : public Straw {
private:
	Buffer BufferPtr;
	int Index;
	int Length;
public:
	CacheStraw(int size) : BufferPtr(size), Index(0), Length(0)
	{
	}
	bool Is_Valid()
	{
		return BufferPtr.Get_Buffer() != 0;
	}
	~CacheStraw() {}
	int Get(void* source,int slen)
	{
		char *src = (char *)source;
		int len = slen;
		int ret = 0;
		int len2;
		if (BufferPtr.Get_Buffer())
		{
			for (int i = source == 0;!i && len > 0;i = len2 == 0)
			{
				if (Length > 0 )
				{
					int sz = len;
					if (len > this->Length)
					{
						sz = Length;
					}
					memmove(src,(char *)BufferPtr.Get_Buffer() + Index,sz);
					len -= sz;
					Index += sz;
					ret += sz;
					Length -= sz;
					src += sz;
				}
				if (!len)
				{
					break;
				}
				len2 = Straw::Get(BufferPtr.Get_Buffer(),BufferPtr.Get_Size());
				Length = len2;
				Index = 0;
			}
		}
		return ret;
	}
};

class Pipe
{
private:
	Pipe* ChainTo;
	Pipe* ChainFrom;
public:
	Pipe() : ChainTo(0), ChainFrom(0)
	{
	}
	virtual ~Pipe()
	{
		if (ChainTo)
		{
			ChainTo->ChainFrom = ChainFrom;
		}
		if (ChainFrom)
		{
			ChainFrom->Put_To(ChainTo);
		}
		ChainFrom = 0;
		ChainTo = 0;
	}
	virtual int Flush()
	{
		if (ChainTo)
		{
			return ChainTo->Flush();
		}
		else
		{
			return 0;
		}
	}
	virtual int End()
	{
		return Flush();
	}
	virtual void Put_To(Pipe* pipe)
	{
		if (ChainTo != pipe)
		{
			if (pipe && pipe->ChainFrom)
			{
				pipe->ChainFrom->Put_To(0);
				pipe->ChainFrom = 0;
			}
			if (ChainTo)
			{
				ChainTo->ChainFrom = 0;
				ChainTo->Flush();
			}
			ChainTo = pipe;
			if (pipe)
			{
				pipe->ChainFrom = this;
			}
		}
	}
	virtual int Put(const void *source, int length)
	{
		if (ChainTo)
		{
			return ChainTo->Put(source, length);
		}
		return length;
	}
};

class BufferPipe : public Pipe
{
private:
	Buffer BufferPtr;
	int Index;
public:
	BufferPipe(void *data,int size) : BufferPtr(data,size), Index(0)
	{
	}
	virtual int Put(const void *source,int length)
	{
		if (BufferPtr.Get_Buffer() && source && length > 0)
		{
			int len = length;
			int size = BufferPtr.Get_Size();
			if (size)
			{
				len = size - Index;
				if (len > length)
				{
					len = length;
				}
			}
			if (len > 0)
			{
				memmove((char *)(BufferPtr.Get_Buffer()) + Index,source,len);
			}
			Index += len;
			return len;
		}
		return 0;
	}
};

class FilePipe : public Pipe
{
private:
	FileClass* File;
	bool HasOpened;
public:
	FilePipe(FileClass *file) : File(file), HasOpened(false)
	{
	}
	virtual ~FilePipe()
	{
		if (File && HasOpened)
		{
			HasOpened = false;
			File->Close();
			File = 0;
		}
	}
	virtual int End()
	{
		int ret = Flush();
		if (File && HasOpened)
		{
			HasOpened = false;
			File->Close();
		}
		return ret;
	}
	virtual int Put(const void *source,int length)
	{
		if (File && source && length > 0)
		{
			if (!File->Is_Open())
			{
				HasOpened = true;
				File->Open(2);
			}
			return File->Write((void *)source,length);
		}
		else
		{
			return 0;
		}
	}
};
