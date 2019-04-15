#ifndef APP_H
#define APP_H
#define max_index 14
#define min_index -10
#define MAX_OFFSET 5
#define MIN_OFFSET -5
#define MAX_BPM 240
#define MIN_BPM 60
#define DAC_ADDR (char*) 0x4000741C //Audio output DAC location
#define VOLUME_MIN 0
#define VOLUME_MAX 20
#define NO_OF_SLAVES 2

//The periods for the different notes avalible in brother John
const int periods[] = { 2024, 1911, 1803, 1702, 1607,     //-10 to -6
                        1516, 1431, 1351, 1275, 1203,     //-5 to -1
                        1136, 1072, 1012, 955,  901,      //0 to 4
                        851,  803,  758,  715,  675,      //5 to 9
                        637,  601,  568,  536,  506};     //10 to 14
                        
const int john[] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0}; //the "note sheet" for brother John
const int beats[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 1, 1, 2};
const int safe_notes[] = {1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0}; //indicates "safe" transitions in quarter note increments
char *DAC2 = DAC_ADDR;

#define TONE(x) \
        ((Time)(periods[(x) + 10])); //conversion macro for tones 

#define TIME_TO_BPM(x) \
        ((int)(60000 / MSEC_OF(x))); //conversion macro for time to BPM (int number)
        
#define BPM_TO_TIME(x) \
        ((Time)MSEC((60000 / x))); //conversion macro for BPM to TIME (Time value)


//Structs for different object types
typedef struct {
    Object super;
   	char DAC_OUTPUT;
	Time period;
	int volume, deadline, mute;
} Voice;

typedef struct {
	Object super;
	int background_loop_range;
	int deadline;
	int counter;
	long sum;
	long max;
} Load;

typedef struct {
    Object super;
    Time tempo;
    int counter;
    int offset;
    int play;
    int mute;
    char slave;
    int count;
    char buf[20];
    int myNum;
    int canon;
    int slaves;
    int clockIndex;
    int eigth;
    int newTempo;
	int buttonPressedFirstTime;
	int firstCANMsgReceived;
	int c;
	int t1;
	int	t2;
	int t3;
	uchar msgIndex;
} Controller;

//increase this each quarter note (i.e. twice per half note and once every 2 8ths)
int incClock (Controller *self, int unused);
int safe_to_change(Controller *self, int canon);
//Controller functions.
void reader(Controller*, int); //given by instructors. 
void receiver(Controller*, int); //given by instructors.

void buttonPressed(Controller*, int);
void printCANMsg(Controller*, CANMsg*);
int startSong(Controller*, int);
int nextNote(Controller*, int);
int muteSet(Controller*, int);
int pause(Controller*, int);
int reset(Controller*, int);
int tempoChange(Controller*, int);
int tempoSet(Controller*, int);
int offsetChange(Controller*, int);
int volumeSet(Voice*, int);
int offsetSet(Controller*, int);
int canonSet(Controller*, int);
int sendChangeTempo(Controller*, int);


/*Gets the keyboard interrupts and parses the command,
 * and then executes it.
 */
int noteReader(Controller *self, int c);

//Does what the name entails
int getVolume(Voice *self, int unused);
int volumeInc(Voice *self, int increase);
int volumeDec(Voice *self, int decrease);

int getMute(Voice *self, int unused);
int mute(Voice *self, int unused);

int changePeriod(Voice *self, int period);
int changeTone(Voice *self, int ny); //uses the macro TONE(x), but does the same as changePeriod

/*Creates a square wave by writing high/low value
 * (0 and voice->volume) to the DAC output adress.
 * If the voice is muted, this will not happen
 * 
 * Calls itself indefinitly with an SEND
 * call, with the voice's period as the 
 * offset and 100 usec as deadline. 
 * 
 * If deadlines are disabled the call is 
 * instead an AFTER with the same offset
 * as the SEND call
 */
int squareWave(Voice *self, int unused);

//for deadline enable/disable
int getDeadline(Voice *self, int unused);
int deadlineVoice(Voice *self, int unused);



//does what the names entail
int getLoad(Load *self, int unused);
int loadInc(Load *self, int unused);
int loadDec(Load *self, int unused);

//for deadline enable/disable
int deadlineLoad(Load *self, int unused);

/*Creates "dead time" on the processor by 
 * running an empty loop x number of times,
 * where x is the value of 
 * self->background_loop_range.
 * 
 * Calls itself indefinitly with an SEND
 * call, with the voice's period as the 
 * offset and 100 usec as deadline. 
 * 
 * If deadlines are disabled the call is 
 * instead an AFTER with the same offset
 * as the SEND call
 */
int background(Load *self, int unused);




const char *text[] = {"Bro","der ","Ja","cob.\n",
                      "Bro","der ","Ja","cob.\n",
                      "So","ver ","du?\n",
                      "So","ver ","du?\n",
                      "Hör ","du ","in","te ","klo","ckan?\n",
                      "Hör ","du ","in","te ","klo","ckan?\n",
                      "Ding ", "ding ","dong.\n",
                      "Ding ", "ding ","dong.\n\n\n"};

#endif