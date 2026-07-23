/*
  Generates a PNG screenshot of the NINE50 plugin editor UI.

  Build & run:
    cmake -B build -DBUILD_SCREENSHOT=ON
    cmake --build build --target NINE50Screenshot -j8
    ./build/Source/NINE50Screenshot

  Output: docs/screenshot.png (relative to repo root)
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

class NINE50ScreenshotApp : public juce::JUCEApplication {
public:
    NINE50ScreenshotApp() = default;

    const juce::String getApplicationName() override { return "NINE50 Screenshot"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override {
        // Create the processor and its editor
        processor = std::make_unique<NINE50AudioProcessor>();
        editor = std::unique_ptr<NINE50AudioProcessorEditor>(
            dynamic_cast<NINE50AudioProcessorEditor*>(processor->createEditorAndMakeActive()));

        jassert(editor != nullptr);

        // Lay out the editor at its natural size
        editor->setSize(600, 400);

        // Process pending UI messages so layout and paint are settled
        for (int i = 0; i < 10; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

        // Capture the snapshot
        auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());

        // Resolve output path relative to the executable location
        auto outputFile = juce::File::getCurrentWorkingDirectory()
                              .getChildFile("docs")
                              .getChildFile("screenshot.png");

        outputFile.getParentDirectory().createDirectory();

        juce::PNGImageFormat png;
        std::unique_ptr<juce::FileOutputStream> stream(new juce::FileOutputStream(outputFile));

        if (stream != nullptr && stream->openedOk()) {
            if (png.writeImageToStream(snapshot, *stream)) {
                std::cout << "Screenshot saved: " << outputFile.getFullPathName() << std::endl;
            } else {
                std::cerr << "Error: failed to write PNG" << std::endl;
            }
        } else {
            std::cerr << "Error: failed to open output file" << std::endl;
        }

        // Clean up
        editor.reset();
        processor.reset();

        quit();
    }

    void shutdown() override {}

    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<NINE50AudioProcessor> processor;
    std::unique_ptr<NINE50AudioProcessorEditor> editor;
};

START_JUCE_APPLICATION(NINE50ScreenshotApp)
