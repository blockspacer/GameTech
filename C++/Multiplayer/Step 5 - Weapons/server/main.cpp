#include "GameModel.hpp"
#include "MessageQueueServer.hpp"

#include <chrono>
#include <google/protobuf/stubs/common.h>
#include <iostream>
#include <thread>

//
// Doing this so I can think in milliseconds, but everything else is
// needed in microseconds.
const auto SIMULATION_UPDATE_RATE_US = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(100));

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    //
    // Get the network messaging service and game model initialized and ready to run
    if (!MessageQueueServer::instance().initialize(3000))
    {
        std::cout << "Failed to initialize the networking messaging server, terminating..." << std::endl;
        exit(0);
    }
    GameModel model;

    //
    // Get the network message queue up and running
    if (!model.initialize())
    {
        std::cout << "Failed to initialize the message queue" << std::endl;
        exit(0);
    }

    //
    // Grab an initial time-stamp to get the elapsed time working
    auto previousTime = std::chrono::system_clock::now();

    //
    // Get the server loop running.  The game loop runs inside of this loop
    auto running = bool{true};
    while (running)
    {
        //
        // Figure out the elapsed time in microseconds.  Need this to pass on to
        // the game model.
        auto currentTime = std::chrono::system_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime);
        //
        // If we are running faster than the simulate update rate, then go
        // to sleep for a bit so we don't burn up the CPU unnecessarily.
        auto sleepTime = SIMULATION_UPDATE_RATE_US - elapsedTime;
        if (sleepTime > std::chrono::microseconds(0))
        {
            std::this_thread::sleep_for(sleepTime);
        }

        // Recompute elapsed time after doing the sleep, to get an accurate time duration
        currentTime = std::chrono::system_clock::now();
        elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime);
        previousTime = currentTime;

        //
        // Execute the game loop steps.  Because this is an ECS model, there is only an update.
        model.update(elapsedTime, currentTime);
    }

    //
    // Gracefully shutdown the network message service and game model
    model.shutdown();
    MessageQueueServer::instance().shutdown();

    //
    // Do the same for the Google Protocol Buffers library
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}