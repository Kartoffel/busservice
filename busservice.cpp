#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "busservice.h"

unsigned int    numOvertakes        = 0;
unsigned int    waitTime            = 0; // Seconds passengers have waited
unsigned int    totalOutPassengers  = 0; // Passengers who exited the bus
unsigned int    totalInPassengers   = 0; // Passengers who entered the bus
unsigned int    totalForcedOutPass  = 0; // Passengers forced out at 24:00
unsigned int    forcedOutPass       = 0; // Passengers forced out at 24:00
unsigned long   totalAvailability   = 0; // % of seated passengers / total
unsigned int    totalAvailNum       = 0; // Used for averaging
unsigned int    totalBuses          = 0;

int             lastLine            = 1;

struct cBusStation {
    int periodEmitBus = 0;
    int periodsEmitBus[3] = {300, 600, 1800}; // Rush, day and night
    float busDepartureCost = 0.0;
    float income = 0.0;
    float costs = 0.0;
    float profit = 0.0;
    const float avgTicketPrice = 0.9;
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
    const float maxVelocity = 16.67; // m/s
    float trafficDelay = 0; // % of maxVelocity to use
} driver;

struct cPassenger {
    int waitingTime = 0;
};

struct cBusStop {
    float position = 0.0;
    struct cPassenger *waitingPassengers[maxWaitingPassengers];
    float weight = 10;
    bool busyMorningIn = false;
    bool busyMorningOut = false;
    bool busyEveningIn = false;
    bool busyEveningOut = false;
};
struct cBusStop busStops[numStops];

struct cBusStopProperties {
    float weight;
    bool busyMorningIn;
    bool busyMorningOut;
    bool busyEveningIn;
    bool busyEveningOut;
};
struct cBusStopProperties busStopProps[numStops] = {
    {40, true, true, true, true},   //station
    {25, false, false, true, false},  //piazza
    {5, false, false, false, false},  //stadion
    {5, false, true, false, false},  //glaspoort
    {15, true, true, true, true},   //strijp-s
    {7, false, false, false, true},  //cederlaan
    {3, false, false, false, false},  //evoluon
    {5, false, false, false, false},  //bredalaan
    {3, false, false, false, false},  //hurksestraat
    {25, false, false, false, true}, //p+r meerhoven
    {13, false, false, false, true}, //landforum
    {20, true, false, false, true},  //grasrijk&polders
    {8, false, false, false, false},  //meerrijk&smelen
    {20, true, true, true, true},  //zandrijk&heemweg/centrum
    {15, true, true, true, true}, //cargo forum&city centrum
    {7, false, false, false, false},  //flight forum&bossebaan
    {17, false, false, false, true}, //de klokkenmaker&de naaldenmaker
    {45, true, true, true, true},   //airport&sondervick
    {17, true, false, false, false},  //kometenlaan&mira
    {7, false, false, false, false},  //flight forum&bossebaan
    {15, true, true, true, true},  //cargo forum&city centrum
    {20, true, true, true, true},  //zandrijk&heemweg/centrum
    {8, false, false, false, false},  //meerrijk&smelen
    {20, true, false, false, true},  //grasrijk&polders
    {13, true, false, false, false},  //landforum
    {25, true, false, false, false},  //p+r meerhoven
    {3, false, false, false, false},  //hurksestraat
    {5, false, false, false, false},  //bredalaan
    {3, false, false, false, false},  //evoluon
    {7, true, false, false, false},   //cederlaan
    {15, true, true, true, true},   //strijp-s
    {5, false, false, true, false},  //glaspoort
    {5, false, false, false, false},  //stadion
    {25, false, true, false, false}, //piazza
};

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
} clk;

//#include "vis.cpp"

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
    printf("%d: Removed bus %d\n", clk.timeOfDay, index);
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
    float timeOfDay24h = 24.0 * clk.timeOfDay / (maxTOD);
    float c,a,passengers;
    if (busStops[curStop].busyMorningOut)
        c = 1.0;
    else
        c = 0.3;

    if (busStops[curStop].busyEveningOut)
        a = 1.0;
    else
        a = 0.3;

    if (timeOfDay24h <= 12) {
        passengers = ((busStops[curStop].weight * c * (14 *
            exp (1 * (timeOfDay24h - 7.0)) / (1 + exp(1.2 *
            (timeOfDay24h - 7.0))) + 1)) / 10);
    } else {
        passengers = ((busStops[curStop].weight * a * (14 *
            exp (1 * (-timeOfDay24h + 18.0)) / (1 + exp(1.2 *
            (-timeOfDay24h + 18.0))) + 1)) / 10);
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

        // Bus nearing a stop
        if (nextPos >= nextStop(buses[i]->position).position) {
            cBusStopRef newStop = nextStop(buses[i]->position);

            if (otherBusAtStop(nextStop(buses[i]->position).position, i)) {
                // Another bus is already at the stop. Overtake it
#ifdef DEBUG
                printf("%d: Bus %d overtaking bus at stop %d\n",
                    clk.timeOfDay, i, newStop.index);
#endif
                buses[i]->position = nextPos + 0.1;
                numOvertakes += 1;
            } else {
                // Arrival at a bus stop, unload passengers
                buses[i]->position = newStop.position;
#ifdef DEBUG
                printf("%d: Bus %d arriving at stop %d. Unloading passengers\n",
                            clk.timeOfDay, i, newStop.index);
#endif

                for (int j = 0; j < exitingPassengers(newStop.index); j++){
                    if (buses[i]->passengersOnBoard > 0) {
                        buses[i]->passengersOnBoard--;
                        busStation.income += busStation.avgTicketPrice;
                        totalOutPassengers += 1;
                    }
                }
            }
        }

        // Check if the bus is back at the station
        if (buses[i]->position >= numStops * avgStopDistance) {
            while (buses[i]->passengersOnBoard > 0) {
                buses[i]->passengersOnBoard--;
                busStation.income += busStation.avgTicketPrice;
                totalOutPassengers += 1;
                totalForcedOutPass += 1;
            }
            removeBus(i);
            continue;
        }

        // Check if the bus is at a stop
        if (atStop(buses[i]->position) != -1) {
            int curStop = atStop(buses[i]->position);
#ifdef DEBUG
            printf("%d: Bus %d is at stop %d\n", clk.timeOfDay,
                    i, curStop);
#endif

            if (numWaitingPassengers(curStop) > 0
                    && buses[i]->passengersOnBoard
                    < (buses[i]->seats + buses[i]->standings)) {
                // Bus can take more passengers
                removePassenger(curStop);
                buses[i]->passengersOnBoard++;
                totalInPassengers++;
                continue;
            }
        }

        // If the bus is not at a stop, is at a stop and full or is at a stop
        // with no waiting passengers, continue.
        if (atStop(buses[i]->position) == -1
                || (atStop(buses[i]->position) != -1
                    && (numWaitingPassengers(atStop(buses[i]->position)) == 0
                        || buses[i]->passengersOnBoard
                            >= (buses[i]->seats + buses[i]->standings)))) {
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
                clk.timeOfDay, busStop);
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
                    clk.timeOfDay, busStop);
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
        printf("%d: Failed to spawn passenger at stop %d!\n", clk.timeOfDay,
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
    float timeOfDay24h = 24.0 * clk.timeOfDay / (maxTOD);

    if ((timeOfDay24h >= 7 && timeOfDay24h <= 11)
        || (timeOfDay24h >= 14 && timeOfDay24h <= 19)) {
        // Rush hour
        busStation.periodEmitBus = busStation.periodsEmitBus[0];
    } else if (timeOfDay24h >= 22 || timeOfDay24h <= 4) {
        // Night
        busStation.periodEmitBus = busStation.periodsEmitBus[2];
    } else {
        // Day
        busStation.periodEmitBus = busStation.periodsEmitBus[1];
    }

    if (timeOfDay24h <= 12) {
        driver.trafficDelay = ((-6*exp(pow(timeOfDay24h-7.0,1.0)))/(1+exp(pow(timeOfDay24h-7.0,1.1)))+10)
            / 10;
    }else{
        driver.trafficDelay = ((-10*exp(pow(-timeOfDay24h+19.0,1.0)))/(1+exp(pow(-timeOfDay24h+19.0,1.2)))+10)
            / 10;
    }
}

void initializeModel(void) {
    // Use average velocity (obtained from trafficDelay equations) to calculate
    // the approximate cost of sending out a bus
    busStation.busDepartureCost = 20.52 + 16.15 * (22.8 / (driver.maxVelocity *
        0.737 * 3.6));

    for (int i = 0; i < numStops; i++) {
        busStops[i].position = i * avgStopDistance;
        busStops[i].weight = busStopProps[i].weight;
        busStops[i].busyMorningIn = busStopProps[i].busyMorningIn;
        busStops[i].busyMorningOut = busStopProps[i].busyMorningOut;
        busStops[i].busyEveningIn = busStopProps[i].busyEveningIn;
        busStops[i].busyEveningOut = busStopProps[i].busyEveningOut;
    }
}

void tick(void) {
    updateTimeVariables();
    updateWaitingPassengers();

    updateBuses();

    // If it's time, spawn a passenger at a stop
    float timeOfDay24h = 24 * clk.timeOfDay / maxTOD;
    for (int i = 0; i < numStops; i++) {
        float passengerPeriod, c, a;
        float f0 = 0.075; // Obtained through amazing calculations
        if (busStops[i].busyMorningIn)
            c = 1.0;
        else
            c = 0.3;

        if (busStops[i].busyEveningIn)
            a = 1.0;
        else
            a = 0.3;

        if (timeOfDay24h <= 12) {
            passengerPeriod = 60.0 / ((busStops[i].weight * f0 * c * (14 *
                exp (1 * (timeOfDay24h - 7)) / (1 + exp(1.2 *
                (timeOfDay24h - 7.0))) + 1)) / 10);
        } else {
            passengerPeriod = 60.0 / ((busStops[i].weight * f0 * a * (14 *
                exp (1 * (-timeOfDay24h + 18)) / (1 + exp(1.2 *
                (-timeOfDay24h + 18))) + 1)) / 10);
        }

        if (clk.timeOfDay % (int) round(passengerPeriod) == 0){
#ifdef DEBUG
            printf("%d: Spawning passenger at stop %d\n", clk.timeOfDay, i);
#endif
            spawnPassenger(i);
        }
    }

    // If it's time, emit a new bus
    if (clk.timeOfDay % busStation.periodEmitBus == 0) {
        int newLine;
        if (lastLine == 0){
            newLine = 1;
        }else{
            newLine = 0;
        }
        if (emitBus(newLine)) {
#ifdef DEBUG
            printf("%d: Emitted new bus on line %d\n", clk.timeOfDay, newLine);
#endif
            busStation.costs += busStation.busDepartureCost;
            totalBuses += 1;
        }else{
#ifdef DEBUG
            printf("%d: Failed to emit new bus on line %d\n",
                    clk.timeOfDay, newLine);
#endif
        }
#ifdef DEBUG
        printf("Number of buses on line: %d\n", numBuses());
#endif
        lastLine = newLine;
    }

    clk.timeOfDay += 1;
}

// Remove all buses, collect income
void cleanupModel(void) {
    for (int i = 0; i < maxBuses; i++) {
        if (buses[i] == NULL) {
            continue;
        }
        for (int j = 0; j < buses[i]->passengersOnBoard; j++) {
            busStation.income += busStation.avgTicketPrice;
            totalOutPassengers += 1;
            forcedOutPass += 1;
        }
        removeBus(i);
    }
}

void printResults(void) {
    printf("Total departed buses: %d\n", totalBuses);
    printf("Total passengers: %d\n", totalInPassengers);
    printf("Total paid passengers: %d\n",totalOutPassengers);
    printf("%d passengers forced out during the day\n", totalForcedOutPass);
    printf("%d passengers forced out at midnight\n", forcedOutPass);
    int remainingPassengers = 0;
    for (int i = 0; i < numStops; i++) {
        remainingPassengers += numWaitingPassengers(i);
    }
    printf("Passengers remaining at stops: %d\n", remainingPassengers);

    float avgWaitTime = waitTime / totalOutPassengers;
    printf("\nAverage passenger waiting time (s): %.1f\n", avgWaitTime);

    float avgSeatAvailability = totalAvailability / (float) totalAvailNum;
    printf("Average seat availability: %.1f%%\n", avgSeatAvailability);

    printf("Number of overtakes: %d\n", numOvertakes);

    printf("Costs: %.2f, income: %.2f\n", busStation.costs, busStation.income);
    busStation.profit = 0.5 * busStation.income - busStation.costs;
    printf("Profit (including 50%% tax on income): %.1f (%.1f%%)\n",
            busStation.profit, 100 * busStation.profit / busStation.costs);
}

int main(void) {
#ifdef VIS_CPP_
    initializeScreen();
#endif
    initializeModel();

    while (clk.timeOfDay <= maxTOD) {
        tick();
#ifdef VIS_CPP_
        handleScreenEvents();
        if (quit) {
            break;
        }

        //if (clk.timeOfDay > busStation.periodEmitBus){
        if (clk.timeOfDay > 8*3600){
            drawScreen();
            SDL_Delay(6);
        }
#endif
    }

    cleanupModel();
    printResults();

#ifdef VIS_CPP_
    while (!quit){
        handleScreenEvents();
    }
    closeScreen();
#endif

    return 0;
}
