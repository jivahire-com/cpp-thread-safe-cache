#
# Copyright (c) Microsoft Corporation.
#

# simple script to decode binary of power log entries
# Adapted from Pioneer, authored originally by Tim Phillips

import argparse
import matplotlib.pyplot as plt
import struct
import re
import matplotlib
import importlib.util

def ensure_package_installed(package_name, install_name=None):
    if importlib.util.find_spec(package_name) is None:
        print(f"Package '{package_name}' is not installed. Please install it using: pip install {install_name or package_name}")

# Ensure required packages are installed
ensure_package_installed('matplotlib')
ensure_package_installed('PyQt5', 'pyqt5')

# Set the Matplotlib backend to Qt5Agg as an alternative to TkAgg
matplotlib.use('Qt5Agg')

def decodeDstAvl(data):
    (timestamp, socpwr, soccap, vcpupwr, vcpucap) = struct.unpack("<Q4f8x", data)
    return { 'socpwr': socpwr, 'soccap': soccap, 'vcpupwr': vcpupwr, 'vcpucap': vcpucap }

def decodePID(data):
    (timestamp, piderror, pid_p, pid_i, pid_d, resavl, resscaled) = struct.unpack("<Q4f2I", data)
    return { 'piderror': piderror, 'pid_p': pid_p, 'pid_i': pid_i, 'pid_d': pid_d } # , 'resavl': resavl, 'resscaled': resscaled

def decodePlimit(data):
    (timestamp, cores0, cores1, plimit, rackcap) = struct.unpack("<Q2QB?6x", data)
    return { 'cores': [cores0, cores1], 'plmt': plimit } # , 'rackcap': rackcap

def decodePrioritySelections(data):
    (timestamp, pri0, pri1, pri2, pri3, pri4, pri5, pri6, pri7) = struct.unpack("<Q8B16x", data)
    return { 'pri0': pri0, 'pri1': pri1, 'pri2': pri2, 'pri3': pri3, 'pri4': pri4, 'pri5': pri5, 'pri6': pri6, 'pri7': pri7 }

# returns first, last indexes of data based on the ts (timestamp) keys
def dataFirstLast(datalist):
    startidx = 0
    endidx = 0
    
    for idx in range(0, len(datalist)):
        if (datalist[idx]['type']!=0xFF):
            startidx = idx
            break
    lastts = datalist[startidx]['ts']
    lastdelta = lastts - datalist[len(datalist)-1]['ts']
    for idx in range(startidx+1, len(datalist)):
        delta = (datalist[idx]['ts'] - lastts)
        if ((datalist[idx]['ts'] < lastts) or (delta > 0.050) or (delta < 0)) or (endidx == 0):
            endidx = idx - 1        
        lastts = datalist[idx]['ts']
    if (lastdelta <= .050) and (lastdelta >= 0):
        startidx = endidx + 1

    return (startidx, endidx)

# for each core in 'cores' key expands other keys to one per core        
def expandCores(decoded):
    new = {}
    for key in decoded:
        if key == 'cores':
            continue
        for core in range(0, 128):
            aidx = 1 if (core>=64) else 0
            present = (1<<(core%64)) & decoded['cores'][aidx] != 0
            if present:
                newkey = f"{key}_core{core}"
                new[newkey]=decoded[key]
    return new

# function to print updating percentage
def printPerc(value, value2=None):
    if value2 is not None:
        print(f"\b\b\b{int((value*100)/value2):03d}",end='', flush=True)
    elif value==0:
        print("[000%]\b\b", end='')
    else:
        print("\b\b\b100", flush=True)
        
    return

# plotkeys is a list of lists, where each is a plot and the keys that should be graphed on that plot
def plot(data, plotkeys):
    print("Generating plots ", end='')
    printPerc(0)
    fig, graphs = plt.subplots(figsize = (16,8), nrows=len(plotkeys), ncols=1, sharex=True, constrained_layout=True)
    fig.suptitle('', fontsize=16)
        
    for plotidx in range(0, len(plotkeys)):
        printPerc(plotidx, len(plotkeys))
        if len(plotkeys) > 1:
            graph = graphs[plotidx]
        else:
            graph = graphs
        plotlist = plotkeys[plotidx]
        graph.set_title(f"Plot{plotidx}")
        for keyidx in plotlist:
            times = []
            values = []
            for entry in data:
                if keyidx in entry:
                    times.append(entry['ts'])
                    values.append(entry[keyidx])
            graph.plot(times, values, label=[keyidx])
            graph.legend(bbox_to_anchor=(1,1), loc="upper left")

    plt.xlabel("Time (s)")
    printPerc(100)
    plt.show()
    plt.savefig("pwr_log_plot.png")
    return

# generate a csv, with the keys given in csvkeys
def generateCsv(data, csvkeys, csvfile):
    print(f"Generating csv ({csvfile}) ", end='')

    printPerc(0)
    
    with open(csvfile,"w") as f:
        csvstr = ""
        for key in csvkeys:
            csvstr = csvstr + key + ","
        f.write(csvstr+"\n")
        for dataidx in range(0, len(data)):
            entry = data[dataidx]
            printPerc(dataidx, len(data))
            csvstr = ""
            for key in csvkeys:
                value = ""
                if key in entry:
                    value = f"{entry[key]}"
                csvstr = csvstr + value + ","
            f.write(csvstr+"\n")
    printPerc(100)
    

# given a comma separated string, output a list of keys to be plotted, etc
def getPlotKeys(keystring, foundKeys):
    keys = []
    if keystring != None:
        plotkeys=keystring.split(",")
        # for every key we've been asked to plot, if there's not an exact match, try to partial match
        newkeys = [entry for key in plotkeys for entry in foundKeys if (key not in foundKeys) and re.match(key, entry)]
        # sort the partial matches
        newkeys.sort()
        # remove the keys from the original list if they didn't exist
        keys = [key for key in plotkeys if key in foundKeys]
        # create a final set of keys to plot
        keys = keys + newkeys

    return keys

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()

    parser.add_argument("input_file", help="path to input binary")
    parser.add_argument("--plot1", help="comma separated list of values to plot")
    parser.add_argument("--plot2", help="comma separated list of values to plot")
    parser.add_argument("--plot3", help="comma separated list of values to plot")
    parser.add_argument('-l', '--list', help="list found data keys", action='store_true')
    parser.add_argument("--csvfile", help="Name of CSV output file (default=power.csv)", default="power.csv")
    parser.add_argument("--csv", help="comma separated list of values to add to .csv file")
    parser.add_argument("--entry_type", help="Specify the entry type to display", type=int)
    args = parser.parse_args()
    
    print ("Opening file {}".format(args.input_file))
    
    log_struct = '<Q24x'  # timestamp & type, 24 byte payload
    log_len = struct.calcsize(log_struct)

    #unpack function
    log_unpack = struct.Struct(log_struct).unpack_from

    values = []
    entries = []
    foundKeys = set()

    with open(args.input_file, "rb") as f:
        while True:
            data = f.read(log_len)
            if not data: 
                #print("End of file reached.")  # Debug print
                break
            #print(f"Raw data: {data}")  # Debug print
            (timestamp,) = struct.unpack(log_struct, data)
            #print(f"Unpacked timestamp: {timestamp}")  # Debug print
            # upper 8 bits are type
            type = (timestamp >> 56) & 0xFF
            #print(f"Entry type: {type}")  # Debug print
            # timestamp is the rest
            timestamp = (timestamp & ((1 << 56)-1)) / 125000000
            #print(f"Processed timestamp: {timestamp}")  # Debug print
            entries.append({'ts': timestamp, 'type': type, 'data': data});
            #print(f"Entry added: {{'ts': {timestamp}, 'type': {type}}}")  # Debug print

    # Add a print statement to display the number of entries after reading the file
    print(f"Total number of entries read: {len(entries)}")

    (sidx, eidx) = dataFirstLast(entries)

    # start timestamp to use for calculating time since start
    start_ts = entries[sidx]['ts']
    end_ts = entries[eidx]['ts']
    end_time = end_ts - start_ts

    print(f"Start index (sidx): {sidx}, Start timestamp (start_ts): {start_ts}")
    print(f"End index (eidx): {eidx}, End timestamp (end_ts): {end_ts}")

    # print percentage
    print("Processing input ", end='')
    printPerc(0)

    # Add a dictionary to count entries by type
    type_counts = {}
    
    while (sidx != eidx):
        entry = entries[sidx]
        ts = entry['ts'] - start_ts
        
        # print percentage complete
        printPerc(ts, end_time)
        
        # decode current entry
        decoded = {}
        if (entry['type'] == 1):
            decoded = decodeDstAvl(entry['data'])
        if (entry['type'] == 2):
            decoded = decodePID(entry['data'])
        if (entry['type'] == 3):
            decoded = decodePlimit(entry['data'])
        if (entry['type'] == 4):
            decoded = decodePrioritySelections(entry['data'])

        # if entry had cores field, expand entry to one per core included in cores bitfield
        if 'cores' in decoded:
            decoded = expandCores(decoded)

        # add timestamp to decoded entry
        decoded['ts'] = ts
        values.append(decoded)
        
        # add to foundkeys
        newfound = {key for key in decoded if key != 'ts'}
        foundKeys.update(newfound)

        # Increment the type count
        type_counts[entry['type']] = type_counts.get(entry['type'], 0) + 1

        sidx = (sidx + 1) % len(entries)

    printPerc(100)
    print(f"Found {end_time:0.03f}s of log data")

    # Print the number of entries found for each type
    print("Entries found by type:")
    for entry_type, count in type_counts.items():
        print(f"Type {entry_type}: {count} entries")

    plots = []
    plotkeys = getPlotKeys(args.plot1, foundKeys)
    if (len(plotkeys) > 0): plots.append(plotkeys)
    plotkeys = getPlotKeys(args.plot2, foundKeys)
    if (len(plotkeys) > 0): plots.append(plotkeys) 
    plotkeys = getPlotKeys(args.plot3, foundKeys)
    if (len(plotkeys) > 0): plots.append(plotkeys) 
    
    csvkeys = getPlotKeys(args.csv, foundKeys)
    if (len(csvkeys) > 0): 
        csvkeys = ['ts'] + csvkeys
        generateCsv(values, csvkeys, args.csvfile)

    # Modify the script to generate a plot for each type
    plots_by_type = {}

    # Group data by type for plotting
    for entry in values:
        entry_type = entry.get('type', None)
        if entry_type is not None:
            if entry_type not in plots_by_type:
                plots_by_type[entry_type] = []
            plots_by_type[entry_type].append(entry)

    # Generate a plot for each type
    for entry_type, data in plots_by_type.items():
        print(f"Generating plot for type {entry_type}")
        plotkeys = [key for key in data[0].keys() if key != 'ts' and key != 'type']
        if plotkeys:
            plot(data, [plotkeys])
            plt.savefig(f"pwr_log_plot_type_{entry_type}.png")  # Save each plot with a unique filename

    # Check if the user provided an entry type
    if args.entry_type is not None:
        entry_type_to_display = args.entry_type
        if entry_type_to_display in plots_by_type:
            print(f"Displaying plot for entry type {entry_type_to_display}")
            data = plots_by_type[entry_type_to_display]
            plotkeys = [key for key in data[0].keys() if key != 'ts' and key != 'type']
            if plotkeys:
                plot(data, [plotkeys])
                plt.show()  # Display the plot interactively
        else:
            print(f"Entry type {entry_type_to_display} not found in the data.")

    if len(plots) > 0:
        plot(values, plots) 

    if (args.list):
        print("Found keys")
        print(foundKeys)




