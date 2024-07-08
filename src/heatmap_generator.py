import re
import os
import math
import subprocess
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
from mpl_toolkits.axes_grid1 import make_axes_locatable
import numpy as np

def atomic_throughput(workgroups, device, contention, padding, iters):
    result = subprocess.run(['./atomic_rmw_test.run', '-w '+str(workgroups), '-d '+str(device), '-c '+str(contention), 
                            '-p '+str(padding), '-i '+str(iters)], capture_output=True, text=True)
    match = re.search(r'Throughput: ([\d.]+) atomic operations per microsecond', result.stdout.strip())
    if match:
        return float(match.group(1))
    else:
        return 0

def device_information(workgroups, device, iters):
    result = subprocess.run(['./atomic_rmw_test.run', '-w '+str(workgroups), '-d '+str(device), '-c 1', 
                            '-p 1', '-i '+str(iters)], capture_output=True, text=True)
    match = re.search(r'Device: (.+), workgroups \((\d+), \d+\) x (\d+)', result.stdout.strip())
    return f"{match.group(2)},{match.group(3)}:{match.group(1)}"

def generate_heatmap(coordinates, title, filename):
    # workgroup and title extraction
    title_information = title.split(":", 1)
    workgroup_information = title_information[0].split(",")
    workgroup_size = int(workgroup_information[0])
    workgroups = int(workgroup_information[1])
    final_size = 0
    if workgroup_size * workgroups > 1024:
        final_size = 1024
    else:
        final_size = workgroup_size * workgroups
    grid_scale = int(math.log2(final_size)) + 1
    
    contention = grid_scale
    padding = grid_scale
    data_array = np.zeros((padding, contention))

    # Assign values to the data array based on coordinates
    for x, y, value in coordinates:
        x_index = int(np.log2(x))
        y_index = int(np.log2(y))
        data_array[y_index, x_index] = value

    data_min = math.floor(np.min(data_array))
    data_max = math.floor(np.max(data_array))

    # Set up the figure and axes
    fig_x = 10
    fig_y = 8
    fig, ax = plt.subplots(figsize=(fig_x, fig_y), dpi=300)

    heatmap = ax.imshow(data_array, cmap='viridis', vmin=data_min, vmax=data_max)

    plt.xlabel("Contention", fontsize=16, labelpad=16)
    plt.ylabel("Padding", fontsize=16, labelpad=0)

    # Set the tick locations and labels
    x_ticks = [2 ** i for i in range(contention)]
    ax.set_xticks(np.arange(len(x_ticks)))
    ax.set_xticklabels(x_ticks, fontsize=18)
    y_ticks = [2 ** i for i in range(padding)]
    ax.set_yticks(np.arange(len(y_ticks)))
    ax.set_yticklabels(y_ticks, fontsize=18)

    ax.invert_yaxis()  # Invert the y-axis

    description = title_information[1].split(", ")
   
    plt.title(description[0] + "\nContiguous_Access:_atomic_fetch_add\nWorkgroups: (" + workgroup_information[0] 
                + ", 1) x " + workgroup_information[1], fontsize=20)

    # Add colorbar
    data_step = math.floor((data_max - data_min) / 7)
    cbar_ticks = [math.floor(n) for n in range(data_min, data_max, data_step)]
    divider = make_axes_locatable(ax)
    cax = divider.append_axes("right", size="5%", pad=0.2)
    cbar = plt.colorbar(heatmap, cax=cax, ticks=cbar_ticks)
    cbar.set_label('Atomic Operations per Microsecond', rotation=270, labelpad=24, fontsize=18)
    cbar.ax.tick_params(labelsize=13)

    save_folder = "heatmaps"
    os.makedirs(save_folder, exist_ok=True)

    # Save as SVG
    savedfilename_svg = os.path.join(save_folder, filename.removesuffix(".txt") + ".svg")
    print(f"Saving '{savedfilename_svg}'...")
    plt.savefig(savedfilename_svg, format='svg', bbox_inches='tight')

    # Save as PNG
    savedfilename_png = os.path.join(save_folder, filename.removesuffix(".txt") + ".png")
    print(f"Saving '{savedfilename_png}'...")
    plt.savefig(savedfilename_png, format='png', bbox_inches='tight')

    plt.close()

def main(workgroups, device, rmw_iterations):

    title = device_information(workgroups, device, rmw_iterations) + ", contiguous_access: atomic_fa_relaxed"
   
    #Obtain results for different configurations of contention/padding
    coordinates = []
    for contention in (2**i for i in range(0, 11)):
        for padding in (2**i for i in range(0, 11)):
            value = atomic_throughput(workgroups, device, contention, padding, rmw_iterations)
            coordinates.append((contention, padding, value))
            print("Contention: ", contention, "\tPadding: ", padding, "\tThroughput: ", value)

    #Setup and generate heatmap
    plt.rcParams["font.serif"] = "cmr10, Computer Modern Serif, DejaVu Serif"
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["axes.formatter.use_mathtext"] = True
    plt.rcParams["mathtext.fontset"] = "cm"

    generate_heatmap(coordinates, title, "atomic_heatmap")

if __name__ == "__main__":
     if len(sys.argv) != 4:
        print("Usage: python heatmap.py <workgroups> <device> <rmw_iterations>")
        sys.exit(1)
    
    # Parse command-line arguments
    workgroups = int(sys.argv[1])
    device = int(sys.argv[2])
    rmw_iterations = int(sys.argv[3])

    # Call main function with parsed arguments
    main(workgroups, device, rmw_iterations)