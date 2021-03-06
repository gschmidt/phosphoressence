// __BEGIN_LICENSE__
// Copyright (C) 2009 Michael J. Broxton
// All Rights Reserved.
// __END_LICENSE__

#ifndef __AUDIO_ENGINE_H__
#define __AUDIO_ENGINE_H__

#include <vw/Core.h>
#include <vw/Math/Vector.h>
#include <portaudio.h>

#include <PeParameters.h>
#include <FFT.h>

// Basic audio settings
#define NUM_CHANNELS    (2)
#define FFT_SAMPLES     512
#define AUDIO_SAMPLE_RATE 96000.0

class AudioListener {

  float m_buffer_length; // seconds
  int m_sample_rate;     // hz

protected:

  // An Audio Ring Buffer
  struct AudioBuffer {
    int          read_index;  
    int          write_index;  
    int          max_frame_index;
    float        *samples;
  };
  AudioBuffer m_data;

  // This mutex must be locked by child classes when they access the
  // AudioBuffer.  Perhaps this should be abstracted away behind some
  // function call?
  vw::Mutex m_mutex;

public:
  AudioListener(float buffer_length = 5.0) : 
    m_buffer_length(buffer_length), 
    m_sample_rate(AUDIO_SAMPLE_RATE) {
  
    // Set up the ring buffer for storing the audio
    m_data.read_index = 0;
    m_data.write_index = 0;
    m_data.max_frame_index = m_buffer_length * m_sample_rate; 

    int num_samples = m_data.max_frame_index * NUM_CHANNELS;
    m_data.samples = (float *) malloc( num_samples * sizeof(float) );
    if( !(m_data.samples) ) 
      vw::vw_throw(vw::LogicErr() << "Could not allocate audio recording buffer.\n");
  }

  float sample_rate() const { return m_sample_rate; }

  void audio_callback(float* input, unsigned long frame_count, int num_channels) {
    const float *rptr = (const float*)input;
    float *data_ptr = &(m_data.samples[m_data.write_index * NUM_CHANNELS]);
    
    for( int i=0; i<frame_count; ++i) {
      
      if( input == NULL ) {
        *data_ptr++ = 0.0f;                                   // Left
        if( NUM_CHANNELS == 2 ) *data_ptr++ = 0.0f          ; // Right
      } else {
        *data_ptr++ = *rptr++;                                // Left
        if( NUM_CHANNELS == 2 ) *data_ptr++ = *rptr++;        // Right
      }

      // Reset the data pointer if we have reached the end of the
      // buffer.
      (m_data.write_index)++;
      if ( m_data.write_index >= m_data.max_frame_index ) {
        m_data.write_index = 0;
        data_ptr = &(m_data.samples[0]);
      }
    }
  }

};

/// AudioThread
///
/// This thread capture audio input using PortAudio.  Classes can
/// register as "AudioListeners" to have their callback called when
/// new audio samples become available.
class AudioThread {
  vw::Mutex m_mutex;

  // Port Audio Input Stream
  PaStream *m_stream;

  // List of Audio Listeners
  std::list<boost::shared_ptr<AudioListener> > m_listeners;

public:
  int member_callback(const void *input, void *output,
                      unsigned long frameCount,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags);

  // This static class helps to bridge the gap between PortAudio's C
  // API and the world of C++.  It simply forwards the callback to
  // this instance's member_callback() method.
  static int pa_callback(const void *input, void *output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData ) {
    return ((AudioThread*)userData)->member_callback(input, output, 
                                                     frameCount, timeInfo, 
                                                     statusFlags);
  }
  
  AudioThread(int sample_rate = AUDIO_SAMPLE_RATE);
  ~AudioThread();

  void register_listener(boost::shared_ptr<AudioListener> listener) {
    vw::Mutex::Lock lock(m_mutex);
    m_listeners.push_back(listener);
  }

  void clear_listeners() {
    vw::Mutex::Lock lock(m_mutex);
    m_listeners.clear();
  }

};

#endif // __AUDIOENGINE_H__
