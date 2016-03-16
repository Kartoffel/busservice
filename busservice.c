#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "busservice.h"

#define maxWaitingPassengers 500
#define maxBuses 100

int     numOvertakes        = 0;
int     waitTime            = 0; // Seconds passengers have waited
int     totalPassengers     = 0; // Number of passengers who took the bus
int     seatAvailability    = 0; // Times that passengers had to stand

const float avgStopDistance = 631; // m
const int   numStops        = 36;
const float lineLength      = numStops * avgStopDistance;
const int   maxTOD          = 86400; // Stop model after timeOfDay exceeds this

struct cBusStation {
    int periodEmitBus = 1;
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
    const float maxVelocity = 10; // m/s
    float trafficDelay = 0; // % of maxVelocity to use
} driver;

struct cPassenger {
    int waitingTime = 0;
};

struct cBusStop {
    float position;
    struct cPassenger *waitingPassengers[maxWaitingPassengers];
};
struct cBusStop busStops[numStops];

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
        printf("Unable to emit a bus!\n");
        return false;
    }

    for (int i = 0; i < maxBuses; i++){
        if (buses[i] != NULL) {
            buses[i] = (struct cBus*) malloc(sizeof(cBus));
            buses[i]->line = busLine;
            buses[i]->passengersOnBoard = 0;
            buses[i]->position = 0.0;

            if (busLine == 0) {
                buses[i]->seats = 33;
                buses[i]->standings = 105;
            }else if (busLine == 1) {
                buses[i]->seats = 25;
                buses[i]->standings = 74;
            }
            return true;
        }
    }
    return false;
}

void updateBuses(void) {

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
        printf("Unable to remove passenger from stop %d!\n", busStop);
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
            totalPassengers += 1;
            free(busStops[busStop].waitingPassengers[i]);
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
        printf("Failed to spawn passenger at stop %d!\n", busStop);
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
    // TODO: actually make these time variable
    driver.trafficDelay = 1.0;
    passengerGenerator.averagePeriod = 60;
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

    // If it's time, spawn a passenger at every stop
    if (clock.timeOfDay % passengerGenerator.averagePeriod == 0){
        spawnPassengers();
    }

    // If it's time, emit a new bus


    clock.timeOfDay += 1;
}

void printResults(void) {
    float avgWaitTime = waitTime / totalPassengers;
    printf("Average passenger waiting time (s): %.1f\n", avgWaitTime);
}

int main(void) {
    initializeModel();

    while (clock.timeOfDay <= maxTOD) {
        tick();
    }

    printResults();
    return 0;
}
