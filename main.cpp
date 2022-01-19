#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace std;

#define PI 3.14159265


struct WavHead
{
    WavHead(double duration)
    {
        setDuration(duration);
    }

    WavHead()
    {
        setDataSize(0);
    }


    // Riff chunk
    const char chunk_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size;
    const char format[4] = {'W', 'A', 'V', 'E'};

    // FMT sub-shank
    const char subchunk1_id[4] = {'f', 'm', 't', ' '};
    const uint32_t subchunk1_size = 16;
    const uint16_t audio_format = 1;
    const uint16_t num_channels = 2;
    const uint32_t sample_rate = 44100;
    const uint32_t byte_rate = sample_rate *
            num_channels * (subchunk1_size / 8);
    const uint16_t block_align = num_channels * (subchunk1_size / 8);
    const uint16_t bits_per_sample = 16;

    // Data sub-chunk
    const char subchunk2_id[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2_size;

    void setDataSize(uint32_t data_size)
    {
        chunk_size = data_size + (0x2C - 0x08);
        subchunk2_size = data_size;
    }

    uint32_t getDataSize() const
    {
        return subchunk2_size;
    }

    void setDuration(double duration)
    {
        uint32_t data_size = duration * sample_rate * num_channels * sizeof(uint16_t);
        setDataSize(data_size);
    }

};

void writeHead(ofstream &file, WavHead &wavhead)
{
    file.write(reinterpret_cast<const char*>(&wavhead), sizeof(wavhead));
}

void writeData(ofstream &file, void* data, uint32_t data_size)
{
    file.write(reinterpret_cast<const char*>(data), data_size);
}


enum class ChannelMode
{
    left,
    right,
    both
};

class Sound
{
public:
    Sound(double _duration, const WavHead &_wavhead) :
        wavhead(_wavhead)
    {
        duration = _duration;
        times_count = _duration * wavhead.sample_rate;

        values_l = new double[times_count];
        values_r = new double[times_count];
        values_norm_l = new int16_t[times_count];
        values_norm_r = new int16_t[times_count];

        wav_data = new int16_t[times_count * 2];


        for (int i = 0; i < times_count; i++)
        {
            values_l[i] = 0.;
            values_r[i] = 0.;
        }
        
    }

    ~Sound()
    {
        delete[] values_l;
        delete[] values_r;
        delete[] values_norm_l;
        delete[] values_norm_r;
        delete[] wav_data;
    }

    void addSine(double freq, double amp = 1.0, double phase = 0.0,
        ChannelMode channel = ChannelMode::both)
    {
        for (int i = 0; i < times_count; i++)
        {
            double time = getTime(i);

            if (channel == ChannelMode::left || channel == ChannelMode::both)
            {
                values_l[i] += amp * sin(time * freq * 2 * PI + phase);
            }

            if (channel == ChannelMode::right || channel == ChannelMode::both)
            {
                values_r[i] += amp * sin(time * freq * 2 * PI + phase);
            }
        }
    }

    void addSignal(double(*f)(double t), ChannelMode channel = ChannelMode::both)
    {
        for (int i = 0; i < times_count; i++)
        {
            double time = getTime(i);

            if (channel == ChannelMode::left || channel == ChannelMode::both)
            {
                values_l[i] += f(time);
            }

            if (channel == ChannelMode::right || channel == ChannelMode::both)
            {
                values_r[i] += f(time);
            }
        }
    }


    int16_t* GetWavData()
    {
        convert();
        for (int i = 0; i < times_count; i++)
        {
            wav_data[i * 2] = values_norm_l[i];
            wav_data[i * 2 + 1] = values_norm_r[i];
        }

        return wav_data;
    }


private:
    double getTime(uint32_t i)
    {
        double time = (double)i / wavhead.sample_rate;
        return time;
    }

    double convert()
    {
        double max_module = 0;
        for (int i = 0; i < times_count; i++)
        {
            if (abs(values_l[i]) > max_module)
            {
                max_module = abs(values_l[i]);
            }
            if (abs(values_r[i]) > max_module)
            {
                max_module = abs(values_r[i]);
            }
        }
        
        for (int i = 0; i < times_count; i++)
        {
            values_norm_l[i] = 
                values_l[i] / max_module * numeric_limits<int16_t>::max();
            
            values_norm_r[i] = 
                values_r[i] / max_module * numeric_limits<int16_t>::max();
        }
        
        return max_module;
    }

public:
    const WavHead &wavhead;

    double duration;
    uint32_t times_count;

    double* values_l;
    double* values_r;
    int16_t* values_norm_l;
    int16_t* values_norm_r;
    int16_t* wav_data;
};




int main()
{
    double duration = 5; // seconds

    WavHead wavhead(duration);

    ofstream file;
    file.open("file.wav", ios::binary);
    writeHead(file, wavhead);


 
    Sound sound(duration, wavhead);
    // sound.addSine(500);




    sound.addSignal(
        [](double t) -> double
        {
            if (t > 5./4 && t < 5-5./4)
            {
                return sin(2 * PI / tan(2 * PI * t / 10) * 100);
            }

            return sin(2 * PI * tan(2 * PI * t / 10) * 100);
        });


    writeData(file, sound.GetWavData(), wavhead.getDataSize());


    cout << "done" << endl;
}