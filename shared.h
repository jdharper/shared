#include <atomic>
#include <cassert>
#include <type_traits>


template<class T> class intrusive_ptr {

public:

	typedef T element_type;

	intrusive_ptr() noexcept : ptr(0) { }
	intrusive_ptr(T * p, bool add_ref = true) noexcept(noexcept(intrusive_ptr_add_ref(p))) :
		ptr(p)
	{
		if (add_ref)
			if (ptr) intrusive_ptr_add_ref(ptr);
	}

	intrusive_ptr(intrusive_ptr const & r) noexcept(noexcept(intrusive_ptr_add_ref(r.ptr)))
	{
		if (r.ptr) intrusive_ptr_add_ref(r.ptr);
		ptr = r.ptr;

	}
	intrusive_ptr(intrusive_ptr && r) noexcept
	{
		ptr = r.ptr;
		r.ptr = 0;
	}
	template<class Y> intrusive_ptr(intrusive_ptr<Y> const & r) noexcept(noexcept(intrusive_ptr_add_ref(r.ptr)))
	{
		if (r.ptr) intrusive_ptr_add_ref(r.ptr);
		ptr = r.ptr;
	}
	template<class Y> intrusive_ptr(intrusive_ptr<Y> const && r) noexcept
	{
		ptr = r.ptr;
		r.ptr = 0;
	}

	~intrusive_ptr() noexcept(noexcept(intrusive_ptr_release(ptr))) { if (ptr) intrusive_ptr_release(ptr); }

	intrusive_ptr & operator=(intrusive_ptr const & r) noexcept(noexcept(reset(r.ptr)))
	{
		reset(r.ptr);
		return *this;
	}
	template<class Y> intrusive_ptr & operator=(intrusive_ptr<Y> const & r) noexcept(noexcept(reset(r.ptr)))
	{
		reset(r.ptr);
		return *this;
	}

	intrusive_ptr & operator=(intrusive_ptr const && r) noexcept
	{
		swap(ptr, r.ptr);
		return *this;
	}

	intrusive_ptr & operator=(T * r) noexcept(noexcept(reset(r)))
	{
		reset(r);
		return *this;
	}

	void reset() noexcept(noexcept(intrusive_ptr_release(ptr)))
	{
		if (ptr) intrusive_ptr_release(ptr);
		ptr = 0;
	}
	void reset(T * r) noexcept(noexcept(intrusive_ptr()))
	{
		intrusive_ptr tmp(r);
		swap(tmp);
	}
	void reset(T * r, bool add_ref) noexcept(noexcept(intrusive_ptr()))
	{
		intrusive_ptr tmp(r, add_ref);
		swap(tmp);
	}

	T & operator*() const noexcept { return *ptr; }
	T * operator->() const noexcept { return ptr; }
	T * get() const noexcept { return ptr; }
	T * detach() noexcept { T * tmp = ptr; ptr = 0; return tmp; }

	typedef T * (intrusive_ptr::*dummy_bool)() const;

	operator dummy_bool () const noexcept {
		return ptr ? &intrusive_ptr::get : nullptr; 
	}

	void swap(intrusive_ptr & b) noexcept { std::swap(ptr, b.ptr); }

private:
	element_type *ptr;
};
template <typename T>
void swap(intrusive_ptr<T> & v1, intrusive_ptr<T> & v2) noexcept
{
	v1.swap(v2);
}


class intrusive_ref_counter
{
	friend void intrusive_ptr_add_ref(intrusive_ref_counter *) noexcept;
	friend void intrusive_ptr_release(intrusive_ref_counter *) noexcept;

public:
	intrusive_ref_counter() noexcept { ref_count = 0; }
	intrusive_ref_counter(intrusive_ref_counter const & r) noexcept { ref_count = 0; }

	intrusive_ref_counter & operator=(intrusive_ref_counter const & r) noexcept {}

	unsigned int use_count() const noexcept { return ref_count; }

protected:
	virtual ~intrusive_ref_counter() = default;
private:
	std::atomic<size_t> ref_count;
};

inline void intrusive_ptr_add_ref(intrusive_ref_counter * p) noexcept(noexcept(++(p->ref_count)))
{
	if (!p) return;
	++(p->ref_count);
}

inline void intrusive_ptr_release(intrusive_ref_counter * p) noexcept(noexcept(++(p->ref_count)) && noexcept(delete p))
{
	if (!p) return;
	if (--(p->ref_count) == 0)
	{
		delete p;
	}
}

template <typename T>
class shared;
template <typename T>
class shared_array;


template<typename BASE>
class shared_base : public intrusive_ref_counter
{
	template<typename T>
	struct element_type_helper
	{
		typedef T type;
		static const bool is_array = false;
		static type * to_element(T * p) { return p; }
	};

	template<typename T>
	struct element_type_helper<shared<T>>
	{
		typedef T type;
		static const bool is_array = false;
		static type * to_element(shared<T> * p) { return p ? &p->element : nullptr; }
	};

	template<typename T>
	struct element_type_helper<shared_array<T>>
	{
		typedef T type;
		static const bool is_array = true;
		static type * to_element(shared_array<T> * p) { return p ? p->getArray() : nullptr; }
	};

public:
	typedef typename element_type_helper<BASE>::type element_type;
private:
	static element_type * to_element(BASE * p)
	{
		return element_type_helper<BASE>::to_element(p);
	}

public:
	class group;

	static constexpr bool is_array = element_type_helper<BASE>::is_array;
	class ptr;
	class view_ptr;
	class ptr
	{
		friend class shared_base;
		template<typename T>
		friend class shared_array;
	public:
		ptr() : p(0) { }
		ptr(const ptr & _p) : p(_p.p) { }
		ptr(ptr && _p) : p(std::move(_p.p))	{ }
		explicit ptr(BASE * p) noexcept : p(p)
		{
		}
		ptr & operator =(ptr & _p) noexcept(noexcept(p = _p.p))
		{
			p = _p.p;
			return *this;
		}
		ptr & operator =(ptr && _p) noexcept(noexcept(swap(p, _p.p)))
		{
			swap(p, _p.p);
			return *this;
		}
		const element_type * get() const noexcept { return to_element(p.get());  }
		const element_type & operator *() const noexcept { return *get(); }
		const element_type * operator ->() const noexcept { return get(); }
		const element_type & operator[](size_t n) const  { return get()[n]; }
		typedef const element_type * (ptr::*dummy_bool)() const;
		operator dummy_bool() const noexcept { return p ? &ptr::get : 0; }
		element_type * write()
		{
			if (!p) 
				return 0;
			static_assert(!std::is_const<element_type>::value, "Cannot write to constant type.");
			if (p->use_count() > 1)
			{
				*this = BASE::allocate(*p);
			}
			return to_element(p.get());
		}

		view_ptr view() const;
		ptr cow() const { return *this;  }

	private:
		intrusive_ptr<BASE> p;
	};

	class view_ptr
	{
		friend class ptr;
	public:
		typedef typename BASE::element_type element_type;
		view_ptr() noexcept : p(0) { }
		view_ptr(const view_ptr & _p) noexcept(noexcept(intrusive_ptr<group>(_p.p))): p(_p.p) { }
		view_ptr(view_ptr && _p) noexcept(noexcept(intrusive_ptr<group>(std::move(_p.p))))
			: p(std::move(_p.p)) 
		{ 
		}
		explicit view_ptr(BASE * _p) noexcept(noexcept(intrusive_ptr<group>(new group(_p))))
			: p(new group(_p))
		{
		}

		view_ptr & operator =(const view_ptr & _p) noexcept(noexcept(p = _p.p))
		{
			p = _p.p;
			return *this;
		}
		view_ptr & operator =(view_ptr && _p) noexcept(noexcept(swap(p, _p.p)))
		{
			swap(p, _p.p);
			return *this;
		}
		view_ptr view(bool new_group = false) const noexcept(noexcept(view_ptr(new group(*p))))
		{
			if (new_group == false)
				return *this;
			return view_ptr(new group(*p));
		}
		typedef const element_type view_ptr::*dummy_bool() const;
		operator bool() const noexcept { return p ? &view_ptr::get : 0; }
		element_type * write() { return p->write(); }
		ptr cow() const { return p->p; }
		const element_type * get() const noexcept { return p ? p->get() : nullptr; }
		const element_type & operator *() const noexcept { return *get(); }
		const element_type * operator ->() const noexcept { return get(); }
		const element_type & operator[](size_t n) const { return get()[n]; }

	private:
		view_ptr(group * _p) noexcept(noexcept(intrusive_ptr<group>(_p)))
			: p(_p)
		{
		}
		intrusive_ptr<group> p;
	};

	template <typename ...ARGS,
		typename = decltype(new BASE(*((std::remove_reference_t<ARGS> *)0)...))>
		static ptr allocate(ARGS && ... args)
	{
		BASE * p = new BASE(std::forward<ARGS>(args)...);
		return ptr(p);
	}
protected:
	shared_base() { }
private:
	static BASE * clone(const BASE *p) { return new BASE(*p); }
	class group : public intrusive_ref_counter
	{
		friend class ptr;
		friend class view_ptr;
	public:
		typedef typename BASE::element_type element_type;
		const element_type * get() const noexcept { return p.get(); }
		element_type * write() { return p.write(); }

	private:
		group(BASE * _p) : p(_p) { }
		group(const ptr & _p) : p(_p) {}
		~group() noexcept { }
		ptr p;
	};
};

template <typename T>
inline typename shared_base<T>::view_ptr shared_base<T>::ptr::view() const
{
	auto new_group = new group(*this);
	return view_ptr(new_group);
}

template <typename T>
class shared : public shared_base<shared<T>>
{
private:
	friend class shared_base<shared<T>>;
	template<class T> friend class intrusive_ptr;

	shared(const shared &s) : element(s.element) {}
	shared(shared &&s) : element(std::move(s.element)) {}


	template <typename ...ARGS,
		typename = std::enable_if_t<std::is_constructible<T, ARGS...>::value >>
	shared(ARGS && ... args)
		: element(std::forward<ARGS>(args)...)
	{
	}

	shared() = delete;
private:
	shared & operator =(const shared &) = delete;
	T element;
};

template <typename T>
class shared_array : public shared_base<shared_array<T>>
{
public:
	static ptr allocate_unconstructed(size_t _size)
	{
		size_t alloc_sz = sizeof(shared_array) + _size * sizeof(T) + (alignof(T) != alignof(shared_array) ? alignof(T)-1 : 0);
		void * p = operator new(alloc_sz);
		shared_array * psa = new(p) shared_array(_size);
		return ptr(psa);
	}

	template <typename ...ARGS,
			  typename = std::enable_if_t<std::is_constructible<T, ARGS...>::value >>
	static ptr allocate(size_t _size, ARGS && ... args)
	{
		auto parray = allocate_unconstructed(_size);
		if (sizeof...(ARGS) || !std::is_trivially_constructible<T>::value)
		{
			// Since we just created the array and have the only copy.  
			// It is safe to cast away const fto construct eleements.
			for (T * pt = const_cast<T *>(parray.get()), *pe = pt + _size; pt != pe; ++pt)
				new(pt) T(std::forward<ARGS>(args)...);
		}

		return parray;
	}


private:
	size_t size;
	friend class shared_base<shared_array<T>>;
	template<class T> friend class intrusive_ptr;

	shared_array(size_t _size)
		: size(_size)
	{
	}

	~shared_array()
	{
		T * e = getArray();
		T * p = e + size;
		while (p-- != e)
			p->~T();
	}

	static ptr allocate(const shared_array &r)
	{
		size_t size = r.size;

		size_t alloc_sz = sizeof(shared_array) + size * sizeof(T) + (alignof(T) != alignof(shared_array) ? alignof(T)-1 : 0);
		void * pnew = operator new(alloc_sz);
		shared_array * psa = new(pnew) shared_array(size);

		T * dst = psa->getArray();
		T * src = r.getArray();
		T * end = dst + size;
		while (dst != end)
		{
			new(dst) T(*src);
			++dst;
			++src;
		}
		return shared_array::ptr(psa);

	}

	T* getArray() const 
	{
		intptr_t p = reinterpret_cast<intptr_t>(this + 1);
		if (alignof(T) != alignof(shared_array))
		{
			intptr_t m = p % alignof(T);
			if (m != 0)
				p += alignof(T)-m;
		}
		return reinterpret_cast<T *>(p);
	}
	shared_array() = delete;
	shared_array & operator =(const shared_array &) = delete;
};
