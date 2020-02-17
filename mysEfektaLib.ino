/*
 Name:		mysEfektaLib
 Created:	25.01.2020
 Modified:  25.01.2020
 Author:	Alex aka Sr.FatCat

Demo for library by Berk (Efekta)
gpiot, happy_node
*/

#define MY_DEBUG
#ifndef MY_DEBUG
#define MY_DISABLED_SERIAL
#endif
#define MY_RADIO_NRF5_ESB
int16_t mtwr;
#define MY_TRANSPORT_WAIT_READY_MS (mtwr)

#define PIN_1 (1)
#define PIN_2 (2)

#define CHILD_ID_PIN_1 0
#define CHILD_ID_PIN_2 1
#define CHILD_ID_VIRT 2

#include <MySensors.h>

#include "efektaGpiot.h"
#include "efektaHappyNode.h"

MyMessage msgPin1(CHILD_ID_PIN_1, V_TRIPPED);
MyMessage msgPin2(CHILD_ID_PIN_2, V_TRIPPED);
MyMessage msgVirt(CHILD_ID_VIRT, V_VAR);

CHappyNode happyNode(100, 101, 102, 103, mtwr);
CDream interruptedSleep(1); // количество пинов по которым будут прерывания сна

void before() {
    happyInit();
}

void setup(){
    addDreamPin(PIN_1, NRF_GPIO_PIN_PULLUP, CDream::NRF_PIN_GHANGE); // добавляем описание пинов
    addDreamPin(PIN_2, NRF_GPIO_PIN_PULLUP, CDream::NRF_PIN_GHANGE); 
    initDream();
    happyConfig();
}

void presentation() {
    happySendSketchInfo("HappyNode & Dream test", "V1.0");
    happyPresent(CHILD_ID_PIN_1, S_BINARY, "STATUS Pin1");
    happyPresent(CHILD_ID_PIN_2, S_BINARY, "STATUS Pin2");
    happyPresent(CHILD_ID_VIRT, S_CUSTOM, "STATUS Virtual");
}

void loop() {
    happyProcess();
    int8_t wakeupReson = dream(10000);
    if (wakeupReson == MY_WAKE_UP_BY_TIMER){
        happySend(msgVirt.set((uint16_t)rand()));
    }
    else if (wakeupReson == PIN_1) happySend(msgPin1.set(!msgPin1.getBool()));
    else if (wakeupReson == PIN_2) happySend(msgPin2.set(!msgPin2.getBool()));
}

void receive(const MyMessage & message){
    if (happyCheckAck(message)) return;

}
