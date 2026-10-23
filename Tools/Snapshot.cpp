#include "../Source/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    auto* p = static_cast<OmniarpProcessor*> (createPluginFilter());
    std::unique_ptr<juce::AudioProcessor> owner (p);
    auto set = [p] (const char* id, float v) { if (auto* prm = p->apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
    set ("hold", 1); set ("algo", 2); set ("scale", 2); set ("key", 9); set ("steps", 7); set ("transpose", 5); set ("inkey", 1); set ("gate", 0.62f); set ("swing", 0.18f); set ("oct", 1); set ("octRpt", 2);
    if (argc > 2)   // demo pattern: C E G B up/down over two octaves
    {
        const int demo[] = { 60, 64, 67, 71, 67, 64, 72, 76, 79, 83, 79, 76, 60, 64, 67, 71 };
        for (int i = 0; i < 16; ++i) p->engine.preview[(size_t) i].store (demo[i]);
        p->engine.previewCount.store (16);
        p->engine.previewTick.store (1);
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    juce::File out (argc > 1 ? argv[1] : "snap.png");
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    ed.reset();
    return 0;
}
