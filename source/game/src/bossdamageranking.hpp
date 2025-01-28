#ifndef BOSSDAMAGERANKING_HPP
#define BOSSDAMAGERANKING_HPP

#include "../../common/tables.h"

#include <memory>

namespace bossdamageranking
{

enum class BadAffectType : uint8_t
{
    NONE = 0 << 0,
    POISON = 1 << 0,
    FIRE = 1 << 1,
#ifdef ENABLE_WOLFMAN_CHARACTER
    BLEED = 1 << 2,
#endif
};

/**
 * @brief Boss HP variable type
 */
using boss_hp_t = decltype(TMobTable::dwMaxHP);

/**
 * @brief Boss damage ranking player info
 */
struct BossDamageRankingPlayerInfo
{
    uint32_t player_id{};
    std::array<char, CHARACTER_NAME_MAX_LEN + 1> player_name{};
    uint8_t race{};
    uint64_t damage{};
    uint8_t bad_affect_flag{};
    uint8_t percent_damage{};
};

/**
 * @brief Boss damage ranking player info type alias
 */
using boss_damage_ranking_player_info_t = std::unique_ptr<BossDamageRankingPlayerInfo>;

/**
 * @brief Boss damage ranking player info vector type alias
 */
using boss_damage_ranking_player_info_vec_t = std::vector<boss_damage_ranking_player_info_t>;

class CBossDamageRankingPlayerData
{
public:
    /**
     * @brief Construct a new CBossDamageRankingPlayerManager object
     */
    CBossDamageRankingPlayerData() noexcept = default;

    /**
     * @brief Construct a new CBossDamageRankingPlayerManager object with
     * player info
     *
     * @param player_info The player information to initialize the manager with
     */
    explicit CBossDamageRankingPlayerData(boss_damage_ranking_player_info_t&& player_info) noexcept;

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
    void set_bad_affect_flag(uint32_t player_id, BadAffectType flag) const;

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
    void add_damage(uint32_t player_id, uint64_t damage) const;

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
    boss_damage_ranking_player_info_vec_t& get_sorted_player_info_vec();

    /**
     * @brief Check if the player is in the ranking
     *
     * @param player_id The player's ID to check
     * @return bool True if the player is in the ranking, false otherwise
     */
    [[nodiscard]] bool is_player_in_ranking(uint32_t player_id) const;

    /**
     * @brief Add a player to the boss damage ranking.
     *
     * @param p_character A pointer to the character object representing the
     * player.
     */
    void add_player(LPCHARACTER p_character);

    /**
     * @brief Calc damage percent of each player
     *
     * @param boss_max_hp
     *
     * @details Maybe changeable func..
     */
    void set_player_info_damage_percent(boss_hp_t boss_max_hp) const;

private:
    /**
     * @brief Get player information from the ranking
     *
     * @param player_id The player's ID to retrieve the information for
     * @return std::optional<BossDamageRankingPlayerInfo*> Optional containing
     * the player info if found, otherwise std::nullopt
     */
    [[nodiscard]] std::optional<BossDamageRankingPlayerInfo*> get_player_info(uint32_t player_id) const noexcept;

    /**
     * @brief Players data
     */
    boss_damage_ranking_player_info_vec_t m_players{};
};

/**
 * @brief Boss damage ranking id data
 */
struct BossDamageRankingIdData
{
    uint32_t mob_vnum{};
    uint32_t mob_vid{};
};

/**
 * @brief Boss damage ranking boss info
 */
struct BossDamageRankingBossInfo : BossDamageRankingIdData
{
    boss_hp_t max_hp{};
};

/**
 * @brief Compare a BossDamageRankingBossInfo pointer with a BossDamageRankingIdData object for equality.
 *
 * @param boss_info_ptr Pointer to the BossDamageRankingBossInfo object.
 * @param boss_info Reference to the BossDamageRankingIdData object.
 *
 * @return true if the mob_vid of the BossDamageRankingBossInfo and BossDamageRankingIdData are equal, false otherwise.
 */
[[nodiscard]] inline bool operator==(const BossDamageRankingBossInfo* boss_info_ptr, const BossDamageRankingIdData& boss_info) noexcept
{
    return boss_info_ptr->mob_vid == boss_info.mob_vid && boss_info_ptr->mob_vnum == boss_info.mob_vnum;
}

/**
 * @brief Boss damage ranking boss info type alias
 */
using boss_damage_ranking_boss_info_t = std::unique_ptr<BossDamageRankingBossInfo>;

class CBossDamageRankingBossData
{
public:
    /**
     * @brief Construct a new CBossDamageRankingBossData object with default
     * initialization
     */
    CBossDamageRankingBossData() noexcept = default;

    /**
     * @brief Construct a new CBossDamageRankingBossData object
     *
     * @param boss_info The boss information to initialize the manager with
     */
    explicit CBossDamageRankingBossData(boss_damage_ranking_boss_info_t boss_info) noexcept;

    /**
     * @brief Get the boss information
     *
     * @return BossDamageRankingBossInfo* The boss information, or nullptr if
     * not found
     */
    [[nodiscard]] BossDamageRankingBossInfo* get_boss_info() const noexcept;

    /**
     * @brief Retrieve the player data object associated with this boss data.
     *
     * @return CBossDamageRankingPlayerData* A pointer to the player data
     * object, or nullptr if the object is not valid.
     *
     * @details
     * This function is used to retrieve the player data object associated with
     * this boss data. The player data object stores information about the
     * players who have damaged the boss, such as their player ID, job, level,
     * and damage dealt.
     */
    [[nodiscard]] CBossDamageRankingPlayerData* get_player_data() const noexcept;

private:
    /**
     * @brief Player data ptr
     */
    std::unique_ptr<CBossDamageRankingPlayerData> mp_player_data{};

    /**
     * @brief @brief
     */
    boss_damage_ranking_boss_info_t mp_boss_info{};
};

} // namespace bossdamageranking

#endif // BOSSDAMAGERANKING_HPP
