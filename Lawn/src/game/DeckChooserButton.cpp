#include "DeckChooserButton.h"

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Graphics.h"
#include "SeedPacket.h"
#include "Widget.h"

DeckChooserButton::~DeckChooserButton() {}

DeckChooserButton::DeckChooserButton(
    int theId, int theDeckId, Sexy::ButtonListener *theButtonListener, Sexy::Widget *theParent) :
    Sexy::ButtonWidget(theId, theButtonListener)
{
    mParent = theParent;
    mDeckId = theDeckId;
}

void DeckChooserButton::Draw(Sexy::Graphics *g)
{
    Sexy::ButtonWidget::Draw(g);

    // Six packets in a row, 50px apart starting at x = 16.
    for (int packetIndex = 0; packetIndex < 6; ++packetIndex)
    {
        int posX = 16 + 50 * packetIndex;
        int defIndex = packetIndex + 6 * mDeckId;
        DrawSeedPacket(g,
            (float)posX,
            12.0f,
            (SeedType)gDeckDefs[defIndex],
            SeedType::SEED_NONE,
            0.0f,
            255,
            true,
            false,
            false,
            true);
    }
}
