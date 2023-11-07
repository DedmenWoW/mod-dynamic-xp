/*
Credits
Script reworked by Micrah/Milestorme and Poszer (Poszer is the Best)
Module Created by Micrah/Milestorme
Original Script from AshmaneCore https://github.com/conan513 Single Player Project 
*/

#include <ranges>
#include <format>

#include "Configuration/Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"


class spp_dynamic_xp_rate : public PlayerScript
{
public:
    spp_dynamic_xp_rate() : PlayerScript("spp_dynamic_xp_rate") { }

    std::unordered_map<ObjectGuid, uint8_t> playerLevelCache;
    // Hardcoded special group even if player is not inside any group currently
    static constexpr std::array<ObjectGuid, 5> groupList = { ObjectGuid(uint64(2)), ObjectGuid(uint64(4)), ObjectGuid(uint64(5)), ObjectGuid(uint64(6)), ObjectGuid(uint64(7)) };

    uint8_t GetPlayerLevel(const ObjectGuid& guid)
    {
        // Player is online, update cahce
        if (const Player* player = ObjectAccessor::FindPlayer(guid))
        {
            playerLevelCache[guid] = player->GetLevel();
            return player->GetLevel();
        }

        // Player isn't online, is in cache?

        const auto found = playerLevelCache.find(guid);
        if (found != playerLevelCache.end())
            return found->second;

        // Load player from DB
        QueryResult result = CharacterDatabase.Query("SELECT level FROM characters WHERE guid = '{}'", guid.GetCounter());

        if (!result)
            return 255;

        if (result->GetRowCount() == 0)
            return 255;

        const auto level = result->Fetch()->Get<uint32>();
        playerLevelCache[guid] = level;

        return level;
    }

    uint8_t GetMinLevel(Player* player)
    {
        if (std::find(groupList.begin(), groupList.end(), player->GetGUID()) != groupList.end())
        {
            std::vector<uint8> playerLevels;
            std::transform(groupList.begin(), groupList.end(), std::back_inserter(playerLevels), [this](const ObjectGuid& guid)
                {
                    return GetPlayerLevel(guid);
                });

            const auto lowestPlayer = *std::min_element(playerLevels.begin(), playerLevels.end(), std::less());

            return lowestPlayer;
        }

        if (const auto group = player->GetGroup())
        {
            std::vector<uint8> playerLevels;
            std::transform(group->GetMemberSlots().begin(), group->GetMemberSlots().end(), std::back_inserter(playerLevels), [this](const Group::MemberSlot& slot)
                {
                    return GetPlayerLevel(slot.guid);
                });

            // Clang 13 cannot do this
            //auto playerView = group->GetMemberSlots()
            //| std::views::transform([this](const Group::MemberSlot& slot)
            //    {
            //        return GetPlayerLevel(slot.guid);
            //    });

            const auto lowestPlayer = *std::min_element(playerLevels.begin(), playerLevels.end(), std::less());

            return lowestPlayer;
        }

        return 255u; // max level
    }

    float GetXPFactor(Player* player)
    {
        if (!player->GetGroup()) // Only limiting relative to group
            return 1.f;

        const auto minLevel = GetMinLevel(player);

        const int16_t levelDelta = static_cast<int16_t>(player->GetLevel()) - minLevel;
        if (levelDelta <= 1) // Less than 2 levels difference
            return 1.f;

        switch (levelDelta)
        {
        case 2: return 0.8f;
        case 3: return 0.7f;
        case 4: return 0.6f;
        case 5: return 0.5f;
        case 6: return 0.4f;
        default: break;
        }

        return 0.1f;
    }

    void OnLevelChanged(Player* player, uint8 /*oldlevel*/) override
    {
        playerLevelCache[player->GetGUID()] = player->GetLevel();
        player->CastSpell(player, 11541, true); // Fireworks!
    }

    void OnLogin(Player* player) override
    {
        //if (sConfigMgr->GetOption<bool>("Dynamic.XP.Rate.Announce", true))
        //{
        //    ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00Level Dynamic XP |rmodule.");
        //}
        auto message = fmt::format("|cffFF0000Dynamic XP |ris active, the lowest level in your group is {}, you are getting XP factor {}", GetMinLevel(player), GetXPFactor(player));
        ChatHandler(player->GetSession()).SendSysMessage(message);

    }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 xpSource) override
    {
        auto source = static_cast<PlayerXPSource>(xpSource);

        // Based on group

        amount = amount * GetXPFactor(player);

        //if (sConfigMgr->GetOption<bool>("Dynamic.XP.Rate", true))
        //{
        //    if (player->getLevel() <= 9)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.1-9", 1);
        //    else if (player->getLevel() <= 19)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.10-19", 2);
        //    else if (player->getLevel() <= 29)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.20-29", 3);
        //    else if (player->getLevel() <= 39)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.30-39", 4);
        //    else if (player->getLevel() <= 49)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.40-49", 5);
        //    else if (player->getLevel() <= 59)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.50-59", 6);
        //    else if (player->getLevel() <= 69)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.60-69", 7);
        //    else if (player->getLevel() <= 79)
        //        amount *= sConfigMgr->GetOption<uint32>("Dynamic.XP.Rate.70-79", 8);
        //}
    }
};

void AddSC_dynamic_xp_rate()
{
    new spp_dynamic_xp_rate();
}
