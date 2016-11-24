#pragma once

#include "RawFileClass.h"
class BufferedFileClass : public RawFileClass {
private:
	unsigned char* Buffer;
	unsigned int BufferSize;
	int BufferAvailable;
	int BufferOffset;
public:
	BufferedFileClass() : Buffer(0), BufferSize(0), BufferAvailable(0), BufferOffset(0)
	{
	}
	~BufferedFileClass()
	{
		Reset_Buffer();
	}
	int Read(void* buffer,int size)
	{
		void *tmp = buffer;
		int sz = size;
		int result = 0;
		if (BufferAvailable > 0)
		{
			int buf = BufferAvailable;
			if (buf > size)
			{
				buf = size;
			}
			memcpy(buffer, Buffer + BufferOffset, buf);
			BufferAvailable -= buf;
			BufferOffset += buf;
			sz -= buf;
			tmp = (char *)tmp + buf;
			result += buf;
		}
		if (sz)
		{
			int buf = BufferSize;
			if (!buf)
			{
				buf = 16384;
			}
			if (sz > buf)
			{
				return result + RawFileClass::Read(tmp,sz);
			}
			if (!BufferSize)
			{
				BufferSize = 16384;
				Buffer = new unsigned char[16384];
				BufferAvailable = 0;
				BufferOffset = 0;
			}
			if (!Buffer)
			{
				return 0;
			}
			if (!BufferAvailable)
			{
				BufferAvailable = RawFileClass::Read(Buffer,BufferSize);
				BufferOffset = 0;
			}
			if (BufferAvailable > 0)
			{
				int buf2 = BufferAvailable;
				if (buf2 > sz)
				{
					buf2 = sz;
				}
				if (!Buffer)
				{
					return 0;
				}
				memcpy(tmp,Buffer+BufferOffset,buf2);
				BufferAvailable -= buf2;
				BufferOffset += buf2;
				result += buf2;
			}
		}
		return result;
	}
	int Seek(int pos,int dir)
	{
		if (dir != 1 || pos < 0)
		{
			Reset_Buffer();
		}
		if (BufferAvailable)
		{
			int buf = BufferAvailable;
			if (buf > pos)
			{
				buf = pos;
			}
			BufferAvailable -= buf;
			BufferOffset += buf;
			return RawFileClass::Seek(pos - buf,dir) - BufferAvailable;
		}
		else
		{
			return RawFileClass::Seek(pos,dir);
		}
	}
	int Write(void* buffer,int size)
	{
		return RawFileClass::Write(buffer,size);
	}
	void Close()
	{
		RawFileClass::Close();
		Reset_Buffer();
	}
	void Reset_Buffer()
	{
		if (Buffer)
		{
			delete[] Buffer;
			Buffer = 0;
			BufferSize = 0;
			BufferAvailable = 0;
			BufferOffset = 0;
		}
	}
};
