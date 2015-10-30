import matplotlib.pyplot as plt
import csv

def get_data(filename):
    """Returns a dict where the keys are column headers, and the values are the
    list of values in the column"""
    data = {}
    with open(filename, 'r') as csvfile:
        reader = csv.reader(csvfile)
        headers = next(reader)
        for h in headers:
            data[h] = []
        for datarow in reader:
            assert(len(headers) == len(datarow))
            for i in range(len(headers)):
                if headers[i] == 'Threads':
                    # Special case conversions
                    data[headers[i]].append(int(datarow[i]))
                elif headers[i] in ['libcuckoo', 'tbb']:
                    data[headers[i]].append(float(datarow[i]))
                elif headers[i] == 'Hashpower':
                    data[headers[i]].append(2**int(datarow[i]))
                elif headers[i] == 'Insert Percentage':
                    data[headers[i]].append(int(datarow[i]))
                else:
                    data[headers[i]].append(datarow[i])
    return data

def plot_data(data, title, xlabel, ylabel, linelabels, makexlog=False,
              legendupperleft=True):

    fig = plt.figure(facecolor='white', edgecolor='black')

    ax = fig.add_subplot(111)
    lines = []
    for linelabel in linelabels:
        line, = plt.plot(data[xlabel], data[linelabel], marker='o',
                         linestyle='-', label=linelabel)
        lines.append(line)

    ax.set_axis_bgcolor('white')
    ax.set_title(title)
    if makexlog:
        ax.set_xscale('log', basex=2)
    ax.set_xlabel(xlabel)
    ax.set_xlim((min(data[xlabel])*0.9, max(data[xlabel])*1.1))
    ax.set_ylabel(ylabel)
    ax.grid(False)

    plt.legend(lines, linelabels)
    plt.xticks(data[xlabel])
    plt.tight_layout()

    if legendupperleft:
        legend = plt.legend(loc='upper left')
    else:
        legend = plt.legend()
    legend.get_frame().set_facecolor('none')
    legend.get_frame().set_linewidth(0)
    plt.show()
