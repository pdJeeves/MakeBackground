#ifndef UNIQUECPTR_H
#define UNIQUECPTR_H

template <typename T>
struct UniqueCPtr
{
	UniqueCPtr(T * owned = nullptr) : owned(owned) {}
	UniqueCPtr(UniqueCPtr<T> const& ) = delete;
	UniqueCPtr(UniqueCPtr<T> && ) = delete;

	operator const T*() const { return owned; }
	operator       T*()       { return owned; }

	void reset(T * it)
	{

	}

private:
	T * owned;
};

#endif // UNIQUECPTR_H
