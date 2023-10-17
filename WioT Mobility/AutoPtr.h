#pragma once

template<class T>
class CAutoPtr
{
public:
	CAutoPtr()
		:m_pObject(nullptr)
	{
	}

	CAutoPtr(const CAutoPtr<T> &other)
		:m_pObject(other.m_pObject)
	{
		if (m_pObject)
		{
			m_pObject->AddRef();
		}
	}

	CAutoPtr<T> &operator=(const CAutoPtr<T>& other)
	{
		T* pSaved = m_pObject;

		m_pObject = other.m_pObject;
		if (m_pObject)
		{
			m_pObject->AddRef();
		}
		if (pSaved)
		{
			pSaved->Release();
		}

		return *this;
	}

	bool operator==(const CAutoPtr<T>& other) const
	{
		return m_pObject == other.m_pObject;
	}

	bool operator==(const T *pObject) const
	{
		return m_pObject == pObject;
	}

	CAutoPtr(T* pObject, bool bAddRef = true)
		:m_pObject(pObject)
	{
		if (pObject && bAddRef)
		{
			pObject->AddRef();
		}
	}
	~CAutoPtr()
	{
		if (m_pObject)
		{
			m_pObject->Release();
		}
	}

	operator T* () const
	{
		return m_pObject;
	}

	T* operator->() const
	{
		return m_pObject;
	}

	void Detach()
	{
		m_pObject = nullptr;
	}

	bool Empty()
	{
		return m_pObject == nullptr;
	}
private:
	T* m_pObject;
};

