import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf
import IPython
from scipy.signal import spectrogram, lfilter, freqz, tf2zpk
from scipy.stats import pearsonr

#sentence_list = ('sa1.wav', 'si1473.wav', 'si838.wav', 'sx208.wav', 'sx298.wav', 'sa2.wav', 'si2098.wav', 'sx118.wav', 'sx28.wav', 'sx388.wav')
#query_list = ('q1.wav', 'q2.wav')

sentence_list = ('g_sentence.wav',)
query_list = ('attendance.wav', 'promotion.wav')

def get_feature(audio_path):
    s, fs = sf.read('../' + audio_path)
    f, t, sgr = spectrogram(s, fs, nfft=511, nperseg=400, noverlap=240)

    sgr_log = 10 * np.log10(sgr+1e-20)
    sgr_sum = list()
    for i in range(0, len(sgr_log), 16):
        sgr_sum.append([sum(x) for x in zip(*sgr_log[i:i+16])])

    return sgr_sum

def make_final_graph(sentence):
    # Podgraf signalu zvuku
    s, fs = sf.read('../sentences/' + sentence)
    t = np.arange(s.size) / fs
    fig, axes = plt.subplots(nrows=3,ncols=1, figsize=(16,9))
    axes[0].set_xlabel('Čas [s]')
    axes[0].set_ylabel('Signal')
    axes[0].set_title('"increasing" a "overcharged" vs. ' + sentence)
    axes[0].plot(t, s)

    # Podgraf spektrogramu
    axes[1].set_xlabel('Čas [s]')
    axes[1].set_ylabel('Parametry')

    f_s, t_s, sgr = spectrogram(s, fs, nfft=511, nperseg=400, noverlap=240)

    sgr_log = 10 * np.log10(sgr+1e-20)
    sgr_sum = list()
    for i in range(0, len(sgr_log), 16):
        sgr_sum.append([sum(x) for x in zip(*sgr_log[i:i+16])])
    axes[1].pcolormesh(t_s, range(16) ,sgr_sum)
   
    # Podgraf korelace s queries
    t = np.arange(s.size) / fs
    axes[2].set_xlabel('Čas [s]')
    axes[2].set_ylabel('Koeficient korelace')
    tmp_correlation = correlation_distance(sentence, query_list[0])
    axes[2].plot(t_s[:len(tmp_correlation)], tmp_correlation)
    tmp_correlation = correlation_distance(sentence, query_list[1])
    axes[2].plot(t_s[:len(tmp_correlation)], tmp_correlation)
    axes[2].plot(t_s, len(t_s) * [0.5,])
    axes[2].legend(["increasing", "overcharged", "Hranice rozpoznání slova"])
    plt.tight_layout(),
    plt.savefig('../' + sentence + '.png')
    plt.savefig('../' + sentence + '.jpg')

def correlation_distance(sentence_name, query_name):
    sentence = get_feature('sentences/' + sentence_name)
    query = get_feature('queries/' + query_name)
    correlation_distance_data = list()
    correlation_sum = 0
    for i in range(0, len(sentence[0]) - len(query[0])):
        for j in range(0, 16):
            correlation_sum += pearsonr(query[j], sentence[j][i:i+len(query[0])])[0]
        correlation_distance_data.append(correlation_sum/ 16.)
        correlation_sum = 0
    return correlation_distance_data

for sentence in sentence_list:
    make_final_graph(sentence)
