/************************************************************
* title_name		: Challenge System
* author			: SkyOfDance
* date				: 15.08.2022
************************************************************/
#pragma once	
#ifdef ENABLE_CHALLENGE_SYSTEM
enum ChallengeConf
{
	CHALLENGE_QUEST_NAME_1,//Ýlk 120. seviyeye ulaþan.
	CHALLENGE_QUEST_NAME_2,//Ýlk 300 bin derece yapan.
	CHALLENGE_QUEST_NAME_3,//Ýlk 16 tip seviyeli Pet yapan.
	CHALLENGE_QUEST_NAME_4,//Ýlk 250 seviye Pet yapan.
	MAX_QUEST_LIMIT,
};

typedef struct SChallengeInfo
{
	BYTE bId;
	char szWinner[64 + 1];
	bool bState;
}TChallengeInfo;

class CChallenge : public singleton<CChallenge>
{
public:
	CChallenge();
	virtual ~CChallenge();

	void Initialize();
	void Destroy();
	TChallengeInfo GetInfoDB(BYTE id);
	void SetWinner(BYTE id, LPCHARACTER ch);
	void RecvWinnerList(LPCHARACTER ch);
	bool GetWinner(BYTE id);
	void RefinedChallenge(DWORD vnum, LPCHARACTER ch);
	void UpdateP2P(TChallengeInfo pgg);
protected:
	bool m_Initialize;
	TChallengeInfo m_ChallengeWinnerList[ChallengeConf::MAX_QUEST_LIMIT];


	const DWORD m_Challenge_GiftValue[ChallengeConf::MAX_QUEST_LIMIT] =
	{
		350,//Ýlk 120. seviyeye ulaþan.
		500,//Ýlk 300 bin derece yapan.
		150,//Ýlk 16 tip seviyeli Pet yapan.
		5000,//Ýlk 250 seviye Pet yapan.
		200,//Ýlk Mor Tanrýça Ýtemlerinden birini +9 yapan.
		225,//Ýlk Yeþil Tanrýça yapan.
		300,//Ýlk Nemea Peti+25 yapan.
		300,//Ýlk Nemea Bineði+25 yapan.
		300,//Ýlk Nemea Kask Kostümü+25 basan.
		300,//Ýlk Nemea Giysi Kostümü+25 basan.
		300,//Ýlk Nemea Silah Kostümü+25 basan.
		200,//Ýlk Sage seviyesinde beceri yapan.
		250,//Ýlk Yohara 10.seviyeye ulaþan.
		450,//Ýlk Reborn 10.seviyeye ulaþan.
		750,//Ýlk Astteðmen rütbesine ulaþan.
		1000,//Ýlk Zaman tünelinde 20.seviyeye ulaþan.
		100,//Ýlk Nemea Daðlarý yüzüklerinden birini +9 yapan.
		300,//Ýlk Nemea Daðlarý Eldiveni+0 yapan.
		300,//Ýlk Nemea Daðlarý Kemeri+0 yapan.
		300,//Ýlk Element Týlsýmý+0 yapan.
		500,//Ýlk Biologda Sevgililer Günü Küresi görevini tamamlayan
		600,//Ýlk Nemea Peti+50 yapan.
		600,//Ýlk Nemea Bineði+50 yapan.
		600,//Ýlk Nemea Kask Kostümü+50 basan.
		600,//Ýlk Nemea Giysi Kostümü+50 basan.
		600,//Ýlk Nemea Silah Kostümü+50 basan.
		400,//Ýlk Yeþil Tanrýça Ýtemlerinden birini +9 yapan.
		500,//Ýlk 1 milyon derece yapan.
		500,//Ýlk Yohara 30.seviyeye ulaþan.
		300,//Ýlk Boosterlardan herhangi birini +9 yapan.
		300,//Ýlk Þebnemlerden herhangi birini +9 yapan.
		1000,//Ýlk 100 Emiþ kuþak yapan.
		750,//Ýlk Reborn 20.seviyeye ulaþan.
		750,//Ýlk Nemea Peti+75 yapan.
		750,//Ýlk Nemea Bineði+75 yapan.
		750,//Ýlk Nemea Kask Kostümü+75 basan.
		750,//Ýlk Nemea Giysi Kostümü+75 basan.
		750,//Ýlk Nemea Silah Kostümü+75 basan.
		500,//Ýlk Legendary seviyesinde beceri yapan.
		750,//Ýlk Rüya & Gök Ruhu+100 yapan.
		500,//Ýlk Mavi Tanrýça yapan.
		500,//Ýlk Boosterlardan herhangi birini +15 yapan.
		500,//Ýlk Þebnemlerden herhangi birini +15 yapan.
		500,//Ýlk Rünlerden herhangi birini +9 yapan.
		750,//Ýlk Mavi Tanrýça Ýtemlerinden birini +9 yapan.
		3000,//Ýlk 250 seviye Tanrýça yapan.
		100,//Ýlk Canavardan Koruyan Taþ+10 yapan.
		100,//Ýlk Metin Taþý+10 yapan.
		100,//Ýlk Patron Taþý+10 yapan.
		100,//Ýlk Savunma Taþý+10 yapan.
		1000,//Ýlk Zaman tünelinde 32.seviyeye ulaþan.
		1000,//Ýlk Reborn 30.seviyeye ulaþan.
		1000,//Ýlk Maraþel rütbesine ulaþan.
		1000,//Ýlk Yohara 60.seviyeye ulaþan.
		1000,//Ýlk Nemea Peti+100 yapan.
		1000,//Ýlk Nemea Bineði+100 yapan.
		1000,//Ýlk Nemea Kask Kostümü+100 basan.
		1000,//Ýlk Nemea Giysi Kostümü+100 basan.
		1000,//Ýlk Nemea Silah Kostümü+100 basan.
		1000,//Ýlk Biolog görevlerini bitiren.
		200,//Ýlk Perma Kalkan Cevheri yapan.
		200,//Ýlk Perma Ayakkabý Cevheri yapan.
		200,//Ýlk Perma Kask Cevheri yapan.
		200,//Ýlk Perma Týlsým Cevheri yapan.
		200,//Ýlk Perma Eldiven Cevheri yapan.
		100,//Ýlk Perma Kostüm Cevheri yapan.
		100,//Ýlk Perma Booster Cevheri yapan.
		100,//Ýlk Perma Yüzük Cevheri yapan.
		100,//Ýlk Perma Rüya Ruhu Cevheri yapan.
		500,//Ýlk Zümrüt Eldiveni+0 yapan.
		500,//Ýlk 250 Ortalama Silah yapan.
		750//Ýlk Element Týlsýmý+200 yapan.
	};
};
#endif