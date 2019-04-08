#include "stm32f4xx.h"
#include "TinyTimber.h"
#include "player.h"

// Tone task methods

void startToneTask(ToneTask *self, int unused) {
	self->state = 1;	// Running
	DAC_REG = 0;
	BEFORE(self->deadline, self, runToneTask, unused);
}

void runToneTask(ToneTask *self, int unused) {
	if (self->state == 1) {
		if (!self->muted && DAC_REG == 0) {
			// Set HIGH
			DAC_REG = self->volume;
		} else {
			// Set LOW
			DAC_REG = 0;
		}
		SEND(self->period, self->deadline, self, runToneTask, unused);
	}
}

void killToneTask(ToneTask *self, int unused) {
	self->state = 0;
}

void setPeriod(ToneTask *self, int period) {
	self->period = USEC(period);
}

void setDeadline(ToneTask *self, int deadline) {
	self->deadline = USEC(deadline);
}

void setToneVolume(ToneTask *self, int volume) {
	if (volume > MAX_VOLUME) {
		self->volume = MAX_VOLUME;
	} else if (volume < 1) {
		self->volume = 1;
	} else {
		self->volume = volume;
	}
}

// Player methods

void startPlayer(Player *self, int unused) {
	self->state = 1;	// Running
	self->index = 0;	// Start from beginning
	int tone = self->tone[self->index + self->key];
	self->period = MSEC(2000)/self->length[self->index];
	self->toneTask->period = USEC(self->halfPeriod[tone]);
	BEFORE(self->toneTask->deadline, self->toneTask, startToneTask, unused);
	SEND(self->period, self->toneTask->deadline, self->toneTask, killToneTask, unused);
	SEND(self->period, self->deadline, self, runPlayer, unused);
}

void runPlayer(Player *self, int unused) {
	int tone = self->tone[self->index + self->key];
	// self->toneGap = MSEC(12000/self->tempo);
	self->period = self->toneGap + MSEC(240000/self->length[self->index]/self->tempo);
	self->toneTask->period = USEC(self->halfPeriod[tone]);
	SEND(self->toneGap, self->toneTask->deadline, self->toneTask, startToneTask, unused);
	SEND(self->period, self->toneTask->deadline, self->toneTask, killToneTask, unused);
	SEND(self->period, self->deadline, self, runPlayer, unused);
	self->index++;
}

void setToneTask(Player *self, ToneTask *toneTask) {
	self->toneTask = toneTask;
}

void setKey(Player *self, int key) {
	if (key > MAX_KEY) {
		self->key = MAX_KEY;
	} else if (key < MIN_KEY) {
		self->key = MIN_KEY;
	} else {
		self->key = key;
	}
}

void incKey(Player *self) {
	setKey(self, self->key + 1);
}

void decKey(Player *self) {
	setKey(self, self->key - 1);
}

void setTempo(Player *self, int tempo) {
	if (tempo > MAX_TEMPO) {
		self->tempo = MAX_TEMPO;
	} else if (tempo < MIN_TEMPO) {
		self->tempo = MIN_TEMPO;
	} else {
		self->tempo = tempo;
	}
}

void incTempo(Player *self) {
	setTempo(self, self->tempo + 10);
}

void decTempo(Player *self) {
	setTempo(self, self->tempo - 10);
}

void setVolume(Player *self, int volume) {
	ASYNC(self->toneTask, setToneVolume, volume);
}

void incVolume(Player *self) {
	setVolume(self, self->toneTask->volume + 1);
}

void decVolume(Player *self) {
	setVolume(self, self->toneTask->volume - 1);
}

void mute(Player *self) {
	self->toneTask->muted = 1;
}

void unmute(Player *self) {
	self->toneTask->muted = 0;
}

int isMuted(Player *self) {
	return self->toneTask->muted;
}
