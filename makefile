gsm : GSMDemo.o GSM.o wiringPi.o piHiPri.o piThread.o  
	@cc -o GSMDemo GSMDemo.o GSM.o wiringPi.o piHiPri.o piThread.o -lpthread
GSMDemo.o : GSMDemo.c
	@cc -c GSMDemo.c
GSM.o : GSM.c GSM.h
	@cc -c GSM.c
wiringPi.o : wiringPi.c wiringPi.h
	@cc -c wiringPi.c
piHiPri.o : piHiPri.c 
	@cc -c piHiPri.c
piThread.o : piThread.c
	@cc -c piThread.c
clean : 
	@rm *.o GSMDemo
install : 
	@cc -shared -fpic -o libITEADGSM.so GSM.c wiringPi.c piHiPri.c piThread.c -lpthread
	@sudo mv libITEADGSM.so /usr/lib 
