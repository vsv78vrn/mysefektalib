/*
 Name:		                    efektaHappyNode
 Created:	                    15.02.2020
 Modified:                      15.02.2020
 Autor of an idea & creator:    Berk (Efekta)
 Programming:                   Alex aka Sr.FatCat

Library HappyNode
Resistant to connection quality Node
*/

#define happyInit() happyNode.init()
#define happyConfig() happyNode.config()
#define happySendSketchInfo(x, y) happyNode.sendSketchInform((x), (y))
#define happyPresent(x, y, z) happyNode.perform((x), (y), (z))
#define happyCheckAck(x) happyNode.checkAck((x))
#define happyProcess() happyNode.run()
#define happySend(x) happyNode.sendMsg((x))

void happyPresentation();

extern class CHappyNode{
    enum try_num_mode_e {TRY_PRESENT, TRY_SEND_NO_ECHO, TRY_NO_PARENT};
    uint8_t maxPresentTry = 2;
    uint8_t maxNoEchoSendTry = 5;
    uint8_t maxNoParentTry = 0;

    const uint8_t idAddr;
    const uint8_t parentIdAddr;
    const uint8_t distanceGWAddr;
    const uint8_t presentCompleteAddr;

    const uint8_t numSensors;

    int16_t& myTransportWaitReady;
    bool isLostTransport = false;
    uint8_t failureTry = 0;
    uint8_t sensorsCounter;

    bool isSendAck, isPresentAck;

    inline void setNoPresent(bool regim){ _coreConfig.presentationSent = regim;  _coreConfig.nodeRegistered = regim; }
    void presentationOnlyParent();
    void setHappyMode();
    void checkParent();
 
    uint8_t *sensorsPresentComplete;
    struct {unsigned parent:1; unsigned sketch:1;} isPresentComplete; 
    bool getPresentComplete();
    void loadPresentState();
    void savePresentState();
    void resetPresentState();
public:
    CHappyNode(uint8_t idAddr_, uint8_t parentIdAddr_, uint8_t distanceGWAddr_, uint8_t presentCompleteAddr_, int16_t &myTransportWaitReady_, uint8_t numSensors_ = 8) ;
    void init();
    void setMaxTry(try_num_mode_e , uint8_t );
    void config();
    void presentationStart(); 
    void presentationFinish();
    void sendSketchInform(const char *, const char *);
    void perform(const uint8_t, const mysensors_sensor_t, const char *);
    bool checkAck(const MyMessage &);
    void run();
    void sendMsg(MyMessage &, uint8_t = 1);
} happyNode;

CHappyNode::CHappyNode(uint8_t idAddr_, uint8_t parentIdAddr_, uint8_t distanceGWAddr_, uint8_t presentCompleteAddr_, int16_t &myTransportWaitReady_, uint8_t numSensors_): 
        idAddr(idAddr_), parentIdAddr(parentIdAddr_), distanceGWAddr(distanceGWAddr_), presentCompleteAddr(presentCompleteAddr_), 
        myTransportWaitReady(myTransportWaitReady_), numSensors(numSensors_){ 
    sensorsPresentComplete = new uint8_t[(numSensors_ -1) / 8 + 1]; 
}

void CHappyNode::sendSketchInform(const char *sketch_name, const char *sketch_version){
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
        isPresentAck = true;
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ACK OF THE PRESENTATION IN THE FUNCTION RECEIVE RECEIVED\n"));
        return true;
    }
    if (mGetCommand(message) == C_SET && message.isEcho()) {
        CORE_DEBUG(PSTR(">>>>>>>> MyS: ACK OF THE SEND RECEIVED\n"));
        isSendAck = true;
        return true;
    }
    return false;
}

void CHappyNode::init(){
#ifdef DEBUG_NEW_NODE
    hwWriteConfig(EEPROM_NODE_ID_ADDRESS, 0);
    saveState(idAddr, 0);
#endif    
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
        resetPresentState();
        savePresentState();
    }
    else{ // не новая нода, уже была прописана в какой-то гейт
        myTransportWaitReady = 10000;
        setNoPresent(getPresentComplete());
    }
    CORE_DEBUG(PSTR(">>>>>>>> MyS: MY_TRANSPORT_WAIT_MS =%d\n"), myTransportWaitReady);
}

void CHappyNode::config() {
    if (myTransportWaitReady == 0){ //Только новой ноде
        saveState(idAddr, getNodeId());
        saveState(parentIdAddr, getParentNodeId());
        saveState(distanceGWAddr, getDistanceGW());
        //saveState(presentCompleteAddr, *((uint8_t *)&isPresentComplete));
        CORE_DEBUG(PSTR(">>>>>>>> MyS: new node config %i-%i (%i)\n"), getNodeId(), getParentNodeId(), getDistanceGW());
        CORE_DEBUG(PSTR(">>>>>>>> MyS: new node present is %s\n"), getPresentComplete() ? PSTR("OK"): PSTR("ERR")); 
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
        if (isLostTransport && failureTry > 3) {
            checkParent();
        }
    }

    if (_transportSM.failureCounter > 0) {
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
        transportSwitchSM(stParent);
        isLostTransport = false;
        failureTry = 0;
    }
    else{
        _transportSM.findingParentNode = false;
        CORE_DEBUG(PSTR(">>>>>>>> MyS: PARENT RESPONSE NOT FOUND\n"));
        _transportSM.failedUplinkTransmissions = 0;
        //nosleep = 0;
        /*if (problem_mode_count < 24)
        {
            CORE_DEBUG(PSTR("PROBLEM MODE COUNTER: %d\n"), problem_mode_count);
            problem_mode_count++;
            SLEEP_TIME_W = SLEEP_TIME / 100 * 120;
        }
        else if (problem_mode_count == 24)
        {
            SLEEP_TIME_W = SLEEP_TIME * 30;
            CORE_DEBUG(PSTR("PROBLEM MODE COUNTER: %d\n"), problem_mode_count);
        }*/
    }
}

void CHappyNode::sendMsg(MyMessage& message, uint8_t tryNum){
    if (_transportConfig.parentNodeId == 0) {
        if (send(message)){
            CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND OK TO GW\n"));
        }
        else {
            _transportSM.failedUplinkTransmissions = 0;
            CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR SEND TO GW\n"));
            // isLostTransport = true;
            failureTry++;
        }
    }
    else {
        isSendAck = false;
        send(message, true);
        wait(2500, C_SET, message.getType());
        if (isSendAck){
            CORE_DEBUG(PSTR(">>>>>>>> MyS: SEND OK. NORMAL SEND TO ROUTER\n"));
        }
        else {
            _transportSM.failedUplinkTransmissions = 0;
            CORE_DEBUG(PSTR(">>>>>>>> MyS: ERR SEND TO ROUTER\n"));
            //isLostTransport = true;
            failureTry++;
        }
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
    memset(sensorsPresentComplete, 0, (numSensors - 1) / 8 + 1);
}

void CHappyNode::loadPresentState(){
    uint8_t tmp = loadState(presentCompleteAddr);
    memcpy(&isPresentComplete, &tmp, 1);
    for (int i=0; i < (numSensors-1) /8 + 1; i++){
        sensorsPresentComplete[i] = loadState(presentCompleteAddr + i + 1);    
    }
}

void CHappyNode::savePresentState(){
    saveState(presentCompleteAddr, *((uint8_t *)&isPresentComplete));
    for (int i=0; i < (numSensors-1) /8 + 1; i++){
        saveState(presentCompleteAddr + i + 1,  sensorsPresentComplete[i]);    
    }
}

void CHappyNode::presentationStart(){
    if (getPresentComplete()) resetPresentState();
    sensorsCounter = 0; 
}

void CHappyNode::presentationFinish(){ 
    if (!isPresentComplete.parent) presentationOnlyParent();
    savePresentState(); 
}

void CHappyNode::setMaxTry(try_num_mode_e mode, uint8_t maxTry){
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
     happyNode.presentationFinish();
 }