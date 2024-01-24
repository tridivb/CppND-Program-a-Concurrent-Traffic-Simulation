#include "TrafficLight.h"

#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and
  // _condition.wait() to wait for and receive new messages and pull them from
  // the queue using move semantics. The received object should then be returned
  // by the receive function.

  std::unique_lock<std::mutex> uLock(_mutex);
  _condition.wait(uLock, [this] { return !_queue.empty(); });

  // remove last vector element from queue
  T msg = std::move(_queue.back());
  _queue.pop_back();

  return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new
  // message to the queue and afterwards send a notification.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // perform vector modification under the lock
  std::lock_guard<std::mutex> uLock(_mutex);

  std::cout << "Traffic Light phase changed" << std::endl;
  _queue.push_back(std::move(msg));
  _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() { _currentPhase = TrafficLightPhase::RED; }

void TrafficLight::waitForGreen() {
  // FP.5b : add the implementation of the method waitForGreen, in which an
  // infinite while-loop runs and repeatedly calls the receive function on the
  // message queue. Once it receives TrafficLightPhase::green, the method
  // returns.
  while (true) {
    // just add some wait time to reduce cpu usage
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (_msgQueue->receive() == TrafficLightPhase::GREEN) {
      break;
    }
  }
}

TrafficLightPhase TrafficLight::getCurrentPhase() { return _currentPhase; }

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started
  // in a thread when the public method „simulate“ is called. To do this, use
  // the thread queue in the base class.
  TrafficLight::threads.emplace_back(
      std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the time
  // between two loop cycles and toggles the current phase of the traffic light
  // between red and green and sends an update method to the message queue using
  // move semantics. The cycle duration should be a random value between 4 and 6
  // seconds. Also, the while-loop should use std::this_thread::sleep_for to
  // wait 1ms between two cycles.

  std::lock_guard<std::mutex> uLock(_mutex);

  auto start = std::chrono::high_resolution_clock::now();
  std::random_device rd;   // a seed source for the random number engine
  std::mt19937 gen(rd());  // mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> random_duration(4, 6);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(random_duration(gen)));
    if (TrafficLight::_currentPhase == TrafficLightPhase::RED) {
      start = std::chrono::high_resolution_clock::now();
      _currentPhase = TrafficLightPhase::GREEN;
      _msgQueue->send(std::move(_currentPhase));
    } else {
      _currentPhase = TrafficLightPhase::RED;
      auto end = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      // wait 1 millisecond between two cycles
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}