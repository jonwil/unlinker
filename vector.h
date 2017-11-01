#pragma once
#include "CriticalSectionClass.h"
#include <new>

template <typename T> class NoEqualsClass
{
public:
	bool operator== (const T &src)
	{
		return false;
	};
	bool operator!= (const T &src)
	{
		return true;
	};
};

class NoInitClass {
	public:
		void operator () (void) const {};
};
template <class T> class VectorClass {
protected:
	T *Vector; // 0004
	int VectorMax; // 0008
	bool IsValid; // 000C
	bool IsAllocated; // 000D
	bool VectorClassPad[2]; // 000E
public:
	VectorClass(NoInitClass const &)
	{
	}
	explicit VectorClass(int size=0, T const * array=0) : Vector(0),VectorMax(size),IsValid(true),IsAllocated(false)
	{
		if (size)
		{
			if (array)
			{
				Vector = new((void*)array) T[size];
			}
			else
			{
				Vector = new T[size];
				IsAllocated = true;
			}
		}
	}
	VectorClass(VectorClass<T> const & vector) : Vector(0),VectorMax(0),IsValid(true),IsAllocated(false)
	{
		*this = vector;
	}
	VectorClass<T> &operator= (VectorClass<T> const &vector)
	{
		if (this != &vector)
		{
			Clear();
			VectorMax = vector.Length();
			if (VectorMax)
			{
				try {
					Vector = new T[VectorMax];
					IsAllocated = true;
					for (int index = 0; index < VectorMax; index++)
					{
						Vector[index] = vector[index];
					}
				}
				catch (std::bad_alloc& ba) {
				}
			}
			else
			{
				Vector = 0;
				IsAllocated = false;
			}
		}
		return *this;
	}

#if _MSC_VER >= 1600
	/* id theft and destruction of evidence */
	VectorClass(VectorClass<T>&& vector): Vector(vector.Vector), VectorMax(vector.VectorMax), IsValid(vector.IsValid), IsAllocated(vector.IsAllocated)
	{
		vector.Vector = 0;
	}

	VectorClass<T>& operator=(VectorClass<T>&& vector)
	{
		if (this != &vector)
		{
			delete[] Vector;
			Vector = vector.Vector;
			VectorMax = vector.VectorMax;
			IsValid = vector.IsValid;
			IsAllocated = vector.IsAllocated;
			vector.Vector = 0;
		}
		return *this;
	}
#endif

	virtual ~VectorClass()
	{
		Clear();
	}

	T & operator[](int index)
	{
		return(Vector[index]);
	} 

	T const & operator[](int index) const
	{
		return(Vector[index]);
	}

	virtual bool operator== (VectorClass<T> const &vector) const
	{
		if (VectorMax == vector.Length())
		{
			for (int index = 0; index < VectorMax; index++)
			{
				if (Vector[index] != vector[index])
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}
	virtual bool Resize(int newsize, T const * array=0)
	{
		if (newsize)
		{
			T *newptr;
			IsValid = false;
			if (!array)
			{
				newptr = new T[newsize];
			}
			else
			{
				newptr = new((void*)array) T[newsize];
			}
			IsValid = true;
			if (!newptr)
			{
				return false;
			}
			if (Vector != NULL)
			{
				int copycount = (newsize < VectorMax) ? newsize : VectorMax;
				for (int index = 0; index < copycount; index++)
				{
					newptr[index] = std::move(Vector[index]);
				}
				if (IsAllocated)
				{
					delete[] Vector;
					Vector = 0;
				}
			}
			Vector = newptr;
			VectorMax = newsize;
			IsAllocated = (Vector && !array);
		}
		else
		{
			Clear();
		}
		return true;
	}
	virtual void Clear(void)
	{
		if (Vector)
		{
			if (IsAllocated) delete[] Vector;
			Vector = 0;
			VectorMax = 0;
			IsAllocated = false;
		}
	}
	int Length(void) const
	{
		return VectorMax;
	}
	virtual int ID(T const *ptr)
	{
		if (!IsValid)
		{
			return 0;
		}
		return(((unsigned long)ptr - (unsigned long)&(*this)[0]) / sizeof(T));
	}
	virtual int ID(T const &object)
	{
		if (!IsValid)
		{
			return 0;
		}
		for (int index = 0; index < VectorMax; index++)
		{
			if ((*this)[index] == object)
			{
				return index;
			}
		}
		return -1;
	}
}; // 0010

template <class T> class DynamicVectorClass : public VectorClass<T> {
protected:
	int ActiveCount = 0; // 0010
	int GrowthStep = 10; // 0014
public:

	explicit DynamicVectorClass(unsigned size = 0, T const *array = 0) : VectorClass<T>(size, array)
	{

	}

	DynamicVectorClass(const DynamicVectorClass<T>& vector): VectorClass<T>(vector), GrowthStep(vector.GrowthStep), ActiveCount(vector.ActiveCount)
	{
		/* nothing */
	}

    template <int size>
    explicit DynamicVectorClass<T>(T (&arr)[size]) : VectorClass<T>(size, arr)
    {

    }

	DynamicVectorClass<T> & operator =(DynamicVectorClass<T> const &rvalue)
	{
		VectorClass<T>::operator =(rvalue);
		ActiveCount = rvalue.ActiveCount;
		GrowthStep = rvalue.GrowthStep;
		return *this;
	}

#if _MSC_VER >= 1600
	/* stealing candy from babies */
	DynamicVectorClass(DynamicVectorClass<T>&& vector): VectorClass<T>(std::move(vector)), GrowthStep(vector.GrowthStep), ActiveCount(vector.ActiveCount)
	{
		/* nothing */
	}

	DynamicVectorClass<T>& operator=(DynamicVectorClass<T>&& vector)
	{
		if (this != &vector)
		{
			VectorClass<T>::operator =(std::move(vector));
			ActiveCount = vector.ActiveCount;
			GrowthStep = vector.GrowthStep;
		}
		return *this;
	}
#endif

	bool operator== (const DynamicVectorClass &src)
	{
		return false;
	}
	bool operator!= (const DynamicVectorClass &src)
	{
		return true;
	}
	bool Resize(int newsize, T const *array = 0)
	{
		if (VectorClass<T>::Resize(newsize, array))
		{
			if (Length() < ActiveCount)
			{
				ActiveCount = Length();
			}
			return true;
		}
		return false;
	}
	void Clear(void)
	{
		ActiveCount = 0;
		VectorClass<T>::Clear();
	}
	void Reset_Active(void)
	{
		ActiveCount = 0;
	}
	void Set_Active(int count)
	{
		ActiveCount = count;
	}
	int Count(void) const
	{
		return(ActiveCount);
	}
	bool Add(T const &object)
	{
		if (ActiveCount >= Length())
		{
			if ((IsAllocated || !VectorMax) && GrowthStep > 0)
			{
				if (!Resize(Length() + GrowthStep))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		(*this)[ActiveCount++] = object;
		return true;
	}
	bool Add_Head(T const &object)
	{
		return Insert(0, object);
	}
	bool Insert(int index,T const &object)
	{
		if (index < 0)
		{
			return false;
		}
		if (index > ActiveCount)
		{
			return false;
		}
		if (ActiveCount >= Length())
		{
			if ((IsAllocated || !VectorMax) && GrowthStep > 0)
			{
				if (!Resize(Length() + GrowthStep))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		for (int i = ActiveCount; i > index; --i)
		{
			Vector[i] = Vector[i-1];
		}
		(*this)[index] = object;
		ActiveCount++;
		return true;
	}

#if _MSC_VER >= 1600
	/* these versions carry move semantics for capable objects */
	bool Add(T&& object)
	{
		if (ActiveCount >= Length())
		{
			if ((IsAllocated || !VectorMax) && GrowthStep > 0)
			{
				if (!Resize(Length() + GrowthStep))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		(*this)[ActiveCount++] = std::move(object);
		return true;
	}

	bool Add_Head(T&& object)
	{
		return Insert(0, std::move(object));
	}

	bool Insert(int index, T&& object)
	{
		if (index < 0)
		{
			return false;
		}
		if (index > ActiveCount)
		{
			return false;
		}
		if (ActiveCount >= Length())
		{
			if ((IsAllocated || !VectorMax) && GrowthStep > 0)
			{
				if (!Resize(Length() + GrowthStep))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		for (int i = ActiveCount; i > index; --i)
		{
			Vector[i] = std::move(Vector[i-1]);
		}
		(*this)[index] = std::move(object);
		ActiveCount++;
		return true;
	}
#endif

	bool DeleteObj(T const &object)
	{
		int id = ID(object);
		if (id != -1)
		{
			return Delete(id);
		}
		return false;
	}
	bool Delete(int index)
	{
		if (index < ActiveCount)
		{
			ActiveCount--;
			for (int i = index; i < ActiveCount; i++)
			{
				(*this)[i] = std::move((*this)[i+1]);
			}
			return true;
		}
		return false;
	}
	void Delete_All(void)
	{
		int len = VectorMax;
		Clear();
		Resize(len);
	}
	int Set_Growth_Step(int step)
	{
		return(GrowthStep = step);
	}
	int Growth_Step(void)
	{
		return GrowthStep;
	}
	virtual int ID(T const *ptr)
	{
		return(VectorClass<T>::ID(ptr));
	}
	virtual int ID(T const &object)
	{
		for (int index = 0; index < Count(); index++)
		{
			if ((*this)[index] == object)
			{
				return(index);
			}
		}
		return -1;
	}
	T *Uninitialized_Add(void)
	{
		if (ActiveCount >= Length())
		{
			if (GrowthStep > 0)
			{
				if (!Resize(Length() + GrowthStep))
				{
					return NULL;
				}
			}
			else
			{
				return NULL;
			}
		}
		return &((*this)[ActiveCount++]);
	}
	void Add_Multiple(int number_to_add)
	{
		for (int i = 0;i < number_to_add;i++)
		{
			Uninitialized_Add();
		}
	}
}; // 0018


template <typename T> class PointerStack
{
public:
	inline PointerStack(T* initial_item)
	{
		m_pStack[0] = initial_item;
		m_iDepth = 1;
	}

	inline T* Pop()
	{
		UL_ASSERT(m_iDepth >= 0);
		return m_pStack[--m_iDepth];
	}

	inline void Push(T* data)
	{
		UL_ASSERT(m_iDepth < 128);
		if (m_iDepth >= 128) return;
		m_pStack[m_iDepth++] = data;
	}

	int Depth()
	{
		return m_iDepth;
	}

private:
	T* m_pStack[128];
	int m_iDepth;
};

template <class T> class SimpleVecClass {
protected:
	T *Vector; // 0004
	int VectorMax; // 0008
public:
	explicit SimpleVecClass(int size = 0)
	{
		Vector = 0;
		VectorMax = 0;
		if (size > 0)
		{
			Resize(size);
		}
	}
	virtual ~SimpleVecClass()
	{
		if (Vector)
		{
			delete[] Vector;
			Vector = 0;
			VectorMax = 0;
		}
	}

	SimpleVecClass(const SimpleVecClass<T>& vector)
	{
		Vector = (T*)(new char[vector.VectorMax * sizeof(T)]);
		VectorMax = vector.VectorMax;
		memcpy(Vector,vector.Vector, VectorMax * sizeof(T));
	}

	SimpleVecClass<T>& operator =(const SimpleVecClass<T>& vector)
	{
		if (this != &vector)
		{
			delete[] Vector;
			Vector = (T*)(new char[vector.VectorMax * sizeof(T)]);
			VectorMax = vector.VectorMax;
			memcpy(Vector,vector.Vector, VectorMax * sizeof(T));
		}
		return *this;
	}

#if _MSC_VER >= 1600
	/* more move semantics, yay! */
	SimpleVecClass(SimpleVecClass<T>&& vector): Vector(vector.Vector), VectorMax(vector.VectorMax)
	{
		vector.Vector = 0;
	}

	SimpleVecClass<T>& operator =(SimpleVecClass<T>&& vector)
	{
		if (this != &vector)
		{
			delete[] Vector;
			Vector = vector.Vector;
			VectorMax = vector.VectorMax;
			vector.Vector = 0;
		}
		return *this;
	}
#endif

	virtual bool Resize(int newsize)
	{
		if (VectorMax == newsize)
		{
			return true;
		}
		if (newsize > 0)
		{
			T *vec = new T[newsize];
			if (Vector)
			{
				int count = VectorMax;
				if (newsize < count)
				{
					count = newsize;
				}
				memcpy(vec,Vector,count*sizeof(T));
				delete[] Vector;
				Vector = 0;
			}
			Vector = vec;
			VectorMax = newsize;
		}
		else
		{
			VectorMax = 0;
			if (Vector)
			{
				delete[] Vector;
				Vector = 0;
			}
		}
		return true;
	}
	virtual bool Uninitialised_Grow(int newsize)
	{
		if (newsize <= VectorMax)
		{
			return true;
		}
		if (newsize > 0)
		{
			if (Vector)
			{
				delete[] Vector;
			}
			Vector = new T[newsize];
			VectorMax = newsize;
		}
		return true;
	}

	void Uninitialized_Resize(int newsize)
	{
		UL_ASSERT(newsize > 0);
		delete[] Vector;
		Vector = new T[newsize];
		VectorMax = newsize;
	}

	int Length() const
	{
		return VectorMax;
	}
	T &operator[](int index)
	{
		return Vector[index];
	}
	T const &operator[](int index) const
	{
		return Vector[index];
	}

    T* Peek()
    {
        return Vector;
    }

    const T* Peek() const
    {
        return Vector;
    }

	void Zero_Memory()
	{
		if (Vector != NULL)
		{
			memset(Vector,0,VectorMax * sizeof(T));
		}
	}
}; // 000C



template <class T> class SimpleDynVecClass :
	public SimpleVecClass<T>
{

protected:

	int ActiveCount; // 000C

	bool Grow(int new_size_hint)
	{
		int new_size = max(VectorMax + VectorMax/4,VectorMax + 4);
		new_size = max(new_size,new_size_hint);
		return Resize(new_size);
	}
	bool Shrink(void)
	{
		if (ActiveCount < VectorMax/4)
		{
			return Resize(ActiveCount);
		}
		return true;
	}
	
public:
	virtual ~SimpleDynVecClass()
	{
		ActiveCount = 0;
	}
	explicit SimpleDynVecClass(int size = 0) : SimpleVecClass<T>(size)
	{
		ActiveCount = 0;
	}

	SimpleDynVecClass(const SimpleDynVecClass<T>& vector): SimpleVecClass(vector), ActiveCount(vector.ActiveCount)
	{
		/* nothing */
	}

	SimpleDynVecClass<T>& operator =(const SimpleDynVecClass<T>& vector)
	{
		if (this != &vector)
		{
			SimpleVecClass<T>::operator =(vector);
			ActiveCount = vector.ActiveCount;
		}
		return *this;
	}

#if _MSC_VER >= 1600
	/* move semantics ftw */
	SimpleDynVecClass(SimpleDynVecClass<T>&& vector): SimpleVecClass(std::move(vector)), ActiveCount(vector.ActiveCount)
	{
		/* nothing */
	};

	SimpleDynVecClass<T>& operator =(SimpleDynVecClass<T>&& vector)
	{
		if (this != &vector)
		{
			SimpleVecClass::operator =(std::move(vector));
			ActiveCount = vector.ActiveCount;
		}
		return *this;
	}
#endif

	int Find_Index(T const & object)
	{
		for (int index = 0;index < Count();index++)
		{
			if (Vector[index] == object)
			{
				return index;
			}
		}
		return -1;
	}

	int Count() const
	{
		return ActiveCount;
	}
	T &operator[](int index)
	{
		return Vector[index];
	}
	T const &operator[](int index) const
	{
		return Vector[index];
	}
	bool Resize(int newsize)
	{
		if (SimpleVecClass<T>::Resize(newsize))
		{
			if (VectorMax < ActiveCount)
			{
				ActiveCount = VectorMax;
			}
			return true;
		}
		return false;
	}
	bool Add(T const& data, int new_size_hint = 0)
	{
		if (ActiveCount >= VectorMax)
		{
			if (!Grow(new_size_hint))
			{
				return false;
			}
		}
		Vector[ActiveCount++] = data;
		return true;
	}
	T *Add_Multiple(int number_to_add)
	{
		int index = ActiveCount;
		ActiveCount += number_to_add;
		if (ActiveCount >= VectorMax)
		{
			Grow( ActiveCount );
		}
		return &Vector[index];
	}
	bool Add_Head(const T& object)
	{
		return Insert(0, object);
	}
	bool Insert(int index, const T& object)
	{
		UL_ASSERT(index >= 0 && index <= ActiveCount);
		if (ActiveCount >= VectorMax)
		{
			if (!Grow(0))
			{
				return false;
			}
		}
		if (index < ActiveCount)
		{
			memmove(&Vector[index+1], &Vector[index], (ActiveCount-index) * sizeof(T));
		}
		Vector[index] = object;
		++ActiveCount;
		return true;
	}
	bool Delete(int index,bool allow_shrink = true)
	{
		if (index < ActiveCount-1)
		{
			memmove(&(Vector[index]),&(Vector[index+1]),(ActiveCount - index - 1) * sizeof(T));
		}
		ActiveCount--;
		if (allow_shrink)
		{
			Shrink();
		}
		return true;
	}
	bool Delete(T const & object,bool allow_shrink = true)
	{
		int id = Find_Index(object);
		if (id != -1)
		{
			return Delete(id,allow_shrink);
		}
		return false;
	}
	bool Delete_Range(int start,int count,bool allow_shrink = true)
	{
		if (start < ActiveCount - count)
		{
			memmove(&(Vector[start]),&(Vector[start + count]),(ActiveCount - start - count) * sizeof(T));
		}
		ActiveCount -= count;
		if (allow_shrink)
		{
			Shrink();
		}
		return true;
	}
	void Delete_All(bool allow_shrink = true)
	{
		ActiveCount = 0;
		if (allow_shrink)
		{
			Shrink();
		}
	}

	void qsort(int (*compareCallback)(const T&, const T&))
	{
		::qsort(Vector, ActiveCount, sizeof(T), (int (*)(const void*, const void*))compareCallback);
	}

	bool isEmpty() const { return ActiveCount == 0; }

}; // 0010


class GenericList;
class GenericNode {
public:
	GenericNode(void) : NextNode(0), PrevNode(0) {}
	virtual ~GenericNode(void) {Unlink();}
	GenericNode(GenericNode & node) {node.Link(this);}
	GenericNode & operator = (GenericNode & node)
	{
		if (&node != this)
		{
			node.Link(this);
		}
		return(*this);
	}
	void Unlink(void)
	{
		if (Is_Valid())
		{
			PrevNode->NextNode = NextNode;
			NextNode->PrevNode = PrevNode;
			PrevNode = 0;
			NextNode = 0;
		}
	}
	GenericList * Main_List(void) const
	{
		GenericNode const * node = this;
		while (node->PrevNode)
		{
			node = PrevNode;
		}
		return((GenericList *)this);
	}
	void Link(GenericNode * node) 
	{
		UL_ASSERT(node != (GenericNode *)0);
		node->Unlink();
		node->NextNode = NextNode;
		node->PrevNode = this;
		if (NextNode) NextNode->PrevNode = node;
		NextNode = node;
	}
	GenericNode * Next(void) const {return(NextNode);}
	GenericNode * Next_Valid(void) const
	{
		return ((NextNode && NextNode->NextNode) ? NextNode : (GenericNode *)0);
	}
	GenericNode * Prev(void) const {return(PrevNode);}
	GenericNode * Prev_Valid(void) const
	{
		return ((PrevNode && PrevNode->PrevNode) ? PrevNode : (GenericNode *)0);
	}
	bool Is_Valid(void) const {return(this != (GenericNode *)0 && NextNode != (GenericNode *)0 && PrevNode != (GenericNode *)0);}
protected:
	GenericNode * NextNode; // 0004
	GenericNode * PrevNode; // 0008
}; // 000C
class GenericList {
public:
	GenericList(void)
	{
		FirstNode.Link(&LastNode);
	}
	virtual ~GenericList(void)
	{
		while (FirstNode.Next()->Is_Valid())
		{
			FirstNode.Next()->Unlink();
		}
	}
	GenericNode * First(void) const {return(FirstNode.Next());}
	GenericNode * First_Valid(void) const 
	{
		GenericNode *node = FirstNode.Next();
		return (node->Next() ? node : (GenericNode *)0);
	}
	GenericNode * Last(void) const {return(LastNode.Prev());}
	GenericNode * Last_Valid(void) const
	{
		GenericNode *node = LastNode.Prev();
		return (node->Prev() ? node : (GenericNode *)0);
	}
	bool Is_Empty(void) const {return(!FirstNode.Next()->Is_Valid());}
	void Add_Head(GenericNode * node) {FirstNode.Link(node);}
	void Add_Tail(GenericNode * node) {LastNode.Prev()->Link(node);}
	int Get_Valid_Count(void) const 
	{
		GenericNode * node = First_Valid();
		int counter = 0;
		while(node)
		{
			counter++;
			node = node->Next_Valid();
		}
		return counter;
	}
protected:
	GenericNode FirstNode; // 0004
	GenericNode LastNode; // 0010
private:
	GenericList(GenericList & list);
	GenericList & operator = (GenericList const &);
}; // 001C
template<class T> class List;
template<class T>
class Node : public GenericNode
{
public:
	List<T> * Main_List(void) const {return((List<T> *)GenericNode::Main_List());}
	T Next(void) const {return((T)GenericNode::Next());}
	T Next_Valid(void) const {return((T)GenericNode::Next_Valid());}
	T Prev(void) const {return((T)GenericNode::Prev());}
	T Prev_Valid(void) const {return((T)GenericNode::Prev_Valid());}
	bool Is_Valid(void) const {return(GenericNode::Is_Valid());}
}; // 000C
template<class T>
class List : public GenericList
{
public:
	List(void) {};
	T First(void) const {return((T)GenericList::First());}
	T First_Valid(void) const {return((T)GenericList::First_Valid());}
	T Last(void) const {return((T)GenericList::Last());}
	T Last_Valid(void) const {return((T)GenericList::Last_Valid());}
	void Delete(void) {while (First()->Is_Valid()) delete First();}
private:
	List(List<T> const & rvalue);
	List<T> operator = (List<T> const & rvalue);
}; // 001C
template<class T>
class DataNode : public GenericNode
{
	T Value;
public:
	DataNode() {};
	DataNode(T value) { Set(value); };
	void Set(T value) { Value = value; };
	T Get() const { return Value; };
	DataNode<T> * Next(void) const { return (DataNode<T> *)GenericNode::Next(); }
	DataNode<T> * Next_Valid(void) const { return (DataNode<T> *)GenericNode::Next_Valid(); }
	DataNode<T> * Prev(void) const { return (DataNode<T> *)GenericNode::Prev(); }
	DataNode<T> * Prev_Valid(void) const { return (DataNode<T> *)GenericNode::Prev_Valid(); }
};
template<class C, class D>
class ContextDataNode : public DataNode<D>
{
	C Context;
public:
	ContextDataNode() {};
	ContextDataNode(C context, D data) { Set_Context(context); Set(data); }
	void Set_Context(C context) { Context = context; };
	C Get_Context() { return Context; };
};
template<class C, class D>
class SafeContextDataNode : public ContextDataNode<C,D>
{
public:
	SafeContextDataNode(C context, D data) : ContextDataNode<C,D>(context, data) { }
private:
	SafeContextDataNode();
};
template<class PRIMARY, class SECONDARY>
class DoubleNode
{
	void Initialize() { Primary.Set(this); Secondary.Set(this); };
	PRIMARY PrimaryValue;
	SECONDARY SecondaryValue;
public:
	typedef DoubleNode<PRIMARY, SECONDARY> Type;
	DataNode<Type *> Primary;
	DataNode<Type *> Secondary;
	DoubleNode() { Initialize(); };
	DoubleNode(PRIMARY primary, SECONDARY secondary) { Initialize(); Set_Primary(primary); Set_Secondary(secondary); };
	void Set_Primary(PRIMARY value) { PrimaryValue = value; };
	void Set_Secondary(SECONDARY value) { SecondaryValue = value; };
	PRIMARY Get_Primary() { return PrimaryValue; };
	SECONDARY Get_Secondary() { return SecondaryValue; };
	void Unlink() { Primary.Unlink(); Secondary.Unlink(); };
};

template <typename T1,class T2> class IndexClass {
	struct NodeElement {
		T1 ID;
		T2 Data;
		NodeElement(T1 const &id, T2 const &data) : ID(id), Data(data)
		{
		}
		NodeElement() : ID(0), Data(0)
		{
		}
		bool operator==(NodeElement const &elem)
		{
			return ID == elem.ID;
		}
		bool operator<(NodeElement const &elem)
		{
			return ID < elem.ID;
		}
	};
	NodeElement* IndexTable;
	int IndexCount;
	int IndexSize;
	unsigned char IsSorted;
	const NodeElement* Archive;
public:
	IndexClass() : IndexTable(0), IndexCount(0), IndexSize(0), IsSorted(false), Archive(0)
	{
		Invalidate_Archive();
	}
	~IndexClass()
	{
		Clear();
	}
	bool Remove_Index(const T1 &ID)
	{
		int pos = -1;
		for (int i = 0;i < IndexCount;i++)
		{
			if (IndexTable[i].ID == ID)
			{
				pos = i;
				break;
			}
		}
		if (pos == -1)
		{
			return false;
		}
		else
		{
			for (int i = pos;i < IndexCount;i++)
			{
				IndexTable[i] = IndexTable[i+1];
			}
		}
		IndexCount--;
		IndexTable[IndexCount].ID = 0;
		IndexTable[IndexCount].Data = 0;
		Invalidate_Archive();
		return true;
	}
	bool Is_Present(const T1 &ID)
	{
		if (IndexCount)
		{
			if (Is_Archive_Same(ID))
			{
				return true;
			}
			else
			{
				NodeElement *node = Search_For_Node(ID);
				if (node)
				{
					Set_Archive(node);
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		return false;
	}
	bool Add_Index(const T1 &ID,const T2 &Data)
	{
		if (IndexCount + 1 <= IndexSize)
		{
			IndexTable[IndexCount].ID = ID;
			IndexTable[IndexCount++].Data = Data;
			IsSorted = false;
			return true;
		}
		int size = IndexSize;
		if (!size)
		{
			size = 10;
		}
		if (Increase_Table_Size(size))
		{
			IndexTable[IndexCount].ID = ID;
			IndexTable[IndexCount++].Data = Data;
			IsSorted = false;
			return true;
		}
		else
		{
			return false;
		}
	}
	int Count() const
	{
		return IndexCount;
	}
	const T2 &operator[](T1 const &ID)
	{
		static const T2 x;
		if (Is_Present(ID))
		{
			return Archive->Data;
		}
		return x;
	}
	void Invalidate_Archive()
	{
		Archive = 0;
	}
	void Clear()
	{
		if (IndexTable)
		{
			delete[] IndexTable;
		}
		IndexTable = 0;
		IndexCount = 0;
		IndexSize = 0;
		IsSorted = 0;
		Invalidate_Archive();
	}
	bool Is_Archive_Same(const T1 &ID)
	{
		return Archive && Archive->ID == ID;
	}
	NodeElement *Search_For_Node(const T1 &ID)
	{
		if (IndexCount)
		{
			if (!IsSorted)
			{
				qsort(IndexTable,IndexCount,sizeof(NodeElement),search_compfunc);
				Invalidate_Archive();
				IsSorted = true;
			}
			NodeElement elem(ID,0);
			return Binary_Search<NodeElement>(IndexTable,IndexCount,elem);
		}
		return false;
	}
	void Set_Archive(NodeElement const *archive)
	{
		Archive = archive;
	}
	bool Increase_Table_Size(int amount)
	{
		if (amount >= 0)
		{
			int newsize = IndexSize + amount;

			NodeElement *newindex;
			try {
				newindex = new NodeElement[newsize];
				UL_ASSERT(IndexCount < newsize);
				for (int i = 0; i < this->IndexCount; i++)
				{
					newindex[i].ID = IndexTable[i].ID;
					newindex[i].Data = IndexTable[i].Data;
				}
				if (IndexTable)
					delete[] IndexTable;
				IndexTable = newindex;
				IndexSize += amount;
				Invalidate_Archive();
				return true;
			}
			catch (std::bad_alloc& ba) {
			}
		}
		return false;
	}
	T1 Fetch_ID_By_Position(int position)
	{
		return IndexTable[position].ID;
	}
	T2 Fetch_By_Position(int position)
	{
		return IndexTable[position].Data;
	}
	static int search_compfunc(void  const *ptr2, void  const *ptr1)
	{
		if (*(NodeElement *)ptr2 == *(NodeElement *)ptr1)
		{
			return 0;
		}
		else
		{
			if (*(NodeElement *)ptr1 < *(NodeElement *)ptr2)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
}; // 0014
template <typename T> T *Binary_Search(T *list, int count, T &var)
{
	T *list2 = list;
	int pos = count;
	while (pos > 0)
	{
		T *list3 = &list2[pos / 2];
		if (var.ID >= list3->ID)
		{
			if (list3->ID == var.ID)
			{
				return &list2[pos / 2];
			}
			list2 = list3 + 1;
			pos = pos - pos / 2 - 1;
		}
		else
		{
			pos /= 2;
		}
	}
	return 0;
}
