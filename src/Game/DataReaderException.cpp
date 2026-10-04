// DataReaderException -- transcribed from libGameMain.so
// Bodies are the IDA decompilation with syntactic changes only;
// see build\phase2-kickoff.md for the refactor rules.

#include "DataReaderException.h"
// ==========================================================================
// DataReaderException::~DataReaderException()
// @00189750  24 bytes
// ==========================================================================
DataReaderException::~DataReaderException()
{
    // The binary's D1/D0 pair stores the vtable pointer (0x6816d0) and then calls
    // std::exception::~exception. Both are what the compiler emits for a real
    // polymorphic base, so the body is empty on purpose.
}
