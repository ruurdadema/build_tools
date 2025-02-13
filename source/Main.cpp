#include <memory>

#include "MainComponent.h"

class GuiAppApplication final : public juce::JUCEApplication
{
public:
    GuiAppApplication() {}

    const juce::String getApplicationName() override
    {
        return PROJECT_PRODUCT_NAME;
    }

    const juce::String getApplicationVersion() override
    {
        return PROJECT_VERSION_STRING;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise (const juce::String& commandLine) override
    {
        std::ignore = commandLine;
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        std::ignore = commandLine;
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name) :
            DocumentWindow (
                name,
                juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (backgroundColourId),
                allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
#else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
#endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (GuiAppApplication)
