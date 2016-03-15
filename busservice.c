#include <stdio.h>
#include "busservice.h"

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

struct cDriver {
    const float maxVelocity = 10; // m/s
    float trafficDelay = 0; // % of maxVelocity to use
} driver;

struct cPassenger {
    int waitingTime = 0;
};

struct cBusStop {
    float position;
    struct cPassenger waitingPassengers[1024];
};
struct cBusStop busStops[numStops];

struct cPassengerGenerator {
    int averagePeriod;
} passengerGenerator;

struct cClock {
    int timeOfDay = 1; // 1-86400
} clock;

// Update time dependent variables
void updateTimeVariables(void){
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


    clock.timeOfDay += 1;
}

void printResults(void) {
    printf("Stop 2 position: %0.2f\n", busStops[1].position);
}

int main(void) {
    initializeModel();

    while (clock.timeOfDay <= maxTOD) {
        tick();
    }

    printResults();
    return 0;
}
