#include "GSM.h"

int i; int call; extern gFD;
int *j[30]; int ppp; char *k; int* m; int a; int b; int ver;int reg;

void main(void)
{
	serialBegin(9600);
	TurnOn(9600);          		//module power on
	InitParam(PARAM_SET_1);		//configure the module  
	Echo(1);               		//enable AT echo 

	while(1)
	{
		call=CallStatus();
		printf("call number: %d\n",call);
		switch (call)
		{
      			case CALL_NONE:
        		printf("no call\n");
       	 		break;
      			case CALL_INCOM_VOICE:
        		printf("incoming voice call\n");
       			delay(5000);
        		PickUp();
        		break;
      			case CALL_ACTIVE_VOICE:
        		printf("active voice call\n");
        		delay(5000);
        		HangUp();
        		break;
      			case CALL_NO_RESPONSE:
        		printf("no response\n");
        		break;
    		}

	delay(5000);
	}
	
}
