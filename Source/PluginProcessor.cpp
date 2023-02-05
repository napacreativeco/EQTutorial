/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQTutorialAudioProcessor::EQTutorialAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
//==============================================================================
// CONSTRUCTOR
//==============================================================================
{
}

EQTutorialAudioProcessor::~EQTutorialAudioProcessor()
{
}

//==============================================================================
const juce::String EQTutorialAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQTutorialAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQTutorialAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQTutorialAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQTutorialAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQTutorialAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQTutorialAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQTutorialAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQTutorialAudioProcessor::getProgramName (int index)
{
    return {};
}

void EQTutorialAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
// PREPARE
//==============================================================================
void EQTutorialAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Setup the spec
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; // because we are using mono due to dsp
    spec.sampleRate = sampleRate;

    // Prepare the filters with the spec
    // it is given to the chain, then accessible to the filters
    leftChain.prepare(spec);
    rightChain.prepare(spec);


    // get the settings
    auto chainSettings = getChainSettings(treeState);

    // setup Peak coefficients
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibals));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

}

void EQTutorialAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQTutorialAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//==============================================================================
// PROCESS BLOCK
//==============================================================================
void EQTutorialAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // clear buffer
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // UPDATE
    // PARAMETERS
    
    // get the settings
    auto chainSettings = getChainSettings(treeState);

    // setup Peak coefficients
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibals));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;




    // PROCESS
    // AUDIO

    // wrap the buffer in an AudioBlock
    juce::dsp::AudioBlock<float> block(buffer);

    // extract each channel from the buffer
    // store in a block of it's own
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(0);

    // wrap each block in a Context, so the chain can use it
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // send the Contexts to the chain
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool EQTutorialAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQTutorialAudioProcessor::createEditor()
{
    // return new EQTutorialAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EQTutorialAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EQTutorialAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}




ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& treeState) {
    ChainSettings settings;

    settings.lowCutFreq = treeState.getRawParameterValue("lowcut_freq")->load();
    settings.highCutFreq = treeState.getRawParameterValue("highcut_freq")->load();
    settings.peakFreq = treeState.getRawParameterValue("peak_freq")->load();
    settings.peakGainInDecibals = treeState.getRawParameterValue("peak_gain")->load();
    settings.peakQuality = treeState.getRawParameterValue("peak_quality")->load();
    settings.lowCutSlope = treeState.getRawParameterValue("lowcut_slope")->load();
    settings.highCutSlope = treeState.getRawParameterValue("highcut_slope")->load();

    return settings;
}




//==============================================================================
// PARAMETER LAYOUT
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout EQTutorialAudioProcessor::createParameterLayout() {

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // add LowCut parameter float
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowcut_freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.f, 1.f, 1.f),
                                                           20.f));
    // add HighCut parameter float
    layout.add(std::make_unique<juce::AudioParameterFloat>("highcut_freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.f, 1.f, 1.f),
                                                           20000.f));
    // add Peak parameter float
    layout.add(std::make_unique<juce::AudioParameterFloat>("peak_freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.f, 1.f, 1.f),
                                                           750.f));
    // add Peak parameter float
    layout.add(std::make_unique<juce::AudioParameterFloat>("peak_gain", 
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.0f, 24.f, 0.5f, 1.f),
                                                           0.0f));
    // add Q parameter float
    layout.add(std::make_unique<juce::AudioParameterFloat>("peak_quality", 
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    // decalre empty StringArray
    juce::StringArray stringArray;

    // loop 0 - 4 and create a new string with the suffix db/Oct
    // add all strings to the StringArray
    for (int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i*12);
        str << " dc/Oct";
        stringArray.add(str);
    }

    // add LowCut slope
    layout.add(std::make_unique<juce::AudioParameterChoice>("lowcut_slope",
                                                            "LowCut Slope",
                                                            stringArray,
                                                            0));
    // add HighCut slope
    layout.add(std::make_unique<juce::AudioParameterChoice>("highcut_slope", 
                                                            "HighCut Slope",
                                                            stringArray,
                                                            0));

    return layout;
};

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQTutorialAudioProcessor();
}
