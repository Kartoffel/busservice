#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "busservice.h"

unsigned int    numOvertakes        = 0;
unsigned int    waitTime            = 0; // Seconds passengers have waited
unsigned int    totalPassengers     = 0; // Passengers who took the bus
unsigned long   totalAvailability   = 0; // % of seated passengers / total
unsigned int    totalAvailNum       = 0; // Used for averaging
unsigned int    totalBuses          = 0;

int         lastLine        = 1;

struct cBusStation {
    int periodEmitBus = 300;
    float busDepartureCost = 0.0;
    float income = 0.0;
    float costs = 0.0;
    float profit = 0.0;
    const float avgTicketPrice = 1.65;
} busStation;

struct cBus {
    int line; // Line 0 or 1
    int seats; // Line 0: 33, 1: 25
    int standings; // Line 0: 105, 1: 74
    int passengersOnBoard;
    float position;
};
struct cBus *buses[maxBuses];

struct cDriver {
    const float maxVelocity = 14; // m/s
    float trafficDelay = 0; // % of maxVelocity to use
} driver;

struct cPassenger {
    int waitingTime = 0;
};

struct cBusStop {
    float position = 0.0;
    struct cPassenger *waitingPassengers[maxWaitingPassengers];
    float weight = 10;
    bool busyMorning = false;
    bool busyEvening = false;
};
struct cBusStop busStops[numStops];

// Used as a reference
struct cBusStopRef {
    int index;
    float position;
};

struct cPassengerGenerator {
    int averagePeriod;
} passengerGenerator;

struct cClock {
    int timeOfDay = 1; // 1-86400
} clock;

// Number of buses being driven
int numBuses(void) {
    int number = 0;
    for (int i = 0; i < maxBuses; i++){
        if (buses[i] != NULL){
            number++;
        }
    }
    return number;
}

// Emit a new bus from the bus station
bool emitBus(int busLine) {
    if (numBuses() >= maxBuses) {
        return false;
    }

    for (int i = 0; i < maxBuses; i++){
        if (buses[i] == NULL) {
            buses[i] = (struct cBus*) malloc(sizeof(cBus));
            buses[i]->line = busLine;
            buses[i]->passengersOnBoard = 0;
            buses[i]->position = 0.0;

            if (busLine == 0) {
                buses[i]->seats = 33;
                buses[i]->standings = 105;
            } else if (busLine == 1) {
                buses[i]->seats = 25;
                buses[i]->standings = 74;
            }
            return true;
        }
    }
    return false;
}

bool removeBus(int index) {
    free(buses[index]);
    buses[index] = NULL;
#ifdef DEBUG
    printf("%d: Removed bus %d\n", clock.timeOfDay, index);
#endif
    return true;
}

// returns index of stop if at stop, otherwise -1
int atStop(float curPos) {
    int stop = -1;
    for (int i = 0; i < numStops; i++) {
        if (busStops[i].position == curPos){
            stop = i;
        }
    }
    return stop;
}

bool otherBusAtStop(float curPos, int curBus){
    for (int i = 0; i < maxBuses; i++) {
        if (i == curBus || buses[i] == NULL)
            continue;

        if (atStop(curPos) != -1 && curPos == buses[i]->position)
            return true;
    }
    return false;
}

// find index/position of next stop
struct cBusStopRef nextStop(float curPos) {
    struct cBusStopRef stop;

    if (curPos >= ((numStops - 1) * avgStopDistance)) {
        // Next stop is station
        stop.index = 0;
        stop.position = numStops * avgStopDistance;
        return stop;
    }

    for (int i = 0; i < numStops; i++) {
        if (busStops[i].position > curPos){
            stop.index = i;
            stop.position = busStops[i].position;
            return stop;
        }
    }

    return stop;
}

// Calculate passengers to exit bus
int exitingPassengers(int curStop){
    float timeOfDay24h = 24 * clock.timeOfDay / maxTOD;
    float c = busStops[curStop].busyMorning ? 0.1 : 1.0;
    float a = busStops[curStop].busyEvening ? 0.1 : 1.0;
    float passengers;

    if (timeOfDay24h <= 12) {
        passengers = (busStops[curStop].weight * c * 6 * exp(
            -pow(timeOfDay24h - 9, 2) / 15) + 3) / 10;
    } else {
        passengers = (busStops[curStop].weight * a * 6 * exp(
            -pow(timeOfDay24h - 17, 2) / 15) + 3) / 10;
    }

    return (int) round(passengers);
}

void updateBuses(void) {
    for (int i = 0; i < maxBuses; i++) {
        if ( buses[i] == NULL) {
            continue;
        }

        // Calculate next bus position
        float nextPos = buses[i]->position +
            driver.maxVelocity * driver.trafficDelay;

        if (nextPos >= nextStop(buses[i]->position).position) {
            cBusStopRef newStop = nextStop(buses[i]->position);

            if (otherBusAtStop(nextStop(buses[i]->position).position, i)) {
                // Another bus is already at the stop. Overtake it
#ifdef DEBUG
                printf("%d: Bus %d overtaking bus at stop %d\n",
                    clock.timeOfDay, i, newStop.index);
#endif
                buses[i]->position = nextPos + 0.1;
                numOvertakes += 1;
            } else {
                // Arrival at a bus stop, unload passengers
                buses[i]->position = newStop.position;
#ifdef DEBUG
                printf("%d: Bus %d arriving at stop %d. Unloading passengers\n",
                            clock.timeOfDay, i, newStop.index);
#endif

                for (int j = 0; j < exitingPassengers(newStop.index); j++){
                    if (buses[i]->passengersOnBoard > 0) {
                        buses[i]->passengersOnBoard--;
                        busStation.income += busStation.avgTicketPrice;
                        totalPassengers += 1;
                    }
                }
            }
        }

        // Check if the bus is back at the station
        if (buses[i]->position >= numStops * avgStopDistance) {
            while (buses[i]->passengersOnBoard > 0) {
                buses[i]->passengersOnBoard--;
                busStation.income += busStation.avgTicketPrice;
                totalPassengers += 1;
            }
            removeBus(i);
            continue;
        }

        // Check if the bus is at a stop
        if (atStop(buses[i]->position) != -1) {
            int curStop = atStop(buses[i]->position);
#ifdef DEBUG
            printf("%d: Bus %d is at stop %d\n", clock.timeOfDay,
                    i, curStop);
#endif

            if (numWaitingPassengers(curStop) > 0
                    && buses[i]->passengersOnBoard
                    < (buses[i]->seats + buses[i]->standings)) {
                // Bus can take more passengers
                removePassenger(curStop);
                buses[i]->passengersOnBoard++;
                continue;
            }
        }

        // If the bus is not at a stop, is at a stop and full or is at a stop
        // with no waiting passengers, continue.
        if (atStop(buses[i]->position) == -1
                || (atStop(buses[i]->position) != -1
                    && (numWaitingPassengers(atStop(buses[i]->position))
                        == 0
                        || buses[i]->passengersOnBoard
                            > (buses[i]->seats + buses[i]->standings)))) {
            buses[i]->position = nextPos;
            // Calculate seat availability
            int sittingPassengers, seatAvailability = 0;

            if (buses[i]->passengersOnBoard <= buses[i]->seats) {
                sittingPassengers = buses[i]->passengersOnBoard;
            } else {
                sittingPassengers = buses[i]->seats;
            }

            if (buses[i]->passengersOnBoard > 0) {
                seatAvailability = 100 * sittingPassengers
                    / buses[i]->passengersOnBoard;
            }

            totalAvailability += seatAvailability;
            totalAvailNum += 1;
        }
    }
}

int numWaitingPassengers(int busStop) {
    int number = 0;
    for (int i = 0; i < maxWaitingPassengers; i++){
        if (busStops[busStop].waitingPassengers[i] != NULL) {
            number++;
        }
    }
    return number;
}

// Remove a passenger from a bus stop, update the average waiting time
bool removePassenger(int busStop) {
    if (numWaitingPassengers(busStop) == 0){
#ifdef DEBUG
        printf("%d: Unable to remove passenger from stop %d!\n",
                clock.timeOfDay, busStop);
#endif
        return false;
    }

    int maxWaitingTime = 0;

    for (int i = 0; i < maxWaitingPassengers; i++){
        if (busStops[busStop].waitingPassengers[i] != NULL
                && busStops[busStop].waitingPassengers[i]->waitingTime
                > maxWaitingTime){
            maxWaitingTime = busStops[busStop].waitingPassengers[i]
                ->waitingTime;
        }
    }
    for (int i = 0; i < maxWaitingPassengers; i++){
        if (busStops[busStop].waitingPassengers[i] != NULL
                && busStops[busStop].waitingPassengers[i]->waitingTime
                == maxWaitingTime){
            waitTime += busStops[busStop].waitingPassengers[i]
                ->waitingTime;
            free(busStops[busStop].waitingPassengers[i]);
            busStops[busStop].waitingPassengers[i] = NULL;
#ifdef DEBUG
            printf("%d: Removed passenger from stop %d\n",
                    clock.timeOfDay, busStop);
#endif
            return true;
        }
    }
    return false;
}

// Increment the waiting time of all waiting passengers
void updateWaitingPassengers(void) {
    for (int i = 0; i < numStops; i++) {
        for (int j = 0; j < maxWaitingPassengers; j++){
            if (busStops[i].waitingPassengers[j] != NULL) {
                busStops[i].waitingPassengers[j]->waitingTime++;
            }
        }
    }
}

// Spawns a passenger at a stop
bool spawnPassenger(int busStop) {
    if (numWaitingPassengers(busStop) >= maxWaitingPassengers) {
#ifdef DEBUG
        printf("%d: Failed to spawn passenger at stop %d!\n", clock.timeOfDay,
                busStop);
#endif
        return false;
    }

    for (int i = 0; i < maxWaitingPassengers; i++){
        if (busStops[busStop].waitingPassengers[i] == NULL) {
            busStops[busStop].waitingPassengers[i] =
                (struct cPassenger*) malloc(sizeof(cPassenger));
            busStops[busStop].waitingPassengers[i]->waitingTime = 0;
            return true;
        }
    }
    return false;
}

// Spawns a passenger at all stops
void spawnPassengers(void) {
    for (int i = 0; i < numStops; i++){
        spawnPassenger(i);
    }
}

// Update time dependent variables
void updateTimeVariables(void) {
    float timeOfDay24h = 24 * clock.timeOfDay / maxTOD;

    if (timeOfDay24h <= 12) {
        driver.trafficDelay = -4 * exp(-pow(timeOfDay24h - 9, 2) / 15) + 10 / 10;
    }else{
        driver.trafficDelay = -6 * exp(-pow(timeOfDay24h - 17, 2) / 15) + 10 / 10;
    }
}

void initializeModel(void) {
    busStation.busDepartureCost = 20.52 + ( 16.15 * driver.maxVelocity / 22.8 );

    for (int i = 0; i < numStops; i++) {
        busStops[i].position = i * avgStopDistance;
    }
}

void tick(void) {
    updateTimeVariables();
    updateWaitingPassengers();

    updateBuses();

    // If it's time, spawn a passenger at a stop
    float timeOfDay24h = 24 * clock.timeOfDay / maxTOD;
    for (int i = 0; i < numStops; i++) {
        float passengerPeriod;
        float f0 = 0.5;
        float c = busStops[i].busyMorning ? 1.0 : 0.1;
        float a = busStops[i].busyEvening ? 1.0 : 0.1;

        if (timeOfDay24h <= 12) {
            passengerPeriod = 60 / (busStops[i].weight * f0 * c * 6 * exp(
                -pow(timeOfDay24h - 9, 2) / 15) / 10 + 3);
        } else {
            passengerPeriod = 60 / (busStops[i].weight * f0 * a * 6 * exp(
                -pow(timeOfDay24h - 17, 2) / 15) / 10 + 3);
        }

        if (clock.timeOfDay % (int) round(passengerPeriod) == 0){
#ifdef DEBUG
            printf("%d: Spawning passenger at stop %d\n", clock.timeOfDay, i);
#endif
            spawnPassenger(i);
        }
    }

    // If it's time, emit a new bus
    if (clock.timeOfDay % busStation.periodEmitBus == 0) {
        int newLine;
        if (lastLine == 0){
            newLine = 1;
        }else{
            newLine = 0;
        }
        if (emitBus(newLine)) {
#ifdef DEBUG
            printf("%d: Emitted new bus on line %d\n", clock.timeOfDay, newLine);
#endif
            busStation.costs += busStation.busDepartureCost;
            totalBuses += 1;
        }else{
#ifdef DEBUG
            printf("%d: Failed to emit new bus on line %d\n",
                    clock.timeOfDay, newLine);
#endif
        }
#ifdef DEBUG
        printf("Number of buses on line: %d\n", numBuses());
#endif
        lastLine = newLine;
    }

    clock.timeOfDay += 1;
}

void printResults(void) {
    printf("Total departed buses: %d\n", totalBuses);
    printf("Total transported passengers: %d\n", totalPassengers);

    float avgWaitTime = waitTime / totalPassengers;
    printf("Average passenger waiting time (s): %.1f\n", avgWaitTime);

    float avgSeatAvailability = totalAvailability / (float) totalAvailNum;
    printf("Average seat availability: %.1f%%\n", avgSeatAvailability);

    printf("Number of overtakes: %d\n", numOvertakes);

    printf("Costs: %.2f, income: %.2f\n", busStation.costs, busStation.income);

    int remainingPassengers = 0;
    for (int i = 0; i < numStops; i++) {
        remainingPassengers += numWaitingPassengers(i);
    }
    printf("Passengers remaining at stops: %d\n", remainingPassengers);
}

int main(void) {
    initializeModel();

    while (clock.timeOfDay <= maxTOD) {
        tick();
    }

    printResults();
    return 0;
}
