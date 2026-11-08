#include "VoiceChat.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>

struct VoiceChat::Impl
{
    struct Playback { VoicePacket packet;float gain,pan,clarity; };
    std::atomic<bool> stop{false},talking{false},muted{false},flush{false};
    std::atomic<int> deviceStatus{0};
    std::atomic<bool> devicesEnabled{true};
    std::mutex mutex;
    std::deque<VoicePacket> capture;
    std::array<std::deque<Playback>,4> playback;
    std::thread worker;

    void run()
    {
        WAVEFORMATEX mono{};mono.wFormatTag=WAVE_FORMAT_PCM;mono.nChannels=1;
        mono.nSamplesPerSec=voiceRate;mono.wBitsPerSample=16;mono.nBlockAlign=2;mono.nAvgBytesPerSec=voiceRate*2;
        auto stereo=mono;stereo.nChannels=2;stereo.nBlockAlign=4;stereo.nAvgBytesPerSec=voiceRate*4;
        HWAVEIN input=nullptr;HWAVEOUT output=nullptr;
        std::array<std::array<std::int16_t,voiceSamples>,4> inputSamples{};
        std::array<std::array<std::int16_t,voiceSamples*2>,2> outputSamples{};
        std::array<WAVEHDR,4> inputHeaders{};
        std::array<WAVEHDR,2> outputHeaders{};
        std::array<float,4> filtered{};
        std::array<bool,2> outputQueued{};
        std::uint32_t sequence=0;
        bool triedInput=false,triedOutput=false;
        const auto closeInput=[&]
        {
            if(!input)return;
            waveInReset(input);
            for(auto& header:inputHeaders)if(header.dwFlags&WHDR_PREPARED)waveInUnprepareHeader(input,&header,sizeof(header));
            waveInClose(input);input=nullptr;
        };
        const auto closeOutput=[&]
        {
            if(!output)return;
            waveOutReset(output);
            for(auto& header:outputHeaders)if(header.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(output,&header,sizeof(header));
            waveOutClose(output);output=nullptr;outputQueued.fill(false);
        };
        while(!stop.load())
        {
            if(flush.exchange(false)){closeInput();closeOutput();filtered.fill(0.f);triedInput=triedOutput=false;}
            const bool enabled=talking.load() && !muted.load() && devicesEnabled.load();
            if(!enabled){closeInput();triedInput=false;}
            if(enabled && !input && !triedInput)
            {
                triedInput=true;
                if(waveInOpen(&input,WAVE_MAPPER,&mono,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)
                {input=nullptr;deviceStatus.store(-1);}
                else
                {
                    bool ready=true;
                    for(std::size_t i=0;i<inputHeaders.size();++i)
                    {
                        auto& header=inputHeaders[i];header={};
                        header.lpData=reinterpret_cast<LPSTR>(inputSamples[i].data());header.dwBufferLength=voiceSamples*2;
                        if(waveInPrepareHeader(input,&header,sizeof(header))!=MMSYSERR_NOERROR ||
                            waveInAddBuffer(input,&header,sizeof(header))!=MMSYSERR_NOERROR){ready=false;break;}
                    }
                    if(!ready || waveInStart(input)!=MMSYSERR_NOERROR){closeInput();deviceStatus.store(-1);}
                    else deviceStatus.store(1);
                }
            }
            if(input)for(std::size_t i=0;i<inputHeaders.size();++i)
            {
                auto& header=inputHeaders[i];if(!(header.dwFlags&WHDR_DONE))continue;
                if(header.dwBytesRecorded==voiceSamples*2 && talking.load() && !muted.load())
                {
                    VoicePacket packet;packet.sequence=++sequence;
                    for(std::size_t sample=0;sample<voiceSamples;++sample)packet.samples[sample]=voiceEncodeSample(inputSamples[i][sample]);
                    std::lock_guard<std::mutex> lock(mutex);
                    if(talking.load() && !muted.load())
                    {if(capture.size()>=4)capture.pop_front();capture.push_back(packet);}
                }
                header.dwBytesRecorded=0;waveInAddBuffer(input,&header,sizeof(header));
            }
            bool hasPlayback=false;
            {std::lock_guard<std::mutex> lock(mutex);for(const auto& queue:playback)hasPlayback=hasPlayback || !queue.empty();}
            if(devicesEnabled.load() && hasPlayback && !output && !triedOutput && !muted.load())
            {
                triedOutput=true;
                if(waveOutOpen(&output,WAVE_MAPPER,&stereo,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)
                {output=nullptr;deviceStatus.store(-2);}
                else for(std::size_t i=0;i<outputHeaders.size();++i)
                {
                    auto& header=outputHeaders[i];header={};
                    header.lpData=reinterpret_cast<LPSTR>(outputSamples[i].data());header.dwBufferLength=voiceSamples*4;
                    if(waveOutPrepareHeader(output,&header,sizeof(header))!=MMSYSERR_NOERROR){closeOutput();deviceStatus.store(-2);break;}
                    outputQueued[i]=false;
                }
            }
            if(output)for(std::size_t buffer=0;buffer<outputHeaders.size();++buffer)
            {
                auto& header=outputHeaders[buffer];if(outputQueued[buffer] && !(header.dwFlags&WHDR_DONE))continue;
                std::array<float,voiceSamples*2> mix{};
                for(int actor=0;actor<4;++actor)
                {
                    Playback next;bool available=false;
                    {std::lock_guard<std::mutex> lock(mutex);if(!playback[actor].empty()){next=playback[actor].front();playback[actor].pop_front();available=true;}}
                    if(!available)continue;
                    const float alpha=1.f-std::exp(-6.2831853f*(350.f+next.clarity*6000.f)/voiceRate);
                    const float left=std::sqrt((1.f-next.pan)*.5f)*next.gain,right=std::sqrt((1.f+next.pan)*.5f)*next.gain;
                    for(std::size_t sample=0;sample<voiceSamples;++sample)
                    {
                        filtered[actor]+=(voiceDecodeSample(next.packet.samples[sample])/32768.f-filtered[actor])*alpha;
                        mix[sample*2]+=filtered[actor]*left;mix[sample*2+1]+=filtered[actor]*right;
                    }
                }
                for(std::size_t i=0;i<mix.size();++i)outputSamples[buffer][i]=muted.load()?0:static_cast<std::int16_t>(std::clamp(mix[i],-.95f,.95f)*32767.f);
                if(waveOutWrite(output,&header,sizeof(header))!=MMSYSERR_NOERROR){closeOutput();deviceStatus.store(-2);break;}
                outputQueued[buffer]=true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        closeInput();closeOutput();
    }
};

VoiceChat::VoiceChat():impl_(std::make_unique<Impl>()) {impl_->worker=std::thread(&Impl::run,impl_.get());}
void VoiceChat::setDevicesEnabled(bool enabled){impl_->devicesEnabled.store(enabled);reset();}
VoiceChat::~VoiceChat(){impl_->stop.store(true);if(impl_->worker.joinable())impl_->worker.join();}
void VoiceChat::setTalking(bool held)
{
    impl_->talking.store(held);
    if(!held){std::lock_guard<std::mutex> lock(impl_->mutex);impl_->capture.clear();}
}
void VoiceChat::setMuted(bool muted){impl_->muted.store(muted);if(muted)reset();}
void VoiceChat::play(const VoicePacket& packet,float gain,float pan,float clarity)
{
    if(packet.actor<0 || packet.actor>3 || impl_->muted.load() || !std::isfinite(gain) || !std::isfinite(pan) || !std::isfinite(clarity))return;
    std::lock_guard<std::mutex> lock(impl_->mutex);auto& queue=impl_->playback[packet.actor];
    if(queue.size()>=4)queue.pop_front();
    queue.push_back({packet,std::clamp(gain,0.f,1.f),std::clamp(pan,-1.f,1.f),std::clamp(clarity,0.f,1.f)});
}
std::vector<VoicePacket> VoiceChat::captured()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    std::vector<VoicePacket> result(impl_->capture.begin(),impl_->capture.end());impl_->capture.clear();return result;
}
void VoiceChat::reset()
{
    impl_->talking.store(false);impl_->flush.store(true);
    std::lock_guard<std::mutex> lock(impl_->mutex);impl_->capture.clear();for(auto& queue:impl_->playback)queue.clear();
}
int VoiceChat::status() const noexcept{return impl_->deviceStatus.load();}
