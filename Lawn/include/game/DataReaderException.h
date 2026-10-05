// DataReaderException -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class DataReaderException : public std::exception`
// (RTTI "19DataReaderException", primary base "St9exception" at offset 0,
// vtable 0x6816c8).
//
//   [  0] 0x189750  DataReaderException::~DataReaderException   (D1)
//   [  1] 0x189770  DataReaderException::~DataReaderException   (D0)
//   [  2] 0x2c5d18  std::exception::what() const
// MSVC x86 emits ONE slot for a virtual destructor where Itanium emits two, so
// the declaration order below is one shorter and otherwise the same. Slot 2 is
// std::exception's own and is therefore not re-declared here.
//
// std::exception already carries the vptr (its MSVC x86 layout is vptr + a
// 16-byte message buffer on x86, or vptr + padding), so this class adds no data
// member of its own -- the transcribed eight-byte opaque block was a guess.

#pragma once

#include "ReconTypes.h"
#include <exception>

class DataReaderException : public std::exception
{
public:
    virtual ~DataReaderException();
};
