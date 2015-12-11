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

typedef enum {
	ONBATTERY,
	LOWBATTERY,
	ONSOLAR,
	CHARGING,
	CHARGECOMPLETE,
	FAULT
} battery_status_t;

typedef enum {
	SINGLECOLOR,
	RGBFADE,
	COLORFADE1,
	COLORFADE2
} led_mode_t;

iMailboxStatus getStatus(void);
void updateStatus(void);
void incrementUptimeSeconds(void);
char getMode(void);
void setMode(char);

#endif /* USER_STATUS_H_ */
