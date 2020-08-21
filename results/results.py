import math
import numpy as np
import matplotlib.pyplot as plt

def is_pow2(x):
    return math.log2(x).is_integer()


def get_results_from_list(idx, lines, results):
    ir_size = lines[idx].split()[4]
    results['keys'].append(ir_size)

    blocks = {}
    for i in range(1, 6):
        out = lines[idx + i].split()
        blocks[out[0][:-1]] = 10.0 / float(out[1])

    results[ir_size] = blocks
    return idx+6


def get_results_from_file(file):
    file_text = open(file, 'r')
    lines = file_text.readlines()
    
    pow_results = { 'keys': [] }
    prime_results = { 'keys': [] }

    idx = 0
    while idx < len(lines):
        line = lines[idx].strip()

        if len(line) == 0:
            idx += 1
            continue

        if line.split()[0] == 'Running':
            ir_size = line.split()[4]
            pow2 = is_pow2(int(ir_size))

            results = pow_results if pow2 else prime_results
            idx = get_results_from_list(idx, lines, results)
            continue
        
        idx += 1

    return pow_results, prime_results

def plot_results(results, title='', file=None):
    keys = results['keys']
    types = ['JuceConv', 'JuceFIR', 'InnerProdFIR', 'InnerProdNoWrapFIR', 'SimdFIR']

    ind = np.arange(len(keys))
    width = 0.15
    plt.figure()
    for i, t in enumerate(types):
        times = []
        for k in keys:
            times.append(results[k][t])

        offset = (width * (len(types) - 1) / 2) - width / 2
        plt.bar(ind - offset + width * i, times, width, label=t)

    plt.ylabel('Speed [seconds audio / ms]')
    plt.xlabel('IR Length [samples]')
    plt.title(title)

    plt.xticks(ind + width / 2, keys)
    plt.legend(loc='best')

    if file:
        plt.savefig(file)

win_pow, win_prime = get_results_from_file('results/results_win.txt')
mac_pow, mac_prime = get_results_from_file('results/results_mac.txt')

plot_results(win_pow, 'Power of 2 Benchmarks (Windows)', 'results/figures/win_pow.png')
plot_results(win_prime, 'Prime Benchmarks (Windows)', 'results/figures/win_prime.png')
plot_results(mac_pow, 'Power of 2 Benchmarks (Macintosh)', 'results/figures/mac_pow.png')
plot_results(mac_prime, 'Prime Benchmarks (Macintosh)', 'results/figures/mac_prime.png')
