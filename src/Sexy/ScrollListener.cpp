#include "ScrollListener.h"

// Empty base. AlmanacDialog overrides the slot.
void Sexy::ScrollListener::ScrollPosition(int theId, double thePosition)
{
    (void)theId;
    (void)thePosition;
}
