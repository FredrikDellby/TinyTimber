#ifndef PLAYER_H
#define PLAYER_H

#include "stm32f4xx.h"
#include "TinyTimber.h"
//#include "stm32f4xx_usart.h"

#define SONG_SIZE 32
#define N_PERIODS 25
#define MAX_VOLUME 20
#define MIN_KEY -5
#define MAX_KEY 5
#define MIN_TEMPO 100
#define MAX_TEMPO 200
#define TONES {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0}
#define LENGTHS {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 2, 8, 8, 8, 8, 4, 4, 8, 8, 8, 8, 4, 4, 4, 4, 2, 4, 4, 2}
#define HALF_PERIODS {2024, 1911, 1803, 1702, 1607, 1516, 1431, 1351, 1275, 1203, 1136, 1072, 1012, 955, 901, 851, 803, 758, 715, 675, 637, 601, 568, 536, 506}

#define DAC_REG *((char *) 0x4000741C)

typedef struct {
	Object super;
	Time period, deadline;
	int state;
	int volume;
	int muted;
} ToneTask;

#define initToneTask() {initObject(), USEC(0), USEC(50), 0, 10, 0};
void startToneTask(ToneTask *self, int unused);
void runToneTask(ToneTask *self, int unused);
void killToneTask(ToneTask *self, int unused);
void setPeriod(ToneTask *self, int period);
void setDeadline(ToneTask *self, int deadline);
void setToneVolume(ToneTask *self, int volume);

typedef struct {
	Object super;
	ToneTask *toneTask;
	Time period, deadline, toneGap;
	int state, key, tempo, index;	
	int tone[SONG_SIZE], length[SONG_SIZE], halfPeriod[N_PERIODS];
} Player;

#define initPlayer() {initObject(), NULL, USEC(0), USEC(100), MSEC(50), 0, 0, 120, 0, TONES, LENGTHS, HALF_PERIODS}
void startPlayer(Player *self, int unused);
void runPlayer(Player *self, int unused);
void setToneTask(Player *self, ToneTask *toneTask);
void setKey(Player *self, int key);
void incKey(Player *self);
void decKey(Player *self);
void setTempo(Player *self, int tempo);
void incTempo(Player *self);
void decTempo(Player *self);
void setVolume(Player *self, int volume);
void incVolume(Player *self);
void decVolume(Player *self);
void mute(Player *self);
void unmute(Player *self);
int isMuted(Player *self);

#endif