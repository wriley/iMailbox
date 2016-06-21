#ifndef USER_STATUS_H_
#define USER_STATUS_H_

struct __attribute__((aligned(4))) iMailboxStatus {
	uint32_t uptimeSeconds;
	uint32_t colorSingle;
	uint32_t colorFade1;
	uint32_t colorFade2;
	uint16_t lightReading;
	uint16_t lightThreshold;
	uint8_t ledMode;
	uint8_t ledShow;
	uint8_t batteryStatus;
	uint8_t padding;
};

typedef enum {
	ONBATTERY,
	LOWBATTERY,
	ONSOLAR,
	CHARGING,
	CHARGECOMPLETE,
	FAULT
} battery_status_t;

typedef enum {
	OFF,
	SINGLECOLOR,
	RGBFADE,
	COLORFADE1,
	COLORFADE2
} led_mode_t;

struct iMailboxStatus getStatus(void);
void updateStatus(void);
void incrementUptimeSeconds(void);
char getMode(void);
void setMode(char);
uint32_t getColorSingle(void);
void setColorSingle(uint32_t);
uint16_t getLightThreshold(void);
void setLightThreshold(uint16_t);
void loadStatus(void);
void saveStatus(void);

#endif /* USER_STATUS_H_ */
