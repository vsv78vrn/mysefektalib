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

int16_t myTransportComlpeteMS;
#define MY_TRANSPORT_WAIT_READY_MS (myTransportComlpeteMS)

//#define MY_SEND_RSSI 254
//#define MY_SEND_BATTERY 253
#define MY_SEND_RESET_REASON 252
#define MY_RESET_REASON_TEXT

#define PIN_1 (1)
#define PIN_2 (2)

#define CHILD_ID_PIN_1 0
#define CHILD_ID_PIN_2 1
#define CHILD_ID_VIRT 2

#include <MySensors.h>

//#include "efektaGpiot.h"
#include "efektaHappyNode.h"

MyMessage msgVirt(CHILD_ID_VIRT, V_VAR);

CHappyNode happyNode(100); // Адрес c которого будут храниться пользовательские данные

void before() {
    happyInit();
}

void setup(){
    happyConfig();
}

void happyPresentation() {
    happySendSketchInfo("HappyNode & Dream test", "V1.0");
    happyPresent(CHILD_ID_VIRT, S_CUSTOM, "STATUS Virtual");
}

void loop() {
    happyProcess();

    const uint32_t t = millis();
    static uint32_t prev_t = t;

    if (t - prev_t >= 20000) {
        happyNode.sendBattery(253);
        happyNode.sendSignalStrength(254);
        prev_t = t;
    }

    int8_t wakeupReson = happyNode.smartSleep(10000);
    if (wakeupReson == MY_WAKE_UP_BY_TIMER){
        happySend(msgVirt.set((uint16_t)rand()));
    }
}

void receive(const MyMessage & message){
    if (happyCheckAck(message)) return;

}
