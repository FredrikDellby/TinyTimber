#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include "player.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_CALLS 500
#define BUF_SIZE 30
#define BUF_INIT "000000000000000000000000000000"
#define MIN_INTER_ARRIVAL_TIME MSEC(200)

typedef struct {
    Object super;
	Player *player;
    char buf[BUF_SIZE];
	int count, buttonPressedFirstTime;
	Time lastTimePressed;
} App;

void setPlayer(App*, Player*);
void reader(App*, int);
void receiver(App*, int);
void buttonPressed(App*, int);

App app = {initObject(), NULL, BUF_INIT, 0, 0, MSEC(0)};
Player player = initPlayer();
ToneTask toneTask = initToneTask();
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
SysIO sysIO0 = initSysIO(SIO_PORT0, &app, buttonPressed);
Timer tim0 = initTimer();

void setPlayer(App *self, Player *player) {
	self->player = player;
}

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
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
	Time interArrivalTime;

	if (self->buttonPressedFirstTime == 0) {
		self->buttonPressedFirstTime = 1;
		self->lastTimePressed = CURRENT_OFFSET();
		T_RESET(&tim0);
	} else {
		interArrivalTime = T_SAMPLE(&tim0) + CURRENT_OFFSET() - self->lastTimePressed;
		if (interArrivalTime > MIN_INTER_ARRIVAL_TIME) {
			snprintf(self->buf, BUF_SIZE, "Inter-arrival time: %d ms\n", MSEC_OF(interArrivalTime));
			SCI_WRITE(&sci0, self->buf);
			self->lastTimePressed = T_SAMPLE(&tim0) + CURRENT_OFFSET();
		}
		
	}
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
	SIO_INIT(&sysIO0);
    SCI_WRITE(&sci0, "Hello, hello...\n");

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
