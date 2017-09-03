// Name: haptic_tuner_v8
// Program for Arduino based haptic-feedback tuner for music.
// by Austin Pursley 2017

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <FreqMeasure.h>
#include "tuning.h"

//pin connection variables
const int PIN_FLAT_B = 2;
const int PIN_FLAT_A = 3;
const int PIN_IN_TUNE = 4;
const int PIN_SHARP_A = 5;
const int PIN_SHARP_B = 6;
//On Arduino Uno, audio input pin is 8. Check freqMeasure website for other platforms.

//reference frequecy array
float referenceFreq[108];
//variables
int count = 0;
double sum = 0;
double waitcount = 0;
int motorNum;
float frequency;
float CPUcount;
float CPUcountFreq;
float th;
Tuning tuning;


void setup() {
  Serial.begin(57600);
  pinMode(PIN_FLAT_B,OUTPUT);
  pinMode(PIN_FLAT_A,OUTPUT);
  pinMode(PIN_IN_TUNE,OUTPUT);
  pinMode(PIN_SHARP_A,OUTPUT);
  pinMode(PIN_SHARP_B,OUTPUT); 
  //create array of in-tune reference frequencies on an equal-tempered scale.
  float f0 = 440.0; //centered on A4, 440 Hz
  float a = 1.059463094359;
  for (int i = -57; i < 50; i++){
    referenceFreq[i+57] = f0 * (pow(a,i));
    Serial.println(referenceFreq[i + 57]);
  } 
  FreqMeasure.begin();  
}

void loop() {  
  if (FreqMeasure.available()) {
    waitcount = 0;
    count = count + 1;
    //establish initial sum of CPUcount
    if (count <= 25) {
      CPUcount = FreqMeasure.read();
      sum = sum + CPUcount; 
      
    //after enough counts check for outliers
    } else if ((count > 25) && (count < 250)) {
      CPUcount = FreqMeasure.read();
      CPUcountFreq = (FreqMeasure.countToFrequency(CPUcount));
      frequency = (FreqMeasure.countToFrequency(sum / count));
    //reject frequency value if it is outside of threshold th
    th = 0.09;
    if (((1+th) * frequency < CPUcountFreq) || ((1-th) * frequency > CPUcountFreq)) {
      //reject, add an average from previous values instead
      sum = sum + (sum / (count-1)); 
    } else { //accept value
      sum = sum + CPUcount;
    }
    
    //count limit reached, time to calculate frequency
    } else {
      frequency = FreqMeasure.countToFrequency(sum / count);
      tuning = what_is_tuning(frequency);
      Serial.println(frequency);
      //in tune
      if (tuning.cent <= 0.1) { 
        motorNum = 3;
      //little out of tune
      } else if (tuning.cent <= 0.15) { 
        if (tuning.isSharp == true) {
          motorNum = 4;
        } else {
          motorNum = 2;
        }
      //out of tune
      } else { 
        if (tuning.isSharp == true) {
          motorNum = 5;
        } else {
          motorNum = 1;
        }
      }  
      //vibrate motor according to cent
      vibrateMotor(motorNum); 
      sum = 0;
      count = 0;
    }
  } else {
    ++waitcount;
    //after awhile of no readings, turn off all motors
    if(waitcount == 20000) {  
      vibrateMotor(0);
      waitcount = 0;
    }
  }
}

/*
 *  Functions
 */

Tuning what_is_tuning(float frequency) {
  //find closest reference frequency by comparing measured frequency to array
  Tuning tuning; //struct with int for cent and bool for sharp/flat. See header.
  float closeFreq;
  float freqDiffFlat;
  float freqDiffSharp;
  long int cent = 0;
  for(byte i = 0; i < 108; i++) {  
    if (abs(frequency - referenceFreq[i]) < abs(frequency - referenceFreq[i + 1])) { 
      closeFreq = referenceFreq[i];
      freqDiffFlat = abs(closeFreq-referenceFreq[i - 1]); 
      freqDiffSharp = abs(closeFreq-referenceFreq[i + 1]);
      break;
    }
  }
  //is note sharp or flat, and how many cents?
  float freqDistance = abs(frequency - closeFreq);
  if (frequency > closeFreq) { //check if sharp
    tuning.isSharp = true;
    tuning.cent = freqDistance / freqDiffSharp;
  } else {
    tuning.isSharp = false; //flat
    tuning.cent = freqDistance / freqDiffFlat;
  }
  return tuning;
}

void vibrateMotor(int motorNum) {  
  //turn on motor according to cents sharp/flat.
  if (motorNum == 3) { // in tune
    digitalWrite(PIN_FLAT_B, LOW);
    digitalWrite(PIN_FLAT_A, LOW);
    digitalWrite(PIN_IN_TUNE, HIGH);
    digitalWrite(PIN_SHARP_A, LOW);
    digitalWrite(PIN_SHARP_B, LOW);
  } else if (motorNum == 4) { //little sharp
    digitalWrite(PIN_FLAT_B, LOW);
    digitalWrite(PIN_FLAT_A, LOW);
    digitalWrite(PIN_IN_TUNE, LOW);
    digitalWrite(PIN_SHARP_A, HIGH);
    digitalWrite(PIN_SHARP_B, LOW);
  } else if (motorNum == 2) { //litte flat
    digitalWrite(PIN_FLAT_B, LOW);
    digitalWrite(PIN_FLAT_A, HIGH);
    digitalWrite(PIN_IN_TUNE, LOW);
    digitalWrite(PIN_SHARP_A, LOW);
    digitalWrite(PIN_SHARP_B, LOW);
  } else if (motorNum == 5) { //sharp
    digitalWrite(PIN_FLAT_B, LOW);
    digitalWrite(PIN_FLAT_A, LOW);
    digitalWrite(PIN_IN_TUNE, LOW);
    digitalWrite(PIN_SHARP_A, LOW);
    digitalWrite(PIN_SHARP_B, HIGH);
  } else if (motorNum == 1) { //flat
    digitalWrite(PIN_FLAT_B, HIGH);
    digitalWrite(PIN_FLAT_A, LOW);
    digitalWrite(PIN_IN_TUNE, LOW);
    digitalWrite(PIN_SHARP_A, LOW);
    digitalWrite(PIN_SHARP_B, LOW);
  } else if (motorNum == 0) { //all off
    digitalWrite(PIN_FLAT_B, LOW);
    digitalWrite(PIN_FLAT_A, LOW);
    digitalWrite(PIN_IN_TUNE, LOW);
    digitalWrite(PIN_SHARP_A, LOW);
    digitalWrite(PIN_SHARP_B, LOW);
  }
}

