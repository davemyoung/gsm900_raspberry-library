#include "GSM.h"

#include "wiringPi.h"
//#define DEBUG_PRINT
//#define DEBUG_SMS_ENABLED
int gFD;			// variables connected with communication buffer

byte *p_comm_buf;               // pointer to the communication buffer
byte comm_buf_len;              // num. of characters in the buffer
byte rx_state;                  // internal state of rx state machine
byte last_speaker_volume;
uint16_t start_reception_tmout; // max tmout for starting reception
uint16_t interchar_tmout;       // previous time in msec.
unsigned long prev_time;        // previous time in msec.

byte comm_buf[COMM_BUF_LEN+1];

#ifdef DEBUG_PRINT
/**********************************************************
Two methods print out debug information to the standard output
- it means to the serial line.
First method prints string.
Second method prints integer numbers.

Note:
=====
The serial line is connected to the GSM module and is 
used for sending AT commands. There is used "trick" that GSM
module accepts not valid AT command strings because it doesn't
understand them and still waits for some valid AT command.
So after all debug strings are sent we send just AT<CR> as
a valid AT command and GSM module responds by OK. So previous 
debug strings are overwritten and GSM module is not influenced
by these debug texts 


string_to_print:  pointer to the string to be print out
last_debug_print: 0 - this is not last debug info, we will
                      continue with sending... so don't send
                      AT<CR>(see explanation above)
                  1 - we are finished with sending debug info 
                      for this time and finished AT<CR> 
                      will be sent(see explanation above)

**********************************************************/
void DebugPrint(const char *string_to_print, byte last_debug_print)
{
  if (last_debug_print) 
  {
    printf("%d",string_to_print);

    SendATCmdWaitResp("AT", 500, 50, "OK", 1);
  }
  else printf(string_to_print);
printf("\n");
}

void DebugPrintD(int number_to_print, byte last_debug_print)
{
  printf("%d",number_to_print);
  if (last_debug_print)
  {
    SendATCmdWaitResp("AT", 500, 50, "OK", 1);
  }
printf("\n");
}
#endif

/**********************************************************
  Checks if the GSM module is responding 
  to the AT command
  - if YES nothing is made 
  - if NO GSM module is turned on 
**********************************************************/
void TurnOn(long baud_rate)
{
  	int i;

	pullUpDnControlGpio(GSM_ON,PUD_DOWN);
	pullUpDnControlGpio(GSM_RESET,PUD_DOWN);
	SetCommLineStatus(CLS_ATCMD);
  	serialBegin(baud_rate);

	#ifdef DEBUG_PRINT
	// parameter 0 - because module is off so it is not necessary 
	// to send finish AT<CR> here
	DebugPrint("DEBUG: baud ", 0);
	DebugPrintD(baud_rate, 0);
	#endif

	if (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp("AT", 500, 100, "OK", 5)) 
	{		//check power
    	// there is no response => turn on the module

		#ifdef DEBUG_PRINT
			// parameter 0 - because module is off so it is not necessary 
			// to send finish AT<CR> here
			DebugPrint("DEBUG: GSM module is off\r\n", 0);
			DebugPrint("DEBUG: start the module\r\n", 0);
		#endif

		// generate turn on pulse

		digitalWrite(GSM_ON, HIGH);
		delay(1200);
		//sleep(2);
		digitalWrite(GSM_ON, LOW);
		delay(5000);
		//sleep(5);
	}
	else
	{
		#ifdef DEBUG_PRINT
			// parameter 0 - because module is off so it is not necessary 
			// to send finish AT<CR> here
			DebugPrint("DEBUG: GSM module is on\r\n", 0);
		#endif
	}

	if (AT_RESP_ERR_DIF_RESP == SendATCmdWaitResp("AT", 500, 100, "OK", 5)) 
	{		//check OK

		#ifdef DEBUG_PRINT
			// parameter 0 - because module is off so it is not necessary 
			// to send finish AT<CR> here
			DebugPrint("DEBUG: the baud is not ok\r\n", 0);
		#endif
			  //SendATCmdWaitResp("AT+IPR=9600", 500, 50, "OK", 5);
		
		for (i=1;i<7;i++)
		{
			switch (i) 
			{
				case 1:
					serialBegin(4800);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 4800\r\n", 0);
					#endif
				break;
				case 2:
					serialBegin(9600);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 9600\r\n", 0);
					#endif
				break;
				case 3:
					serialBegin(19200);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 19200\r\n", 0);
					#endif
				break;
				case 4:
					serialBegin(38400);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 38400\r\n", 0);
					#endif
				break;
				case 5:
					serialBegin(57600);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 57600\r\n", 0);
					#endif
				break;
				case 6:
					serialBegin(115200);
					#ifdef DEBUG_PRINT
						DebugPrint("DEBUG: provo Baud 115200\r\n", 0);
					#endif
				break;
				// if nothing else matches, do the default
				// default is optional
			}
			/*
			p_char = strchr((char *)(comm_buf),',');
			p_char1 = p_char+2; // we are on the first phone number character
			p_char = strchr((char *)(p_char1),'"');
			if (p_char != NULL) {
			*p_char = 0; // end of string
			strcpy(phone_number, (char *)(p_char1));
			}
			*/  

			delay(100);
			/*sprintf (buff,"AT+IPR=%f",baud_rate);
				#ifdef DEBUG_PRINT
					// parameter 0 - because module is off so it is not necessary 
					// to send finish AT<CR> here
					DebugPrint("DEBUG: Stringa ", 0);
					DebugPrint(buff, 0);
				#endif
				*/
			serialPuts("AT+IPR=");
			//serialPuts(baud_rate);
			serialPuts(ChangeIToS(baud_rate));
			serialPuts("\r"); // send <CR>
			delay(500);
			serialBegin(baud_rate);
			delay(100);
			if (AT_RESP_OK == SendATCmdWaitResp("AT", 500, 100, "OK", 5))
			{
					#ifdef DEBUG_PRINT
						// parameter 0 - because module is off so it is not necessary
						// to send finish AT<CR> here
						DebugPrint("DEBUG: ricevuto ok da modulo, baud impostato: ", 0);
						DebugPrintD(baud_rate, 0);
					#endif
					break;
			}

		}

			// communication line is not used yet = free
			SetCommLineStatus(CLS_FREE);
			// pointer is initialized to the first item of comm. buffer
			p_comm_buf = &comm_buf[0];

		if (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp("AT", 500, 50, "OK", 5)) 
		{
			#ifdef DEBUG_PRINT
				// parameter 0 - because module is off so it is not necessary 
				// to send finish AT<CR> here
				DebugPrint("DEBUG: No answer from the module\r\n", 0);
			#endif
		}
		else
		{

			#ifdef DEBUG_PRINT
				// parameter 0 - because module is off so it is not necessary 
				// to send finish AT<CR> here
				DebugPrint("DEBUG: 1 baud ok\r\n", 0);
			#endif
		}


	}
	else
	{
		#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: 2 GSM module is on and baud is ok\r\n", 0);
		#endif

	}

  SetCommLineStatus(CLS_FREE);
  // send collection of first initialization parameters for the GSM module    
  InitParam(PARAM_SET_0);
}

/**********************************************************
  Sends parameters for initialization of GSM module

  group:  0 - parameters of group 0 - not necessary to be registered in the GSM
          1 - parameters of group 1 - it is necessary to be registered
**********************************************************/
void InitParam(byte group)
{

  switch (group) 
  {
    case PARAM_SET_0:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;

	  	#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: configure the module PARAM_SET_0\r\n", 0);
		#endif
      SetCommLineStatus(CLS_ATCMD);

      // Reset to the factory settings
      SendATCmdWaitResp("AT&F", 1000, 50, "OK", 5);
      // switch off echo
      //SendATCmdWaitResp("ATE0", 500, 50, "OK", 5);
      // setup fixed baud rate
      //SendATCmdWaitResp("AT+IPR=9600", 500, 50, "OK", 5);
      // setup mode
      //SendATCmdWaitResp("AT#SELINT=1", 500, 50, "OK", 5);
      // Switch ON User LED - just as signalization we are here
      //SendATCmdWaitResp("AT#GPIO=8,1,1", 500, 50, "OK", 5);
      // Sets GPIO9 as an input = user button
      //SendATCmdWaitResp("AT#GPIO=9,0,0", 500, 50, "OK", 5);
      // allow audio amplifier control
      //SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 50, "OK", 5);
      // Switch OFF User LED- just as signalization we are finished
      //SendATCmdWaitResp("AT#GPIO=8,0,1", 500, 50, "OK", 5);
      SetCommLineStatus(CLS_FREE);
      break;

    case PARAM_SET_1:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;

	  	#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: configure the module PARAM_SET_1\r\n", 0);
		#endif
      SetCommLineStatus(CLS_ATCMD);

      // Request calling line identification
      SendATCmdWaitResp("AT+CLIP=1", 500, 50, "OK", 5);
      // Mobile Equipment Error Code
      SendATCmdWaitResp("AT+CMEE=0", 500, 50, "OK", 5);
      // Echo canceller enabled 
      //SendATCmdWaitResp("AT#SHFEC=1", 500, 50, "OK", 5);
      // Ringer tone select (0 to 32)
      //SendATCmdWaitResp("AT#SRS=26,0", 500, 50, "OK", 5);
      // Microphone gain (0 to 7) - response here sometimes takes
      // more than 500msec. so 1000msec. is more safety
      //SendATCmdWaitResp("AT#HFMICG=7", 1000, 50, "OK", 5);
      // set the SMS mode to text
      SendATCmdWaitResp("AT+CMGF=1", 500, 50, "OK", 5);
      // Auto answer after first ring enabled
      // auto answer is not used
      //SendATCmdWaitResp("ATS0=1", 500, 50, "OK", 5);

      // select ringer path to handsfree
      //SendATCmdWaitResp("AT#SRP=1", 500, 50, "OK", 5);
      // select ringer sound level
      //SendATCmdWaitResp("AT+CRSL=2", 500, 50, "OK", 5);
      // we must release comm line because SetSpeakerVolume()
      // checks comm line if it is free
      SetCommLineStatus(CLS_FREE);
      // select speaker volume (0 to 14)
      //SetSpeakerVolume(9);
      // init SMS storage
      InitSMSMemory();
      // select phonebook memory storage
      SendATCmdWaitResp("AT+CPBS=\"SM\"", 1000, 50, "OK", 5);
      break;
  }

}
/**********************************************************
Method initializes memory for the incoming SMS in the Telit
module - SMSs will be stored in the SIM card

!!This function is used internally after first registration
so it is not necessary to used it in the user sketch

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free

        OK ret val:
        -----------
        0 - SMS memory was not initialized
        1 - SMS memory was initialized

**********************************************************/
char InitSMSMemory(void) 
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not initialized yet

  // Disable messages about new SMS from the GSM module 
  SendATCmdWaitResp("AT+CNMI=2,0", 1000, 50, "OK", 2);

  // send AT command to init memory for SMS in the SIM card
  // response:
  // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
  if (AT_RESP_OK == SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", 2000, 2000, "+CPMS:", 10)) 
  {
    ret_val = 1;
  }
  else ret_val = 0;

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Initializes receiving process

start_comm_tmout    - maximum waiting time for receiving the first response
character (in msec.)
max_interchar_tmout - maximum tmout between incoming characters
in msec.
if there is no other incoming character longer then specified
tmout(in msec) receiving process is considered as finished
**********************************************************/
void RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
	rx_state = RX_NOT_STARTED;
	start_reception_tmout = start_comm_tmout;
	interchar_tmout = max_interchar_tmout;
	prev_time = millis();
	comm_buf[0] = 0x00; 		// end of string
	p_comm_buf = &comm_buf[0];
	comm_buf_len = 0;
//	serialFlush(); 		// erase rx circular buffer
}
/**********************************************************
Method checks if receiving process is finished or not.
Rx process is finished if defined inter-character tmout is reached

returns:
RX_NOT_FINISHED = 0,// not finished yet
RX_FINISHED,        // finished - inter-character tmout occurred
RX_TMOUT_ERR,       // initial communication tmout occurred
**********************************************************/
byte IsRxFinished(void)
{
	byte num_of_bytes;
	byte ret_val = RX_NOT_FINISHED;  // default not finished
	int i,j;
	// Rx state machine
	// ----------------

if (rx_state == RX_NOT_STARTED)
{
	// Reception is not started yet - check tmout
	if (!serialDataAvail())
	{
	// still no character received => check timeout
	/*
	#ifdef DEBUG_GSMRX

	DebugPrint("\r\nDEBUG: reception timeout", 0);
	Serial.print((unsigned long)(millis() - prev_time));
	DebugPrint("\r\nDEBUG: start_reception_tmout\r\n", 0);
	Serial.print(start_reception_tmout);


	#endif
	*/
		if ((unsigned long)(millis() - prev_time) >= start_reception_tmout)
		{
			// timeout elapsed => GSM module didn't start with response
			// so communication is takes as finished
			/*
			#ifdef DEBUG_GSMRX
			DebugPrint("\r\nDEBUG: RECEPTION TIMEOUT", 0);
			#endif
			*/
			comm_buf[comm_buf_len] = 0x00;
			ret_val = RX_TMOUT_ERR;
		}
	}
	else
	{
		// at least one character received => so init inter-character
		// counting process again and go to the next state
		prev_time = millis(); // init tmout for inter-character space
		rx_state = RX_ALREADY_STARTED;
	}
}

	if (rx_state == RX_ALREADY_STARTED)
	{
		// Reception already started
		// check new received bytes
		// only in case we have place in the buffer
		num_of_bytes = serialDataAvail();
		// if there are some received bytes postpone the timeout
		if (num_of_bytes) prev_time = millis();

		// read all received bytes
		//printf("%d\n",num_of_bytes);

		while (num_of_bytes)
		{
			//printf("%d\n",num_of_bytes);
			num_of_bytes--;
			//if (comm_buf_len < COMM_BUF_LEN)
			//{
				// we have still place in the GSM internal comm. buffer =>
				// move available bytes from circular buffer
				// to the rx buffer
				// printf("understand\n");
				*p_comm_buf = serialGetchar();

				p_comm_buf++;
				comm_buf_len++;
				comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
				// so after each character we have
				// valid string finished by the 0x00
			//}
			//else
			//{
				// comm buffer is full, other incoming characters
				// will be discarded
				// but despite of we have no place for other characters
				// we still must to wait until
				// inter-character tmout is reached

				// so just readout character from circular RS232 buffer
				// to find out when communication id finished(no more characters
				// are received in inter-char timeout)
				//*p_comm_buf = serialGetchar();
			//}
		}

		// finally check the inter-character timeout
		/*
		#ifdef DEBUG_GSMRX

		DebugPrint("\r\nDEBUG: intercharacter", 0);
		Serial.print((unsigned long)(millis() - prev_time));
		DebugPrint("\r\nDEBUG: interchar_tmout\r\n", 0);
		Serial.print(interchar_tmout);


		#endif
		*/
		if ((unsigned long)(millis() - prev_time) >= interchar_tmout)
		{
			// timeout between received character was reached
			// reception is finished
			// ---------------------------------------------

			/*
			#ifdef DEBUG_GSMRX

			DebugPrint("\r\nDEBUG: OVER INTER TIMEOUT", 0);
			#endif
			*/
			comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
			// but it is not necessary
			ret_val = RX_FINISHED;
		}
	}

	#ifdef DEBUG_GSMRX
	if (ret_val == RX_FINISHED)
	{
		DebugPrint("DEBUG: Received string\r\n", 0);
		for (i=0; i<comm_buf_len; i++)
		{
			serialPuts(byte(comm_buf[i]));
		}
	}
	#endif

return (ret_val);
}
/**********************************************************
Method checks received bytes

compare_string - pointer to the string which should be find

return: 0 - string was NOT received
1 - string was received
**********************************************************/
byte IsStringReceived(char const *compare_string)
{
	int i;
	char *ch;
	byte ret_val = 0;

	if(comm_buf_len)
	{

/*	#ifdef DEBUG_GSMRX
	DebugPrint("DEBUG: Compare the string: \r\n", 0);
	for (i=0; i<comm_buf_len; i++)
	{
	printf("%c",comm_buf[i]);
	}

	DebugPrint("\r\nDEBUG: with the string: \r\n", 0);
	printf(compare_string);
	DebugPrint("\r\n", 0);
	#endif
*/
	ch = strstr((char *)comm_buf, compare_string);
/*
	for(i=0;i<200;i++)
	{
		printf("%c",comm_buf[i]);
	}
	printf("\n");
	printf("the compare string is :%s\n",compare_string);
*/
		if (ch != NULL)
		{
			ret_val = 1;
			#ifdef DEBUG_PRINT
			DebugPrint("\r\nDEBUG: expected string was received\r\n", 0);
			#endif

		}
		else
		{
			#ifdef DEBUG_PRINT
			//printf("%s\n",*ch);
			DebugPrint("\r\nDEBUG: expected string was NOT received\r\n", 0);
			#endif

		}
	}

return (ret_val);
}

/**********************************************************
Method waits for response

start_comm_tmout    - maximum waiting time for receiving the first response
character (in msec.)
max_interchar_tmout - maximum tmout between incoming characters
in msec.
return:
RX_FINISHED         finished, some character was received

RX_TMOUT_ERR        finished, no character received
initial communication tmout occurred
**********************************************************/
byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
	byte status;

	RxInit(start_comm_tmout, max_interchar_tmout);
	// wait until response is not finished
	do
	{
	status = IsRxFinished();
	}
	while (status == RX_NOT_FINISHED);
	return (status);
}

/**********************************************************
Method waits for response with specific response string

start_comm_tmout    - maximum waiting time for receiving the first response
character (in msec.)
max_interchar_tmout - maximum tmout between incoming characters
in msec.
expected_resp_string - expected string
return:
RX_FINISHED_STR_RECV,     finished and expected string received
RX_FINISHED_STR_NOT_RECV  finished, but expected string not received
RX_TMOUT_ERR              finished, no character received
initial communication tmout occurred
**********************************************************/
byte WaitRespAdd(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
char const *expected_resp_string)
{
byte status;
byte ret_val;

RxInit(start_comm_tmout, max_interchar_tmout);
// wait until response is not finished
do
{
status = IsRxFinished();
} while (status == RX_NOT_FINISHED);

	if (status == RX_FINISHED)
	{
	// something was received but what was received?
	// ---------------------------------------------

		if(IsStringReceived(expected_resp_string))
		{
		// expected string was received
		// ----------------------------
		ret_val = RX_FINISHED_STR_RECV;
		}
		else ret_val = RX_FINISHED_STR_NOT_RECV;
	}
	else
	{
		// nothing was received
		// --------------------
		ret_val = RX_TMOUT_ERR;
	}
return (ret_val);
}


/**********************************************************
Method sends AT command and waits for response

return:
AT_RESP_ERR_NO_RESP = -1,   // no response received
AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
AT_RESP_OK = 1,             // response_string was included in the response
**********************************************************/
char SendATCmdWaitResp(char *AT_cmd_string,uint16_t start_comm_tmout, uint16_t max_interchar_tmout,char const *response_string,byte no_of_attempts)
{
	byte status;

	char ret_val = AT_RESP_ERR_NO_RESP;
	byte i;

	for (i = 0; i < no_of_attempts; i++)
	{
	// delay 500 msec. before sending next repeated AT command
	// so if we have no_of_attempts=1 tmout will not occurred
		if (i > 0) delay(500);
		write(gFD,AT_cmd_string,strlen(AT_cmd_string));
		serialPuts("\r");

		status = WaitResp(start_comm_tmout, max_interchar_tmout);

		if (status == RX_FINISHED)
		{
		// something was received but what was received?
		// ---------------------------------------------
			if(IsStringReceived(response_string))
			{
				ret_val = AT_RESP_OK;
				break;  // response is OK => finish
			}
			else ret_val = AT_RESP_ERR_DIF_RESP;
		}
		else
		{
		// nothing was received
		// --------------------
		ret_val = AT_RESP_ERR_NO_RESP;
		}

	}


return (ret_val);
}

/*int millis(void)
{
	int i;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	i = (int)(tv.tv_usec);
	return i;
}*/
/**********************************************************
Method picks up an incoming call

return:
**********************************************************/
void PickUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  serialPuts("ATA");
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method hangs up incoming or active call

return:
**********************************************************/
void HangUp(void)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  serialPuts("ATH");
  SetCommLineStatus(CLS_FREE);
}
/**********************************************************
Method checks status of call

return:
      CALL_NONE         - no call activity
      CALL_INCOM_VOICE  - incoming voice
      CALL_ACTIVE_VOICE - active voice
      CALL_NO_RESPONSE  - no response to the AT command 
      CALL_COMM_LINE_BUSY - comm line is not free
**********************************************************/
byte CallStatus(void)
{
  byte ret_val = CALL_NONE;

  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  serialPuts("AT+CPAS");

  // 5 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(5000, 50)) 
  {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }
  else
  {
    // something was received but what was received?
    // ---------------------------------------------
    // ready (device allows commands from TA/TE)
    // <CR><LF>+CPAS: 0<CR><LF> <CR><LF>OK<CR><LF>
    // unavailable (device does not allow commands from TA/TE)
    // <CR><LF>+CPAS: 1<CR><LF> <CR><LF>OK<CR><LF> 
    // unknown (device is not guaranteed to respond to instructions)
    // <CR><LF>+CPAS: 2<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // ringing
    // <CR><LF>+CPAS: 3<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
    // call in progress
    // <CR><LF>+CPAS: 4<CR><LF> <CR><LF>OK<CR><LF> - NO CALL
//    if(IsStringReceived("0"))
//    {
      // ready - there is no call
      // ------------------------
//      ret_val = CALL_NONE;
	//printf("call phone number is 0\n");
//    }
    if(IsStringReceived("3"))
    {
      // incoming call
      // --------------
      ret_val = CALL_INCOM_VOICE;
	//printf("call phone number is 3\n");
    }
    else if(IsStringReceived("4"))
    {
      // active call
      // -----------
      ret_val = CALL_ACTIVE_VOICE;
	//printf("call phone number is 4\n");
    }
    	else if(IsStringReceived("0"))
	{
	ret_val=CALL_NONE;
	}
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);

}

/*
* serialOpen:
*	Open and initialise the serial port, setting all the right
*	port parameters - or as many as are required - hopefully!
*********************************************************************************
*/

int serialOpen (char *device, int baud)
{
struct termios options ;
speed_t myBaud ;
int     status, fd ;

	switch (baud)
	{
	case     50:	myBaud =     B50 ; break ;
	case     75:	myBaud =     B75 ; break ;
	case    110:	myBaud =    B110 ; break ;
	case    134:	myBaud =    B134 ; break ;
	case    150:	myBaud =    B150 ; break ;
	case    200:	myBaud =    B200 ; break ;
	case    300:	myBaud =    B300 ; break ;
	case    600:	myBaud =    B600 ; break ;
	case   1200:	myBaud =   B1200 ; break ;
	case   1800:	myBaud =   B1800 ; break ;
	case   2400:	myBaud =   B2400 ; break ;
	case   9600:	myBaud =   B9600 ; break ;
	case  19200:	myBaud =  B19200 ; break ;
	case  38400:	myBaud =  B38400 ; break ;
	case  57600:	myBaud =  B57600 ; break ;
	case 115200:	myBaud = B115200 ; break ;
	case 230400:	myBaud = B230400 ; break ;

	default:
	return -2 ;
	}

if ((fd = open (device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
	return -1 ;

	fcntl (fd, F_SETFL, O_RDWR) ;
	tcgetattr (fd, &options) ;
	cfmakeraw   (&options) ;
	cfsetispeed (&options, myBaud) ;
	cfsetospeed (&options, myBaud) ;

	options.c_cflag |= (CLOCAL | CREAD) ;
	options.c_cflag &= ~PARENB ;
	options.c_cflag &= ~CSTOPB ;
	options.c_cflag &= ~CSIZE ;
	options.c_cflag |= CS8 ;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
	options.c_oflag &= ~OPOST ;

	options.c_cc [VMIN]  =   0 ;
	options.c_cc [VTIME] = 100 ;

	tcsetattr (fd, TCSANOW | TCSAFLUSH, &options) ;
	ioctl (fd, TIOCMGET, &status);
	status |= TIOCM_DTR ;
	status |= TIOCM_RTS ;
	ioctl (fd, TIOCMSET, &status);
	usleep (10000);

return fd ;
}

void serialFlush (void)
{
	tcflush (gFD, TCIOFLUSH) ;
}

void serialClose (void)
{
	close (gFD) ;
}

void serialPutchar (unsigned char c)
{
	write (gFD, &c, 1) ;
}

void serialPuts (char *s)
{
	char *i= "\r";

	write(gFD,s,strlen(s));
	write(gFD,i,strlen(i));
}
char *ChangeIToS(int IntNU)
{
	char *String;
	
	sprintf(String,"%d",IntNU);

	return String;

}
void serialPrintf (char *message, ...)
{
	va_list argp ;
	char buffer [1024] ;

	va_start (argp, message) ;
	vsnprintf (buffer, 1023, message, argp) ;
	va_end (argp) ;

	serialPuts(buffer) ;
}

int serialDataAvail (void)
{
	int result ;

	if (ioctl (gFD, FIONREAD, &result) == -1)
	return -1 ;

	return result ;
}

int serialGetchar (void)
{
	uint8_t x;

	if (read (gFD, &x, 1) != 1)
	return -1 ;
	return ((int)x) & 0xFF;
}


void serialBegin(int baud)
{
	wiringPiSetup();
	gFD = serialOpen("/dev/ttyAMA0",baud);
}

void SetCommLineStatus(byte new_status)
{
comm_line_status = new_status;
}

byte GetCommLineStatus(void)
{
return comm_line_status;
}
/**********************************************************
Function to enable or disable echo
Echo(1)   enable echo mode
Echo(0)   disable echo mode
**********************************************************/

void Echo(byte state)
{
	if (state == 0 || state == 1)
	{
	  SetCommLineStatus(CLS_ATCMD);
	  #ifdef DEBUG_PRINT
	  DebugPrint("DEBUG Echo\r\n",1);
	  #endif
	  serialPuts("ATE");
	  switch(state)
	  {
		case 0:
		serialPuts("0");
		break;
		case 1:
		serialPuts("1");
		break;
		default:
		break;
	  }
	  //serialPuts((int)state);    
	  serialPuts("\r");
	  delay(500);
	  SetCommLineStatus(CLS_FREE);
	}
}
int LibVer(void)
{
  return (GSM_LIB_VERSION);
}
/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  serialPuts("AT+CREG?");
  // 5 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  status = WaitResp(5000, 50); 

  if (status == RX_FINISHED) 
  {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) 
	  {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) 
	  {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else 
	{
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else 
  {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
 

  return (ret_val);
}
byte IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}
byte IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}

/*Method sends SMS*/
/**********************************************************
Method sends SMS

number_str:   pointer to the phone number string
message_str:  pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS("00XXXYYYYYYYYY", "SMS text");
**********************************************************/
char SendSMS(char *number_str, char *message_str) 
{
  char ret_val = -1;
  byte i;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);  
  ret_val = 0; // still not send
  // try to send SMS 3 times in case there is some problem
  for (i = 0; i < 3; i++) 
  {
    // send  AT+CMGS="number_str"
    serialPuts("AT+CMGS=\"");
    serialPuts(number_str);
    serialPuts("\"\r");

    // 1000 msec. for initial comm tmout
    // 50 msec. for inter character timeout
    if (RX_FINISHED_STR_RECV == WaitRespAdd(1000, 50, ">")) 
	{
      // send SMS text
      serialPuts(message_str);
/*
//#ifdef DEBUG_SMS_ENABLED
	// SMS will not be sent = we will not pay => good for debugging
      serialPuts(ChangeIToS(0x1b));
      if (RX_FINISHED_STR_RECV == WaitRespAdd(7000, 50, "OK")) 
	  {
//#else 
      serialPuts(ChangeIToS(0x1a));
	  //mySerial.flush(); // erase rx circular buffer
      if (RX_FINISHED_STR_RECV == WaitRespAdd(7000, 5000, "+CMGS")) 
	  {
//#endif
        // SMS was send correctly 
        ret_val = 1;
		#ifdef DEBUG_PRINT
			//DebugPrint("SMS was send correctly \r\n", 0);
		#endif
        break;
      }
      else continue;

      }
	  else 
	  {
	  // try again
	  continue;
	  }
*/    }
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method sends SMS to the specified SIM phonebook position

sim_phonebook_position:   SIM phonebook position <1..20>
message_str:              pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS(1, "SMS text");
**********************************************************/
char SendSMSSpecified(byte sim_phonebook_position, char *message_str) 
{
  char ret_val = -1;
  char sim_phone_number[20];

  ret_val = 0; // SMS is not send yet
  if (sim_phonebook_position == 0) return (-3);
  if (1 == GetPhoneNumber(sim_phonebook_position, sim_phone_number)) 
  {
    // there is a valid number at the spec. SIM position
    // => send SMS
    // -------------------------------------------------
    ret_val = SendSMS(sim_phone_number, message_str);
  }
  return (ret_val);

}
/**********************************************************
Method finds out if there is present at least one SMS with
specified status

Note:
if there is new SMS before IsSMSPresent() is executed
this SMS has a status UNREAD and then
after calling IsSMSPresent() method status of SMS
is automatically changed to READ

required_status:  SMS_UNREAD  - new SMS - not read yet
                  SMS_READ    - already read SMS                  
                  SMS_ALL     - all stored SMS

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout

        OK ret val:
        -----------
        0 - there is no SMS with specified status
        1..20 - position where SMS is stored 
                (suitable for the function GetGSM())


an example of use:
        GSM gsm;
        char position;  
        char phone_number[20]; // array for the phone number string
        char sms_text[100];

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // read new SMS
          gsm.GetGSM(position, phone_num, sms_text, 100);
          // now we have phone number string in phone_num
          // and SMS text in sms_text
        }
**********************************************************/
char IsSMSPresent(byte required_status) 
{
  char ret_val = -1;
  char *p_char;
  byte status;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not present

  switch (required_status) 
  {
    case SMS_UNREAD:
      serialPuts("AT+CMGL=\"REC UNREAD\"\r");
      break;
    case SMS_READ:
      serialPuts("AT+CMGL=\"REC READ\"\r");
      break;
    case SMS_ALL:
      serialPuts("AT+CMGL=\"ALL\"\r");
      break;
  }

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(5000, 1500); 
  // wait response is finished
  do {
    if (IsStringReceived("OK"))
	{ 
      // perfect - we have some response, but what:

      // there is either NO SMS:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 SMS
      // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
      // <CR><LF> <data> <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  


  switch (status) 
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED:
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived("+CMGL:"))
	  { 
        // there is some SMS with status => get its position
        // response is:
        // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
        // <CR><LF> <data> <CR><LF>OK<CR><LF>
        p_char = strchr((char *)comm_buf,':');
        if (p_char != NULL) 
		{
          ret_val = atoi(p_char+1);
        }
      }
      else 
	  {
        // other response like OK or ERROR
        ret_val = 0;
      }

      // here we have WaitResp() just for generation tmout 20msec. in case OK was detected
      // not due to receiving
      WaitResp(20, 20); 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads SMS from specified memory(SIM) position

position:     SMS position <1..20>
phone_number: a pointer where the phone number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding also string terminating 0x00 character
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS       - no SMS was not found at the specified position
        GETSMS_UNREAD_SMS   - new SMS was found at the specified position
        GETSMS_READ_SMS     - already read SMS was found at the specified position
        GETSMS_OTHER_SMS    - other type of SMS was found 


an example of usage:
        GSM gsm;
        char position;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // there is new SMS => read it
          gsm.GetGSM(position, phone_num, sms_text, 100);
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }        
**********************************************************/
char GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len) 
{
  char ret_val = -1;
  char *p_char; 
  char *p_char1;
  byte len;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  phone_number[0] = 0;  // end of string for now
  ret_val = GETSMS_NO_SMS; // still no SMS
  
  //send "AT+CMGR=X" - where X = position
  serialPuts("AT+CMGR=");
  serialPuts(ChangeIToS((int)position));  
  serialPuts("\r");

  // 5000 msec. for initial comm tmout
  // 100 msec. for inter character tmout
  switch (WaitRespAdd(5000, 100, "+CMGR")) 
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // OK was received => there is NO SMS stored in this position
      if(IsStringReceived("OK")) 
	  {
        // there is only response <CR><LF>OK<CR><LF> 
        // => there is NO SMS
        ret_val = GETSMS_NO_SMS;
      }
      else if(IsStringReceived("ERROR")) 
	  {
        // error should not be here but for sure
        ret_val = GETSMS_NO_SMS;
      }
      break;

    case RX_FINISHED_STR_RECV:
      // find out what was received exactly

      //response for new SMS:
      //<CR><LF>+CMGR: "REC UNREAD","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>OK<CR><LF>
      if(IsStringReceived("\"REC UNREAD\"")) 
	  { 
        // get phone number of received SMS: parse phone number string 
        // +XXXXXXXXXXXX
        // -------------------------------------------------------
        ret_val = GETSMS_UNREAD_SMS;
      }
      //response for already read SMS = old SMS:
      //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>
      else if(IsStringReceived("\"REC READ\"")) 
	  {
        // get phone number of received SMS
        // --------------------------------
        ret_val = GETSMS_READ_SMS;
      }
      else {
        // other type like stored for sending.. 
        ret_val = GETSMS_OTHER_SMS;
      }

      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),',');
      p_char1 = p_char+2; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) 
	  {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }


      // get SMS text and copy this text to the SMS_text buffer
      // ------------------------------------------------------
      p_char = strchr(p_char+1, 0x0a);  // find <LF>
      if (p_char != NULL) 
	  {
        // next character after <LF> is the first SMS character
        p_char++; // now we are on the first SMS character 

        // find <CR> as the end of SMS string
        p_char1 = strchr((char *)(p_char), 0x0d);  
        if (p_char1 != NULL) 
		{
          // finish the SMS text string 
          // because string must be finished for right behaviour 
          // of next strcpy() function
          *p_char1 = 0; 
        }
        // in case there is not finish sequence <CR><LF> because the SMS is
        // too long (more then 130 characters) sms text is finished by the 0x00
        // directly in the WaitResp() routine

        // find out length of the SMS (excluding 0x00 termination character)
        len = strlen(p_char);

        if (len < max_SMS_len) 
		{
          // buffer SMS_text has enough place for copying all SMS text
          // so copy whole SMS text
          // from the beginning of the text(=p_char position) 
          // to the end of the string(= p_char1 position)
          strcpy(SMS_text, (char *)(p_char));
        }
        else 
		{
          // buffer SMS_text doesn't have enough place for copying all SMS text
          // so cut SMS text to the (max_SMS_len-1)
          // (max_SMS_len-1) because we need 1 position for the 0x00 as finish 
          // string character
          memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
          SMS_text[max_SMS_len] = 0; // finish string
        }
      }
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads SMS from specified memory(SIM) position and
makes authorization - it means SMS phone number is compared
with specified SIM phonebook position(s) and in case numbers
match GETSMS_AUTH_SMS is returned, otherwise GETSMS_NOT_AUTH_SMS
is returned

position:     SMS position to be read <1..20>
phone_number: a pointer where the tel. number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding terminating 0x00 character

first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received SMS phone number is NOT authorized at all, so every
                      SMS is considered as authorized (GETSMS_AUTH_SMS is returned)
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS           - no SMS was found at the specified position
        GETSMS_NOT_AUTH_SMS     - NOT authorized SMS found at the specified position
        GETSMS_AUTH_SMS         - authorized SMS found at the specified position


an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        // authorize SMS with SIM phonebook positions 1..3
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 1, 3)) {
          // new authorized SMS was detected at the SMS position 1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }

        // don't authorize SMS with SIM phonebook at all
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 0, 0)) {
          // new SMS was detected at the SMS position 1
          // because authorization was not required
          // SMS is considered authorized
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }
**********************************************************/
char GetAuthorizedSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
                           byte first_authorized_pos, byte last_authorized_pos)
{
  char ret_val = -1;
  byte i;

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG GetAuthorizedSMS\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrintD(position, 0);
    DebugPrint("      #5: ", 0);
    DebugPrintD(first_authorized_pos, 0);
    DebugPrint("      #6: ", 0);
    DebugPrintD(last_authorized_pos, 1);
#endif  

  ret_val = GetSMS(position, phone_number, SMS_text, max_SMS_len);
  if (ret_val < 0) 
  {
    // here is ERROR return code => finish
    // -----------------------------------
  }
  else if (ret_val == GETSMS_NO_SMS) 
  {
    // no SMS detected => finish
    // -------------------------
  }
  else if (ret_val == GETSMS_READ_SMS)
  {
    // now SMS can has only READ attribute because we have already read
    // this SMS at least once by the previous function GetSMS()
    //
    // new READ SMS was detected on the specified SMS position =>
    // make authorization now
    // ---------------------------------------------------------
    if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) 
	{
      // authorization is not required => it means authorization is OK
      // -------------------------------------------------------------
      ret_val = GETSMS_AUTH_SMS;
    }
    else {
      ret_val = GETSMS_NOT_AUTH_SMS;  // authorization not valid yet
      for (i = first_authorized_pos; i <= last_authorized_pos; i++) 
	  {
        if (ComparePhoneNumber(i, phone_number)) 
		{
          // phone numbers are identical
          // authorization is OK
          // ---------------------------
          ret_val = GETSMS_AUTH_SMS;
          break;  // and finish authorization
        }
      }
    }
  }
  return (ret_val);
}
/**********************************************************
Method deletes SMS from the specified SMS position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - SMS was not deleted
        1 - SMS was deleted
**********************************************************/
char DeleteSMS(byte position) 
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not deleted yet
  
  //send "AT+CMGD=XY" - where XY = position
  serialPuts("AT+CMGD=");
  serialPuts(ChangeIToS((int)position));  
  serialPuts("\r");


  // 5000 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  switch (WaitRespAdd(5000, 50, "OK")) 
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // OK was received => SMS deleted
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR => SMS was not deleted
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
/**********************************************************
Method reads phone number string from specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string

        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)

an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string

        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1 
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/
char GetPhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  char *p_char; 
  char *p_char1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not found yet
  phone_number[0] = 0; // phone number not found yet => empty string
  
  //send "AT+CPBR=XY" - where XY = position
  serialPuts("AT+CPBR=");
  serialPuts(ChangeIToS((int)position));  
  serialPuts("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitRespAdd(5000, 50, "+CPBR")) 
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // response in case valid phone number stored:
      // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
      // <CR><LF>OK<CR><LF>

      // response in case there is not phone number:
      // <CR><LF>OK<CR><LF>
      p_char = strchr((char *)(comm_buf),'"');
      if (p_char != NULL) 
	  {
        p_char++;       // we are on the first phone number character
        // find out '"' as finish character of phone number string
        p_char1 = strchr((char *)(p_char),'"');
        if (p_char1 != NULL) 
		{
          *p_char1 = 0; // end of string
        }
        // extract phone number string
        strcpy(phone_number, (char *)(p_char));
        // output value = we have found out phone number string
        ret_val = 1;
      }
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method writes phone number string to the specified SIM position

position:     SMS position <1..20>
phone_number: phone number string for the writing

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not written
        1 - phone number was written
**********************************************************/
char WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  serialPuts("AT+CPBW=");
  serialPuts(ChangeIToS((int)position));  
  serialPuts(",\"");
  serialPuts(phone_number);
  serialPuts("\"\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitRespAdd(5000, 50, "OK")) 
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method del phone number from the specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not deleted
        1 - phone number was deleted
**********************************************************/
char DelPhoneNumber(byte position)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY
  // where XY = position
  serialPuts("AT+CPBW=");
  serialPuts(ChangeIToS((int)position));  
  serialPuts("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitRespAdd(5000, 50, "OK"))
  {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Function compares specified phone number string 
with phone number stored at the specified SIM position

position:       SMS position <1..20>
phone_number:   phone number string which should be compare

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same


an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char ComparePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;
  char sim_phone_number[20];

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrintD(position, 0);
    DebugPrint("      #2: ", 0);
    DebugPrint(phone_number, 1);
#endif


  ret_val = 0; // numbers are not the same so far
  if (position == 0) return (-3);
  if (1 == GetPhoneNumber(position, sim_phone_number)) 
  {
    // there is a valid number at the spec. SIM position
    // -------------------------------------------------
    if (0 == strcmp(phone_number, sim_phone_number)) 
	{
      // phone numbers are the same
      // --------------------------
#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber: Phone numbers are the same", 1);
#endif
      ret_val = 1;
    }
  }
  return (ret_val);
}
/**********************************************************
Method sets speaker volume

speaker_volume: volume in range 0..14

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char SetSpeakerVolume(byte speaker_volume)
{
  
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // remember set value as last value
  if (speaker_volume > 14) speaker_volume = 14;
  // select speaker volume (0 to 14)
  // AT+CLVL=X<CR>   X<0..14>
  serialPuts("AT+CLVL=");
  serialPuts(ChangeIToS((int)speaker_volume));    
  serialPuts("\r"); // send <CR>
  // 10 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(10000, 50)) 
  {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) 
	{
      last_speaker_volume = speaker_volume;
      ret_val = last_speaker_volume; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method increases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char IncSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value < 14) 
  {
    current_speaker_value++;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 14;

  return (ret_val);
}

/**********************************************************
Method decreases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
char DecSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value > 0) 
  {
    current_speaker_value--;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 0;

  return (ret_val);
}
/**********************************************************
Method sends DTMF signal
This function only works when call is in progress

dtmf_tone: tone to send 0..15

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0.. tone
**********************************************************/
char SendDTMFSignal(byte dtmf_tone)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // e.g. AT+VTS=5<CR>
  serialPuts("AT+VTS=");
  serialPuts(ChangeIToS((int)dtmf_tone));    
  serialPuts("\r");
  // 1 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(1000, 50)) 
  {
    ret_val = -2; // ERROR
  }
  else 
  {
    if(IsStringReceived("OK")) 
	{
      ret_val = dtmf_tone; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
byte IsUserButtonEnable(void) 
{
	return (module_status & STATUS_USER_BUTTON_ENABLE);
}
void DisableUserButton(void) 
{
	module_status &= ~STATUS_USER_BUTTON_ENABLE;
}
void EnableUserButton(void) 
{
	module_status |= STATUS_USER_BUTTON_ENABLE;
}
/**********************************************************
Method returns state of user button


return: 0 - not pushed = released
        1 - pushed
**********************************************************/
byte IsUserButtonPushed(void)
{
  byte ret_val = 0;
  if (CLS_FREE != GetCommLineStatus()) return(0);
  SetCommLineStatus(CLS_ATCMD);
}
/**********************************************************
Method calls the specific number

number_string: pointer to the phone number string 
               e.g. gsm.Call("+420123456789"); 

**********************************************************/
void CallS(char *number_string)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATDxxxxxx;<CR>
  serialPuts("ATD");
  serialPuts(number_string);    
  serialPuts(";\r");
  // 10 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  WaitResp(10000, 50);
  SetCommLineStatus(CLS_FREE);
}

/**********************************************************
Method calls the number stored at the specified SIM position

sim_position: position in the SIM <1...>
              e.g. gsm.Call(1);
**********************************************************/
void Call(int sim_position)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  // ATD>"SM" 1;<CR>
  serialPuts("ATD>\"SM\" ");
  serialPuts(ChangeIToS(sim_position));    
  serialPuts(";\r");

  // 10 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  WaitResp(10000, 50);

  SetCommLineStatus(CLS_FREE);
}
/**********************************************************
Method checks status of call(incoming or active) 
and makes authorization with specified SIM positions range

phone_number: a pointer where the tel. number string of current call will be placed
              so the space for the phone number string must be reserved - see example
first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received incoming phone number is NOT authorized at all, so every
                      incoming is considered as authorized (CALL_INCOM_VOICE_NOT_AUTH is returned)

return: 
      CALL_NONE                   - no call activity
      CALL_INCOM_VOICE_AUTH       - incoming voice - authorized
      CALL_INCOM_VOICE_NOT_AUTH   - incoming voice - not authorized
      CALL_ACTIVE_VOICE           - active voice
      CALL_INCOM_DATA_AUTH        - incoming data call - authorized
      CALL_INCOM_DATA_NOT_AUTH    - incoming data call - not authorized  
      CALL_ACTIVE_DATA            - active data call
      CALL_NO_RESPONSE            - no response to the AT command 
      CALL_COMM_LINE_BUSY         - comm line is not free
**********************************************************/
byte CallStatusWithAuth(char *phone_number,
                             byte first_authorized_pos, byte last_authorized_pos)
{
  byte ret_val = CALL_NONE;
  byte search_phone_num = 0;
  byte i;
  byte status;
  char *p_char; 
  char *p_char1;

  phone_number[0] = 0x00;  // no phonr number so far
  if (CLS_FREE != GetCommLineStatus()) return (CALL_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  serialPuts("AT+CLCC");

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(5000, 1500); 
  // wait response is finished
  do {
    if (IsStringReceived("OK\r\n")) 
	{ 
      // perfect - we have some response, but what:

      // there is either NO call:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 call
      // +CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145<CR><LF>
      // <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  // generate tmout 30msec. before next AT command
  delay(30);

  if (status == RX_FINISHED) 
  {
    // something was received but what was received?
    // example: //+CLCC: 1,1,4,0,0,"+420XXXXXXXXX",145
    // ---------------------------------------------
    if(IsStringReceived("+CLCC: 1,1,4,0,0")) 
	{ 
      // incoming VOICE call - not authorized so far
      // -------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_VOICE_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,1,4,1,0")) 
	{ 
      // incoming DATA call - not authorized so far
      // ------------------------------------------
      search_phone_num = 1;
      ret_val = CALL_INCOM_DATA_NOT_AUTH;
    }
    else if(IsStringReceived("+CLCC: 1,0,0,0,0")) 
	{ 
      // active VOICE call - GSM is caller
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,0,0"))
	{ 
      // active VOICE call - GSM is listener
      // -----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_VOICE;
    }
    else if(IsStringReceived("+CLCC: 1,1,0,1,0")) 
	{ 
      // active DATA call - GSM is listener
      // ----------------------------------
      search_phone_num = 1;
      ret_val = CALL_ACTIVE_DATA;
    }
    else if(IsStringReceived("+CLCC:"))
	{ 
      // other string is not important for us - e.g. GSM module activate call
      // etc.
      // IMPORTANT - each +CLCC:xx response has also at the end
      // string <CR><LF>OK<CR><LF>
      ret_val = CALL_OTHERS;
    }
    else if(IsStringReceived("OK"))
	{ 
      // only "OK" => there is NO call activity
      // --------------------------------------
      ret_val = CALL_NONE;
    }

    
    // now we will search phone num string
    if (search_phone_num) 
	{
      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),'"');
      p_char1 = p_char+1; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) 
	  {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }
      
      if ( (ret_val == CALL_INCOM_VOICE_NOT_AUTH) 
           || (ret_val == CALL_INCOM_DATA_NOT_AUTH)) 
		{

        if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) 
		{
          // authorization is not required => it means authorization is OK
          // -------------------------------------------------------------
          if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
          else ret_val = CALL_INCOM_DATA_AUTH;
        }
        else
		{
          // make authorization
          // ------------------
          SetCommLineStatus(CLS_FREE);
          for (i = first_authorized_pos; i <= last_authorized_pos; i++)
		  {
            if (ComparePhoneNumber(i, phone_number)) 
			{
              // phone numbers are identical
              // authorization is OK
              // ---------------------------
              if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
              else ret_val = CALL_INCOM_DATA_AUTH;
              break;  // and finish authorization
            }
          }
        }
      }
    }
    
  }
  else 
  {
    // nothing was received (RX_TMOUT_ERR)
    // -----------------------------------
    ret_val = CALL_NO_RESPONSE;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
