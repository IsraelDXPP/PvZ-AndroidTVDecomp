// ListListener. The three defaults do nothing; a subclass replaces the ones
// it handles.

#include "ListListener.h"

void Sexy::ListListener::ListClicked(int theId, int theIndex, int theClickCount)
{
    (void)theId;
    (void)theIndex;
    (void)theClickCount;
}

void Sexy::ListListener::ListClosed(int theId)
{
    (void)theId;
}

void Sexy::ListListener::ListHiliteChanged(int theId, int theIndex, int thePrevIndex)
{
    (void)theId;
    (void)theIndex;
    (void)thePrevIndex;
}
