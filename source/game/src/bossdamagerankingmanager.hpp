#ifndef BOSSDAMAGERANKINGMANAGER_HPP
#define BOSSDAMAGERANKINGMANAGER_HPP

#include "bossdamageranking.hpp"
#include "packet.h"

namespace bossdamageranking {
class CBossDamageRankingManager final : public singleton<CBossDamageRankingManager> {
    /**
     * @brief
     */
    using boss_damage_ranking_boss_data_t = std::unique_ptr<CBossDamageRankingBossData>;

    /**
     * @brief
     */
    using boss_damage_ranking_vec_t = std::vector<boss_damage_ranking_boss_data_t>;

    /**
     * @brief
     */
    using ranking_container_t = std::tuple<std::vector<SPacketGCBossDamageRankingInfo>, std::vector<LPCHARACTER>>;

    using validate_data_t = std::pair<CBossDamageRankingBossData*, CBossDamageRankingPlayerData*>;

  public:
    /**
     * @brief Initialize the boss damage ranking manager
     */
    void initialize() noexcept;

    /**
     * @brief Add player to damage list.
     *
     * @param p_character
     * @param boss_id_data
     */
    void add_player_to_list(LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data) const;

    /**
     * @brief Process damage dealt to a boss.
     *
     * @param p_character The character ptr.
     * @param boss_id_data The boss ID and mob VID to identify the boss.
     * @param damage The amount of damage dealt to the boss.
     */
    void damage_process(LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data, uint64_t damage) const;

    /**
     * @brief Set a bad affect flag for a character in the damage ranking of a boss.
     *
     * @param boss_id_data The boss ID and mob VID to identify the boss.
     * @param player_id The character ID to set the flag for.
     * @param type The bad affect flag to be set.
     */
    void set_bad_affect(const BossDamageRankingIdData& boss_id_data, uint32_t player_id, BadAffectType type) const;

    /**
     * @brief Add a boss to the list of bosses tracked by the damage ranking manager.
     *
     * @param boss_data The boss information to add to the list.
     */
    void add_boss_to_list(const BossDamageRankingBossInfo& boss_data);

    /**
     * @brief Erase a boss from the list of bosses tracked by the damage ranking manager.
     *
     * @param boss_id_data The boss ID and mob VID to identify the boss to be erased.
     */
    void erase_boss_from_list(const BossDamageRankingIdData& boss_id_data);

    /**
     * @brief Check if boss is in the ranking
     *
     * @param mob_vnum
     *
     * @return bool
     */
    [[nodiscard]] bool is_boss_in_ranking(uint32_t mob_vnum) const noexcept;

    /**
     * @brief Send the boss damage ranking to all players that are currently logged in and in the ranking.
     *
     * @param boss_id_data The boss ID and mob VID to send the ranking for.
     */
    void send_rankings_to_players(const BossDamageRankingIdData& boss_id_data) const;

    /**
     * @brief Given a sorted vector of player information, creates a vector of
     * LPCHARACTER objects which can be used to send the rankings to the players.
     *
     * @param sorted_vec The sorted vector of player information
     *
     * @return std::vector<LPCHARACTER> The vector of LPCHARACTER objects
     */
    static std::vector<LPCHARACTER> get_character_vector_from_sorted_vec(
        const boss_damage_ranking_player_info_vec_t& sorted_vec);

    /**
     * @brief Create a vector of packet information for the boss damage ranking
     *
     * @param sorted_vec The sorted vector of player information
     *
     * @return std::vector<SPacketGCBossDamageRankingInfo> The vector of packet
     * information
     */
    static std::vector<SPacketGCBossDamageRankingInfo> create_ranking_info_vector(
        const boss_damage_ranking_player_info_vec_t& sorted_vec);

    /**
     * @brief Send boss damage rankings to a character
     *
     * @param p_character The character to which the rankings should be sent
     * @param info_vec The vector of ranking information to be sent
     */
    static void send_ranking_to_player(
        LPCHARACTER p_character, const std::vector<SPacketGCBossDamageRankingInfo>& info_vec);

    /**
     * @brief Reload the boss damage ranking manager from the database
     */
    void reload() noexcept;

  private:
    /**
     * @brief Check boss is valid
     *
     * @param boss_id_data
     * @return bool
     */
    [[nodiscard]] bool check_valid_boss(const BossDamageRankingIdData& boss_id_data) const;

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
    std::optional<ranking_container_t> create_ranking_container(const BossDamageRankingIdData& boss_id_data) const;

    /**
     * @brief Retrieve boss information based on boss ID and mob VID.
     *
     * @param boss_id_data The boss ID and mob VID to retrieve the information for.
     *
     * @return std::optional<BossDamageRankingBossInfo*> Optional containing the boss info if found, otherwise
     *
     * std::nullopt.
     */
    [[nodiscard]] std::optional<CBossDamageRankingBossData*> get_boss_info(
        const BossDamageRankingIdData& boss_id_data) const;

    /**
     * @brief
     * @param p_character
     * @param boss_id_data
     * @return
     */
    std::optional<validate_data_t> validate_and_get_data(
        LPCHARACTER p_character, const BossDamageRankingIdData& boss_id_data) const;

    /**
     * @brief
     * @param player_data
     * @param p_character
     */
    static void ensure_player_in_ranking(CBossDamageRankingPlayerData* player_data, LPCHARACTER p_character);

    /**
     * @brief Vector of boss damage ranking boss data
     */
    boss_damage_ranking_vec_t m_boss_info_vec{};

    /**
     * @brief Set of boss vnums
     */
    std::unordered_set<uint32_t> m_set_boss_vnum{};
};

/**
 * @brief Boss damage ranking manager instance
 */

CBossDamageRankingManager& boss_dmg_ranking_manager();

} // namespace bossdamageranking

#endif // BOSSDAMAGERANKINGMANAGER_HPP
