/*
 * ? Author: LWT
 * ! Date: 20.10.2024
 * * Description:
 */
#include "stdafx.h"
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamageranking.hpp"
#include "char.h"

namespace bossdamageranking
{

/**
 * @brief Construct a new CBossDamageRankingPlayerManager object with player
 * info
 *
 * @param player_info The player information to initialize the manager with
 */
CBossDamageRankingPlayerData::CBossDamageRankingPlayerData(boss_damage_ranking_player_info_t&& player_info) noexcept
{
    m_players.emplace_back(std::move(player_info));
}

/**
 * @brief Set a bad affect flag for a player in the ranking.
 *
 * @param player_id The player's ID to set the flag for.
 * @param flag The bad affect flag to be set.
 *
 * @details This function retrieves the player information for the given
 * player ID and sets the specified bad affect flag. If the player is not
 * found in the ranking, the function exits without making any changes.
 */
void CBossDamageRankingPlayerData::set_bad_affect_flag(uint32_t player_id, BadAffectType flag) const
{
    const auto& player_info_ptr{get_player_info(player_id)};

    if (std::nullopt == player_info_ptr)
    {
        return;
    }

    (*player_info_ptr)->bad_affect_flag |= static_cast<uint8_t>(flag);
}

/**
 * @brief Add damage to a player's damage in the ranking
 *
 * @param player_id The character ID.
 * @param damage The damage to add
 *
 * @details This function retrieves the player information for the given
 * player ID and adds the specified damage to the player's damage in the
 * ranking. If the player is not found in the ranking, the function exits
 * without making any changes.
 */
void CBossDamageRankingPlayerData::add_damage(uint32_t player_id, uint64_t damage) const
{
    const auto& player_info_ptr{get_player_info(player_id)};

    if (std::nullopt == player_info_ptr)
    {
        return;
    }

    (*player_info_ptr)->damage += damage;
}

/**
 * @brief Retrieve the vector of player information sorted in descending order by damage
 *
 * @details This function returns a vector of player information sorted in descending order by damage.
 * If the C++20 standard library is available, it uses the `std::ranges::sort` algorithm to sort the vector.
 * Otherwise, it uses the `std::sort` algorithm with a custom comparison function to sort the vector.
 * The returned vector is a copy of the internal vector of player information.
 *
 * @return std::vector<boss_damage_ranking_player_info_t> The sorted vector of player information
 */
boss_damage_ranking_player_info_vec_t& CBossDamageRankingPlayerData::get_sorted_player_info_vec()
{
    static constexpr uint8_t sort_limit{5U};

    const auto vec_elm_index{sort_limit < m_players.size() ? sort_limit : m_players.size()};

#if __cplusplus >= 202002L
    std::ranges::partial_sort(m_players, std::next(m_players.begin(), vec_elm_index), std::ranges::greater{},
                              &BossDamageRankingPlayerInfo::damage);
#else
    std::partial_sort(m_players.begin(), std::next(m_players.begin(), vec_elm_index), m_players.end(),
                      [](const auto& lhs, const auto& rhs) { return lhs->damage > rhs->damage; });
#endif

    return m_players;
}

/**
 * @brief Get player information from the ranking
 *
 * @param player_id The player's ID to retrieve the information for
 * @return std::optional<BossDamageRankingPlayerInfo*> Optional containing
 * the player info if found, otherwise std::nullopt
 */
std::optional<BossDamageRankingPlayerInfo*>
CBossDamageRankingPlayerData::get_player_info(const uint32_t player_id) const noexcept
{
    const auto check_pred_func{[player_id](const boss_damage_ranking_player_info_t& player_info)
                               { return player_info->player_id == player_id; }};

#if __cplusplus >= 202002L
    const auto find_iter{std::ranges::find_if(m_players, check_pred_func)};
#else
    const auto find_iter{std::find_if(m_players.begin(), m_players.end(), check_pred_func)};
#endif

    if (find_iter == m_players.cend())
    {
        return std::nullopt;
    }

    return find_iter->get();
}

/**
 * @brief Check if the player is in the ranking
 *
 * @param player_id The player's ID to check
 * @return bool True if the player is in the ranking, false otherwise
 */
bool CBossDamageRankingPlayerData::is_player_in_ranking(const uint32_t player_id) const
{
    if (m_players.empty())
    {
        return false;
    }

    const auto& player_iter{get_player_info(player_id)};

    return player_iter.has_value();
}

/**
 * @brief Add a player to the boss damage ranking.
 *
 * @param p_character A pointer to the character object representing the
 * player.
 */
void CBossDamageRankingPlayerData::add_player(const LPCHARACTER p_character)
{
    if (nullptr == p_character)
    {
        return;
    }

    if (is_player_in_ranking(p_character->GetPlayerID()))
    {
        return;
    }

#if __cplusplus >= 202002L
    // Designated Initialization, C++20
    BossDamageRankingPlayerInfo info{
        .player_id = p_character->GetPlayerID(),
        .race = static_cast<uint8_t>(p_character->GetRaceNum()),
    };
#else
    BossDamageRankingPlayerInfo info{p_character->GetPlayerID(), static_cast<uint8_t>(p_character->GetRaceNum())};
#endif

    strncpy(info.player_name.data(), p_character->GetName(), info.player_name.size() - 1);

    m_players.emplace_back(std::make_unique<BossDamageRankingPlayerInfo>(info));
}

/**
 * @brief Calc damage percent of each player
 *
 * @param boss_max_hp
 *
 * @details Maybe changeable func..
 */
void CBossDamageRankingPlayerData::set_player_info_damage_percent(boss_hp_t boss_max_hp) const
{
    if (0U == boss_max_hp)
    {
        return;
    }

    for (const auto& player : m_players)
    {
        static constexpr auto min_percent{std::numeric_limits<unsigned long long>::min()};

        static constexpr auto max_percent{100ULL};

        static constexpr uint8_t damage_multiplier{100U};

        const auto percent{std::clamp(player->damage * damage_multiplier / boss_max_hp, min_percent, max_percent)};
        player->percent_damage = static_cast<uint8_t>(percent);
    }
}

/**
 * @brief Construct a new CBossDamageRankingBossData object
 *
 * @param boss_info The boss information to initialize the manager with
 */
CBossDamageRankingBossData::CBossDamageRankingBossData(boss_damage_ranking_boss_info_t boss_info) noexcept
    : mp_boss_info{std::move(boss_info)}
{
    mp_player_data = std::make_unique<CBossDamageRankingPlayerData>();
}

/**
 * @brief Get the boss information
 *
 * @return BossDamageRankingBossInfo* The boss information, or nullptr if not
 * found
 */
BossDamageRankingBossInfo* CBossDamageRankingBossData::get_boss_info() const noexcept 
{
    return mp_boss_info.get(); 
}

/**
 * @brief Retrieve the player data object associated with this boss data.
 *
 * @return CBossDamageRankingPlayerData* A pointer to the player data object,
 * or nullptr if the object is not valid.
 *
 * @details
 * This function is used to retrieve the player data object associated with
 * this boss data. The player data object stores information about the
 * players who have damaged the boss, such as their player ID, job, level,
 * and damage dealt.
 */
CBossDamageRankingPlayerData* CBossDamageRankingBossData::get_player_data() const noexcept
{
    return mp_player_data.get();
}

} // namespace bossdamageranking

#endif // BOSS_DAMAGE_RANKING_PLUGIN
