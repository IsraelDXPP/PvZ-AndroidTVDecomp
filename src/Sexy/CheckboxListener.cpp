#include "CheckboxListener.h"

// Empty default: a listener that does not care about the toggle still compiles.
void Sexy::CheckboxListener::CheckboxChecked(int theId, bool theChecked)
{
    (void)theId;
    (void)theChecked;
}
