#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_CREDITSTIMING_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_CREDITSTIMING_H

enum CreditWordType
{
    WORD_AA,
    WORD_EE,
    WORD_AW,
    WORD_OH,
    WORD_OFF
};

enum CreditBrainType
{
    BRAIN_FLY_ON,
    BRAIN_FAST_ON,
    BRAIN_NEXT_WORD,
    BRAIN_FAST_OFF,
    BRAIN_FLY_OFF,
    BRAIN_OFF
};

// MV cues: animation frame, sunflower mouth, word's x position and brain motion.
struct CreditsTiming
{
    float mFrame;
    CreditWordType mWordType;
    int mWordX;
    CreditBrainType mBrainType;
};

extern const CreditsTiming gCreditsTiming[];
extern const int gCreditsTimingCount;

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_CREDITSTIMING_H
