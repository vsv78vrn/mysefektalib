/*
 Name:		                    efektaHappyNode
 Created:	                    15.02.2020
 Modified:                      15.02.2020
 Autor of an idea & creator:    Berk (Efekta)
 Programming:                   Alex aka Sr.FatCat

Library HappyNode
Resistant to connection quality Node
*/

#include <stdint.h>

#define happyInit() happyNode.init()
#define happyConfig() happyNode.config()
#define happySendSketchInfo(x, y) happyNode.sendSketchInform((x), (y))
#define happyPresent(x, y, z) happyNode.perform((x), (y), (z))
#define happyCheckAck(x) happyNode.checkAck((x))
#define happyProcess() happyNode.run()
#define happySend(x) happyNode.sendMsg((x))

class CHappyNode{
    const uint8_t maxPresentTry = 2;

    const uint8_t idAddr;
    const uint8_t parentIdAddr;
    const uint8_t distanceGWAddr;
    const uint8_t presentCompleteAddr;

    int16_t& myTransportWaitReady;
    uint8_t myParrent, oldMyParrent;
    bool isSendRouteParent, isNoGatewayMode, isUpdateTransportParam, isFindParentProcess;

    bool isSendAck, isPresentAck, isPresentComlete = true;

    inline void gateway_fail() {isNoGatewayMode = true; isUpdateTransportParam = false;}
    inline void no_present(){ _coreConfig.presentationSent = true;  _coreConfig.nodeRegistered = true; }
    void present_only_parent();
    void happy_node_mode();
    void update_Happy_transport();
    void check_parent();
    void find_parent_process();

public:
    CHappyNode(uint8_t idAddr_, uint8_t parentIdAddr_, uint8_t distanceGWAddr_, int16_t &myTransportWaitReady_) : idAddr(idAddr_), parentIdAddr(parentIdAddr_), distanceGWAddr(distanceGWAddr_), myTransportWaitReady(myTransportWaitReady_) {}
    void init();
    void config();
    void sendSketchInform(const char *, const char *);
    void perform(const uint8_t, const mysensors_sensor_t, const char *);
    bool checkAck(const MyMessage &);
    void run();
    void sendMsg(MyMessage &);
};

extern CHappyNode happyNode;

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
    if (!isSketchPresent) isPresentComlete = false;
}

void CHappyNode::perform(const uint8_t childSensorId, const mysensors_sensor_t sensorType, const char *description){
    isPresentAck = false;
    for (int i = 0; i < maxPresentTry; i++){
        present(childSensorId, sensorType, description, 1);
        wait(2500, C_PRESENTATION, sensorType);
        CORE_DEBUG(PSTR("MyS: TEST WAIT AFTER PRESENT SENSOR\n"));
        if (isPresentAck)
            break;
        _transportSM.failedUplinkTransmissions = 0;
        if (i < maxPresentTry - 1)
        {
            sleep(1500);
            wait(50);
        }
    }
    if (!isPresentAck) isPresentComlete = false;
}

bool CHappyNode::checkAck(const MyMessage &message){
    if (mGetCommand(message) == 0){
        isPresentAck = true;
        CORE_DEBUG(PSTR("MyS: !!!ACK OF THE PRESENTATION IN THE FUNCTION RECEIVE RECEIVED!!!\n"));
        return true;
    }
    if (mGetCommand(message) == C_SET && message.isEcho()) {
        CORE_DEBUG(PSTR("MyS: !!!ACK OF THE SEND RECEIVED!!!\n"));
        isSendAck = true;
        return true;
    }
    return false;
}

void CHappyNode::init(){
    if (hwReadConfig(EEPROM_NODE_ID_ADDRESS) == 0){
        hwWriteConfig(EEPROM_NODE_ID_ADDRESS, 255);
    }
    if (loadState(idAddr) == 0){
        saveState(idAddr, 255);
    }
    CORE_DEBUG(PSTR("MYS: EEPROM NODE ID: %d\n"), hwReadConfig(EEPROM_NODE_ID_ADDRESS));
    CORE_DEBUG(PSTR("MYS: USER MEMORY SECTOR NODE ID: %d\n"), loadState(idAddr));

    if (hwReadConfig(EEPROM_NODE_ID_ADDRESS) == 255){
        myTransportWaitReady = 0;
        saveState(presentCompleteAddr, 0);
    }
    else{
        isPresentComlete = loadState(presentCompleteAddr);
        if (isPresentComlete) no_present();
        myTransportWaitReady = 10000;
    }
    CORE_DEBUG(PSTR("MY_TRANSPORT_WAIT_MS: %d\n"), myTransportWaitReady);
}

void CHappyNode::config() {
    if (myTransportWaitReady == 0){
        uint8_t myid = getNodeId();
        saveState(idAddr, myid);
        myParrent = _transportConfig.parentNodeId;
        oldMyParrent = myParrent;
        saveState(parentIdAddr, myParrent);
        saveState(distanceGWAddr, _transportConfig.distanceGW);
    }
    else {
        uint8_t myid = getNodeId();
        if (myid != loadState(idAddr)){
            saveState(idAddr, myid);
        }
        if (isTransportReady()){
            myParrent = _transportConfig.parentNodeId;
            if (myParrent != loadState(parentIdAddr)){
                oldMyParrent = myParrent;
                saveState(parentIdAddr, myParrent);
            }
            if (_transportConfig.distanceGW != loadState(distanceGWAddr)){
                saveState(distanceGWAddr, _transportConfig.distanceGW);
            }
            if (isPresentComlete) {
                present_only_parent();
            }
            else {
                presentNode();
                saveState(presentCompleteAddr, isPresentComlete);

            }
        }
        else{
            no_present();
            // flag_fcount = 1;
            // err_delivery_beat = 5;
            _transportConfig.nodeId = loadState(idAddr); //myid;
            _transportConfig.parentNodeId = loadState(parentIdAddr);
            _transportConfig.distanceGW = loadState(distanceGWAddr);
            myParrent = _transportConfig.parentNodeId;
            happy_node_mode();
            gateway_fail();
        }
    }
}

void CHappyNode::present_only_parent(){
    if (oldMyParrent != myParrent){
        CORE_DEBUG(PSTR("MyS: SEND LITTLE PRESENT:) WITH PARENT ID\n"));
        if (_sendRoute(build(_msgTmp, 0, NODE_SENSOR_ID, C_INTERNAL, 6).set(myParrent))){
            isSendRouteParent = false;
            oldMyParrent = myParrent;
        }
        else{
            isSendRouteParent = true;
        }
    }
}

void CHappyNode::happy_node_mode() {
  _transportSM.findingParentNode = false;
  _transportSM.transportActive = true;
  _transportSM.uplinkOk = true;
  _transportSM.pingActive = false;
  transportSwitchSM(stReady);
  _transportSM.failureCounter = 0;
}

void CHappyNode::update_Happy_transport() {
    CORE_DEBUG(PSTR("MyS: UPDATE TRANSPORT CONFIGURATION\n"));
    myParrent = _transportConfig.parentNodeId;
    if (myParrent != loadState(parentIdAddr)){
        saveState(parentIdAddr, myParrent);
    }
    if (_transportConfig.distanceGW != loadState(distanceGWAddr)){
        saveState(distanceGWAddr, _transportConfig.distanceGW);
    }
    present_only_parent();
    wait(50);
    //nosleep = 0;
    isUpdateTransportParam = false;
}

void CHappyNode::run(){
    if (isUpdateTransportParam){
        update_Happy_transport();
    }
    if (isSendRouteParent){
        present_only_parent();
    }
    
    if (isTransportReady() == true) {
        if (!isNoGatewayMode) {
            if(isFindParentProcess){
                find_parent_process();
            }
        }
        else{
            check_parent();
        }
    }

    if (_transportSM.failureCounter > 0) {
        _transportConfig.parentNodeId = loadState(parentIdAddr);
        _transportConfig.nodeId = loadState(idAddr); //myid;
        _transportConfig.distanceGW = loadState(distanceGWAddr);
        myParrent = _transportConfig.parentNodeId;
        //nosleep = 0;
        //flag_fcount = 1;
        //err_delivery_beat = 5;
        happy_node_mode();
        gateway_fail();
    }
}

void CHappyNode::check_parent(){
    _transportSM.findingParentNode = true;
    CORE_DEBUG(PSTR("MyS: SEND FIND PARENT REQUEST, WAIT RESPONSE\n"));
    _sendRoute(build(_msg, 255, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_REQUEST).set(""));
    wait(1500, C_INTERNAL, I_FIND_PARENT_RESPONSE);
    if (_msg.sensor == 255 && mGetCommand(_msg) == C_INTERNAL && _msg.type == I_FIND_PARENT_RESPONSE){
        CORE_DEBUG(PSTR("MyS: PARENT RESPONSE FOUND, FIND PARENT PROCESS\n"));
        transportSwitchSM(stParent);
        isNoGatewayMode = false;
        isFindParentProcess = true;
        //SLEEP_TIME_W = SLEEP_TIME;
        //problem_mode_count = 0;
    }
    else{
        _transportSM.findingParentNode = false;
        CORE_DEBUG(PSTR("MyS: PARENT RESPONSE NOT FOUND\n"));
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

void CHappyNode::find_parent_process(){
    isUpdateTransportParam = true;
    isFindParentProcess = false;
    CORE_DEBUG(PSTR("MyS: STANDART TRANSPORT MODE IS RESTORED\n"));
}

void CHappyNode::sendMsg(MyMessage& message){
    if (_transportConfig.parentNodeId == 0) {
        if (send(message)){
            //err_delivery_beat = 0;
            if (isNoGatewayMode){
                isNoGatewayMode = false;
                CORE_DEBUG(PSTR("MyS: NORMAL GATEWAY MODE\n"));
                //err_delivery_beat = 0;
            }
        }
        else {
            _transportSM.failedUplinkTransmissions = 0;
            // if (err_delivery_beat < 5)
            // {
            //     err_delivery_beat++;
            // }
            // if (err_delivery_beat == 4)
            // {
                if (!isNoGatewayMode){
                    gateway_fail();
                    CORE_DEBUG(PSTR("MyS: LOST GATEWAY MODE\n"));
                }
            // }
        }
    }
    else {
        isSendAck = false;
        send(message, true);
        wait(2500, C_SET, message.getType());
        if (isSendAck){
            // Ack_TL = 0;
            // err_delivery_beat = 0;
            if (isNoGatewayMode){             
                isNoGatewayMode = false;
                CORE_DEBUG(PSTR("MyS: NORMAL GATEWAY MODE\n"));
                //err_delivery_beat = 0;
            }
        }
        else {
            _transportSM.failedUplinkTransmissions = 0;
            // if (err_delivery_beat < 5)
            // {
            //     err_delivery_beat++;
            // }
            // if (err_delivery_beat == 4)
            // {
                if (!isNoGatewayMode)
                {
                    gateway_fail();
                    CORE_DEBUG(PSTR("MyS: LOST GATEWAY MODE\n"));
                }
            // }
        }
    }
 }