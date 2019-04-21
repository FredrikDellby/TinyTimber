#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdio.h>
#include "application.h"
#include "sioTinyTimber.h"
#include <stdlib.h>
#include <string.h>
#define BUF_SIZE 30

#define SLAVE 0
typedef struct {
	Object super;
	CANMsg msgQueue[10];
	char buf[BUF_SIZE];
	int first, last, size;
} CANBuffer;

Voice voice0 = {initObject(), 0, USEC(500), 7, 1, 1}; //defaults: wave low, 1kHz, 7 volume, deadline on, mute on
Controller controller = {initObject(), MSEC(500), 0, 0, 0, 1, SLAVE, 0, 0, 0, 0, 1, 31, 0, 0, 0, 0, 0, 0, 0, 0}; //defaults: Tempo: 120BPM, first note: 0, offset: 0, playing: FALSE, mute: TRUE, slave: FALSE
Serial sci0 = initSerial(SCI_PORT0, &controller, reader); //reader
Can can0 = initCan(CAN_PORT0, &controller, receiver);
SysIO sysIO0 = initSysIO(SIO_PORT0, &controller, buttonPressed);
Timer tim0 = initTimer();
Timer tim1 = initTimer();
CANBuffer canBuf;

void canBufferInit(CANBuffer*);
void runCANBufferTask(CANBuffer*, int);
//----------------------------------------------------MISC----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------CONTROLLER-------------------------------------------------------
#define MIN_INTER_ARRIVAL_TIME 150

// CANBuffer methods
void canBufferInit(CANBuffer *self) {
	self->first = 0;
	self->last = 0;
	self->size = 0;
}

void runCANBufferTask(CANBuffer *self, int unused) {
	SCI_WRITE(&sci0, "CANBufferTask...\n");
	CANMsg msg = self->msgQueue[(self->first)++];
	self->first %= 10;
	self->size--;
	printCANMsg(&controller, &msg);

	if (self->size > 0) {
		SEND(MSEC(1000), MSEC(100), self, runCANBufferTask, unused);
	}
}

void printCANMsg(Controller *self, CANMsg *msg) {
	snprintf(self->buf, BUF_SIZE, "Can msg ID: %d\n", msg->msgId);
	SCI_WRITE(&sci0, self->buf);
	snprintf(self->buf, BUF_SIZE, "Can msg text: %s\n", msg->buff);
	SCI_WRITE(&sci0, self->buf);
}

void receiver(Controller *self, int unused) {
	CANMsg msg;
	CAN_RECEIVE(&can0, &msg);
	Time interArrivalTime;
	CAN_RECEIVE(&can0, &msg);
	//if (self->firstCANMsgReceived == 0) {
		//self->firstCANMsgReceived = 1;
		//T_RESET(&tim1);
		printCANMsg(self, &msg);
	//}
	//else {
		//interArrivalTime = T_SAMPLE(&tim1);
		//T_RESET(&tim1);
		/*if(canBuf.size < 10){
			//if((canBuf.size > 0 ) )  {
				//canBuf.msgQueue[++canBuf.last] = msg;
				//canBuf.size++;

			} else if (interArrivalTime < MSEC(1000) ) {
				canBuf.msgQueue[++canBuf.last] = msg;
				canBuf.size++;
				
				//SEND(MSEC(1000) - interArrivalTime, MSEC(100), self, runCANBufferTask, unused);
			} else {*/
			//	printCANMsg(self, &msg);
			//}
		//}
	//}
	if (self->slave) {
		switch (msg.buff[0]) {
			case 'v':
				if (msg.buff[1] == 255) {
					ASYNC(&voice0, volumeDec, 1);
				} else if (msg.buff[1] == 254) {
					ASYNC(&voice0, volumeInc, 1);
				} else if (msg.buff[1] > 0 && msg.buff[1] <= VOLUME_MAX) {
					ASYNC(&voice0, volumeSet, msg.buff[1]);
				}
			break;
			
			case 't':
				if (msg.buff[1] >= MIN_BPM && msg.buff[1] <= MAX_BPM) ASYNC(self, tempoSet, msg.buff[1]);
			break;
			
			case 'o':
			case 'k':
				if (msg.buff[1] >= MIN_OFFSET+5 && msg.buff[1] <= MAX_OFFSET+5) ASYNC(self, offsetSet, msg.buff[1]-5);
			break;
			
			case 's':
				if (!msg.buff[1] || msg.buff[1] == self->slave) {
					ASYNC(self, startSong, 0);
				}
			break;
			
			case 'e':
				ASYNC(self, pause, 0);
				ASYNC(self, reset, 0); 
			break;
			
			case 'p':
				ASYNC(self, pause, 0);
			break;
			
			case 'a':
				msg.buff[0] = 'b';
				msg.buff[1] = self->slave;
				CAN_SEND(&can0, &msg);
			break;
			
			case 'm':
				if (msg.buff[1] == self->slave) {
					self->slave = 0; //Now I am the master
				}

			default: 
				SCI_WRITE(&sci0, "Invalid CAN command received!\n");
			break;
		}
		char tmp[30];
		
		sprintf(tmp, "Recieved message: %c \n", msg.buff[0]);
		SCI_WRITE(&sci0, tmp);
		sprintf(tmp, "Recieved value: %d \n", msg.buff[1]);
		SCI_WRITE(&sci0, tmp);
	} else {
		if (msg.buff[0] == 'b') {
			char tmp[20]; 
			sprintf(tmp, "%d", msg.buff[1]);
			SCI_WRITE(&sci0, "Device #");
			SCI_WRITE(&sci0, tmp);
			SCI_WRITE(&sci0, " has responded\n");
		}
	}
}

void buttonPressed(Controller *self, int unused) {
	int sec, int msec;
	int interArrivalTime = 0;
	int tempo = 0;
	//sio_toggle(&sysIO0, 0);
	CANMsg msg;
	//char *ptr1 = self->buf, *ptr2 = msg.buff;
	if (self->buttonPressedFirstTime == 0) {
		self->buttonPressedFirstTime = 1;
		T_RESET(&tim0);
	} else {
		interArrivalTime = 200//SEC_OF(T_SAMPLE(&tim0));
		//sec = SEC_OF(T_SAMPLE(&tim0));
		//msec = MSEC_OF(T_SAMPLE(&tim0));
		//interArrivalTime = sec*1000 + msec;

		T_RESET(&tim0);
		if (interArrivalTime > MIN_INTER_ARRIVAL_TIME) {
			if (self->c == 0) {
				self->t1 = interArrivalTime;
				self->c++;
			} else if (self->c == 1) {
				if (abs(self->t1 - interArrivalTime) < 100) {
					self->t2 = interArrivalTime;
					self->c++;
				} else {
					self->t1 = interArrivalTime;
					self->c = 0;
				}
			} else {
				if ((abs(self->t2 - interArrivalTime) < 100) && (abs(self->t1 - interArrivalTime) < 100)) {
					self->t3 = interArrivalTime;
					tempo = 60000/((self->t1 + self->t2 + self->t3)/3); 
					snprintf(self->buf, BUF_SIZE, "Tempo: %d BPM\n", tempo);
					SCI_WRITE(&sci0, self->buf);
				} else {
					self->t1 = interArrivalTime;
				}
				self->c = 0;
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

void reader(Controller *self, int c) {
	//if master, control directly
	if (!self->slave) { 
		noteReader(self, c);
	} else {
		SCI_WRITE(&sci0, "Not master. Unable to take keyboard commands \n");
	}
}

//Parser of keyboard inputs to internal commands
int noteReader(Controller *self, int c) {
	CANMsg msg;
	msg.nodeId = self->slave;
	msg.msgId = 0;
	msg.length = 2;
	char tmp[20]; 
	char send = 1; //assume sending
	SCI_WRITECHAR(&sci0, (char) c);
	switch ((char) c) {
		//begin number parser
		case '-':
			self->buf[0] = c;
			self->count = 1;
			send = 0;
		break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			self->buf[self->count] = c;
			self->count++;
			send = 0;
		break;
		//end number parser

		//setTempo with parsed number
		case 't':
		case 'T':
			self->buf[self->count] = '\0';
			self->count = 0; 
			self->myNum = atoi(self->buf);
			sprintf(tmp, "myNum: %d \n", self->myNum);
			SCI_WRITE(&sci0, tmp);
			if (self->play && self->myNum <= MAX_BPM && self->myNum >= MIN_BPM { //if we're playing we have to await a safe note. Queue a change
				self->newTempo = self->myNum;
				sprintf(tmp, "newTempo: %d \n", self->newTempo);
				SCI_WRITE(&sci0, tmp);
				send = 0; //as we're waiting for a safe note, we have nothing to send at this moment
			} else {
				//otherwise we're not playing, and we can change the tempo at any time
				ASYNC(self, tempoSet, self->myNum);
				msg.buff[0] = 't';
				msg.buff[1] = self->myNum;
			} for (int i = 0; i < 20; i++) {
				self->buf[i] = 0;
			}
		break;
		
		//setVolume with parsed number
		case 'v':
		case 'V':
			self->buf[self->count] = '\0';
			self->count = 0; 
			self->myNum = atoi(self->buf);
			ASYNC(&voice0, volumeSet, self->myNum);
			msg.buff[0] = 'v';
			msg.buff[1] = self->myNum;
			for (int i = 0; i < 20; i++) {
				self->buf[i] = 0;
			}
		break;
		
		//set offset with parsed number
		case 'o':
		case 'O':
			self->buf[self->count] = '\0';
			self->count = 0; 
			self->myNum = atoi(self->buf);
			ASYNC(self, offsetSet, self->myNum);
			msg.buff[0] = 'o';
			msg.buff[1] = self->myNum+5;
			for(int i = 0; i < 20; i++) {
				self->buf[i] = 0;
			}
		break;
		//set canon with parsed number (no sending)
		case 'c':
		case 'C':
			if (!self->play) {
				self->buf[self->count] = '\0';
				self->count = 0; 
				self->myNum = atoi(self->buf);
				sprintf(tmp, "Canon %d", self->myNum);
				SCI_WRITE(&sci0, tmp);
				for (int i = 0; i < 20; i++) {
					self->buf[i] = 0;
				}
				ASYNC(&controller, canonSet, self->myNum);
			} else {
				SCI_WRITE(&sci0, "Can't change canon during play!");
			}
			send = 0;
		break;
		//mute cases
		case 'm': 
		case 'M': 
			ASYNC(self, muteSet, 1);
			SCI_WRITE(&sci0, "mute");
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 'v';
			msg.buff[1] = 0; //set volume to 0
		break;

		//vol inc cases
		case '.':
		case ':': 
			sprintf(tmp, "%d", volumeInc(&voice0,1)); //somewhat unsafe way of doing, but shouldn't really affect the execution
			msg.buff[0] = 'v';
			msg.buff[1] = getVolume(&voice0, 0);
			SCI_WRITE(&sci0, "Volume: ");
			SCI_WRITE(&sci0, tmp);
			SCI_WRITE(&sci0,"\n");
		break;

		//vol dec cases 
		case ',':
		case ';': 
			sprintf(tmp, "%d", volumeDec(&voice0,1));//somewhat unsafe way of doing, but shouldn't really affect the execution
			SCI_WRITE(&sci0,"Volume: ");
			SCI_WRITE(&sci0, tmp);
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 'v';
			msg.buff[1] = getVolume(&voice0, 0);
		break;

		case ' ':
			ASYNC(self, startSong, 0);
			SCI_WRITE(&sci0, "Starting Song");
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 's';
			msg.buff[1] = 0;
			if (self->canon) send = 0; //if not chorus, start other devices elsewhere
		break;
		
		case 'p':
		case 'P':
			ASYNC(self, pause, 0);
			SCI_WRITE(&sci0, "Paused");
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 'p';
		break;
		
		case 'r':
		case 'R':
			ASYNC(self, reset, 0);
			ASYNC(self, pause, 0);
			SCI_WRITE(&sci0, "Reset");
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 'e';
			self->slaves = 1;
		break;
		
		//master change case
		case 'u':
		case 'U':
			self->buf[self->count] = '\0';
			self->count = 0; 
			self->myNum = atoi(self->buf);
			if (self->myNum <= NO_OF_SLAVES) {
				self->slave = self->myNum;
				msg.buff[0] = 'm';
				msg.buff[1] = self->myNum;
			} else {
				send = 0;
			}
		break;
		
		default:
			SCI_WRITE(&sci0, "\nThat key doesn't affect the device\n");
		break;
	}

	if (send) { 
		CAN_SEND(&can0, &msg);
		char tmp2[30];
		sprintf(tmp2, "Message sent: %c", msg.buff[0]);
		SCI_WRITE(&sci0, tmp2);
	}
	return 0;
}

int incClock (Controller *self, int inc) {
	if (self->play) {
		self->clockIndex = ((self->clockIndex + inc) % 32);
		AFTER(self->tempo, self, incClock, 1);
		//SCI_WRITE(&sci0, "Clock updated");
	}
	return 0;
}

//Checks if transition of tempo would be safe at current note
//with regards to the current canon
//parameter in: clock object, current canon to eval
//output: 0 unsafe, 1 safe
int safe_to_change(Controller *self, int canon) {
	int true = 1; //assume safe to change
	int tmp;
	if (!canon) {
		switch (self->counter) {
			case 0: case 4: case 8: case 11: case 26: case 29:
			return 1;
			break;
			default: 
			return 0;
		}
	} else if (canon == 4) {
		switch(self->counter) {
			case 0: case 4: case 8: case 11: 
			return 1;
			break;
			default:
			return 0;
		}	
	} else {//(canon == 8){
		switch (self->counter) {
			case 8: case 11: 
				return 1;
			break;
			default:
				return 0;
		}
	}
}
			/*
			return ; //chorus mode. Always able to change tempo
}
}
	
	//for as many slaves as we have (master is a slave to itself)
	for(int i = 0; i <= NO_OF_SLAVES; i++){
		tmp = self->clockIndex - canon * i; //calculate slave's next position
		while(tmp < 0) tmp = tmp + 32; //fix for negative values
		true *= safe_notes[tmp]; //true && slave's note
		if(!true) break; //if not true anymore we don't have to eval others
	}
	//return the result
	if(true){
		SCI_WRITE(&sci0, "Safe to change\n");
	} else {
		SCI_WRITE(&sci0, "Unsafe to change\n");
	}
	return true;*/

int startSong(Controller *self, int unused) {
	SCI_WRITE(&sci0, "startSong\n");
	if (!self->play) {
		self->play = 1;
		self->mute = 0;
		SCI_WRITE(&sci0, "\n\nSong playing\n\n");
		incClock(self, 1); //start the quarter note clock
		ASYNC(self, nextNote, 0);
	}
	return 0;
}

int nextNote(Controller *self, int n) {
	CANMsg msg; //msgID: 0, NodeID: slaveID, Msglen: 2, initial message: {'s' 0}
	msg.msgId = 0;
	msg.nodeId = self->slave;
	msg.length = 2;
	msg.buff[0] = 's';
	msg.buff[1] = 0;

	if (self->play) {
		if (self->counter == 32) {
			self->counter = 0;
		}
		Time b;
		switch (beats[self->counter]) {
		//eight note
			case 0: b = (self->tempo)/2;
			break;

			//half note
			case 2: b = (self->tempo)*2;
			break;

			//quarter note
			default: b = (self->tempo);
			break;
		}

		int p = periods[john[self ->counter] + self->offset + 10];
		SYNC(&voice0, changeTone, p);
		if (!self->mute) SYNC(&voice0, mute, 0);
		SEND(b-MSEC(50), USEC(100), &voice0, mute, 1);
		SEND(b, USEC(100), self, nextNote, 0);
	}
	
	//If we're to start another slave, do it
	if (self->canon && self->slaves <= NO_OF_SLAVES && self->counter % self->canon == 0 && self->counter != 0) {
		msg.buff[1] = self->slaves;
		self->slaves++;
		SCI_WRITE(&sci0, "Start slave #");
		SCI_WRITECHAR(&sci0, msg.buff[1] + 48);
		SCI_WRITECHAR(&sci0, '\n');
		char tmp[20];
		sprintf(tmp, "Count = %d \n", self->counter);
		SCI_WRITE(&sci0, tmp);
		CAN_SEND(&can0, &msg);
	}
	//check if a tempo change is queued and if it is, if it's safe to change tempo
	if (self->newTempo && safe_to_change(self, self->canon)) {
		AFTER(MSEC(1),self, sendChangeTempo, 0); //delay the change somewhat
	}
	self->counter++;
	return 0;
}

int sendChangeTempo(Controller *self, int unused) {
	CANMsg msg;
	msg.nodeId = self->slave;
	msg.msgId = 0;
	msg.length = 2;
	msg.buff[0] = 't';
	msg.buff[1] = self->newTempo;
	CAN_SEND(&can0, &msg);
	ASYNC(self, tempoSet, self->newTempo);
	self->newTempo = 0;
	return 0;
}

int muteSet(Controller *self, int i) {
	SYNC(&voice0, mute, 1);
	self->mute = !self->mute;
	return 0;
}

int reset(Controller *self, int unused) {
	self->counter = 0;
	SYNC(&voice0, mute, 1);
	SCI_WRITE(&sci0, "\n\nSong reset to beginning\n\n");
	return 0;
}

int pause(Controller *self, int unused) {
	self->play=0;
	SCI_WRITE(&sci0, "\n\nSong paused\n\n");
	return 0;
}

int canonSet(Controller *self, int n) {
	if(!self->play && n >= 0 && n <= 8){
		self->canon = n;
	}
	return 0;
}

//takes a change (positive or negative) and applies it on current tempo (if within range)
int tempoChange(Controller *self, int n) {
	char tmp[20]; 
	int currentBPM = TIME_TO_BPM(self->tempo);
	sprintf(tmp, "%d", currentBPM);
	SCI_WRITE(&sci0, tmp);
	SCI_WRITE(&sci0, " is the current BPM\n");
	currentBPM += n; //change BPM
	if (currentBPM < MIN_BPM) currentBPM = MIN_BPM;
	sprintf(tmp, "%d", currentBPM);
	SCI_WRITE(&sci0, tmp);
	SCI_WRITE(&sci0, " is the new BPM\n");

	self->tempo = BPM_TO_TIME(currentBPM); //overwrite old BPM
	return 0;
}

int tempoSet(Controller *self, int n) {
	//int BPM = self->myNum);
	char tmp[20];
	sprintf(tmp, "Input: %d", n );
	SCI_WRITE(&sci0, "tempoSet \n");
	SCI_WRITE(&sci0, tmp);
	int BPM;
	if (n >= MIN_BPM && n <= MAX_BPM) { // if BPM higher than min and lower than max
		BPM = n; //new BPM
	}
	self->tempo = BPM_TO_TIME(BPM); //set new BPM (if no change, set will set it to the old value)
	int tmpint = MSEC_OF(self->tempo);
	sprintf(tmp, "New BPM: %d", tmpint );
	SCI_WRITE(&sci0, tmp);
	SCI_WRITE(&sci0, "\n");
	return 0;
}

//changes current offset by the value n, and checks that it remains within bounds
int offsetChange(Controller *self, int n) {
	char tmp[10];
	int off = self->offset;
	if ((off+n <= MAX_OFFSET && n > 0) ||
			(off+n >= MIN_OFFSET && n < 0)) {
		off += n;
		if (off > MAX_OFFSET) off = MAX_OFFSET;
		if (off < MIN_OFFSET) off = MIN_OFFSET;
	}
	sprintf(tmp, "%d", off);
	SCI_WRITE(&sci0, "\n\nOffset is now ");
	SCI_WRITE(&sci0, tmp);
	SCI_WRITE(&sci0, "\n\n");
	self->offset = off;
	return 0;
}

//sets the offset to the value n if n is within offset bounds
int offsetSet(Controller *self, int n) {
	if (n <= MAX_OFFSET && n >= MIN_OFFSET) self->offset = n;
	return 0;
}

//-------------------------------------------------CONTROLLER--------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------VOICE----------------------------------------------------------

int getVolume(Voice *self, int unused) {
	return self->volume;
}

int volumeSet(Voice *self, int vol) {
	if (vol <= VOLUME_MAX && vol >= VOLUME_MIN) {
		self->volume = vol;
		SCI_WRITE(&sci0, "Volume updated\n");
	}
	return self->volume;
}

int volumeInc(Voice *self, int increase){
	if (self->volume < VOLUME_MAX) {
		self->volume = self->volume + increase;
		if(self->volume > 20) self->volume = 20;
	}
	return self->volume;
}

int volumeDec(Voice *self, int decrease) {
	if (self->volume < VOLUME_MAX) {
		self->volume = self->volume - decrease;
		if (self->volume < 1) self->volume = 1;
	}
	return self->volume;
}

int getMute(Voice *self, int unused) {
	return self->mute;
}

int mute(Voice *self, int n) {
	return self-> mute = n;
}

int changePeriod(Voice *self, int period) {
	self->period = USEC(period);
	return 0;
}

int changeTone(Voice *self, int ny) {
	self->period = USEC(ny);
	return 0;
}

/*Sends a square wave to the Audio DAC*/
int squareWave(Voice *self, int unused) {
	if (!self->mute) {
		self->DAC_OUTPUT = (!self->DAC_OUTPUT) * self->volume; //create the wave if we're not mute 
		*DAC2 = self->DAC_OUTPUT; //send the new value to the DAC
	}

	if (self-> deadline) {
		SEND(self->period, USEC(100), self, squareWave, 0);
	} else {
		AFTER(self->period, self, squareWave, 0); //call to self with the period as delay
	}
	return 0;
}

int getDeadline(Voice *self, int unused) {
	return self->deadline;
}

int deadlineVoice(Voice *self, int unused) {
	return self->deadline = !self->deadline;
}

//----------------------------------------------------VOICE----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------MISC-----------------------------------------------------------


void startApp(Controller *self, int arg) {
	CAN_INIT(&can0);
	SCI_INIT(&sci0);
	canBufferInit(&canBuf);
	SIO_INIT(&sysIO0);
	//SCI_WRITE(&sci0, "Hello. John player program start\n");
	ASYNC(&voice0, squareWave, USEC(500));
	controller.slaves = 1;
	//ASYNC(&controller, startSong, 0);
}

int main() {
	INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
	INSTALL(&sysIO0, sio_interrupt, SIO_IRQ0);
	TINYTIMBER(&controller, startApp, 0);
	return 0;
}
