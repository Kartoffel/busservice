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
 * Class overview:
 *
 * CLASS busStation
 *  periodEmitBus
 *  income
 *  costs
 *  profit
 *  avgTicketPrice
 *
 *  emitBus(frequencyEmitBus)
 *  collectIncome(income)
 *
 * CLASS bus
 *  line
 *  seats
 *  standings
 *  passengersOnBoard
 *  position
 *
 *  move(position)
 *  collectIncome(passengers,position)
 *  useStop()
 *
 * CLASS driver
 *  timeOfDay
 *  trafficDelay
 *  maxVelocity
 *  driveBus()
 *
 * CLASS clock
 *  timeOfDay
 *
 *  increment()
 *
 * CLASS busStop
 *  waitingPassengers
 *  arrivingPassengers
 *  position
 *  weight
 *  busyMorning
 *  busyEvening
 *
 *  generatePassenger()
 *  removePassenger()
 *
 * CLASS passengerGenerator
 *  averageFrequency
 *  timeOfDay
 *
 *  generatePassenger()
 *
 * CLASS passenger
 *  waitingTime
 */
#ifndef BUSSERVICE_H_
#define BUSSERVICE_H_

extern int numOvertakes;
extern int waitTime;
extern int totalPassengers;
extern int seatAvailability;

extern const int maxTOD;
extern const float avgStopDistance;
extern const float lineLength;
extern const int numStops;

int numWaitingPassengers(int busStop);
bool removePassenger(int busStop);
void updateWaitingPassengers(void);
bool spawnPassenger(int busStop);
void spawnPassengers(void);
void updateTimeVariables(void);
void initializeModel(void);
void tick(void);
void printResults(void);

#endif
