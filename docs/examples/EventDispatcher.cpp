#include <events.hpp>

class Producer {
public:
    // An event dispatcher that calls back the progress to functions using an integer parameter and returning a boolean.
    Texturize::EventDispatcher<bool, int> ProgressHandler;

public:
    void DoWork() {
        int i = 0;
        
        for (int i(0); i < 100; ++i) {
            // ...do something...
            
            // If some callback does not want the work to continue, return.
            if (!ProgressHandler.execute(i))
                break;
        }
    }
};

// Define the callback function.
int OnProgress(int progress) {
    std::cout << "Progress: " << progress << "%" << std::endl;
    return true;
}

// Create a producer instance.
Producer producer;

int main(int argc, const char** argv)
{
    // The producer will call the OnProgress function with each iteration.
    producer.ProgressHandler.add(OnProgress);

    // The same can be achieved inline using a lambda expression.    
    producer.ProgressHandler.add([](int progress) -> bool {
        std::cout << "Progress: " << progress << "%" << std::endl;
        return true;
    });
}