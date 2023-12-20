simple myTic_1
{
    parameters:
        @display("i=,cyan");
        @display("i=block/routing");
    gates:
        inout gate[];
}

simple myToc_1
{
    parameters:
        @display("i=,cyan");
        @display("i=block/routing");
    gates:
        inout gate[];
}

simple myTxc_0
{
    parameters:
        volatile double delayTime @unit(s);   // delay before sending back message
        @display("i=,gold");
    gates:
        input in;
        output out;
}

simple myTxc_3
{
    parameters:
        @display("i=,cyan");
        @signal[arrival](type="long");
        @statistic[hopCount](title="hop count"; source="arrival"; record=vector,stats; interpolationmode=none);
        @display("i=block/routing");
    gates:
        inout gate[];
}

network myTictoc
{
    parameters:
        int n @prompt("Number of client") = default(2);
        @figure[description](type=text; pos=5,20; font=,,bold; 
        	text="Random routing example - displaying last hop count");
        @figure[lasthopcount](type=text; pos=5,35; text="last hopCount: N/A");
    types:
        channel Channel extends ned.DelayChannel
        {
            delay = 100ms;
        }
    submodules:
        tic_1: myTic_1;
        toc_1[n]: myToc_1;
        tic_0[6]: myTxc_0;
        tic_3[6]: myTxc_3;
    connections:
        for i = 1..n{
            tic_1.gate++ <--> Channel <--> toc_1[i-1].gate++;
        }

        for j = 0..4{
            tic_0[j].out --> Channel --> tic_0[j+1].in;
        }
        tic_0[5].out --> Channel --> tic_0[0].in;
        
        tic_3[0].gate++ <--> Channel <--> tic_3[1].gate++;
        tic_3[1].gate++ <--> Channel <--> tic_3[2].gate++;
        tic_3[2].gate++ <--> Channel <--> tic_3[3].gate++;
        tic_3[2].gate++ <--> Channel <--> tic_3[4].gate++;
        tic_3[4].gate++ <--> Channel <--> tic_3[5].gate++;
}
