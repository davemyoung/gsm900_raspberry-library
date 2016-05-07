#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wiringPi.h"

#define byte 	unsigned char
#define word 	unsigned int

#define HIGH	1
#define LOW 	0

#define GSM_LIB_VERSION 	100
// pins definition
#define GSM_ON              0 // connect GSM Module turn ON to pin 77
#define GSM_RESET           1 // connect GSM Module RESET to pin 35

#define DTMF_DATA_VALID     14 // connect DTMF Data Valid to pin 14
#define DTMF_DATA0          72 // connect DTMF Data0 to pin 72
#define DTMF_DATA1          73 // connect DTMF Data1 to pin 73
#define DTMF_DATA2          74 // connect DTMF Data2 to pin 74
#define DTMF_DATA3          75 // connect DTMF Data3 to pin 75

// length for the internal communication buffer
#define COMM_BUF_LEN        300

// some constants for the IsRxFinished() method
#define RX_NOT_STARTED      0
#define RX_ALREADY_STARTED  1

// some constants for the InitParam() method
#define PARAM_SET_0   0
#define PARAM_SET_1   1

// DTMF signal is NOT valid
//#define DTMF_NOT_VALID      0x10

// status bits definition
#define STATUS_NONE                 0
#define STATUS_INITIALIZED          1
#define STATUS_REGISTERED           2
#define STATUS_USER_BUTTON_ENABLE   4

enum sms_type_enum
{
	SMS_UNREAD,
	SMS_READ,
	SMS_ALL,

	SMS_LAST_ITEM
};

enum comm_line_status_enum
{
	// CLS like CommunicationLineStatus
	CLS_FREE,   // line is free - not used by the communication and can be used
	CLS_ATCMD,  // line is used by AT commands, includes also time for response
	CLS_DATA,   // for the future - line is used in the CSD or GPRS communication
	CLS_LAST_ITEM
};

enum rx_state_enum
{
	RX_NOT_FINISHED = 0,      // not finished yet
	RX_FINISHED,              // finished, some character was received
	RX_FINISHED_STR_RECV,     // finished and expected string received
	RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
	RX_TMOUT_ERR,             // finished, no character received
	// initial communication tmout occurred
	RX_LAST_ITEM
};

enum at_resp_enum
{
	AT_RESP_ERR_NO_RESP = -1,   // nothing received
	AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
	AT_RESP_OK = 1,             // response_string was included in the response

	AT_RESP_LAST_ITEM
};

enum registration_ret_val_enum
{
	REG_NOT_REGISTERED = 0,
	REG_REGISTERED,
	REG_NO_RESPONSE,
	REG_COMM_LINE_BUSY,

	REG_LAST_ITEM
};

enum call_ret_val_enum
{
	CALL_NONE = 0,
	CALL_INCOM_VOICE,
	CALL_ACTIVE_VOICE,
	CALL_INCOM_VOICE_AUTH,
	CALL_INCOM_VOICE_NOT_AUTH,
	CALL_INCOM_DATA_AUTH,
	CALL_INCOM_DATA_NOT_AUTH,
	CALL_ACTIVE_DATA,
	CALL_OTHERS,
	CALL_NO_RESPONSE,
	CALL_COMM_LINE_BUSY,

	CALL_LAST_ITEM
};

enum getsms_ret_val_enum
{
	GETSMS_NO_SMS   = 0,
	GETSMS_UNREAD_SMS,
	GETSMS_READ_SMS,
	GETSMS_OTHER_SMS,

	GETSMS_NOT_AUTH_SMS,
	GETSMS_AUTH_SMS,

	GETSMS_LAST_ITEM
};

byte module_status;
byte comm_line_status;

// set comm. line status
void SetCommLineStatus(byte new_status);
// get comm. line status
byte GetCommLineStatus(void);

// picks up an incoming call
void PickUp(void);
// hangs up an incomming call
void HangUp(void);
byte CallStatus(void);

void DebugPrint(const char *string_to_print, byte last_debug_print);
void DebugPrintD(int number_to_print, byte last_debug_print);

void TurnOn(long baud_rate);
void InitParam(byte group);
char InitSMSMemory(void); 
// routines regarding communication with the GSM module	和GSM模块通讯部分的程序
void RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
byte IsRxFinished(void);
byte IsStringReceived(char const *compare_string);
byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
byte WaitRespAdd(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,char const *expected_resp_string);
char SendATCmdWaitResp(char *AT_cmd_string,
						uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
						char const *response_string,
						byte no_of_attempts);

//int millis(void);
void Echo(byte state);
//some routies for pi serial
int   serialOpen      (char *device, int baud);
void  serialClose     (void);
void  serialFlush     (void);
void  serialPutchar   (unsigned char c);
void  serialPuts      (char *s);
char *ChangeIToS	  (int IntNU);
void  serialPrintf    (char *message, ...);
int   serialDataAvail (void);
int   serialGetchar   (void);

void  serialBegin     (int baud);
//void  delay			  (int a);
int LibVer(void);
byte CheckRegistration(void);
byte IsRegistered(void);
byte IsInitialized(void);

// SMS's methods		短信部分
char SendSMS(char *number_str, char *message_str);
char SendSMSSpecified(byte sim_phonebook_position, char *message_str);
char IsSMSPresent(byte required_status);
char GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len);
char GetAuthorizedSMS(	byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
						byte first_authorized_pos, byte last_authorized_pos);
char DeleteSMS(byte position);

// Phonebook's methods	sim卡上的电话号码处理
char GetPhoneNumber(byte position, char *phone_number);
char WritePhoneNumber(byte position, char *phone_number);
char DelPhoneNumber(byte position);
char ComparePhoneNumber(byte position, char *phone_number);

// Speaker volume methods - set, increase, decrease	扬声器处理部分
char SetSpeakerVolume(byte speaker_volume);
char IncSpeakerVolume(void);
char DecSpeakerVolume(void);

// sends DTMF signal	发送双音多频信号
char SendDTMFSignal(byte dtmf_tone);

// User button methods	用户按键处理	
byte IsUserButtonEnable(void);
void DisableUserButton(void);
void EnableUserButton(void);
byte IsUserButtonPushed(void);

void CallS(char *number_string);
void Call(int sim_position);
byte CallStatusWithAuth(char *phone_number,
						byte first_authorized_pos, byte last_authorized_pos);
