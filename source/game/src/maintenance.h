#ifdef ENABLE_MAINTENANCE_SYSTEM

class CMaintenance : public singleton<CMaintenance>
{
public:
	CMaintenance();
	virtual ~CMaintenance();

	void StartMaintenance(DWORD dwTime, DWORD dwTime2);
	void StartMaintenanceTimer(DWORD dwTime, DWORD dwTime2);
	void MaintenancePacket(DWORD dwTime, DWORD dwTime2);
	void MaintenanceP2PPacket(bool bStatus, DWORD dwTime, DWORD dwTime2);

	void CancelMaintenance();
	void CancelMaintenanceTimer();

	void SetMaintenance(bool bStatus) { bMaintenance = bStatus; }
	bool IsMaintenance() { return bMaintenance; }

	void SetRemainingTime(DWORD dwTime) { dwRemainingTime = dwTime; }
	DWORD GetRemainingTime() { return dwRemainingTime; }

	void SetMaintenanceTime(DWORD dwTime) { dwMaintenanceTime = dwTime; }
	DWORD GetMaintenanceTime() { return dwMaintenanceTime; }

	const char* SecondToHM(DWORD dwTime);

protected:
	DWORD dwRemainingTime;
	DWORD dwMaintenanceTime;
	bool bMaintenance;
};

#endif