#include <JuceHeader.h>

namespace
{
    // Dedicated category so we never pull in JUCE's own module unit tests.
    constexpr const char* kNine50TestCategory = "nine50";
}

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
int main (int, char**)
{
    ConsoleLogger logger;
    juce::Logger::setCurrentLogger (&logger);

    const juce::ScopeGuard onExit { [&]
    {
        juce::Logger::setCurrentLogger (nullptr);
        juce::DeletedAtShutdown::deleteAll();
    }};

    ConsoleUnitTestRunner runner;
    runner.runTestsInCategory (kNine50TestCategory);

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (auto* result = runner.getResult (i))
            failures += result->failures;

    std::cout << "NINE50 tests complete — failures: " << failures << std::endl;
    return failures == 0 ? 0 : 1;
}
