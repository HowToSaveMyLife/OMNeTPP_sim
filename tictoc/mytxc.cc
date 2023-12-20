#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "mytictoc_m.h"

using namespace omnetpp;

// 第一个网络模拟的是具有丢包、定时重发的中继转发
// class myTic_1是第一个网络的中转站
class myTic_1 : public cSimpleModule
{
  private:
    simtime_t timeout;
    MyTictocMsg *timeoutEvent;
    MyTictocMsg *message;

  public:
    myTic_1();
    virtual ~myTic_1();
    
  protected:
    virtual void sendCopyOf(MyTictocMsg *msg);
    virtual MyTictocMsg *generateMessage(int source, int dest, char content);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(myTic_1);

myTic_1::myTic_1()
{
    timeoutEvent = nullptr;
}

myTic_1::~myTic_1()
{
    cancelAndDelete(timeoutEvent);
}

void myTic_1::initialize()
{
    timeout = 1.0;
    timeoutEvent = generateMessage(0, 0, 'E');

    EV << "Sending initial message\n";
    message = generateMessage(0, intuniform(0, gateSize("gate")-1), 'M');
    sendCopyOf(message);
    scheduleAt(simTime()+timeout, timeoutEvent);
}

void myTic_1::handleMessage(cMessage *msg)
{
    MyTictocMsg *ttmsg = check_and_cast<MyTictocMsg *>(msg);
    if (ttmsg->getMsg() == 'E') {
        EV << "Msg lost\n";
        sendCopyOf(message);
        scheduleAt(simTime()+timeout, timeoutEvent);
    }
    else {
        EV << "Timer cancelled.\n";
        cancelEvent(timeoutEvent);
        message = ttmsg;

        sendCopyOf(message);
        scheduleAt(simTime()+timeout, timeoutEvent);
    }
}

MyTictocMsg *myTic_1::generateMessage(int source, int dest, char content)
{
    char msgname[20];
    sprintf(msgname, "tic-%d-to-%d", source, dest);
    MyTictocMsg *msg = new MyTictocMsg(msgname);
    msg->setSource(source);
    msg->setDestination(dest);
    msg->setMsg(content);
    return msg;
}

void myTic_1::sendCopyOf(MyTictocMsg *msg)
{
    MyTictocMsg *copy = generateMessage(msg->getSource(), msg->getDestination(), msg->getMsg());
    send(copy, "gate$o", copy->getDestination());
}

// class myToc_1第一个网络的用户端
class myToc_1 : public cSimpleModule
{
  protected:
    virtual MyTictocMsg *generateMessage(int source, int dest, char content);
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(myToc_1);

MyTictocMsg *myToc_1::generateMessage(int source, int dest, char content)
{
    char msgname[20];
    sprintf(msgname, "toc-%d-to-%d", source, dest);
    MyTictocMsg *msg = new MyTictocMsg(msgname);
    msg->setSource(source);
    msg->setDestination(dest);
    msg->setMsg(content);
    return msg;
}

void myToc_1::handleMessage(cMessage *msg)
{
    MyTictocMsg *ttmsg = check_and_cast<MyTictocMsg *>(msg);
    if (uniform(0, 1) < 0.1) {
        EV << "\"Losing\" message.\n";
        bubble("message lost");
        delete ttmsg;
    }
    else {
        EV << "Sending back same message as acknowledgement.\n";
        int n = getVectorSize();
        int index = getIndex();
        int dest = intuniform(0, getVectorSize()-1);
        while (dest == index)
        {
            dest = intuniform(0, getVectorSize()-1);
        }
        delete ttmsg;
        msg = generateMessage(index, dest, 'M');
        send(msg, "gate$o", 0);
    }
}

//
//
// 第二个网络模拟是单向链路，没有丢包，没有定时重发，但具有延迟
class myTxc_0 : public cSimpleModule
{
  private:
    long numSent;
    long numReceived;


  protected:
    virtual MyTictocMsg *generateMessage();
    virtual void forwardMessage(MyTictocMsg *msg);
    virtual void refreshDisplay() const override;

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(myTxc_0);

void myTxc_0::initialize()
{
    numSent = 0;
    numReceived = 0;
    WATCH(numSent);
    WATCH(numReceived);

    if (getIndex() == 0) {
        MyTictocMsg *msg = generateMessage();
        numSent++;
        scheduleAt(0.0, msg);
    }
}

void myTxc_0::handleMessage(cMessage *msg)
{
    MyTictocMsg *ttmsg = check_and_cast<MyTictocMsg *>(msg);

    if (ttmsg->getMsg() == 'M') {
        ttmsg->setMsg('S');
        simtime_t delay = par("delayTime");
        scheduleAt(simTime()+delay, ttmsg);
    }else if (ttmsg->getDestination() == getIndex()) {
        ttmsg->setMsg('M');
        int hopcount = ttmsg->getHopCount();
        EV << "Message " << ttmsg << " arrived after " << hopcount << " hops.\n";
        numReceived++;
        delete ttmsg;
        bubble("ARRIVED, starting new one!");

        EV << "Generating another message: ";
        MyTictocMsg *newmsg = generateMessage();
        EV << newmsg << endl;
        forwardMessage(newmsg);
        numSent++;
    }
    else {
        ttmsg->setMsg('M');
        forwardMessage(ttmsg);
    }
}

MyTictocMsg *myTxc_0::generateMessage()
{
    int src = getIndex();
    int n = getVectorSize();
    int dest = intuniform(0, n-1);
    while (dest == src)
    {
        dest = intuniform(0, n-1);
    } 

    char msgname[20];
    sprintf(msgname, "tic-%d-to-%d", src, dest);

    MyTictocMsg *msg = new MyTictocMsg(msgname);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

void myTxc_0::forwardMessage(MyTictocMsg *msg)
{
    msg->setHopCount(msg->getHopCount()+1);

    int n = getIndex();

    EV << "Forwarding message " << msg << "on" << n << "\n";
    send(msg, "out");
}

void myTxc_0::refreshDisplay() const
{
    char buf[40];
    sprintf(buf, "rcvd: %ld sent: %ld", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

//
//
// 第三个网络仿照tictoc17，仅更改了网络节点之间的关系和发送包的数据
class myTxc_3 : public cSimpleModule
{
  private:
    simsignal_t arrivalSignal;

  protected:
    virtual MyTictocMsg *generateMessage();
    virtual void forwardMessage(MyTictocMsg *msg);

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(myTxc_3);

void myTxc_3::initialize()
{
    arrivalSignal = registerSignal("arrival");

    if (getIndex() == 0) {
        MyTictocMsg *msg = generateMessage();
        scheduleAt(0.0, msg);
    }
}

void myTxc_3::handleMessage(cMessage *msg)
{
    MyTictocMsg *ttmsg = check_and_cast<MyTictocMsg *>(msg);

    if (ttmsg->getDestination() == getIndex()) {
        int hopcount = ttmsg->getHopCount();
        emit(arrivalSignal, hopcount);

        if (hasGUI()) {
            char label[50];

            sprintf(label, "last hopCount = %d", hopcount);

            cCanvas *canvas = getParentModule()->getCanvas();
            cTextFigure *textFigure = check_and_cast<cTextFigure*>(canvas->getFigure("lasthopcount"));

            textFigure->setText(label);
        }

        EV << "Message " << ttmsg << " arrived after " << hopcount << " hops.\n";
        bubble("ARRIVED, starting new one!");

        delete ttmsg;

        EV << "Generating another message: ";
        MyTictocMsg *newmsg = generateMessage();
        EV << newmsg << endl;
        forwardMessage(newmsg);
    }
    else {
        forwardMessage(ttmsg);
    }
}

MyTictocMsg *myTxc_3::generateMessage()
{
    int src = getIndex();
    int n = getVectorSize();
    int dest = intuniform(0, n-1);
    while (dest == src)
    {
        dest = intuniform(0, n-1);
    } 

    char msgname[20];
    sprintf(msgname, "tic-%d-to-%d", src, dest);

    MyTictocMsg *msg = new MyTictocMsg(msgname);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

void myTxc_3::forwardMessage(MyTictocMsg *msg)
{
    msg->setHopCount(msg->getHopCount()+1);

    int n = gateSize("gate");
    int k = intuniform(0, n-1);

    EV << "Forwarding message " << msg << " on gate[" << k << "]\n";
    send(msg, "gate$o", k);
}