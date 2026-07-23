#include <JuceHeader.h>

//==============================================================================
class ConsoleLogger final : public juce::Logger
{
    void logMessage (const juce::String& message) override
    {
        std::cout << message << std::endl;
    }
};

//==============================================================================
class ConsoleUnitTestRunner final : public juce::UnitTestRunner
{
    void logMessage (const juce::String& message) override
    {
        juce::Logger::writeToLog (message);
    }
};

//==============================================================================
int main (int, char **)
{
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger (&logger);

    const juce::ScopeGuard onExit { [&]
    {
        juce::Logger::setCurrentLogger (nullptr);
        juce::DeletedAtShutdown::deleteAll();
    }};

    ConsoleUnitTestRunner runner;
    runner.runAllTests();

    return runner.getNumResults() > 0 && runner.getResult (0)->failures == 0 ? 0 : 1;
}
