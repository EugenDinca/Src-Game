/*
 * author			 : Kio
 * file_creation_date : 12.02.2023
 * file_name			 : player_block.h
 */

class CPlayerBlock : public singleton<CPlayerBlock>
{
public:
	CPlayerBlock(void);
	virtual ~CPlayerBlock(void);

	auto BlockPlayer(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName, bool isP2P = false) -> void;
	auto UnBlockPlayer(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName, bool isP2P = false) -> void;

	auto IsPlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> bool;

	auto PlayerBlockPacket(LPCHARACTER ch, const std::string &strBlockedPlayerName, BYTE bType) -> void;
	auto WhisperPacket(LPDESC d, const char *szText, const std::string &strBlockedPlayerName) -> void;

	auto LoadPlayerBlockList(LPCHARACTER ch) -> void;

	auto InsertPlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> void;
	auto DeletePlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> void;

	auto ChangeName(const std::string &strOldName, const std::string &strNewName) -> void;

	auto BootPlayerBlockList(void) -> void;

private:
	std::map<std::string, std::set<std::string>> m_map_PlayerBlock;
};