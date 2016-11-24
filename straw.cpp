#include "general.h"
#include "straw.h"

Straw::Straw() : ChainTo(0), ChainFrom(0)
{
}

Straw::~Straw()
{
	if (ChainTo)
	{
		ChainTo->ChainFrom = ChainFrom;
	}
	if (ChainFrom)
	{
		ChainFrom->Get_From(ChainFrom);
	}
	ChainTo = 0;
	ChainFrom = 0;
}

void Straw::Get_From(Straw* straw)
{
	if (ChainTo != straw)
	{
		if ((straw) && (ChainFrom))
		{
			ChainFrom->Get_From(0);
			ChainFrom = 0;
		}
		if (ChainTo)
		{
			ChainFrom = 0;
		}
		ChainTo = straw;
		if (straw)
		{
			straw->ChainFrom = this;
		}
	}
}

int Straw::Get(void* source,int slen)
{
	if (!ChainTo)
	{
		return 0;
	}
	return ChainTo->Get(source,slen);
}

Buffer::Buffer(void* buffer,long size)
{
	BufferPtr = buffer;
	Size = size;
	IsAllocated = false;
	if ((!buffer) && (size > 0))
	{
		BufferPtr = new char[size];
		IsAllocated = true;
	}
}

Buffer::~Buffer()
{
	if (IsAllocated)
	{
		delete[] BufferPtr;
	}
	BufferPtr = 0;
	Size = 0;
	IsAllocated = false;
}

BufferStraw::BufferStraw(void* buffer, int size) : BufferPtr(buffer,size), Index(0)
{
}

BufferStraw::~BufferStraw()
{
}

int BufferStraw::Get(void* source,int slen)
{
	if ((!BufferPtr.Get_Buffer()) || (!source) || (slen <= 0))
	{
		return 0;
	}
	int bsz = BufferPtr.Get_Size();
	int sz = slen;
	if (bsz)
	{
		bsz -= Index;
		if (slen > bsz)
		{
			sz = bsz;
		}
	}
	if (sz)
	{
		memmove(source,((char *)BufferPtr.Get_Buffer())+Index,sz);
	}
	Index += sz;
	return sz;
}

FileStraw::FileStraw(FileClass& f) : Straw()
{
	File = &f;
	HasOpened = false;
}

FileStraw::~FileStraw()
{
	if (File)
	{
		if (HasOpened)
		{
			File->Close();
			HasOpened = false;
			File = 0;
		}
	}
}

int FileStraw::Get(void* source,int slen)
{
	if (File && source && slen)
	{
		if (!File->Is_Open())
		{
			HasOpened = true;
			if (!File->Is_Available() || !File->Open())
			{
				return 0;
			}
		}
		return File->Read(source,slen);
	}
	else
	{
		return 0;
	}
}
