#include "prog2_sr.h"
#include "string.h"

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

struct info
{
    struct pkt cur_package;
    int cur_checksum;
    int ACK;
    int wait_ACK;
    int SEQ;
    struct info * next;
    struct info * prev;
    int is_head;
    float end_time;
    int is_acked;
};

struct msg_buffer
{
    struct msg cur_message;
    struct msg_buffer * next;
};

struct info * A_side = NULL;
struct info * B_side = NULL;
struct info * cur_head = NULL;
struct info * B_buffer_head = NULL;
struct msg_buffer * cur_buffer_head = NULL;
struct msg_buffer * cur_buffer_tail = NULL;

static int WAITING_ACK = 1;
static int timer_interval = 20;
static int window_size = 8;
static int buffer_max_size = 50;
int next_seqnum;
int expected_seqnum;
int base;
int num_of_cur_packet;
int num_of_msg_in_buffer;
int num_of_saved_packet_in_receiver;
int fake_A_buffer_num;
int cur_interrupt_seq;

int cal_checksum(struct pkt input_package)
{
    int cur_checksum = 0;
    for(int i = 0; i < 20; i++){
        cur_checksum = cur_checksum + input_package.payload[i];
    }
    cur_checksum = cur_checksum + input_package.seqnum;
    cur_checksum = cur_checksum + input_package.acknum;
    return cur_checksum;
}

void print_msg(struct msg message){
    printf("check data: Message data is   :");
    for(int i = 0; i < 20; i++){
        printf("%c",message.data[i]);
    }
    printf("\n");
}

void print_payload(struct pkt packet){
    printf("check data: Payload content is:");
    for(int i = 0; i < 20; i++){
        printf("%c",packet.payload[i]);
    }
    printf("\n");
}


void print_msg_buffer(){
    struct msg_buffer * temp = cur_buffer_head;
    for(int i = 0; i < num_of_msg_in_buffer; i++){
        //printf("+==========================================");
        print_msg(temp->cur_message);
        temp = temp->next;
    }
    return;
}

void printout_seq_in_A(){
    struct info * temp = cur_head;
    printf("current packet number in window is: %d and they are: |", num_of_cur_packet);
    for(int i = 0; i < num_of_cur_packet; i++){
        printf(" %d ",temp->cur_package.seqnum);
        temp = temp->next;
    }
    printf("|\n");
    return;
}


void printout_seq_in_B(){
    struct info * temp = B_buffer_head;
    printf("current packet number in window is: %d and they are: |", num_of_saved_packet_in_receiver);
    for(int i = 0; i < num_of_saved_packet_in_receiver; i++){
        printf(" %d ",temp->cur_package.seqnum);
        temp = temp->next;
    }
    printf("|\n");
    return;
}

void append_msg_to_window(struct msg message)
{

    struct pkt transport_pkt;
    int temp_checksum;

    //memmove(transport_pkt.payload, message.data, 20);
    for(int i = 0; i < 20; i++){
        transport_pkt.payload[i] = message.data[i];
    }

    print_msg(message);
    print_payload(transport_pkt);

    transport_pkt.seqnum = next_seqnum;
    transport_pkt.acknum = next_seqnum;

    temp_checksum = cal_checksum(transport_pkt);
    transport_pkt.checksum = temp_checksum;

    //struct info * cur_state = (struct info *)malloc(sizeof(struct info));
    A_side->cur_checksum = temp_checksum;
    A_side->cur_package = transport_pkt;
    A_side->wait_ACK = WAITING_ACK;
    A_side->ACK = next_seqnum;
    A_side->SEQ = next_seqnum;

    A_side->end_time = time + timer_interval;
    //if(next_seqnum == base){
        starttimer(A_side->SEQ,timer_interval);
    //}
    next_seqnum = next_seqnum + 1;
    num_of_cur_packet = num_of_cur_packet + 1;
    fake_A_buffer_num = fake_A_buffer_num + 1;

    return;
}

/* called from layer 5, passed the data to be sent to other side */

int A_output(struct msg message)
{

    printf("-A_output(mst);\n");
    printf("-Sender gets message from layer5, sender outputs packet to layer3(network).\n");
    printf("-Initial state: seqnum is %d, acknum is %d\n", A_side->SEQ, A_side->ACK);
    print_msg(message);

    printf("next_seqnum now is: %d\n", next_seqnum);
    if(next_seqnum >= base + window_size){
            printf("-Sender side is full with next_seqnum: %d, base: %d.\n",next_seqnum,base);
            printf("-Check number of package in buffer, put this message into buffer if buffer is not full.\n");
            printf("-Current number of message in buffer: %d.\n",num_of_msg_in_buffer);
            if(num_of_msg_in_buffer < buffer_max_size)
            {
                cur_buffer_tail->cur_message = message;
                cur_buffer_tail->next =  (struct msg_buffer *)malloc(sizeof(struct msg_buffer));
                cur_buffer_tail = cur_buffer_tail->next;
                num_of_msg_in_buffer = num_of_msg_in_buffer + 1;
                printf("-Addded to buffer, now the number of messages in buffer is:%d\n",num_of_msg_in_buffer);
                //print_msg_buffer();
                return 0;
            }
            //print_msg_buffer();
            printf("-Both buffer and window are full, give up and exit.\n");
            exit(0);
            return 0;
    }

    append_msg_to_window(message);
    tolayer3(A, A_side->cur_package);

    printf("-next_seqnum becomes: %d\n", next_seqnum);
    printf("-Packet sent.\n");
    printf("-Current state: seqnum is %d, acknum is %d. Check cur_package: %d, %d\n", A_side->cur_package.seqnum, A_side->cur_package.acknum, cur_head->cur_package.seqnum, cur_head->cur_package.acknum);
    printf("-Current number of packets in window: %d\n", num_of_cur_packet);
    print_payload(A_side->cur_package);

    A_side->next = (struct info *)malloc(sizeof(struct info));

    A_side = A_side->next;

    //printf("==========CHECK:%d++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n",cur_head->cur_package.seqnum);
    printout_seq_in_A();
    return 0;
}

/* called from layer 3, when a packet arrives for layer 4 */
int A_input(struct pkt packet)
{
    printout_seq_in_A();
    printf("-A_input(pkt);\n");
    printf("-Sender side received acked packet.\n");
    struct info * prev_one, * match_one;
    int is_in_window = 0;
    match_one = cur_head;
    prev_one = cur_head;
    for(int i = 0; i < num_of_cur_packet; i++){
        //printf("==========CHECK acknum: %d================\n",match_one->cur_package.seqnum);
        if(match_one->cur_package.seqnum == packet.acknum){
            is_in_window = 1;
            match_one->is_acked = 1;
            break;
        }
        //prev_one = match_one;
        match_one = match_one->next;
    }

    if(is_in_window == 0){
        printf("Acknum may be corrupted and cannot be found in the window. Acknum: %d\n", packet.acknum);
        return 0;
    }

    if(match_one->wait_ACK != WAITING_ACK){
        printf("-Sender side is not wariting for this ACK with acknum %d, cannot input acked packet.\n", packet.acknum);
        return 0;
    }

    if(match_one->SEQ != packet.acknum){
        printf("-acked num is not consistent with sender's current seqnum, ignored. \n");
        printf("-Sender's seqnum: %d, packet acknum: %d. \n", match_one->SEQ, packet.acknum);
        return 0;
    }

    printf("-Acked num received: %d, sender no longer waiting for acknum.\n", packet.acknum);
    match_one->wait_ACK = 0;
    stoptimer(packet.seqnum);

    if(base != packet.seqnum){
        printf("-Base number is not acked, keep waiting and do not move the window with head seqnum: %d.\n", packet.seqnum);
        //return 0;
    }
    while(cur_head->is_acked == 1 && num_of_cur_packet != 0){
        base = base + 1;
        cur_head = cur_head->next;
        num_of_cur_packet = num_of_cur_packet - 1;
    }
    if(num_of_cur_packet == 0)
    {
        cur_head = (struct info *)malloc(sizeof(struct info));
        A_side = cur_head;
    }

    while(num_of_msg_in_buffer > 0 && num_of_cur_packet < window_size){
        printf("-There are/is %d message in buffer, move the first one to window.\n",num_of_msg_in_buffer);
        printout_seq_in_A();
        print_msg_buffer();
        append_msg_to_window(cur_buffer_head->cur_message);
        A_side->end_time = time + timer_interval;
        tolayer3(A, A_side->cur_package);
        print_payload(A_side->cur_package);
        A_side->next = (struct info *)malloc(sizeof(struct info));
        A_side = A_side->next;


        printout_seq_in_A();
        cur_buffer_head = cur_buffer_head->next;

        num_of_msg_in_buffer = num_of_msg_in_buffer - 1;
        printf("-First message in buffer moved, now number of message in buffer is : %d\n", num_of_msg_in_buffer);

    }
    if(num_of_msg_in_buffer == 0)
    {
        cur_buffer_head = (struct msg_buffer *)malloc(sizeof(struct msg_buffer));
        cur_buffer_tail = cur_buffer_head;
    }




        //starttimer(A,timer_interval);

    //printout_seq_in_A();
    //printf("==========CHECK:%d++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n",cur_head->cur_package.seqnum);
    printf("\n================Ack number: %d================\n",packet.acknum);

    return 0;
}

/* called when A's timer goes off */

int A_timerinterrupt() {
    printf("-A_timerinterrupt();\n");
    struct info * temp = cur_head;
    printf("Interrupts with seqnum %d: \n", cur_interrupt_seq);
    for(int i = 0; i < num_of_cur_packet;i++){
        if(temp->cur_package.seqnum == cur_interrupt_seq){
            //stoptimer(cur_interrupt_seq);
            starttimer(cur_interrupt_seq,timer_interval);
            printf("-Resend packet with seqnum: %d, acknum: %d, payload: %sf\n",temp->cur_package.seqnum,temp->cur_package.acknum,temp->cur_package.payload);
            tolayer3(A,temp->cur_package);
            return 0;
        }
        temp = temp->next;
    }
    printf("-Time interrupts, but sender does not wait for ack, stop.\n");
    return 0;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
int A_init() {
    A_side = (struct info *)malloc(sizeof(struct info));
    A_side->SEQ = 0;
    A_side->ACK = 0;
    A_side->wait_ACK = 0;
    next_seqnum = 1;
    base = 1;
    num_of_cur_packet = 0;
    cur_head = A_side;

    cur_buffer_tail = (struct msg_buffer *)malloc(sizeof(struct msg_buffer));
    //cur_buffer_head->next = (struct msg_buffer *)malloc(sizeof(struct msg_buffer));
    cur_buffer_head = cur_buffer_tail;
    num_of_msg_in_buffer = 0;
    fake_A_buffer_num = 0;
    printf("-Initialized A(sender side): seqnum is %d, acknum is %d, wait_ack is %d\n", A_side->SEQ, A_side->ACK,A_side->wait_ACK = 0);
    return 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
int B_input(struct pkt packet)
{

    printf("-B_input(pkt);\n");
    printf("-Receive packet.\n");
    print_payload(packet);
    int temp_checksum;
    temp_checksum = cal_checksum(packet);
    if(temp_checksum != packet.checksum){
        printf("-Checksum corrupted. Current checksum: %d, packet checksum: %d.\n", temp_checksum, packet.checksum);
        return 0;
    }

    struct pkt back_pkt;
    back_pkt.acknum = packet.seqnum;
    back_pkt.seqnum = packet.seqnum;
    back_pkt.checksum = cal_checksum(back_pkt);

    if(expected_seqnum < packet.seqnum){
        struct info * iter = B_buffer_head;
        for(int i = 0; i < num_of_saved_packet_in_receiver; i++){
            if(iter->cur_package.seqnum == packet.seqnum){
                printf("-This packet already bufferred with seqnum: %d, ignored\n", packet.seqnum);
                return 0;
            }
            if(i != num_of_saved_packet_in_receiver - 1){
                iter = iter->next;
            }
        }
        iter = B_buffer_head;
        struct info * B_temp = (struct info *)malloc(sizeof(struct info));
        B_temp->cur_package = packet;
        B_temp->SEQ = packet.seqnum;
        B_temp->ACK = packet.acknum;
        B_temp->next = NULL;
        tolayer3(B,back_pkt);
        printf("-Receiver side send acked number by packet back to sender. acknum: %d.\n", back_pkt.acknum);

        for(int i = 0; i < num_of_saved_packet_in_receiver; i++){
            if(i == 0 && packet.seqnum < iter->cur_package.seqnum){
                B_temp->next = iter;
                B_buffer_head = B_temp;
            }else if(i == num_of_saved_packet_in_receiver - 1 && packet.seqnum > iter->cur_package.seqnum){
                //iter->next = B_temp;
                B_side = B_temp;
                B_side->next = (struct info *)malloc(sizeof(struct info));
                B_side = B_side->next;
            }else if(packet.seqnum > iter->cur_package.seqnum && packet.seqnum < iter->next->cur_package.seqnum){
                B_temp->next = iter->next;
                iter->next = B_temp;
            }else{
                iter = iter->next;
            }
            num_of_saved_packet_in_receiver = num_of_saved_packet_in_receiver + 1;
        }
        return 0;
    }

    if(expected_seqnum > packet.seqnum){
        tolayer3(B,back_pkt);
        printf("-Receiver side already receives the packet, just resend acked number by packet back to sender. acknum: %d.\n", back_pkt.acknum);
        return 0;
    }
    expected_seqnum = expected_seqnum + 1;
    printf("-Receiver receives packet successfully with seqnum %d.\n", packet.seqnum);
    printf("-Send the packet to layer file with payload content.\n");
    print_payload(packet);
    tolayer5(B, packet.payload);

    printf("-Receiver side send acked number by packet back to sender. acknum: %d.\n", back_pkt.acknum);
    tolayer3(B,back_pkt);

    struct info * iter = B_buffer_head;
    while(expected_seqnum == iter->cur_package.seqnum && num_of_saved_packet_in_receiver > 0){
        printf("-Begin to send packet in buffer\n");
        printf("-Send the packet to layer file with payload content.\n");
        print_payload(iter->cur_package);
        tolayer5(B, packet.payload);
        expected_seqnum = expected_seqnum + 1;
        num_of_saved_packet_in_receiver = num_of_saved_packet_in_receiver - 1;
        if(num_of_saved_packet_in_receiver == 0){
            B_side = (struct info *)malloc(sizeof(struct info));
            B_buffer_head = B_side;
            break;
        }
        if(num_of_saved_packet_in_receiver != 0){
            B_buffer_head = B_buffer_head->next;
            iter = iter->next;
        }
    }

    return 0;
}

/* called when B's timer goes off */
int B_timerinterrupt() {return 0;}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
int B_init()
{
    B_side = (struct info *)malloc(sizeof(struct info));
    B_side->SEQ = 0;
    B_side->ACK = 0;
    B_side->wait_ACK = 0;
    B_buffer_head = B_side;
    expected_seqnum = 1;
    num_of_saved_packet_in_receiver = 0;
    printf("-Initialized B(receiver side): seqnum is %d, acknum is %d, wait_ack is %d\n", B_side->SEQ, B_side->ACK, B_side->wait_ACK = 0);
    return 0;
}

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

int main()
{
  struct event * eventptr;
  struct msg msg2give;
  struct pkt pkt2give;

  int i, j;

  init();
  A_init();
  B_init();

  for (;; ) {
    eventptr = evlist; /* get next event to simulate */
    if (NULL == eventptr) {
      goto terminate;
    }
    evlist = evlist->next; /* remove this event from event list */
    if (evlist != NULL) {
      evlist->prev = NULL;
    }
    if (TRACE >= 2) {
      printf("\nEVENT time: %f,", eventptr->evtime);
      printf("  type: %d", eventptr->evtype);
      if (eventptr->evtype == 0) {
        printf(", timerinterrupt  ");
      } else if (eventptr->evtype == 1) {
        printf(", fromlayer5 ");
      } else {
        printf(", fromlayer3 ");
      }
      printf(" entity: %d\n", eventptr->eventity);
    }
    time = eventptr->evtime; /* update time to next event time */
    if (nsim == nsimmax) {
      break; /* all done with simulation */
    }
    if (eventptr->evtype == FROM_LAYER5) {
      generate_next_arrival(); /* set up future arrival */
      /* fill in msg to give with string of same letter */
      j = nsim % 26;
      for (i = 0; i < 20; i++) {
        msg2give.data[i] = 97 + j;
      }
      if (TRACE > 2) {
        printf("          MAINLOOP: data given to student: ");
        for (i = 0; i < 20; i++) {
          printf("%c", msg2give.data[i]);
        }
        printf("\n");
      }
      nsim++;
      if (eventptr->eventity == A) {
        A_output(msg2give);
      }
    } else if (eventptr->evtype == FROM_LAYER3) {
      pkt2give.seqnum = eventptr->pktptr->seqnum;
      pkt2give.acknum = eventptr->pktptr->acknum;
      pkt2give.checksum = eventptr->pktptr->checksum;
      for (i = 0; i < 20; i++) {
        pkt2give.payload[i] = eventptr->pktptr->payload[i];
      }
      if (eventptr->eventity == A) { /* deliver packet by calling */
        A_input(pkt2give);           /* appropriate entity */
      } else {
        B_input(pkt2give);
      }
      free(eventptr->pktptr); /* free the memory for packet */
    } else if (eventptr->evtype == TIMER_INTERRUPT) {
      printf("Arrives TIMER_INTERRUPT event with packet: %d\n", eventptr->pktptr->seqnum);
      printf("Check whether time is valid, start time: %f, current time: %f.\n", eventptr->evtime-timer_interval, time);
      //if (eventptr->eventity == A) {
        cur_interrupt_seq = eventptr->eventity;
        A_timerinterrupt();
      //} else {
      //  B_timerinterrupt();
      //}
    } else {
      printf("INTERNAL PANIC: unknown event type \n");
    }
    free(eventptr);
  }
  return 0;

terminate:
  printf(
    " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
    time, nsim);
  return 0;
}

void init() /* initialize the simulator */
{
  int i;
  float sum, avg;

  printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
  printf("Enter the number of messages to simulate: ");
  scanf("%d", &nsimmax);
  printf("Enter  packet loss probability [enter 0.0 for no loss]:");
  scanf("%f", &lossprob);
  printf("Enter packet corruption probability [0.0 for no corruption]:");
  scanf("%f", &corruptprob);
  printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
  scanf("%f", &lambda);
  printf("Enter TRACE:");
  scanf("%d", &TRACE);

  srand(rand_seed); /* init random number generator */
  sum = 0.0;   /* test random number generator for students */
  for (i = 0; i < 1000; i++) {
    sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
  }
  avg = sum / 1000.0;
  if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n");
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
  }

  ntolayer3 = 0;
  nlost = 0;
  ncorrupt = 0;

  time = 0.0;              /* initialize time to 0.0 */
  generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
  double mmm = INT_MAX;         /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                      /* individual students may need to change mmm */
  x = rand_r(&rand_seed) / mmm; /* x should be uniform in [0,1] */
  return x;
}

/************ EVENT HANDLINE ROUTINES ****************/
/*  The next set of routines handle the event list   */
/*****************************************************/
void generate_next_arrival()
{
  double x;
  struct event * evptr;

  if (TRACE > 2) {
    printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
  }

  x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
  /* having mean of lambda        */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime = time + x;
  evptr->evtype = FROM_LAYER5;
  if (BIDIRECTIONAL && (jimsrand() > 0.5)) {
    evptr->eventity = B;
  } else {
    evptr->eventity = A;
  }
  insertevent(evptr);
}

void insertevent(struct event * p)
{
  struct event * q, * qold;

  if (TRACE > 2) {
    printf("            INSERTEVENT: time is %lf\n", time);
    printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
  }
  q = evlist;      /* q points to header of list in which p struct inserted */
  if (NULL == q) { /* list is empty */
    evlist = p;
    p->next = NULL;
    p->prev = NULL;
  } else {
    for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next) {
      qold = q;
    }
    if (NULL == q) { /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    } else if (q == evlist) { /* front of list */
      p->next = evlist;
      p->prev = NULL;
      p->next->prev = p;
      evlist = p;
    } else { /* middle of list */
      p->next = q;
      p->prev = q->prev;
      q->prev->next = p;
      q->prev = p;
    }
  }
}



void printevlist()
{
  struct event * q;
  printf("--------------\nEvent List Follows:\n");
  for (q = evlist; q != NULL; q = q->next) {
    printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
      q->eventity);
  }
  printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
{
  struct event * q;

  if (TRACE > 2) {
    printf("          STOP TIMER: stopping timer at %f\n", time);
  }

  for (q = evlist; q != NULL; q = q->next) {
    if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
      /* remove this event */
      if (NULL == q->next && NULL == q->prev) {
        evlist = NULL;              /* remove first and only event on list */
      } else if (NULL == q->next) { /* end of list - there is one in front */
        q->prev->next = NULL;
      } else if (q == evlist) { /* front of list - there must be event after */
        q->next->prev = NULL;
        evlist = q->next;
      } else { /* middle of list */
        q->next->prev = q->prev;
        q->prev->next = q->next;
      }
      free(q);
      return;
    }
  }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB, float increment)
{
  struct event * q;
  struct event * evptr;

  if (TRACE > 2) {
    printf("          START TIMER: starting timer at %f\n", time);
  }

  /* be nice: check to see if timer is already started, if so, then  warn */
  for (q = evlist; q != NULL; q = q->next) {
    if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
      printf("Warning: attempt to start a timer that is already started\n");
      //return;
    }
  }

  /* create future event for when timer goes off */
  evptr = (struct event *)malloc(sizeof(struct event));
  //A_side->end_time = time;
  evptr->evtime = time + increment;
  evptr->evtype = TIMER_INTERRUPT;
  evptr->eventity = AorB;
  evptr->pktptr = (struct pkt *)malloc(sizeof(struct pkt));

  evptr->pktptr->seqnum = A_side->cur_package.seqnum;
  evptr->pktptr->acknum = A_side->cur_package.acknum;

  insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
  struct pkt * mypktptr;
  struct event * evptr, * q;
  float lastime, x;
  int i;

  ntolayer3++;

  /* simulate losses: */
  if (jimsrand() < lossprob) {
    nlost++;
    if (TRACE > 0) {
      printf("          TOLAYER3: packet being lost\n");
    }
    return;
  }

  /*
   * make a copy of the packet student just gave me since he/she may decide
   * to do something with the packet after we return back to him/her
   */

  mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
  mypktptr->seqnum = packet.seqnum;
  mypktptr->acknum = packet.acknum;
  mypktptr->checksum = packet.checksum;
  for (i = 0; i < 20; ++i) {
    mypktptr->payload[i] = packet.payload[i];
  }
  if (TRACE > 2) {
    printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
      mypktptr->acknum, mypktptr->checksum);
    for (i = 0; i < 20; ++i) {
      printf("%c", mypktptr->payload[i]);
    }
    printf("\n");
  }

  /* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
  evptr->eventity = (AorB + 1) & 1; /* event occurs at other entity */
  evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */

  /*
   * finally, compute the arrival time of packet at the other end.
   * medium can not reorder, so make sure packet arrives between 1 and 10
   * time units after the latest arrival time of packets
   * currently in the medium on their way to the destination
   */

  lastime = time;
  for (q = evlist; q != NULL; q = q->next) {
    if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity)) {
      lastime = q->evtime;
    }
  }
  evptr->evtime = lastime + 1 + 9 * jimsrand();

  /* simulate corruption: */
  if (jimsrand() < corruptprob) {
    ncorrupt++;
    if ((x = jimsrand()) < .75) {
      mypktptr->payload[0] = 'Z'; /* corrupt payload */
    } else if (x < .875) {
      mypktptr->seqnum = 999999;
    } else {
      mypktptr->acknum = 999999;
    }
    if (TRACE > 0) {
      printf("          TOLAYER3: packet being corrupted\n");
    }
  }

  if (TRACE > 2) {
    printf("          TOLAYER3: scheduling arrival on other side\n");
  }
  insertevent(evptr);
}

void tolayer5(int AorB, const char * datasent)
{
  (void)AorB;
  int i;
  if (TRACE > 2) {
    printf("          TOLAYER5: data received: ");
    for (i = 0; i < 20; i++) {
      printf("%c", datasent[i]);
    }
    printf("\n");
  }
}
