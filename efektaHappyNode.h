/*
 Name:		                    efektaHappyNode
 Created:	                    15.02.2020
 Modified:                      15.02.2020
 Autor of an idea & creator:    Berk (Efekta)
 Programming:                   Alex aka Sr.FatCat

Library HappyNode
Resistant to connection quality Node
*/
#pragma once

#ifdef MY_PASSIVE_NODE
#error "NOT FOR PASSIVE NODE"
#endif

static_assert(sizeof(MY_TRANSPORT_WAIT_READY_MS) == 2, "INVALID DEFENITION MY_TRANSPORT_WAIT_READY_MS");

#ifdef MY_SEND_RESET_REASON
static_assert(MY_SEND_RESET_REASON >= 0 && MY_SEND_RESET_REASON <= 254, "INVALID DEFENITION MY_SEND_RESET_REASON");
#endif

#ifdef MY_SEND_RSSI
static_assert(MY_SEND_RSSI >= 0 && MY_SEND_RSSI <= 254, "INVALID DEFENITION MY_SEND_RSSI");
#endif

#ifdef MY_SEND_BATTERY
#if (0-MY_SEND_BATTERY-1) != 1 // если MY_SEND_BATTERY определено не просто пустым а с числом
static_assert(MY_SEND_BATTERY >= 0 && MY_SEND_BATTERY <= 254, "INVALID DEFENITION MY_SEND_BATTERY");
#define MY_SEND_BATTERY_VOLTAGE // будем передавать и напряжение в вольтах
#endif
#endif

void happyPresentation();

extern class CHappyNode{
    uint8_t maxPresentTry = 2;
    uint8_t maxNoEchoSendTry = 5;
    uint8_t maxNoParentTry = 0;

    const uint8_t idAddr;
    #define parentIdAddr (idAddr+1)
    #define distanceGWAddr (idAddr+2)
    #define numSensorsAddr (idAddr+3)
    #define presentCompleteAddr (idAddr+4)
    int8_t numSensors = 0;
    #define bytesForSensorsPresent ((numSensors-1)/8 + 1)
    uint8_t sensorsCounter;
    
    int16_t& myTransportWaitReady;
    bool isLostTransport = false;
    uint8_t failureTry = 0;
    
    bool isSendAck, isPresentAck;

    inline void setNoPresent(bool regim){ _coreConfig.presentationSent = regim;  _coreConfig.nodeRegistered = regim; }
    void presentationOnlyParent();
    void setHappyMode();
    void checkParent();
 
    uint8_t *sensorsPresentComplete = nullptr;
    struct {unsigned parent:1; unsigned sketch:1;} isPresentComplete; 
    bool getPresentComplete();
    void loadPresentState();
    void savePresentState();
    void resetPresentState();
public:
    enum try_num_mode_e {TRY_PRESENT, TRY_SEND_NO_ECHO, TRY_NO_PARENT};
    CHappyNode(uint8_t idAddr_) : idAddr(idAddr_), myTransportWaitReady(MY_TRANSPORT_WAIT_READY_MS){}
    void init();
    void setMaxTry(const try_num_mode_e , const uint8_t );
    void config();
    void presentationStart(); 
    void presentationFinish();
    void sendSketchInform(const char *, const char *);
    void perform(const uint8_t, const mysensors_sensor_t, const char *);
    bool checkAck(const MyMessage &);
    void run();
    bool sendMsg(MyMessage &, const uint8_t = 1);
    bool sendSignalStrength();
    bool sendBattery();
    void sendResetReason();
} happyNode;

void CHappyNode::sendSketchInform(const char *sketch_name, const char *sketch_version){
     if (isPresentComplete.sketch) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: Allready present sketch\n"));
        return;
    }

    bool isSketchPresent = true;
    for (int i = 0; i < maxPresentTry; i++)
    {
        if (sendSketchInfo(sketch_name, sketch_version)){
            isSketchPresent = true;
            break;
        }
        isSketchPresent = false;
        _transportSM.failedUplinkTransmissions = 0;
        if (i < maxPresentTry - 1)
        {
            sleep(1000);
            wait(50);
        }
    }
    if (isPresentComplete.sketch = isSketchPresent) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: OK PRESENT SKETCH\n"));
    }
    else {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR PRESENT SKETCH\n"));

    }
}

void CHappyNode::perform(const uint8_t childSensorId, const mysensors_sensor_t sensorType, const char *description){
    if (sensorsPresentComplete[sensorsCounter/8] & (1 << sensorsCounter %8)) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: Allready present SENSOR %i %s (SC %i, SPC %i, BN %i, bN %i)\n"), childSensorId, description, sensorsCounter, sensorsPresentComplete[sensorsCounter/8], sensorsCounter/8, 1 << sensorsCounter % 8);
        sensorsCounter++;
        return;
    }

    isPresentAck = false;
    for (int i = 0; i < maxPresentTry; i++){
        present(childSensorId, sensorType, description, true);
        wait(2500, C_PRESENTATION, sensorType);
        if (isPresentAck) break;
        _transportSM.failedUplinkTransmissions = 0;
        if (i < maxPresentTry - 1) {
            sleep(1500);
            wait(50);
        }
    }
    sensorsPresentComplete[sensorsCounter/8] |= (isPresentAck << sensorsCounter %8);
    if (!isPresentAck) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR PRESENT SENSOR %i %s\n"), childSensorId, description);
    }
    else {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: OK PRESENT SENSOR %i %s\n"), childSensorId, description);
    }
    sensorsCounter++;
}

bool CHappyNode::checkAck(const MyMessage &message){
     if (mGetCommand(message) == C_PRESENTATION){
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ACK OF THE PRESENTATION RECEIVED\n"));
        isPresentAck = true;
        return true;
    }
    if (mGetCommand(message) == C_SET && message.isEcho()) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ACK OF THE SEND RECEIVED\n"));
        isSendAck = true;
        return true;
    }
    if (mGetCommand(message) == C_INTERNAL && message.getType() == I_BATTERY_LEVEL && message.isEcho()) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ACK OF THE BATT LEVEL RECEIVED \n"));
        isSendAck = true;
        return true;
    }
    return false;
}

#ifdef MY_SEND_RESET_REASON
void CHappyNode::sendResetReason(){
    uint16_t nTry = 0;
    bool isSend = false;
    String reason;

#ifdef MY_RESET_REASON_TEXT
    if (NRF_POWER->RESETREAS == 0) reason = "POWER_ON"; 
    else {
        if (NRF_POWER->RESETREAS & (1UL << 0)) reason += "PIN ";
        if (NRF_POWER->RESETREAS & (1UL << 1)) reason += "WDT ";
        if (NRF_POWER->RESETREAS & (1UL << 2)) reason += "SOFT_RESET ";
        if (NRF_POWER->RESETREAS & (1UL << 3)) reason += "LOCKUP";   
        if (NRF_POWER->RESETREAS & (1UL << 16)) reason += "WAKEUP_GPIO ";   
        if (NRF_POWER->RESETREAS & (1UL << 17)) reason += "LPCOMP ";   
        if (NRF_POWER->RESETREAS & (1UL << 17)) reason += "WAKEUP_DEBUG";   
    }
#else
    reason = NRF_POWER->RESETREAS;
#endif

    while (!isSend && nTry++ < 10) {
        isSend = sendMsg(MyMessage(MY_SEND_RESET_REASON, V_VAR2).set(reason.c_str()));
    }
    if (isSend) NRF_POWER->RESETREAS = (0xFFFFFFFF);
}
#endif

#ifdef MY_SEND_BATTERY
bool CHappyNode::sendBattery(){
    wait(1000);

    static uint16_t prevBatteryVoltage = 0;
    bool result = true;
    
    uint16_t batteryVoltage = hwCPUVoltage();
    if (prevBatteryVoltage != batteryVoltage){
        prevBatteryVoltage = batteryVoltage;
        result = sendBatteryLevel(battery_level_in_percent(batteryVoltage), true);
        if (getParentNodeId() != 0){
            CORE_DEBUG(PSTR(">>>>>>>> SEND BATTERY LEVEL AND WAIT ECHO\n"), result);             
            isSendAck = false;
            wait(2500, C_INTERNAL, I_BATTERY_LEVEL);
            result = isSendAck;
        }
#ifdef MY_SEND_BATTERY_VOLTAGE      
        result = sendMsg(MyMessage(MY_SEND_BATTERY, V_VOLTAGE).set((float)batteryVoltage/1000., 2)) && result;
#endif        
    }
    return result;
}
#endif

#ifdef MY_SEND_RSSI
bool CHappyNode::sendSignalStrength(){
    static int16_t prevSignalRSSI = INVALID_RSSI;
    int16_t signalRSSI = transportGetReceivingRSSI();
    if (signalRSSI != prevSignalRSSI){
        prevSignalRSSI = signalRSSI;
        return sendMsg(MyMessage(MY_SEND_RSSI, V_VAR1).set(constrain(map(signalRSSI, -85, -40, 0, 100), 0, 100)));
    }
    return true;
}
#endif

void CHappyNode::init(){
    if (hwReadConfig(EEPROM_NODE_ID_ADDRESS) == 0){
        hwWriteConfig(EEPROM_NODE_ID_ADDRESS, 255);
    }
    if (loadState(idAddr) == 0){
        saveState(idAddr, 255);
    }
    CORE_DEBUG(PSTR(">>>>>>>> MyS: EEPROM NODE ID: %d\n"), hwReadConfig(EEPROM_NODE_ID_ADDRESS));
    CORE_DEBUG(PSTR(">>>>>>>> MyS: USER MEMORY SECTOR NODE ID: %d\n"), loadState(idAddr));

    if (hwReadConfig(EEPROM_NODE_ID_ADDRESS) == 255){ //новая нода
        myTransportWaitReady = 0;
        // resetPresentState();
        // savePresentState();
    }
    else{ // не новая нода, уже была прописана в какой-то гейт
        myTransportWaitReady = 10000;
        loadPresentState();
        setNoPresent(getPresentComplete());
    }
    CORE_DEBUG(PSTR(">>>>>>>> MyS: MY_TRANSPORT_WAIT_MS =%d\n"), myTransportWaitReady);
}

void CHappyNode::config() {
    if (myTransportWaitReady == 0){ //Только новой ноде
        saveState(idAddr, getNodeId());
        saveState(parentIdAddr, getParentNodeId());
        saveState(distanceGWAddr, getDistanceGW());
        savePresentState();
        //saveState(presentCompleteAddr, *((uint8_t *)&isPresentComplete));
        CORE_DEBUG(PSTR(">>>>>>>> MyS: new node config %i-%i (%i)\n"), getNodeId(), getParentNodeId(), getDistanceGW());
#ifdef MY_DEBUG        
        getPresentComplete(); 
#endif        
    }
    else  if (!isTransportReady()) { // пожилая нода
            _transportConfig.nodeId = loadState(idAddr); //myid;
            _transportConfig.parentNodeId = loadState(parentIdAddr);
            _transportConfig.distanceGW = loadState(distanceGWAddr);
            setHappyMode();
            isLostTransport = true;
            CORE_DEBUG(PSTR(">>>>>>>> MyS: TRANSPORT ERR. PARAMS LOAD FROM EEPROM\n"));
            CORE_DEBUG(PSTR(">>>>>>>> MyS: ENTERY HAPPY MODE\n"));
    }
#ifdef MY_SEND_RESET_REASON
    sendResetReason();
#endif    
    //     if (isTransportReady()){ // законектилась
    //         if (getNodeId() != loadState(idAddr)){ // сменился адресс ноды
    //             saveState(idAddr, getNodeId());
    //         }
    //         if (getParentNodeId() != loadState(parentIdAddr)){ //сменился родитель
    //             saveState(parentIdAddr, getParentNodeId());
    //             present_only_parent(); //презентуем только нового родителя
    //          }
    //         if ( getDistanceGW()!= loadState(distanceGWAddr)){ // сменился гейт
    //             presentNode(); // полная презентация по новой
    //             saveState(presentCompleteAddr, isPresentComlete);
    //             saveState(distanceGWAddr, _transportConfig.distanceGW);
    //         }
    //     }
    //     else{ // нету конекта
    //         // flag_fcount = 1;
    //         // err_delivery_beat = 5;
    //         _transportConfig.nodeId = loadState(idAddr); //myid;
    //         _transportConfig.parentNodeId = loadState(parentIdAddr);
    //         _transportConfig.distanceGW = loadState(distanceGWAddr);
    //         myParrent = _transportConfig.parentNodeId;
    //         happy_node_mode();
    //         gateway_fail();/***** del ******/
    //     }
    // }
}

void CHappyNode::presentationOnlyParent(){
    CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND LITTLE PRESENT:) WITH PARENT ID\n"));
    isPresentComplete.parent = _sendRoute(build(_msgTmp, 0, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG).set(_transportConfig.parentNodeId));
    CORE_DEBUG(PSTR(">>>>>>>> MyS: LITTLE PRESENT is %s\n"), isPresentComplete.parent ? PSTR("OK") : PSTR("ERR"));

}

void CHappyNode::setHappyMode() {
  _transportSM.findingParentNode = false;
  _transportSM.transportActive = true;
  _transportSM.uplinkOk = true;
  _transportSM.pingActive = false;
  transportSwitchSM(stReady);
  _transportSM.failureCounter = 0;
}

void CHappyNode::run(){
    if (isTransportReady() == true) { //Если есть связь
        //если была потеря связи - проверяем
        //если родителя нет - ищем
        //проверка презентаций
        if (isLostTransport) {
            checkParent();
            //failureTry = 0;
        }
#ifdef MY_SEND_RSSI
        sendSignalStrength();
#endif
#ifdef MY_SEND_BATTERY
        sendBattery();
#endif

    }

    if (_transportSM.failureCounter > 0 || failureTry >= maxNoEchoSendTry) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ENTERY HAPPY MODE\n"));
        _transportConfig.parentNodeId = loadState(parentIdAddr);
        _transportConfig.nodeId = loadState(idAddr); //myid;
        _transportConfig.distanceGW = loadState(distanceGWAddr);
        isLostTransport = true;
        setHappyMode();
     }
}

void CHappyNode::checkParent(){
    _transportSM.findingParentNode = true;
    CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND FIND PARENT REQUEST, WAIT RESPONSE\n"));
    _sendRoute(build(_msg, 255, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_REQUEST).set(""));
    wait(1500, C_INTERNAL, I_FIND_PARENT_RESPONSE);
    if (_msg.sensor == 255 && mGetCommand(_msg) == C_INTERNAL && _msg.type == I_FIND_PARENT_RESPONSE){
        CORE_DEBUG(PSTR(">>>>>>>> MyS: PARENT RESPONSE FOUND\n"));
        CORE_DEBUG(PSTR(">>>>>>>> MyS: Parent = %i (%i)\n"), _transportConfig.parentNodeId, _msg.getSender());
        transportSwitchSM(stParent);
        isLostTransport = false;
        failureTry = 0;
    }
    else{
        _transportSM.findingParentNode = false;
        CORE_DEBUG(PSTR(">>>>>>>> MyS: PARENT RESPONSE NOT FOUND\n"));
        _transportSM.failedUplinkTransmissions = 0;
    }
}

bool CHappyNode::sendMsg(MyMessage& message, const uint8_t tryNum){
    for (int i=0; i<tryNum; i++){
        if (_transportConfig.parentNodeId == 0) {
            if (send(message)){
                CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND OK TO GW\n"));
                failureTry = 0;
            }
            else {
                _transportSM.failedUplinkTransmissions = 0;
                CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR SEND TO GW\n"));
                failureTry++;
            }
        }
        else {
            isSendAck = false;
            send(message, true);
            wait(2500, C_SET, message.getType());
            if (isSendAck){
                CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND OK. NORMAL SEND TO ROUTER\n"));
                failureTry = 0;
            }
            else {
                _transportSM.failedUplinkTransmissions = 0;
                CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR SEND TO ROUTER\n"));
                failureTry++;
            }
        }
        if (i < tryNum -1) wait(200);
    }
}

bool CHappyNode::getPresentComplete(){
    bool result = isPresentComplete.sketch && isPresentComplete.parent;
    CORE_DEBUG(PSTR(">>>>>>>> MyS: node present sketch is %s / parent is %s\n"), isPresentComplete.sketch ? PSTR("OK") : PSTR("ERR"), isPresentComplete.parent ? PSTR("OK") : PSTR("ERR"));
    if (!result) return false;
    for(int i = 0; i < numSensors; i++ ) {
        result = sensorsPresentComplete[ i /8] & (1 << i % 8);
        CORE_DEBUG(PSTR(">>>>>>>> MyS: node present sensor %i ==> %s\n"), i, result ? PSTR("OK") : PSTR("ERR"));
        if (!result) return false;
    }
    return true;
}

void CHappyNode::resetPresentState(){
    isPresentComplete.sketch = isPresentComplete.parent = false;
    if (numSensors > 0) memset(sensorsPresentComplete, 0, bytesForSensorsPresent);
}

void CHappyNode::loadPresentState(){
    uint8_t tmp = loadState(presentCompleteAddr);
    memcpy(&isPresentComplete, &tmp, 1);
    numSensors = loadState(numSensorsAddr);
     if (numSensors > 0) {
        sensorsPresentComplete = new uint8_t[bytesForSensorsPresent]; 
        for (int i=0; i < bytesForSensorsPresent; i++){
            sensorsPresentComplete[i] = loadState(presentCompleteAddr + i + 1);    
        }
     }
}

void CHappyNode::savePresentState(){
    saveState(numSensorsAddr, numSensors);
    saveState(presentCompleteAddr, *((uint8_t *)&isPresentComplete));
    if (numSensors > 0) {
        for (int i = 0; i < bytesForSensorsPresent; i++)        {
            saveState(presentCompleteAddr + i + 1, sensorsPresentComplete[i]);
        }
    }
}

void CHappyNode::presentationStart(){
    if (!sensorsPresentComplete){ // если сенсоры еще не считали ни разу
        sensorsPresentComplete = new uint8_t[64]; //создадим на максимально возможное количество сенсоров
        memset(sensorsPresentComplete, 0, 64); // занулим
    }
    if (getPresentComplete()) resetPresentState();
    sensorsCounter = 0; 
}

void CHappyNode::presentationFinish(){ 
    if (numSensors == 0 && sensorsCounter > 0) { // посчитали сенсоры первый раз и их не 0
        numSensors =  sensorsCounter;
        uint8_t tmp[bytesForSensorsPresent]; // пересоздадим уже на нужное количество сенсоров
        memcpy(tmp, sensorsPresentComplete, bytesForSensorsPresent);
        delete sensorsPresentComplete;
        sensorsPresentComplete = new uint8_t[bytesForSensorsPresent];
        memcpy(sensorsPresentComplete, tmp, bytesForSensorsPresent);
        CORE_DEBUG(" -------------------- total %i sensors, in %i bytes, mask: %i\n", numSensors, bytesForSensorsPresent, sensorsPresentComplete[0]);
    }
    else if(numSensors == 0 && sensorsCounter == 0){
       delete sensorsPresentComplete;
       sensorsPresentComplete = new uint8_t[1]; // это чтобы больше не пытаться считать заново
       sensorsPresentComplete[0] = 0;
    }
    if (!isPresentComplete.parent) presentationOnlyParent();
    savePresentState(); 
}

void CHappyNode::setMaxTry(const try_num_mode_e mode, const uint8_t maxTry){
    switch(mode){
        case TRY_PRESENT: maxPresentTry = maxTry;
            break;
        case TRY_SEND_NO_ECHO: maxNoEchoSendTry = maxTry;
            break;
        case TRY_NO_PARENT: maxNoParentTry = maxTry;                
    }
}

void presentation(){
     happyNode.presentationStart();
     happyPresentation();
#ifdef MY_SEND_RSSI
    happyNode.perform(MY_SEND_RSSI, S_CUSTOM, PSTR("Signal quality in %"));
#endif
#ifdef MY_SEND_BATTERY_VOLTAGE
    happyNode.perform(MY_SEND_BATTERY, S_MULTIMETER, PSTR("Battery voltage"));
#endif
#ifdef MY_SEND_RESET_REASON
    happyNode.perform(MY_SEND_RESET_REASON, S_CUSTOM, PSTR("Reset reason"));
#endif
     happyNode.presentationFinish();
}

inline void happyInit(){ happyNode.init(); }
inline void happyConfig(){ happyNode.config(); }
inline void happySendSketchInfo(const char *x, const char *y){ happyNode.sendSketchInform(x, y);}
inline void happyPresent(const uint8_t x, const mysensors_sensor_t y, const char *z) {happyNode.perform(x, y, z); }
inline bool happyCheckAck(const MyMessage &x) { return happyNode.checkAck(x); }
inline void happyProcess() { happyNode.run(); }
inline bool happySend(MyMessage &x) { return happyNode.sendMsg(x);}
inline bool happySend(MyMessage &x, const uint8_t y) { return happyNode.sendMsg(x, y);}

#undef parentIdAddr 
#undef distanceGWAddr 
#undef numSensorsAddr 
#undef presentCompleteAddr 
#undef bytesForSensorsPresent 
