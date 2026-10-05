// DeckChooserButton -- a seed-deck preview that draws six packets from gDeckDefs.

#pragma once

#include "ReconTypes.h"
#include "ButtonWidget.h"

class DeckChooserButton : public Sexy::ButtonWidget
{
public:
    virtual ~DeckChooserButton();

    virtual void Draw(Sexy::Graphics* g);

    DeckChooserButton(int theId, int theDeckId, Sexy::ButtonListener* theButtonListener, Sexy::Widget* theParent);

    int mDeckId;
    int mReserved0;
    int mReserved1;
    int mReserved2;

    static const int gDeckDefs[16];
};
