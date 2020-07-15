#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.

    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_queue.empty(); });
    // Retrieve the element from queue, and remove the first vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg; 
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> uLock(_mutex);
    // add vector to queue
    _queue.emplace_back(msg);
    _condition.notify_one(); // notify client after pushing new msg into the queue
}

template <typename T>
int MessageQueue<T>::getSize()
{
    return _queue.size();
}

template <typename T>
void MessageQueue<T>::clear()
{
    return _queue.clear();
}
/* Implementation of class "TrafficLight" */
 
TrafficLight::TrafficLight():_currentPhase(red){}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        // Add 5 ms delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "TL #" << this->getID()
                  << " calling receive. Queue size: " << _msgQueue.getSize()
                  << std::endl
                  << std::flush;
        TrafficLightPhase res = _msgQueue.receive();
        if (res == green)
        {
            return;
        }
        else
        {
            // reset the queue
            std::cout
                << "TL #" << this->getID()
                << " is red. Reset the queue and wait for the green light."
                << std::endl
                << std::flush;
            _msgQueue.clear();
        }
    }
}

TrafficLight::TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}


// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::unique_lock<std::mutex> lock(_mutexStdout);
    std::cout << "TrafficLight#"<< this->getID() <<"::cycleThroughPhases is called"<<std::endl<<std::flush;
    lock.unlock();
    std::chrono::time_point<std::chrono::system_clock> stopWatch;
    // init stop watch
    stopWatch = std::chrono::system_clock::now();
    // based on https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    std::random_device rd;  // used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // tandard mersenne_twister_engine seeded with rd()
    int minDuration= 4; 
    int maxDuration= 6;
    std::uniform_int_distribution<> uniDistRandom(minDuration, maxDuration);
    int cycleDuration = uniDistRandom(gen);
    while (true)
    {
        // Add 1 ms delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // compute time difference to stop watch
        long diffUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - stopWatch).count();
        if (diffUpdate >= cycleDuration)
        {
            // toggle light phase
            if(_currentPhase == red)
            {
                _currentPhase = green;
                
            }
            else
            {
                _currentPhase = red;
                
            }
            lock.lock();
            std::cout << "TrafficLight#"<< this->getID() << " sends msg. Queue size: "<<_msgQueue.getSize() <<std::endl<<std::flush;
            lock.unlock();
            // send the msg queue notification
            auto futureTrfLight = std::async(
                std::launch::async, &MessageQueue<TrafficLightPhase>::send,
                &_msgQueue, std::move(_currentPhase));
            futureTrfLight.wait();

            // reset stop watch for next cycle
            cycleDuration = uniDistRandom(gen);
            stopWatch = std::chrono::system_clock::now();
        }
    } 
}

