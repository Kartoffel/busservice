/* BUS SERVICE MODEL
 *
 * INPUT:
 *  Bus frequency
 *
 * OUTPUT:
 *  Average waiting time passengers
 *  Cost for company
 *  Overtake occurences
 *  Seat availability
 *
 */
#ifndef BUSSERVICE_H_
#define BUSSERVICE_H_

//#define DEBUG

#define maxWaitingPassengers 15
#define maxBuses 50
#define avgStopDistance 631
#define numStops 34
#define lineLength numStops * avgStopDistance
#define maxTOD 86400

extern unsigned int numOvertakes;
extern unsigned int waitTime;
extern unsigned int totalPassengers;
extern unsigned long totalAvailability;
extern unsigned int totalAvailNum;
extern unsigned int totalBuses;

extern int lastLine;

extern struct cBus *buses[maxBuses];
extern struct cDriver driver;
extern struct cBusStop busStops[numStops];
extern struct cBusStopProperties busStopProps[numStops];
extern struct cPassengerGenerator passengerGenerator;
extern struct cClock clk;

int numBuses(void);
bool emitBus(int busLine);
bool removeBus(int index);
int atStop(float curPos);
bool otherBusAtStop(float curPos, int curBus);
struct cBusStopRef nextStop(float curPos);
int exitingPassengers(int curStop);
void updateBuses(void);
int numWaitingPassengers(int busStop);
bool removePassenger(int busStop);
void updateWaitingPassengers(void);
bool spawnPassenger(int busStop);
void spawnPassengers(void);
void updateTimeVariables(void);
void initializeModel(void);
void tick(void);
void cleanupModel(void);
void printResults(void);
int main(void);
#endif
