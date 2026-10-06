// LawnUser -- account, shop prices and recharge tiers.

#pragma once

#include "ReconTypes.h"
#include <map>
#include <vector>

class LawnUser
{
public:
    Sexy::String mAccount;   // +0x00
    Sexy::String mToken;     // +0x04  default "007"
    Sexy::String mChannel;   // +0x08  default "shax"
    Sexy::String mCoinText;  // +0x0C  default "0"; the decimal coin count
    std::map<int, Sexy::String> mPrices; // Prices keyed by shop packet.
    std::vector<SCoinItem> mCoinItems;

    int CoinTierCount() const
    {
        return (int)mCoinItems.size();
    }

    static LawnUser* getCurUser();
};

