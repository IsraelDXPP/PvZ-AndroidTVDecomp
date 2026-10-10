// DeckChooserButton -- a seed-deck preview that draws six packets from gDeckDefs.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_DECKCHOOSERBUTTON_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_DECKCHOOSERBUTTON_H

#include "ReconTypes.h"
#include "ButtonWidget.h"

class DeckChooserButton : public Sexy::ButtonWidget
{
public:
    virtual ~DeckChooserButton();

    virtual void Draw(Sexy::Graphics *g);

    DeckChooserButton(int theId, int theDeckId, Sexy::ButtonListener *theButtonListener, Sexy::Widget *theParent);

    int mDeckId;
    int mReserved0;
    int mReserved1;
    int mReserved2;

    static const int gDeckDefs[16];
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_DECKCHOOSERBUTTON_H
