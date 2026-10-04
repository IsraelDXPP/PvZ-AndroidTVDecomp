// StlCompat.h -- a libstdc++-layout vector, so the binary's container code
// transcribes verbatim.
//
// WHY THIS EXISTS
//
// The bodies in Recon/ manipulate their containers through raw pointers and
// libstdc++'s private members:
//
//     v3 = (_DWORD *)*(this + 10);
//     v4 = (_DWORD *)*(this + 11);
//     if ( v3 == v4 )
//       return (_DWORD *)std::vector<Sexy::Color>::_M_insert_aux(this + 10, v3, a2);
//
// Those names are private members of a different standard library. MSVC's
// vector has no `_M_insert_aux`, and its layout is three different pointers, so
// neither the names nor the offsets can be mapped onto `std::vector`.
//
// Rather than rewrite every one of those bodies -- there are 650 calls, and
// rewriting them would replace the binary's real control flow with my guess at
// what it means -- this header provides a vector with libstdc++'s *actual*
// layout and *actual* member names. The transcribed code then compiles and
// behaves as the binary does, because it is the same data structure.
//
// LAYOUT
//
// libstdc++'s std::vector<T> is three pointers, in this order:
//
//     +0   _M_start            first element
//     +4   _M_finish           one past the last element
//     +8   _M_end_of_storage   one past the allocated storage
//
// which is 12 bytes on the 32-bit target this project builds. The transcribed
// offset arithmetic (`*(this + 10)`, `*(this + 11)`, `*(this + 12)`) addresses
// exactly those three words, so the layout is what makes that arithmetic
// correct rather than merely plausible.
//
// This is not a reimplementation of std::vector. It is the subset the binary
// uses, with the same semantics, so that the transcription can keep the
// binary's own calls.

#ifndef __STLCOMPAT_H__
#define __STLCOMPAT_H__

#include "SexyTypes.h"

#include <cstddef>
#include <cstring>
#include <new>

namespace libstdcxx
{

// ---------------------------------------------------------------------------
// The allocator, reduced to what the binary uses.
// ---------------------------------------------------------------------------
template <class T>
class allocator
{
public:
    typedef T         value_type;
    typedef T*        pointer;
    typedef size_t    size_type;

    pointer allocate(size_type n)
    {
        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type = 0)
    {
        ::operator delete(p);
    }

    void construct(pointer p, const T& v) { new (static_cast<void*>(p)) T(v); }
    void destroy(pointer p) { p->~T(); }
};

// ---------------------------------------------------------------------------
// vector<T>, laid out and named as libstdc++ has it.
//
// Only the members the binary calls are provided. Each one does what the
// libstdc++ member of that name does, including the reallocation behaviour,
// because the surrounding decompiled code reads the three pointers directly and
// depends on them being updated.
// ---------------------------------------------------------------------------
template <class T, class Alloc = allocator<T> >
class vector
{
public:
    typedef T*          iterator;
    typedef const T*    const_iterator;
    typedef size_t      size_type;
    typedef T           value_type;

    // The three words, in libstdc++'s order. Public because the decompiled
    // bodies read and write them by offset.
    T*  _M_start;
    T*  _M_finish;
    T*  _M_end_of_storage;

public:
    vector() : _M_start(0), _M_finish(0), _M_end_of_storage(0) {}

    // -----------------------------------------------------------------------
    // Copying has to be DEEP, exactly as libstdc++'s std::vector is.
    //
    // This class owns its storage through three raw pointers, and it has a
    // destructor that deallocates them. Without these two members the compiler
    // would still generate a copy constructor -- a user-declared destructor
    // does not suppress it -- and that implicit one is memberwise, so it copies
    // the three pointers and the two objects then both free the same block.
    //
    // That is not theoretical: PopAnim, PIEffectDef, PILayerDef and PIEmitter
    // all hold vectors and all have copy constructors in the binary, and the
    // PopAnim pass hit it directly. The binary's own copy constructor deep-copies
    // (it calls _M_allocate and copy-constructs each element), which is what
    // these do.
    // -----------------------------------------------------------------------
    vector(const vector& theOther)
        : _M_start(0), _M_finish(0), _M_end_of_storage(0)
    {
        _M_range_initialize(theOther._M_start, theOther._M_finish);
    }

    // The binary also constructs from a range directly (`vector(first, last)`),
    // which the transcribed call sites spell as a two-argument construction.
    template <class InputIterator>
    vector(InputIterator theFirst, InputIterator theLast)
        : _M_start(0), _M_finish(0), _M_end_of_storage(0)
    {
        _M_range_initialize(theFirst, theLast);
    }

    // Deep copy assignment. libstdc++ reuses the existing storage when it is
    // big enough and reallocates otherwise; the observable behaviour is the
    // same, so this keeps it simple and always rebuilds.
    vector& operator=(const vector& theOther)
    {
        if (this != &theOther)
        {
            size_type aOld = Capacity();
            Clear();
            _M_deallocate(_M_start, aOld);
            _M_start = 0;
            _M_finish = 0;
            _M_end_of_storage = 0;
            _M_range_initialize(theOther._M_start, theOther._M_finish);
        }
        return *this;
    }

    ~vector()
    {
        Clear();
        _M_deallocate(_M_start, Capacity());
    }

    iterator       begin()       { return _M_start; }
    const_iterator begin() const { return _M_start; }
    iterator       end()         { return _M_finish; }
    const_iterator end() const   { return _M_finish; }

    size_type Size() const     { return size_type(_M_finish - _M_start); }
    size_type Capacity() const { return size_type(_M_end_of_storage - _M_start); }
    bool      Empty() const    { return _M_start == _M_finish; }

    T&       operator[](size_type n)       { return _M_start[n]; }
    const T& operator[](size_type n) const { return _M_start[n]; }

    void Clear()
    {
        for (T* p = _M_start; p != _M_finish; ++p)
            p->~T();
        _M_finish = _M_start;
    }

    // -----------------------------------------------------------------------
    // The private members the binary calls by name.
    //
    // These are the libstdc++ internals, kept under their real names so the
    // transcription does not have to rewrite the call sites.
    // -----------------------------------------------------------------------

    // _M_insert_aux(pos, x) -- insert one element before pos.
    iterator _M_insert_aux(iterator thePos, const T& theValue)
    {
        if (_M_finish != _M_end_of_storage)
        {
            // Room available: shift the tail right by one and place the value.
            new (static_cast<void*>(_M_finish)) T(*(_M_finish - 1));
            ++_M_finish;
            for (iterator p = _M_finish - 2; p != thePos; --p)
                *p = *(p - 1);
            *thePos = theValue;
            return thePos;
        }
        _M_realloc_insert(thePos, theValue);
        return thePos;
    }

    // _M_fill_insert(pos, n, x) -- insert n copies of x before pos.
    void _M_fill_insert(iterator thePos, size_type theN, const T& theValue)
    {
        if (theN == 0)
            return;
        size_type aOld = Size();
        size_type aNew = aOld + theN;
        if (aNew > Capacity())
        {
            size_type aCap = Capacity() ? Capacity() * 2 : 1;
            if (aCap < aNew)
                aCap = aNew;
            T* aNewStart = _M_allocate(aCap);
            T* aOut = aNewStart;
            for (iterator p = _M_start; p != thePos; ++p, ++aOut)
                new (static_cast<void*>(aOut)) T(*p);
            for (size_type i = 0; i < theN; ++i, ++aOut)
                new (static_cast<void*>(aOut)) T(theValue);
            for (iterator p = thePos; p != _M_finish; ++p, ++aOut)
                new (static_cast<void*>(aOut)) T(*p);
            Clear();
            _M_deallocate(_M_start, Capacity());
            _M_start = aNewStart;
            _M_finish = aNewStart + aNew;
            _M_end_of_storage = aNewStart + aCap;
            return;
        }
        // Room available: shift the tail right by n.
        size_type aTail = size_type(_M_finish - thePos);
        if (aTail > theN)
        {
            for (size_type i = 0; i < theN; ++i)
                new (static_cast<void*>(_M_finish + i)) T(*(_M_finish - theN + i));
            _M_finish += theN;
            for (iterator p = _M_finish - theN - 1; p >= thePos + theN; --p)
                *p = *(p - theN);
            for (iterator p = thePos; p != thePos + theN; ++p)
                *p = theValue;
        }
        else
        {
            for (size_type i = 0; i < aTail; ++i)
                new (static_cast<void*>(_M_finish + i)) T(*(thePos + i));
            _M_finish += aTail;
            for (iterator p = thePos; p != thePos + theN; ++p)
                *p = theValue;
        }
    }

    // _M_erase(pos) -- remove the element at pos.
    iterator _M_erase(iterator thePos)
    {
        if (thePos + 1 != _M_finish)
            std::memmove(thePos, thePos + 1,
                         size_type(_M_finish - thePos - 1) * sizeof(T));
        --_M_finish;
        _M_finish->~T();
        return thePos;
    }

    // _M_erase(first, last) -- remove a range.
    iterator _M_erase(iterator theFirst, iterator theLast)
    {
        if (theFirst != theLast)
        {
            if (theLast != _M_finish)
                std::memmove(theFirst, theLast,
                             size_type(_M_finish - theLast) * sizeof(T));
            size_type aRemoved = size_type(theLast - theFirst);
            for (iterator p = _M_finish - aRemoved; p != _M_finish; ++p)
                p->~T();
            _M_finish -= aRemoved;
        }
        return theFirst;
    }

    void _M_realloc_insert(iterator thePos, const T& theValue);

    // Copy-on-write bookkeeping has no meaning here: this vector never shares
    // its storage, so there is nothing to un-share. The names exist because the
    // bodies call them.
    void _M_leak_hard() {}
    void _M_mutate(size_type, size_type, size_type) {}

    // The standard interface the bodies also use.
    void push_back(const T& theValue)
    {
        if (_M_finish == _M_end_of_storage)
            _M_realloc_insert(_M_finish, theValue);
        else
        {
            new (static_cast<void*>(_M_finish)) T(theValue);
            ++_M_finish;
        }
    }

    // pop_back() -- the standard member, added for ActiveFontLayer::PopColor,
    // whose binary body @0x310008 is exactly "destroy the last element and
    // subtract 16 from _M_finish" once the vector is non-empty.
    void pop_back()
    {
        --_M_finish;
        _M_finish->~T();
    }

    void clear() { Clear(); }
    size_type size() const { return Size(); }
    bool empty() const { return Empty(); }

    iterator insert(iterator thePos, const T& theValue)
    {
        return _M_insert_aux(thePos, theValue);
    }

    void insert(iterator thePos, size_type theN, const T& theValue)
    {
        _M_fill_insert(thePos, theN, theValue);
    }

    iterator erase(iterator thePos) { return _M_erase(thePos); }

    void resize(size_type theN)
    {
        if (theN < Size())
            _M_erase(_M_start + theN, _M_finish);
        else
            _M_fill_insert(_M_finish, theN - Size(), T());
    }

    // libstdc++ declares resize in both forms and the binary calls the
    // two-argument one in several places (six in PIEffect.cpp, more in
    // PopAnim.cpp). Without it those call sites had to drop the fill value.
    void resize(size_type theN, const T& theValue)
    {
        if (theN < Size())
            _M_erase(_M_start + theN, _M_finish);
        else
            _M_fill_insert(_M_finish, theN - Size(), theValue);
    }

    void reserve(size_type theN)
    {
        if (theN > Capacity())
            _M_realloc(theN);
    }

    T* data() { return _M_start; }
    const T* data() const { return _M_start; }

    T&       front()       { return *_M_start; }
    const T& front() const { return *_M_start; }
    T&       back()        { return *(_M_finish - 1); }
    const T& back() const  { return *(_M_finish - 1); }

private:
    // Copy [theFirst, theLast) into fresh storage. Used by the copy constructor,
    // the range constructor and the copy assignment, which is why it is the one
    // place that knows how a vector is built from existing elements.
    template <class InputIterator>
    void _M_range_initialize(InputIterator theFirst, InputIterator theLast)
    {
        size_type aCount = size_type(theLast - theFirst);
        if (aCount == 0)
            return;
        _M_start = _M_allocate(aCount);
        T* aOut = _M_start;
        for (InputIterator p = theFirst; p != theLast; ++p, ++aOut)
            new (static_cast<void*>(aOut)) T(*p);
        _M_finish = aOut;
        _M_end_of_storage = _M_start + aCount;
    }

    T* _M_allocate(size_type theN)
    {
        return theN ? Alloc().allocate(theN) : 0;
    }

    void _M_deallocate(T* theP, size_type theN)
    {
        if (theP)
            Alloc().deallocate(theP, theN);
    }

    void _M_realloc(size_type theN)
    {
        T* aNewStart = _M_allocate(theN);
        T* aOut = aNewStart;
        for (T* p = _M_start; p != _M_finish; ++p, ++aOut)
            new (static_cast<void*>(aOut)) T(*p);
        size_type aOld = Capacity();
        Clear();
        _M_deallocate(_M_start, aOld);
        _M_start = aNewStart;
        _M_finish = aOut;
        _M_end_of_storage = aNewStart + theN;
    }
};

template <class T, class Alloc>
void vector<T, Alloc>::_M_realloc_insert(iterator thePos, const T& theValue)
{
    size_type aOld = Size();
    size_type aNewCap = Capacity() ? Capacity() * 2 : 1;
    T* aNewStart = _M_allocate(aNewCap);
    T* aOut = aNewStart;
    for (iterator p = _M_start; p != thePos; ++p, ++aOut)
        new (static_cast<void*>(aOut)) T(*p);
    new (static_cast<void*>(aOut)) T(theValue);
    ++aOut;
    for (iterator p = thePos; p != _M_finish; ++p, ++aOut)
        new (static_cast<void*>(aOut)) T(*p);
    Clear();
    _M_deallocate(_M_start, Capacity());
    _M_start = aNewStart;
    _M_finish = aNewStart + aOld + 1;
    _M_end_of_storage = aNewStart + aNewCap;
}

} // namespace libstdcxx

// ---------------------------------------------------------------------------
// The libstdc++ sort internals the bodies call by name.
//
// The decompiler printed `std::__introsort_loop(...)` and
// `std::__final_insertion_sort(...)` -- the two helpers libstdc++'s std::sort
// is built from. MSVC has no such symbols, so Board.cpp reported
// `"__introsort_loop": is not a member of "std"` and could not proceed.
//
// Adding names to namespace std is formally undefined, but this project already
// models libstdc++ internals by name elsewhere (Sexy::RbTree mirrors
// std::_Rb_tree, libstdcxx::vector mirrors std::vector), and the alternative is
// rewriting every call site. The bodies forward to std::sort, which is the
// operation the originals implement; the depth argument is an implementation
// detail of the introsort/insertion-sort split and is ignored.
// ---------------------------------------------------------------------------
namespace std {

template <class _RandomAccessIterator, class _Compare>
inline void __introsort_loop(_RandomAccessIterator theFirst,
                             _RandomAccessIterator theLast,
                             long /*theDepthLimit*/,
                             _Compare theCompare)
{
    std::sort(theFirst, theLast, theCompare);
}

template <class _RandomAccessIterator, class _Compare>
inline void __final_insertion_sort(_RandomAccessIterator theFirst,
                                   _RandomAccessIterator theLast,
                                   _Compare theCompare)
{
    std::sort(theFirst, theLast, theCompare);
}

} // namespace std

#endif // __STLCOMPAT_H__
