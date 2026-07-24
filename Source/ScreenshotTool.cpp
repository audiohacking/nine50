/*
  Generates a PNG screenshot of the NINE50 plugin editor UI.

  Build & run:
    cmake -B build -DBUILD_SCREENSHOT=ON
    cmake --build build --target NINE50Screenshot -j8
    ./build/Source/NINE50Screenshot_artefacts/NINE50Screenshot.app/Contents/MacOS/NINE50Screenshot

  Output: docs/screenshot.png (relative to repo root / cwd)
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

class NINE50ScreenshotApp : public juce::JUCEApplication
{
public:
    NINE50ScreenshotApp() = default;

    const juce::String getApplicationName() override { return "NINE50 Screenshot"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String&) override
    {
        processor = std::make_unique<NINE50AudioProcessor>();
        editor.reset (dynamic_cast<NINE50AudioProcessorEditor*> (processor->createEditor()));
        jassert (editor != nullptr);

        editor->setOpaque (true);
        editor->addToDesktop (juce::ComponentPeer::windowIsTemporary
                              | juce::ComponentPeer::windowIgnoresKeyPresses);
        editor->setVisible (true);
        editor->toFront (false);

        std::cout << "Editor size: " << editor->getWidth() << "x" << editor->getHeight() << std::endl;

        for (int i = 0; i < 20; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);

        auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
        std::cout << "Snapshot size: " << snapshot.getWidth() << "x" << snapshot.getHeight() << std::endl;

        auto outputFile = juce::File::getCurrentWorkingDirectory()
                              .getChildFile ("docs")
                              .getChildFile ("screenshot.png");

        outputFile.getParentDirectory().createDirectory();
        outputFile.deleteFile();

        juce::PNGImageFormat png;
        juce::FileOutputStream stream (outputFile);

        if (stream.openedOk() && png.writeImageToStream (snapshot, stream))
            std::cout << "Screenshot saved: " << outputFile.getFullPathName() << std::endl;
        else
            std::cerr << "Error: failed to write PNG" << std::endl;

        editor->removeFromDesktop();
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

START_JUCE_APPLICATION (NINE50ScreenshotApp)
