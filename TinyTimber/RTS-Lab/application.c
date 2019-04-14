
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include "player.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_CALLS 500
#define BUF_SIZE 30
#define BUF_INIT "000000000000000000000000000000"
#define MIN_INTER_ARRIVAL_TIME 150

typedef struct {
	Object super;
	CANMsg msgQueue[10];
	int first, last, size;
} CANBuffer;

void canBufferInit(CANBuffer*);
void runCANBufferTask(CANBuffer*, int);

typedef struct {
    Object super;
	Player *player;
    char buf[BUF_SIZE];
	int count, buttonPressedFirstTime, firstCANMsgReceived, counter, t1, t2, t3;
	uchar msgIndex;
	
} App;

void setPlayer(App*, Player*);
void reader(App*, int);
void receiver(App*, int);
void buttonPressed(App*, int);
void printCANMsg(App*, CANMsg*);

App app = {initObject(), NULL, BUF_INIT, 0, 0, 0, 0};
Player player = initPlayer();
ToneTask toneTask = initToneTask();
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
SysIO sysIO0 = initSysIO(SIO_PORT0, &app, buttonPressed);
Timer tim0 = initTimer();
Timer tim1 = initTimer();
CANBuffer canBuf;

// CANBuffer methods
void canBufferInit(CANBuffer *self) {
	self->first = 0;
	self->last = 0;
	self->size = 0;
}

void runCANBufferTask(CANBuffer *self, int unused) {
	CANMsg msg = self->msgQueue[self->first++];
	self->size--;
	printCANMsg(&app, &msg);
	
	if (self->size > 0) {
		SEND(MSEC(1000), MSEC(100), self, printCANMsg, unused);
	}
}

void setPlayer(App *self, Player *player) {
	self->player = player;
}

// App methods
void printCANMsg(App *self, CANMsg *msg) {
	snprintf(self->buf, BUF_SIZE, "Can msg ID: %d\n", msg->msgId);
	SCI_WRITE(&sci0, self->buf);
	snprintf(self->buf, BUF_SIZE, "Can msg text: %s\n", msg->buff);
	SCI_WRITE(&sci0, self->buf);
}

void receiver(App *self, int unused) {     
    CANMsg msg;      
	Time interArrivalTime;
	CAN_RECEIVE(&can0, &msg);
	if (self->firstCANMsgReceived == 0) {
		self->firstCANMsgReceived = 1;
		T_RESET(&tim1);
		printCANMsg(self, &msg);
	}
	else {
		interArrivalTime = T_SAMPLE(&tim1);
		if (canBuf.size > 0) {
			canBuf.msgQueue[++canBuf.last] = msg;
		} else if (interArrivalTime < MSEC(1000)) {
			canBuf.msgQueue[++canBuf.last] = msg;
			SEND(MSEC(1000) - interArrivalTime, MSEC(100), self, printCANMsg, unused);
		} else {
			printCANMsg(self, &msg);
		}
	}
}

void reader(App *self, int c) {
	int value;
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	switch (c) {
		case 'i':
			incVolume(&player);
			self->count = 0;
			break;
		case 'd':
			decVolume(&player);
			self->count = 0;
			break;
		case 'm':
			if (isMuted(&player)) {
				unmute(&player);
			} else {
				mute(&player);
			}
			self->count = 0;
			break;
		case 'e':
			self->buf[self->count] = '\0';
			self->count = 0;
			value = atoi(self->buf+1);
			if (self->buf[0] == 'k') {
				setKey(self->player, value);
			} else if (self->buf[0] == 't') {
				setTempo(self->player, value);
			}
			break;
		default:
			self->buf[self->count] = c;
			self->count++;
	}
}

void buttonPressed(App *self, int unused){
	int interArrivalTime = 0; 
	int tempo = 0;
	//sio_toggle(&sysIO0, 0);
	CANMsg msg;
	//char *ptr1 = self->buf, *ptr2 = msg.buff;
	if (self->buttonPressedFirstTime == 0) {
		self->buttonPressedFirstTime = 1;
		T_RESET(&tim0);
		
	} else {
	
		interArrivalTime = MSEC_OF(T_SAMPLE(&tim0));

		T_RESET(&tim0);
		if (interArrivalTime > MIN_INTER_ARRIVAL_TIME) {
		snprintf(self->buf, BUF_SIZE, "Counter: %d \n", self->counter);
				SCI_WRITE(&sci0, self->buf);
			if(self->counter == 0){
				self->t1 = interArrivalTime;
				snprintf(self->buf, BUF_SIZE, "T1: %d ms\n", self->t1);
				SCI_WRITE(&sci0, self->buf);
				self->counter++;
			}
			else if(self->counter == 1){
				int sum = self->t1 - interArrivalTime;
				snprintf(self->buf, BUF_SIZE, "sum: %d ms\n", sum);
				SCI_WRITE(&sci0, self->buf);
				if(self->t1 - interArrivalTime < 99){
					self->t2 = interArrivalTime;
					self->counter++;
					snprintf(self->buf, BUF_SIZE, "T2: %d ms\n", self->t2);
					SCI_WRITE(&sci0, self->buf);
				}
				else{
					self->counter = 0;
				}
			}
			else {
				if(self->t2 - interArrivalTime < 99){
					self->t3 = interArrivalTime;
					tempo = 60000/((self->t1 + self->t2 + self->t3)/3); 
					snprintf(self->buf, BUF_SIZE, "Tempo: %d BPM\n", tempo);
					SCI_WRITE(&sci0, self->buf);
					
					
				}
			self->counter = 0;
			
		}
	
		msg.msgId = self->msgIndex++;
		msg.nodeId = 1;
		snprintf(msg.buff, BUF_SIZE, "%d", interArrivalTime);
		/**
		while (*ptr1 != '\0') {
			SCI_WRITECHAR(&sci0, *ptr1);
			*ptr2++ = ptr1++;
		}
		SCI_WRITECHAR(&sci0, '\n');
		*/
		msg.length = strlen(msg.buff);
		CAN_SEND(&can0, &msg);
		}
	}
}

void startApp(App *self, int arg) {
    //CANMsg msg;
    CAN_INIT(&can0);
	canBufferInit(&canBuf);
    SCI_INIT(&sci0);
	SIO_INIT(&sysIO0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
	/**
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
	*/
	//ASYNC(&player, startPlayer, 0);
}

int main() {
	setPlayer(&app, &player);
	setToneTask(&player, &toneTask);
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
	INSTALL(&sysIO0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}