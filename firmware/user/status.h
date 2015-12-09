#ifndef USER_STATUS_H_
#define USER_STATUS_H_

typedef struct {
	char ledMode;
	char ledShow;
	int lightReading;
	int lightThreshold;
	char batteryStatus;
	long uptimeSeconds;
} iMailboxStatus;

enum {
	OnBattery,
	LowBattery,
	OnSolar,
	Charging,
	ChargeComplete,
	Fault
} BatteryStatus;

iMailboxStatus getStatus(void);
void setStatusUptimeSeconds(long sec);
void incrementUptimeSeconds(void);
long getUptimeSeconds(void);
void updateLightReading(void);
void updateBatteryStatus(void);

#endif /* USER_STATUS_H_ */
