/*
 * ? Author: LWT
 * ! Date: 20.10.2024
 * * Description:
 */
#include "stdafx.h"
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamagerankingmanager.hpp"
#include "char.h"
#include "char_manager.h"
#include "db.h"
#include "mob_manager.h"
#include "networkutils.hpp"
#include "p2p.h"

namespace bossdamageranking {

/**
 * @brief Boss damage ranking manager instance
 */
CBossDamageRankingManager& boss_dmg_ranking_manager()
{
    return CBossDamageRankingManager::instance();
}

/**
 * @brief Initialize the boss damage ranking manager
 */
void CBossDamageRankingManager::initialize() noexcept
{
    const std::string query{"SELECT * FROM boss_dmg_ranking"};

    const std::unique_ptr msg(DBManager::instance().DirectQuery(query.c_str()));

    if (constexpr uint8_t sql_err_load_data{0U}; sql_err_load_data != msg->uiSQLErrno)
    {
        sys_err("CBossDmgRankingManager::initialize - cannot load boss damage ranking data");

        return;
    }

    if (constexpr uint8_t sql_zero_rows{0U}; sql_zero_rows == msg->Get()->uiNumRows)
    {
        sys_err("CBossDmgRankingManager::initialize - no boss damage ranking data found");

        return;
    }

    MYSQL_ROW row{};
    while (nullptr != (row = mysql_fetch_row(msg->Get()->pSQLResult)))
    {
        uint32_t mob_vnum{};
        str_to_number(mob_vnum, row[0]);

        m_set_boss_vnum.emplace(mob_vnum);
    }
}

/**
 * @brief Retrieve boss information based on boss ID and mob VID.
 *
 * @param boss_id_data The boss ID and mob VID to retrieve the information for.
 * @return std::optional<BossDamageRankingBossInfo*> Optional containing the
 * boss info if found, otherwise std::nullopt.
 */
std::optional<CBossDamageRankingBossData*> CBossDamageRankingManager::get_boss_info(
    const BossDamageRankingIdData& boss_id_data) const
{
    if (m_boss_info_vec.empty()) { return std::nullopt; }

    const auto check_pred_func{[boss_id_data](const boss_damage_ranking_boss_data_t& boss_info)
        {
            const auto* const boss_info_ptr{boss_info->get_boss_info()};

            if (nullptr == boss_info_ptr) { return false; }

            return boss_info_ptr->mob_vnum == boss_id_data.mob_vnum && boss_info_ptr->mob_vid == boss_id_data.mob_vid;
        }};

#if __cplusplus >= 202002L
    const auto& boss_info_iter{std::ranges::find_if(m_boss_info_vec, check_pred_func)};
#else
    const auto& boss_info_iter{std::find_if(m_boss_info_vec.begin(), m_boss_info_vec.end(), check_pred_func)};
#endif
    // Check if the boss information was not found
    if (boss_info_iter == m_boss_info_vec.cend()) { return std::nullopt; }

    return boss_info_iter->get();
}

/**
 * @brief Validate character and boss information.
 *
 * @param p_character The character pointer.
 * @param boss_id_data The boss ID and mob VID to identify the boss.
 * @return Optional pair containing boss info and player data if validation succeeds, std::nullopt otherwise.
 */
std::optional<CBossDamageRankingManager::validate_data_t> CBossDamageRankingManager::validate_and_get_data(
    LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data) const
{
    if (nullptr == p_character || nullptr == p_character->GetDesc()) { return std::nullopt; }

    const auto& boss_info = get_boss_info(boss_id_data);

    if (std::nullopt == boss_info) { return std::nullopt; }

    auto* player_data = boss_info.value()->get_player_data();

    if (nullptr == player_data) { return std::nullopt; }

    return std::make_pair(boss_info.value(), player_data);
}

/**
 * @brief Add player to the damage list if not already present.
 *
 * @param player_data The player data object.
 * @param p_character The character pointer.
 */
void CBossDamageRankingManager::ensure_player_in_ranking(
    CBossDamageRankingPlayerData* player_data, LPCHARACTER p_character)
{
    if (const auto player_id{p_character->GetPlayerID()}; !player_data->is_player_in_ranking(player_id))
    {
        player_data->add_player(p_character);
    }
}

/**
 * @brief Add player to damage list.
 *
 * @param p_character
 * @param boss_id_data
 */
void CBossDamageRankingManager::add_player_to_list(
    LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data) const
{
    if (nullptr == p_character || nullptr == p_character->GetDesc()) { return; }

    const auto& boss_info{get_boss_info(boss_id_data)};

    if (std::nullopt == boss_info) { return; }

    auto* player_data{boss_info.value()->get_player_data()};

    if (nullptr == player_data) { return; }

    ensure_player_in_ranking(player_data, p_character);
}

/**
 * @brief Process damage dealt to a boss.
 *
 * @param p_character The character ptr.
 * @param boss_id_data The boss ID and mob VID to identify the boss.
 * @param damage The amount of damage dealt to the boss.
 */
void CBossDamageRankingManager::damage_process(
    LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data, uint64_t damage) const
{
    const auto validation_result = validate_and_get_data(p_character, boss_id_data);

    if (!validation_result.has_value()) { return; }

    auto* boss_info = validation_result->first;
    auto* player_data = validation_result->second;

    ensure_player_in_ranking(player_data, p_character);

    const auto player_id = p_character->GetPlayerID();
    player_data->add_damage(player_id, damage);

    player_data->set_player_info_damage_percent(boss_info->get_boss_info()->max_hp);

    send_rankings_to_players(boss_id_data);
}

/**
 * @brief Set a bad affect flag for a character in the damage ranking of a boss.
 *
 * @param boss_id_data The boss ID and mob VID to identify the boss.
 * @param player_id The character ID to set the flag for.
 * @param type The bad affect flag to be set.
 */
void CBossDamageRankingManager::set_bad_affect(
    const BossDamageRankingIdData& boss_id_data, uint32_t player_id, BadAffectType type) const
{
    const auto& boss_info{get_boss_info(boss_id_data)};

    if (std::nullopt == boss_info) { return; }

    const auto* const player_data{boss_info.value()->get_player_data()};

    player_data->set_bad_affect_flag(player_id, type);

    send_rankings_to_players(boss_id_data);
}

/**
 * @brief Add a boss to the list of bosses tracked by the damage ranking
 * manager.
 *
 * @param boss_data The boss information to add to the list.
 */
void CBossDamageRankingManager::add_boss_to_list(const BossDamageRankingBossInfo& boss_data)
{
    if (!is_boss_in_ranking(boss_data.mob_vnum)) { return; }

    auto p_boss_info{std::make_unique<BossDamageRankingBossInfo>(boss_data)};

    m_boss_info_vec.emplace_back(std::make_unique<CBossDamageRankingBossData>(std::move(p_boss_info)));
}

/**
 * @brief Erase a boss from the list of bosses tracked by the damage ranking
 * manager.
 *
 * @param boss_id_data The boss ID and mob VID to identify the boss to be
 * erased.
 */
void CBossDamageRankingManager::erase_boss_from_list(const BossDamageRankingIdData& boss_id_data)
{
    if (std::nullopt == get_boss_info(boss_id_data)) { return; }

    const auto erase_pred_func{[boss_id_data](const boss_damage_ranking_boss_data_t& boss_info)
        {
            const auto* const boss_info_ptr{boss_info->get_boss_info()};

            if (!boss_info_ptr) { return false; }

            return boss_info_ptr == boss_id_data;
        }};

#if __cplusplus >= 202002L
    std::erase_if(m_boss_info_vec, erase_pred_func);
#else
    m_boss_info_vec.erase(
        std::remove_if(m_boss_info_vec.begin(), m_boss_info_vec.end(), erase_pred_func), m_boss_info_vec.end());
#endif
}

/**
 * @brief Check if boss is in the ranking
 *
 * @param mob_vnum
 *
 * @return bool
 */
bool CBossDamageRankingManager::is_boss_in_ranking(uint32_t mob_vnum) const noexcept
{
#if __cplusplus >= 202002L
    return m_set_boss_vnum.contains(mob_vnum);
#else
    return m_set_boss_vnum.count(mob_vnum);
#endif
}

/**
 * @brief Send the boss damage ranking to all players that are currently logged in and in the ranking.
 *
 * @param boss_id_data The boss ID and mob VID to send the ranking for.
 */
void CBossDamageRankingManager::send_rankings_to_players(const BossDamageRankingIdData& boss_id_data) const
{
    const auto ranking_list{create_ranking_container(boss_id_data)};

    if (std::nullopt == ranking_list) { return; }

    const auto& [info_vec, player_vec]{ranking_list.value()};

    for (auto* const p_character: player_vec)
    {
        if (nullptr == p_character) { continue; }

        send_ranking_to_player(p_character, info_vec);
    }
}

/**
 * @brief Given a sorted vector of player information, creates a vector of
 * LPCHARACTER objects which can be used to send the rankings to the players.
 *
 * @param sorted_vec The sorted vector of player information
 *
 * @return std::vector<LPCHARACTER> The vector of LPCHARACTER objects
 */
std::vector<LPCHARACTER> CBossDamageRankingManager::get_character_vector_from_sorted_vec(
    const boss_damage_ranking_player_info_vec_t& sorted_vec)
{
    std::vector<LPCHARACTER> player_vec(sorted_vec.size());

#if __cplusplus >= 202002L
    std::ranges::transform(sorted_vec,
        player_vec.begin(),
        [](const auto& player_info)
        {
            return CHARACTER_MANAGER::instance().FindByPID(player_info->player_id);
        });
#else
    std::transform(sorted_vec.begin(),
        sorted_vec.end(),
        player_vec.begin(),
        [](const auto& player_info)
        {
            return CHARACTER_MANAGER::instance().FindByPID(player_info->player_id);
        });
#endif
    return player_vec;
}

/**
 * @brief Create a vector of packet information for the boss damage ranking
 *
 * @param sorted_vec The sorted vector of player information
 *
 * @return std::vector<SPacketGCBossDamageRankingInfo> The vector of packet
 * information
 */
std::vector<SPacketGCBossDamageRankingInfo> CBossDamageRankingManager::create_ranking_info_vector(
    const boss_damage_ranking_player_info_vec_t& sorted_vec)
{
    std::vector<SPacketGCBossDamageRankingInfo> info_vec(sorted_vec.size());

    const auto pred_func{[](const boss_damage_ranking_player_info_t& player_info)
        {
            SPacketGCBossDamageRankingInfo info{};
            strncpy(info.name, player_info->player_name.data(), player_info->player_name.size() - 1);
            info.race = player_info->race;
            info.percent_damage = player_info->percent_damage;
            info.bad_affect_flag = player_info->bad_affect_flag;

            return info;
        }};
#if __cplusplus >= 202002L
    std::ranges::transform(sorted_vec, info_vec.begin(), pred_func);
#else
    std::transform(sorted_vec.begin(), sorted_vec.end(), info_vec.begin(), pred_func);
#endif
    return info_vec;
}

/**
 * @brief Send boss damage rankings to a character
 *
 * @param p_character The character to which the rankings should be sent
 * @param info_vec The vector of ranking information to be sent
 */
void CBossDamageRankingManager::send_ranking_to_player(
    LPCHARACTER p_character, const std::vector<SPacketGCBossDamageRankingInfo>& info_vec)
{
#if __cplusplus >= 202002L
    const SPacketGCRankingGeneralInfo init_packet{
        .rank_size = static_cast<uint8_t>(info_vec.size()),
    };
#else
    SPacketGCRankingGeneralInfo init_packet{};
    init_packet.rank_size = static_cast<uint8_t>(info_vec.size());
#endif

    static networkutils::DynamicPacketBuilder packet_builder{};
    packet_builder
        .add_header(HEADER_GC_BOSS_DMG_RANKING, EPacketCGBossDamageRankingSubHeaderType::BOSS_DMG_RANKING_RANK_INFO)
        .add_payload(init_packet)
        .add_payload(info_vec)
        .send_to_client(p_character);
}

/**
 * @brief Create a container with the ranking information and characters for a boss
 *
 * @param boss_id_data The id data of the boss for which the container should be created
 *
 * @return std::tuple<std::vector<SPacketGCBossDamageRankingInfo>, std::vector<LPCHARACTER>>
 * A tuple containing the ranking information and characters for the boss
 *
 * @throws std::runtime_error If the boss info is not found
 */
std::optional<CBossDamageRankingManager::ranking_container_t> CBossDamageRankingManager::create_ranking_container(
    const BossDamageRankingIdData& boss_id_data) const
{
    const auto& p_boss_info{get_boss_info(boss_id_data)};

    if (std::nullopt == p_boss_info) { return std::nullopt; }

    const auto& sorted_vec{p_boss_info.value()->get_player_data()->get_sorted_player_info_vec()};

    const auto info_vec{create_ranking_info_vector(sorted_vec)};

    const auto player_vec{get_character_vector_from_sorted_vec(sorted_vec)};

    return std::make_tuple(info_vec, player_vec);
}

/**
 * @brief Reload the boss damage ranking manager from the database
 */
void CBossDamageRankingManager::reload() noexcept
{
    initialize();

    static constexpr uint8_t p2p_header{HEADER_GG_UPDATE_BOSS_DAMAGE_RANKING};
    P2P_MANAGER::Instance().Send(&p2p_header, sizeof(uint8_t));
}

} // namespace bossdamageranking

#endif // BOSS_DAMAGE_RANKING_PLUGIN
