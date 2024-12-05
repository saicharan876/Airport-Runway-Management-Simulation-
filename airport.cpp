#include <iostream>
#include <queue>
#include <cstdlib>
#include <ctime>
using namespace std;

const int MAX_DEPARTURES = 3;
const int NO_OF_RUNWAYS = 3;
const int EMERGENCY_FUEL = 0;
const int MAX_ARRIVALS = 2;
const int Limit_for_takeoff_queue=5;
const int EMERGENCY_LANDING_RUNWAY=2;

struct Plane
{
    int id;
    int fuel_level;
    int waitingtime;
    bool isLanding;

    // Comparator for priority queue (low fuel has higher priority)
    bool operator<(const Plane &other) const
    {
        return fuel_level > other.fuel_level; // Lower fuel comes first
    }

    Plane(int id, int fuel_level, int waitingtime = 0)
        : id(id), fuel_level(fuel_level), waitingtime(waitingtime), isLanding(!(id % 2)) {}
};

priority_queue<Plane> landingPriorityQueue;
queue<Plane> takeoffQueue[MAX_DEPARTURES];
queue<Plane> EMERGENCY;

int total_TakeoffWaitingTime = 0, total_LandingWaitingTime = 0;
int Departure_Count = 0, Arrival_Count = 0;
int crashCount = 0;
// here we are going check the simulation in each time unit
int timeUnit = 0;

void Generate_Random_Planes()
{

    // We will generate Random planes and add them into simulation
    //  Add up to MAX_ARRIVALS planes to the landing queue
    int No_landing_planes = rand() % MAX_ARRIVALS + 1;
    for (int i = 0; i < No_landing_planes; i++)
    {
        Plane newPlane = {2 * (timeUnit * MAX_ARRIVALS + i), rand() % 5 + 1, 0}; // Random fuel (1-5)
        landingPriorityQueue.push(newPlane);
    }
    // Add up to MAX_ARRIVALS planes to the takeoff queues
    int No_of_takeoff_planes = rand() % MAX_DEPARTURES + 1;
    for (int i = 0; i < No_of_takeoff_planes; i++)
    {
        Plane newPlane = {2 * (timeUnit * MAX_ARRIVALS + i) + 1, 0, 0};

        // Assign to the least busy queue
        int minQueue = 0;
        for (int j = 1; j < NO_OF_RUNWAYS; j++)
        {
            if (takeoffQueue[j].size() < takeoffQueue[minQueue].size())
            {
                minQueue = j;
            }
        }
        takeoffQueue[minQueue].push(newPlane);
    }
}

void Simulation_Per_One_timeunit()
{
    // Here we are going to check the simulation in each time unit
    // Track free runways
    vector<bool> isRunwayFree(NO_OF_RUNWAYS, true);
    vector<Plane> assignedPlanes(NO_OF_RUNWAYS, {0, 0, 0}); // Planes assigned to runways

    // Prioritize emergency landings

    if (!EMERGENCY.empty() && isRunwayFree[EMERGENCY_LANDING_RUNWAY])
    {
        Plane emergencyPlane = EMERGENCY.front();
        EMERGENCY.pop();
        assignedPlanes[EMERGENCY_LANDING_RUNWAY] = emergencyPlane; // Assign to Runway EMERGENCY_LANDING_RUNWAY
        isRunwayFree[EMERGENCY_LANDING_RUNWAY] = false;            // Mark Runway EMERGENCY_LANDING_RUNWAY as occupied
        Arrival_Count++;
        total_LandingWaitingTime += emergencyPlane.waitingtime;

        cout << "Emergency Landing: Plane " << emergencyPlane.id
             << " on Runway EMERGENCY_LANDING_RUNWAY (Fuel: " << emergencyPlane.fuel_level << ")" << endl;
    }

    // Regular landings (exclude Runway EMERGENCY_LANDING_RUNWAY if still free)
    for (int i = 0; i < NO_OF_RUNWAYS; i++)
    {
        if (i == EMERGENCY_LANDING_RUNWAY && isRunwayFree[i])
            continue; // Keep Runway EMERGENCY_LANDING_RUNWAY free unless necessary

        if (isRunwayFree[i] && !landingPriorityQueue.empty())
        {
            Plane landingPlane = landingPriorityQueue.top();
            landingPriorityQueue.pop();

            if (landingPlane.fuel_level > 0)
            {
                assignedPlanes[i] = landingPlane;
                isRunwayFree[i] = false;
                Arrival_Count++;
                total_LandingWaitingTime += landingPlane.waitingtime;

                cout << "Landing: Plane " << landingPlane.id << " on Runway " << i << " (Fuel: " << landingPlane.fuel_level << ")" << endl;
            }
            else
            {
                crashCount++;
                cout << "Crash: Plane " << landingPlane.id << " ran out of fuel!" << endl;
            }
        }
    }

    // Threshold-based takeoff allocation
    int maxQueueSize = 0;
    int maxQueueIndex = -1;
    for (int i = 0; i < NO_OF_RUNWAYS; i++)
    {
        if ((int)takeoffQueue[i].size() > maxQueueSize)
        {
            maxQueueSize = takeoffQueue[i].size();
            maxQueueIndex = i;
        }
    }

    if (maxQueueSize > Limit_for_takeoff_queue)
    { // Handle congestion in takeoff queues
        for (int i = 0; i < NO_OF_RUNWAYS; i++)
        {
            if (isRunwayFree[i])
            {
                Plane takeoffPlane = takeoffQueue[maxQueueIndex].front();
                takeoffQueue[maxQueueIndex].pop();
                assignedPlanes[i] = takeoffPlane;
                isRunwayFree[i] = false;
                Departure_Count++;
                total_LandingWaitingTime += takeoffPlane.waitingtime;

                cout << "Priority Takeoff: Plane " << takeoffPlane.id << " from Runway " << i << " (Threshold Logic)" << endl;
                break;
            }
        }
    }

    // Regular takeoffs
    for (int i = 0; i < NO_OF_RUNWAYS; i++)
    {
        if (isRunwayFree[i])
        {
            for (int j = 0; j < NO_OF_RUNWAYS; j++)
            {
                if (!takeoffQueue[j].empty())
                {
                    Plane takeoffPlane = takeoffQueue[j].front();
                    takeoffQueue[j].pop();

                    assignedPlanes[i] = takeoffPlane;
                    isRunwayFree[i] = false;
                    Departure_Count++;
                    total_TakeoffWaitingTime += takeoffPlane.waitingtime;

                    cout << "Takeoff: Plane " << takeoffPlane.id << " from Runway " << i << endl;
                    break;
                }
            }
        }
    }

    // Update waiting times and fuel levels
    vector<Plane> tempLandingQueue;
    while (!landingPriorityQueue.empty())
    {
        Plane p = landingPriorityQueue.top();
        landingPriorityQueue.pop();

        p.fuel_level--; // Decrease fuel
        p.waitingtime++;

        if (p.fuel_level == EMERGENCY_FUEL)
        {
            EMERGENCY.push(p);
        }
        else
        {
            tempLandingQueue.push_back(p);
        }
    }

    // Rebuild landing priority queue with updated planes
    for (Plane &p : tempLandingQueue)
    {
        landingPriorityQueue.push(p);
    }

    // Update waiting times for takeoff planes
    for (int i = 0; i < NO_OF_RUNWAYS; i++)
    {
        int size = takeoffQueue[i].size();
        for (int j = 0; j < size; j++)
        {
            Plane p = takeoffQueue[i].front();
            takeoffQueue[i].pop();
            p.waitingtime++;
            takeoffQueue[i].push(p);
        }
    }
}

void printStatistics()
{
    cout << "Time Unit: " << timeUnit << endl;

    cout << "Takeoff Queues:" << endl;
    for (int i = 0; i < NO_OF_RUNWAYS; i++)
    {
        cout << "Queue " << i << ": ";
        queue<Plane> temp = takeoffQueue[i];
        while (!temp.empty())
        {
            cout << temp.front().id << " ";
            temp.pop();
        }
        cout << endl;
    }

    cout << "Average Landing Waiting Time: "
         << (Arrival_Count > 0 ? (double)total_LandingWaitingTime / Arrival_Count : 0) << endl;
    cout << "Average Takeoff Waiting Time: "
         << (Departure_Count > 0 ? (double)total_TakeoffWaitingTime / Departure_Count : 0) << endl;
    cout << "Planes Crashed: " << crashCount << endl;
    cout << "----------------------------------" << endl;
}

int main()
{
    srand(time(0));          // Seed for random number generation
    int totalTimeUnits = 10; // Simulation length

    for (timeUnit = 1; timeUnit <= totalTimeUnits; timeUnit++)
    {
        Generate_Random_Planes();
        Simulation_Per_One_timeunit();
        printStatistics();
    }

    return 0;
}