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

//class int;
typedef struct {
	Object super;
	CANMsg msgQueue[10];
	char buf[BUF_SIZE];
	int first, last, size;
} CANBuffer;


Voice voice0 = {initObject(), 0, USEC(500), 7, 1, 1}; //defaults: wave low, 1kHz, 7 volume, deadline on, mute on
Controller controller = {initObject(), MSEC(500), 0, 0, 0, 1, SLAVE, 0, 0, 0, 0, 1, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //defaults: Tempo: 120BPM, first note: 0, offset: 0, playing: FALSE, mute: TRUE, slave: FALSE
Load load = {initObject(), 13500, 0, 0};
Serial sci0 = initSerial(SCI_PORT0, &controller, reader); //reader
Can can0 = initCan(CAN_PORT0, &controller, receiver);
SysIO sysIO0 = initSysIO(SIO_PORT0, &controller, buttonPressed);
BlinkTask blinky = {initObject(), 500, 0};
Timer tim0 = initTimer();
Timer tim1 = initTimer();
CANBuffer canBuf;


void canBufferInit(CANBuffer*);
void runCANBufferTask(CANBuffer*, int);
void runBlinkTask(BlinkTask*, int);
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


void startBlinkTask(BlinkTask *self, int unused){
	self->on = 1;
	BEFORE((MSEC(50)), self, runBlinkTask, unused);
}

void runBlinkTask(BlinkTask *self, int unused ){
	if(self->on){
		sio_write(&sysIO0, 0);
		SEND(MSEC(500), MSEC(50), self, runBlinkTask, unused);
	}
}

void setHalfPeriod(BlinkTask *self, int halfPeriod){
	self->half_period = halfPeriod;
}

void killBlinkTask(BlinkTask *self){
	self->on = 0;
	sio_write(&sysIO0, 1);
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
	snprintf(self->buf, BUF_SIZE, "%s\n", msg->buff);
	SCI_WRITE(&sci0, self->buf);
}

/*
void receiver(Controller *self, int unused) {
    CANMsg msg;
	CAN_RECEIVE(&can0, &msg);
	Time interArrivalTime;
	CAN_RECEIVE(&can0, &msg);
	//if (self->firstCANMsgReceived == 0) {
		//self->firstCANMsgReceived = 1;
		//T_RESET(&tim1);
		//printCANMsg(self, &msg);
	//}
	//else {
		//interArrivalTime = T_SAMPLE(&tim1);
		//T_RESET(&tim1);
	//}
		/*if(canBuf.size < 10){
			//if((canBuf.size > 0 ) )  {
				//canBuf.msgQueue[++canBuf.last] = msg;
				//canBuf.size++;
			
				
			} else if (interArrivalTime < MSEC(1000) ) {
				canBuf.msgQueue[++canBuf.last] = msg;
				canBuf.size++;
				
				//SEND(MSEC(1000) - interArrivalTime, MSEC(100), self, runCANBufferTask, unused);
			} else {*/
	/*printCANMsg(self, &msg);
			//}
		//}
	//}  
	
    if(self->slave){
		switch(msg.buff[0]){
			case 'v':
				
                if(msg.buff[1] == 255){
                    ASYNC(&voice0, volumeDec, 1);
                }
                else if(msg.buff[1] == 254){
                    ASYNC(&voice0, volumeInc, 1);
                } else if (msg.buff[1] > 0 && msg.buff[1] <= VOLUME_MAX){
                    ASYNC(&voice0, volumeSet, msg.buff[1]);
                }
			break;
			
			case 't':
                if(msg.buff[1] >= MIN_BPM && msg.buff[1] <= MAX_BPM) ASYNC(self, tempoSet, msg.buff[1]);
			break;
			
			case 'o':
			case 'k':
                if(msg.buff[1] >= MIN_OFFSET+5 && msg.buff[1] <= MAX_OFFSET+5) ASYNC(self, offsetSet, msg.buff[1]-5);
			break;
			
			case 's':
                if(!msg.buff[1] || msg.buff[1] == self->slave){
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
                if (msg.buff[1] == self->slave){
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
    }else{
        if(msg.buff[0] == 'b'){
			char tmp[20]; 
			sprintf(tmp, "%d", msg.buff[1]);  
			SCI_WRITE(&sci0, "Device #");
			SCI_WRITE(&sci0, tmp);
			SCI_WRITE(&sci0, " has responded\n");
			}
		}
	}
*/   
void receiver(Controller *self, int unused){
	CANMsg msg;
	char buff[100];
	CAN_RECEIVE(&can0, &msg);
	switch(msg.msgId) {
		case 0x10:		// Receiving a Network check message
			ASYNC(self, NetworkCheckAnswer, 0);	// Answering that this board is on the network
			SCI_WRITE(&sci0,"NetworkCheckAnswer\n");
			break;
		case 0x20:	// Receivnig a Tone request 
			ASYNC(self, SaveSongIndex, msg.buff[0]);
			if (msg.nodeId == NODE_ID){
				SCI_WRITE(&sci0, "Playing tone\n");
				ASYNC(self, playTone, msg.buff[0]);
			}
			break;
		case 0x11:	// Receiving a NetworkcheckAnswer 
			self->nodeIDs[self->noNodes] = msg.nodeId;	// Saving the received node ID 
			self->noNodes++;	// Incrementing how many boards exist on the network
			snprintf(buff, 100, "Node %d is on the network, total %d \n", msg.nodeId, self->noNodes);	// Debug message
			SCI_WRITE(&sci0, buff);	
			break;
		case 0x21:	// Playing tone ACK 
			break;
		case 0x40: 	// Tempo change 
			if(msg.buff[1] >= MIN_BPM && msg.buff[1] <= MAX_BPM) ASYNC(self, tempoSet, msg.buff[0]);
			break;
		case 0x50:	// Received master takeover message 
			self->slave = 1;
			break;
		case 0x51:	// Master takeover ACK
			if(self->slave == 0) {
				ASYNC(self, CANsomething, 0);	// Starting to send Tone request 
			}
	}
	
	
}

void SaveSongIndex(Controller *self, int LatestI){
	self->counter = LatestI + 1;
}
void CANsomething(Controller *self, int unused) {
	if(self->nodeCounter == 0){
		SCI_WRITE(&sci0, "Playing tone \n");
		ASYNC(&controller, nextNote, 0);
	} else {
		ASYNC(self, CANTone, self->nodeIDs[self->nodeCounter]);
	}
	if (self->nodeCounter >= self->noNodes-1) {
		self->nodeCounter = 0;
	} else {
		self->nodeCounter++; 
	}
	SIO_WRITE(&sysIO0,1); 
	SCI_WRITE(&sci0, (char*)self->slave);
	if (self->slave == 0) {
		
	}
}

void CANNetworkCheck(Controller *self, int unused) {
	self->noNodes = 1;
	self->nodeIDs[0] = self->myNum;
	CANMsg msg;
	
	msg.msgId = 0x10;
	msg.nodeId = 3;
	msg.length = 0;
	CAN_SEND(&can0, &msg);
}
void NetworkCheckAnswer(Controller *self, int unused) {
	
	CANMsg msg;
	msg.msgId = 0x11;
	msg.nodeId = NODE_ID;
	msg.length = 0;
	int check = 1;
	while(check != 0){
		check = CAN_SEND(&can0, &msg);
	}
	SCI_WRITE(&sci0, "Sending ACK\n");
}
void CANToneAck(Controller *self, int unused){
	CANMsg msg;
	msg.msgId = 0x21;
	msg.nodeId = self->myNum;
	msg.length = 0;
	CAN_SEND(&can0, &msg);
}

void CANClaimMasterACK(Controller* self, int unused){
	CANMsg msg;
	msg.msgId = 0x51;
	msg.nodeId = self->myNum;
	msg.length = 0;
	CAN_SEND(&can0, &msg);
}
void CANClaimMaster(Controller *self, int unused){
	CANMsg msg;
	
	msg.msgId = 0x50;
	msg.nodeId = self->myNum;
	msg.length = 0;
	CAN_SEND(&can0, &msg);
	self->slave = 1;
}

void CANTone(Controller *self, int id){
	CANMsg msg; 
	
	msg.msgId = 0x20;
	msg.nodeId = id;
	msg.length = 1;
	msg.buff[0] = self->counter;
	CAN_SEND(&can0, &msg);
}


void buttonPressed(Controller *self, int unused){
	int interArrivalTime = 0; 
	int tempo = 0;
	int mean = 0;
	int sec, msec;
	  ASYNC(self, muteSet, 1);
	
	//sio_toggle(&sysIO0, 0);
	CANMsg msg;
	msg.msgId = 0x15;
	//char *ptr1 = self->buf, *ptr2 = msg.buff;
	if (self->buttonPressedFirstTime == 0) {
		self->buttonPressedFirstTime = 1;
		T_RESET(&tim0);
		
	} else {
		sec = SEC_OF(T_SAMPLE(&tim0));
		msec = MSEC_OF(T_SAMPLE(&tim0));
		interArrivalTime = sec*1000 + msec;

		T_RESET(&tim0);
		if (interArrivalTime > MIN_INTER_ARRIVAL_TIME) {
			  
			if(self->c == 0){
				self->t1 = interArrivalTime;
				self->c++;
			}
			else if(self->c == 1){
				if(abs(self->t1 - interArrivalTime) < 100){
					self->t2 = interArrivalTime;
					self->c++;
				}
				else{
					self->t1 = interArrivalTime;
					self->c = 0;
				}
			}
			else {
				if((abs(self->t2 - interArrivalTime) < 100)&& (abs(self->t1 - interArrivalTime) < 100) ){
					self->t3 = interArrivalTime;
					mean = (self->t1 + self->t2 + self->t3)/3;
					tempo = 60000/mean; 
					if(tempo <= MAX_BPM && tempo >= MIN_BPM){ 
						ASYNC(&controller, tempoSet, tempo);
						//sio_write(&sysIO0, 1);
						ASYNC(&blinky, setHalfPeriod, mean/2);
						
					}
					//snprintf(self->buf, BUF_SIZE, "%d", tempo);
					//snprintf(self->buf+3, BUF_SIZE, "T\n", tempo);
					
					
					
				}
				else{
					self->t1 = interArrivalTime;
				}
				self->c = 0;
			
		}
		snprintf(self->buf, BUF_SIZE, "Tempo: %d BPM\n", tempo);
		msg.msgId = self->msgIndex++;
		msg.nodeId = 1;
		//msg.buff[0] = 't';
		
		snprintf(self->buf, BUF_SIZE, "Interarrivaltime %d\n", interArrivalTime);
		//SCI_WRITE(&sci0, self->buf);
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
    if(!self->slave){ 
		noteReader(self, c);
	}else{
		if((char)c == 'u'){
			//self->slave = 0;
			noteReader(self, c);
			//SCI_WRITE(&sci0, "I am now master");
		}
		else{
			SCI_WRITE(&sci0, "Not master. Unable to take keyboard commands \n");
		}
    
    }
}


//Parser of keyboard inputs to internal commands
int noteReader(Controller *self, int c){
	CANMsg msg;
	msg.nodeId = self->slave;
	msg.msgId = 0;
	msg.length = 2;

	char tmp[20]; 
	
    char send = 1; //assume sending
    SCI_WRITECHAR(&sci0, (char)c);
	switch((char)c){
        
        //begin number parser    
        case '-':
            self -> buf[0] = c;
            self -> count = 1;
            send = 0;
        break;
        
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            
            self -> buf[self -> count] = c;
            self -> count++;
            send = 0;
            
        break;
        //end number parser

        //setTempo with parsed number
        case 't':
        case 'T':
            self -> buf[self -> count] = '\0';
            self -> count = 0; 
            self -> myNum = atoi(self -> buf);
			sprintf(tmp, "myNum: %d \n", self->myNum);
			SCI_WRITE(&sci0, tmp);
            if(self->play && self->myNum <= MAX_BPM && self->myNum >= MIN_BPM){ //if we're playing we have to await a safe note. Queue a change
				self -> newTempo = self -> myNum;
				sprintf(tmp, "newTempo: %d \n", self->newTempo);
				SCI_WRITE(&sci0, tmp);
				send = 0; //as we're waiting for a safe note, we have nothing to send at this moment
				}
		
            else{
                
            //otherwise we're not playing, and we can change the tempo at any time
            
            ASYNC(self, tempoSet, self->myNum);
            msg.buff[0] = 't';
            msg.buff[1] = self->myNum;
            }
			for(int i = 0; i < 20; i++){
					self->buf[i] = 0;
			}
        break;
            
            //setVolume with parsed number
        case 'v':
        case 'V':
            self -> buf[self -> count] = '\0';
            self -> count = 0; 
            self -> myNum = atoi(self -> buf);
            ASYNC(&voice0, volumeSet, self->myNum);
            msg.buff[0] = 'v';
            msg.buff[1] = self->myNum;
			for(int i = 0; i < 20; i++){
					self->buf[i] = 0;
				}
        break;
        
            //set offset with parsed number
        case 'o':
        case 'O':
            self -> buf[self -> count] = '\0';
            self -> count = 0; 
            self -> myNum = atoi(self -> buf);
            ASYNC(self, offsetSet, self->myNum);
            msg.buff[0] = 'o';
            msg.buff[1] = self->myNum+5;
			for(int i = 0; i < 20; i++){
					self->buf[i] = 0;
				}
        break;
        
        //set canon with parsed number (no sending)
		case 'c':
        case 'C':
            if(!self->play){

            self -> buf[self -> count] = '\0';
            self -> count = 0; 
            self -> myNum = atoi(self -> buf);
			sprintf(tmp, "Canon %d", self->myNum);
			SCI_WRITE(&sci0, tmp);
			for(int i = 0; i < 20; i++){
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
/* Debug code
		//decrease distortion
		case 'o':
		case 'O': 
            sprintf(tmp, "%d", loadDec(&load,0));
            SCI_WRITE(&sci0, "Loop value is now: ");
            SCI_WRITE(&sci0, tmp);
            SCI_WRITE(&sci0, "\n");
            break;

		//increase distortion
		case 'p':
		case 'P': 
            sprintf(tmp, "%d", loadInc(&load,0));
            SCI_WRITE(&sci0, "Loop value is now: ");
            SCI_WRITE(&sci0, tmp);
            SCI_WRITE(&sci0, "\n");
            break;

		case '1': 
		changePeriod(&voice0, 500);
		SCI_WRITE(&sci0, "Frequency: 1000 Hz\n");
		break;
		
		case '2': 
		changePeriod(&voice0, 650);
		SCI_WRITE(&sci0, "Frequency: 769 Hz\n");
		break;
		
		case '3': 
		changePeriod(&voice0, 931);
		SCI_WRITE(&sci0, "Frequency: 537 Hz\n");
		break;

		case 'd':
		case 'D': deadlineLoad(&load,0);
		deadlineVoice(&voice0,0);
		if(getDeadline(&voice0,0)){
			SCI_WRITE(&sci0, "Deadline enabled\n");
		}else{
			SCI_WRITE(&sci0, "Deadline disabled\n");
		}
		break;
	*/
		
    
        case ' ':
            ASYNC(self, startSong, 0);
            
			SCI_WRITE(&sci0, "Starting Song");
			SCI_WRITE(&sci0,"\n");
			msg.buff[0] = 's';
			msg.buff[1] = 0;
            if(self->canon) send = 0; //if not chorus, start other devices elsewhere
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
     /*       
        case '+':
            ASYNC(self, tempoChange, 1);
            break;
        
        case '-':
            ASYNC(self, tempoChange, -1);
            break;
			 * */
              
              /*
		case 'a':
        case 'A':
            ASYNC(self, offsetChange, -1);


            break;
        
        case 's':
        case 'S':
            ASYNC(self, offsetChange, 1);

            break;
			 */

			
        //master change case
        case 'u':
        case 'U':
            self -> buf[self -> count] = '\0';
            self -> count = 0; 
            self -> myNum = atoi(self -> buf);
            if(self -> myNum <= NO_OF_SLAVES){
                self -> slave = self -> myNum;
                msg.buff[0] = 'm';
                msg.buff[1] = self -> myNum;
            } else {
                send = 0;
            }
        break;
         case 'x':
			ASYNC(self, CANNetworkCheck, 0);
			ASYNC(self, CANClaimMaster, 0);
			ASYNC(self, CANsomething, 0);
			break;
		
		case 'z': 
			ASYNC(self, CANNetworkCheck, 0);
			ASYNC(self, CANClaimMaster, 0);
			break;
		
		default:
            SCI_WRITE(&sci0, "\nThat key doesn't affect the device\n");
        break;
	}

	if(send){ 
	CAN_SEND(&can0, &msg);
	char tmp2[30];
	sprintf(tmp2, "Message sent: %c", msg.buff[0]);
	SCI_WRITE(&sci0, tmp2);
	}
    return 0;
	
}

int incClock (Controller *self, int inc){
    if(self->play){
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
int safe_to_change(Controller *self, int canon){
	int true = 1; //assume safe to change
	int tmp;
	
	if(!canon){
		switch(self->counter){
			case 0: case 4: case 8: case 11: case 26: case 29:
			return 1;
			break;
			default: 
			return 0;
		}
	}
	else if (canon == 4){
		switch(self->counter){
			case 0: case 4: case 8: case 11: 
			return 1;
			break;
			default:
			return 0;
		}	
		
	}
	else {//(canon == 8){
		switch(self->counter){
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
	if(true){juyl
		SCI_WRITE(&sci0, "Safe to change\n");
	} else {
		SCI_WRITE(&sci0, "Unsafe to change\n");
	}
	return true;*/
void blinkFunction(BlinkTask *self, int unused){
	//if (self->on ) {
		sio_toggle(&sysIO0, 0);
		SEND(MSEC(self->half_period), USEC(100), self, blinkFunction, 0);
	//}
}

void killBlink(BlinkTask *self, int unused){
	self->on = 0;
	sio_write(&sysIO0, 1);
	
}


int startSong(Controller *self, int unused){
	SCI_WRITE(&sci0, "startSong\n");
	 
	
	

	if(!self->play){
	self -> play = 1;
	self->mute = 0;
	SCI_WRITE(&sci0, "\n\nSong playing\n\n");
    incClock(self, 1); //start the quarter note clock
	ASYNC(self, nextNote, 0);
	if(isLeader()==0){
		ASYNC(&blinky, blinkFunction, 0);
	}
	else{
		sio_write(&sysIO0, 0);
	}
	//ASYNC(&blinky,setHalfPeriod,self->tempo );
	
   
    }
    return 0;
}

int nextNote(Controller *self, int n){
	
    CANMsg msg; //msgID: 0, NodeID: slaveID, Msglen: 2, initial message: {'s' 0}
	msg.msgId = 0;
	msg.nodeId = self->slave;
	msg.length = 2;
	msg.buff[0] = 's';
	msg.buff[1] = 0;
	char tmp[20];
	char tmp2[20];
    /*if(self->play){
		if(self->counter == 32){
			self->counter = 0;
		}
	}*/
	sprintf(tmp, "Counter: %d\n", self->counter);
	SCI_WRITE(&sci0, tmp);
    Time b;
    switch(beats[self->counter]){
        //eight note
        case 0: b = (self->tempo)/2;
        break;
        
        //half note
        case 2: b = (self->tempo)*2;
        break;
        
        //quarter note
        default: b = (self -> tempo);
        break;
        }
	sprintf(tmp2, "b: %d\n", b);
	SCI_WRITE(&sci0, tmp2);

	int p = periods[john[self->counter] + self->offset + 10];
	SYNC(&voice0, changeTone, p);
	if(!self->mute) SYNC(&voice0, mute, 0);
    //SEND(0, USEC(100), &voice0, mute, 0);
	if(self->mute) SYNC(&voice0, mute, 1);
	SEND(b-MSEC(50), MSEC(100), &voice0, mute, 0);
	//SEND(b, MSEC(100), &voice0, mute, 1);
	//ASYNC(&voice0, squareWave, 0);
	if(self->slave == 0){
		if(self->noNodes == 0){
			SEND(b, USEC(100), self, nextNote, 0);
		}
		else{
			SEND(b, USEC(100), self, CANsomething, 0);
		}
	}
	
	//SEND((self->tempo-MSEC(50)), USEC(100), &sysIO0, sio_write, 0);
	
	
    
    
    //If we're to start another slave, do it
	if(self->canon && self->slaves <= NO_OF_SLAVES && self->counter % self->canon == 0 && self->counter != 0){
		msg.buff[1] = self->slaves;
		self->slaves++;
		SCI_WRITE(&sci0, "Start slave #");
        SCI_WRITECHAR(&sci0, msg.buff[1] + 48);
        SCI_WRITECHAR(&sci0, '\n');
		char tmp[20];
		sprintf(tmp, "Count = %d \n", n);
		SCI_WRITE(&sci0, tmp);
        CAN_SEND(&can0, &msg);
	}
    
         //check if a tempo change is queued and if it is, if it's safe to change tempo
     if(self->newTempo && safe_to_change(self, self->canon)){
        AFTER(MSEC(1),self, sendChangeTempo, 0); //delay the change somewhat
     }
	//self->counter++;
    return 0;
}

int playTone(Controller *self, int tone){
	//SYNC(&voice0, revive, 0);
	SYNC(&voice0, mute, 1);
	SYNC(&voice0, revive, 0);
	Time b;
	SCI_WRITE(&sci0, "playTone");
	switch(beats[tone]){
		case 0: b = (self->tempo)/2;
        break;
        
        //half note
        case 2: b = (self->tempo)*2;
        break;
        
        //quarter note
        default: b = (self -> tempo);
        break;
        }
		int p = periods[john[tone] + self->offset + 10];
		SYNC(&voice0, changeTone, p);
		//if(!self->mute) SYNC(&voice0, mute, 1);
		 ASYNC(&voice0, squareWave, 0);
		AFTER(MSEC(0.85*b), &voice0, kill, 0);
		
}



int sendChangeTempo(Controller *self, int unused){
    CANMsg msg;
    msg.nodeId = self -> slave;
    msg.msgId = 0;
    msg.length = 2;
    msg.buff[0] = 't';
    msg.buff[1] = self->newTempo;
    CAN_SEND(&can0, &msg);

    ASYNC(self, tempoSet, self->newTempo);
    self->newTempo = 0;
    return 0;
}

tone_gen(Voice *self, int unused){
	
}

int muteSet(Controller *self, int i){
    
    SYNC(&voice0, mute, 1);
	self->mute = !self->mute;
   
    return 0;
}

int reset(Controller *self, int unused){
	self->counter = 0;
	SYNC(&voice0, mute, 1);
    SCI_WRITE(&sci0, "\n\nSong reset to beginning\n\n");
    
	return 0;
}

int pause(Controller *self, int unused){
	self->play=0;
    SCI_WRITE(&sci0, "\n\nSong paused\n\n");
	return 0;
}
int canonSet(Controller *self, int n){
	if(!self->play && n >= 0 && n <= 8){
		self->canon = n;
	}
	return 0;
}

//takes a change (positive or negative) and applies it on current tempo (if within range)
int tempoChange(Controller *self, int n){
   char tmp[20]; 
   int currentBPM = TIME_TO_BPM(self->tempo);
   
   sprintf(tmp, "%d", currentBPM);
   SCI_WRITE(&sci0, tmp);
   SCI_WRITE(&sci0, " is the current BPM\n");
   
   currentBPM += n; //change BPM
   
   if(currentBPM < MIN_BPM) currentBPM = MIN_BPM;
   
   sprintf(tmp, "%d", currentBPM);
   SCI_WRITE(&sci0, tmp);
   SCI_WRITE(&sci0, " is the new BPM\n");
   
   
   self->tempo = BPM_TO_TIME(currentBPM); //overwrite old BPM
   return 0;
   
}


int tempoSet(Controller *self, int n){
      //int BPM = self->myNum);
	char tmp[20];
	sprintf(tmp, "Input: %d", n );
	SCI_WRITE(&sci0, "tempoSet \n");
	SCI_WRITE(&sci0, tmp);
	int BPM;
    if(n >= MIN_BPM && n <= MAX_BPM){ // if BPM higher than min and lower than max
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
int offsetChange(Controller *self, int n){
    char tmp[10];
    int off = self->offset;
    if((off+n <= MAX_OFFSET && n > 0) ||
       (off+n >= MIN_OFFSET && n < 0)){
        off += n;
        if(off > MAX_OFFSET) off = MAX_OFFSET;
        if(off < MIN_OFFSET) off = MIN_OFFSET;
    }
                sprintf(tmp, "%d", off);
            SCI_WRITE(&sci0, "\n\nOffset is now ");
            SCI_WRITE(&sci0, tmp);
            SCI_WRITE(&sci0, "\n\n");
    self->offset = off;
    return 0;
}

//sets the offset to the value n if n is within offset bounds
int offsetSet(Controller *self, int n){
    if(n <= MAX_OFFSET && n >= MIN_OFFSET) self->offset = n;
    return 0;
}


//-------------------------------------------------CONTROLLER--------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------VOICE----------------------------------------------------------

int isLeader(Controller *self, int unused){
	return self->slave;
}

int getVolume(Voice *self, int unused){
	return self -> volume;
}

int volumeSet(Voice *self, int vol){
	if(vol <= VOLUME_MAX && vol >= VOLUME_MIN){
		self -> volume = vol;
		SCI_WRITE(&sci0, "Volume updated\n");
	}
    return self->volume;
}

int volumeInc(Voice *self, int increase){
	if(self->volume < VOLUME_MAX){
		self -> volume = self -> volume + increase;
        if(self->volume > 20) self -> volume = 20;
	}
    return self->volume;
}

int volumeDec(Voice *self, int decrease){
	if(self->volume < VOLUME_MAX){
		self -> volume = self -> volume - decrease;
        if(self->volume < 1) self -> volume = 1;
	}
    return self->volume;
}

int getMute(Voice *self, int unused){
	return self -> mute;
}

int mute(Voice *self, int n){
	return self-> mute = n;
}

int changePeriod(Voice *self, int period){
	self->period = USEC(period);
    return 0;
}

int changeTone(Voice *self, int ny){

	self->period = USEC(ny);

    return 0;
}

/*Sends a square wave to the Audio DAC*/
int squareWave(Voice *self, int unused){
	char tmp;
	SCI_WRITE(&sci0,"Squarewave\n");
	if(self->on = 1){
		SCI_WRITE(&sci0, "ON\n");
		if(self->mute){
			SCI_WRITE(&sci0, "NOT MUTE\n");
			//self -> DAC_OUTPUT = (!self -> DAC_OUTPUT) * self -> volume; //create the wave if we're not mute 
			*DAC2 = self->volume;
			//*DAC2 = self -> DAC_OUTPUT; //send the new value to the DAC
			self->on = 0;
		}
		else{
			SCI_WRITE(&sci0, "MUTE");
			*DAC2 = 0;
			self->on = 0;
		}
	}
	else{
		SCI_WRITE(&sci0, "OFF");
		*DAC2 = 0;
		self->on = 1;
	}
	if(self->alive){
		SEND(USEC(self->period), USEC(100), self, squareWave, 0);
	}

	/*if(self-> deadline){
		SEND(self->period, USEC(100), self, squareWave, 0);
	}else{
        AFTER(self->period, self, squareWave, 0); //call to self with the period as delay
	}
	 * */
    return 0;
}


void kill(Voice *self, int unused){
	self->alive = 0;
}

void revive(Voice *self, int unused){
	self->alive = 1;
}

int getDeadline(Voice *self, int unused){
	return self -> deadline;
}

int deadlineVoice(Voice *self, int unused){
	return self -> deadline = !self->deadline;
}

//----------------------------------------------------VOICE----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------LOAD-----------------------------------------------------------

int getLoad(Load *self, int unused){
	return self -> background_loop_range;
}

int loadInc(Load *self, int unused){
	//if(self-> background_loop_range < 8000){
		self -> background_loop_range = self -> background_loop_range + 500;
        return 0;
	//}
}

int loadDec(Load *self, int unused){
	if(self -> background_loop_range > 1000){
		self -> background_loop_range = self -> background_loop_range - 500;
	}
    return 0;
}

int deadlineLoad(Load *self, int unused){
	return self -> deadline = !self -> deadline;
}

int background(Load *self, int unused){
	long time_val = USEC_OF(CURRENT_OFFSET());
    char tmp[20];
    if(self -> counter < 500){
        for(int i = 0; i < self -> background_loop_range; i++){}
            long time_val2 = USEC_OF(CURRENT_OFFSET());
            time_val = time_val2 - time_val;
            if(self -> max < time_val) self -> max = time_val; // MAX(old, new)
            self -> sum = self -> sum + time_val;
            self -> counter++;
            if(self -> deadline){
                SEND(USEC(1300), USEC(1300), self, background, 0);
            }else{
                AFTER(USEC(1300), self, background, 0);
            }
    }else{
        sprintf(tmp, "%ld", (self->sum));
        SCI_WRITE(&sci0, "Sum WCET of load is: ");
        SCI_WRITE(&sci0, tmp);
        SCI_WRITECHAR(&sci0, '\n');	

        sprintf(tmp, "%ld", self->max);
        
        SCI_WRITE(&sci0, "Max WCET of load is: ");
        SCI_WRITE(&sci0, tmp);
        SCI_WRITECHAR(&sci0, '\n');
        
        sprintf(tmp, "%ld", (self->sum/500));
        SCI_WRITE(&sci0, "AM WCET of load is: ");
        SCI_WRITE(&sci0, tmp);
        SCI_WRITECHAR(&sci0, '\n');
    }
    return 0;
}

//----------------------------------------------------LOAD-----------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------MISC-----------------------------------------------------------


void startApp(Controller *self, int arg) {

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
	canBufferInit(&canBuf);
	SIO_INIT(&sysIO0);
    //SCI_WRITE(&sci0, "Hello. John player program start\n");
	ASYNC(&voice0, squareWave, USEC(500));
	ASYNC(&sysIO0, sio_write, 1);
	controller.slaves = 1;
	//ASYNC(&load, background, 0);
	//ASYNC(&controller, startSong, 0);

}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
	INSTALL(&sysIO0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&controller, startApp, 0);
    return 0;
}

