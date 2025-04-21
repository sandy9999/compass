import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import matplotlib.markers as mks

plt.rcParams.update({'font.size': 14})
plt.rcParams.update({'font.family': "Times New Roman"})

# color_set = [
#     "#e6a002",
#     "#019e73",
#     "#fd8282"
# ]

# color_set = [
#     "#ff0000",
#     "#00A08A",
#     "#F2AD00"
# ]

color_set = [
    "#D55E00",
    "#009E73",
    "#0072B2",
    "#999999"
]

# color_set = [
#     "#ffd931",
#     "#61d835",
#     "#ff644e"
# ]

fig, (ax4, ax3, ax2, ax1) = plt.subplots(1, 4, figsize=(12, 3.8))

# Define data points
x = [0.3, 3, 80, 400]  # X coordinates
y = [0.1, 0.2, 0.3, 0.8]  # Y coordinates
labels = ['Point A', 'Point B', 'Point C', 'Point D']  # Labels for the points
colors = ['red', 'green', 'blue', 'yellow']  # Colors for each point

#

# Plot on the first subplot
compass_trip_acc = [0.955625]
compass_trip_latency_lan = [0.024]
compass_trip_latency_wan = [0.581]

clustering_trip_acc = [0.9976]
clustering_trip_latency_lan = [14.0578]
clustering_trip_latency_wan = [20.7442]

vanilla_hnsw_acc = [0.977]
vanilla_hnsw_latency_lan = [0.0036]
vanilla_hnsw_latency_wan = [0.0906]

ax4.axhline(y=1, color='#E69F00', linestyle='dashed', linewidth=2, alpha=1)

ax4.scatter(vanilla_hnsw_latency_lan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D', fillstyle='none'), s=50)
ax4.scatter(vanilla_hnsw_latency_wan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D'), s=50)

ax4.scatter(compass_trip_latency_lan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*', fillstyle='none'), s=100)
ax4.scatter(compass_trip_latency_wan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*'), s=100)

ax4.scatter(clustering_trip_latency_lan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^', fillstyle='none'), s=50)
ax4.scatter(clustering_trip_latency_wan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^'), s=50)

# annotation_text = "Better"
# arrow_props = dict(facecolor='black', shrink=0.05, headwidth=8, width=2)

ax4.set_title('LAION', fontsize=14, y=1)
# ax3.set_xlabel('X Coordinate')
ax4.set_xlabel('Latency (s)', fontsize=14)
ax4.set_xscale('log')
ax4.set_xticks([ 0.001, 0.01, 0.1, 1, 10, 100])
ax4.set_xlim(0.001, 100)
ax4.set_yticks([ 0.0, 0.2, 0.4, 0.6, 0.8, 1.0])
ax4.set_ylim(0, 1.05)

ax4.grid(True, zorder=0, linewidth=0.5)
ax4.set_axisbelow(True)

# Create a figure and a set of subplots
# fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 6))  # 1 row, 3 columns

# Plot on the first subplot
compass_trip_acc = [0.944]
compass_trip_latency_lan = [0.027]
compass_trip_latency_wan = [0.585]

clustering_trip_acc = [0.439]
clustering_trip_latency_lan = [35.0559]
clustering_trip_latency_wan = [55.3494]

vanilla_hnsw_acc = [0.953]
vanilla_hnsw_latency_lan = [0.0064]
vanilla_hnsw_latency_wan = [0.1205]

ax3.axhline(y=1, color='#E69F00', linestyle='dashed', linewidth=2, alpha=1)

ax3.scatter(vanilla_hnsw_latency_lan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D', fillstyle='none'), s=50)
ax3.scatter(vanilla_hnsw_latency_wan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D'), s=50)

ax3.scatter(compass_trip_latency_lan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*', fillstyle='none'), s=100)
ax3.scatter(compass_trip_latency_wan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*'), s=100)

ax3.scatter(clustering_trip_latency_lan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^', fillstyle='none'), s=50)
ax3.scatter(clustering_trip_latency_wan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^'), s=50)

# annotation_text = "Better"
# arrow_props = dict(facecolor='black', shrink=0.05, headwidth=8, width=2)

ax3.set_title('SIFT1M', fontsize=14, y=1)
# ax3.set_xlabel('X Coordinate')
ax3.set_xlabel('Latency (s)', fontsize=14)
ax3.set_xscale('log')
ax3.set_xticks([ 0.001, 0.01, 0.1, 1, 10, 100])
ax3.set_xlim(0.001, 100)
ax3.set_yticks([ 0.0, 0.2, 0.4, 0.6, 0.8, 1.0])
ax3.set_ylim(0, 1.05)

# ax3.annotate(annotation_text, xy=(5, 5), xytext=(5, 5),
#                 arrowprops=arrow_props)

# ax3.legend(loc='upper right')
ax3.grid(True, zorder=0, linewidth=0.5)
ax3.set_axisbelow(True)

# Plot on the second subplot
tfidf_trip_acc = [0.061 , 0.103, 0.191, 0.230] 
tfidf_trip_latency_lan = [0.078, 0.304, 2.074, 17.049] 
tfidf_trip_latency_wan = [0.347, 0.924, 3.978, 23.21] 

compass_trip_acc = [0.29]
compass_trip_latency_lan = [0.139]
compass_trip_latency_wan = [0.985]

clustering_trip_acc = [0.133]
clustering_trip_latency_lan = [325.273]
clustering_trip_latency_wan = [350.798]

vanilla_hnsw_acc = [0.291]
vanilla_hnsw_latency_lan = [0.0154]
vanilla_hnsw_latency_wan = [0.1654]

ax2.scatter(vanilla_hnsw_latency_lan, vanilla_hnsw_acc, color=color_set[3], label="Plaintext-HNSW fast", marker=mks.MarkerStyle('D', fillstyle='none'), s=50)
ax2.scatter(vanilla_hnsw_latency_wan, vanilla_hnsw_acc, color=color_set[3], label="Plaintext-HNSW slow", marker=mks.MarkerStyle('D'), s=50)

ax2.scatter(compass_trip_latency_lan, compass_trip_acc, color=color_set[0], label="Compass fast", marker=mks.MarkerStyle('*', fillstyle='none'), s=100, zorder=2)
ax2.scatter(compass_trip_latency_wan, compass_trip_acc, color=color_set[0], label="Compass slow", marker=mks.MarkerStyle('*'), s=100, zorder=2)


ax2.scatter(clustering_trip_latency_lan, clustering_trip_acc, color=color_set[1],label="HE-Cluster fast", marker=mks.MarkerStyle('^', fillstyle='none'), s=50)
ax2.scatter(clustering_trip_latency_wan, clustering_trip_acc, color=color_set[1],label="HE-Cluster slow", marker=mks.MarkerStyle('^'), s=50)

ax2.scatter(tfidf_trip_latency_lan, tfidf_trip_acc, color=color_set[2], label="Inv-ORAM fast", marker=mks.MarkerStyle('s', fillstyle='none'), s=50, zorder=2)
ax2.scatter(tfidf_trip_latency_wan, tfidf_trip_acc, color=color_set[2], label="Inv-ORAM slow", marker=mks.MarkerStyle('s'), s=50, zorder=2)

ax2.axhline(y=0.276, color='#56B4E9', linestyle='dashed', label="BM25", linewidth=2, alpha=1, zorder=1)
ax2.axhline(y=0.231, color='#56B4E9', linestyle='dotted', label="TF-IDF", linewidth=2, alpha=1, zorder=1)
ax2.axhline(y=0.305, color='#E69F00', linestyle='dashed', label="Emed. Brute Force", linewidth=2, alpha=1, zorder=1)



ax2.set_title('TripClick', fontsize=14, y=1)
ax2.set_xlabel('Latency (s)', fontsize=14)
# ax2.set_ylabel('Y Coordinate')
ax2.set_xscale('log')
ax2.set_xticks([ 0.01, 0.1, 1, 10, 100, 1000])
ax2.set_xlim(0.01, 1000)
ax2.set_yticks([ 0.1, 0.2, 0.3, 0.4])
ax2.set_ylim(0, 0.4)
# ax2.legend()
ax2.grid(True, zorder=0, linewidth=0.5)
ax2.set_axisbelow(True)

# Plot on the third subplot
tfidf_trip_acc = [0.016 , 0.023, 0.044, 0.075] 
tfidf_trip_latency_lan = [0.095, 0.383, 2.753, 22.921] 
tfidf_trip_latency_wan = [0.516, 1.153, 4.811, 30.515] 

compass_trip_acc = [0.341]
compass_trip_latency_lan = [0.197]
compass_trip_latency_wan = [1.236]

clustering_trip_acc = [0.117]
clustering_trip_latency_lan = [1769.2]
clustering_trip_latency_wan = [1808.5]

vanilla_hnsw_acc = [0.342]
vanilla_hnsw_latency_lan = [0.0154]
vanilla_hnsw_latency_wan = [0.2054]

ax1.scatter(vanilla_hnsw_latency_lan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D', fillstyle='none'), s=50)
ax1.scatter(vanilla_hnsw_latency_wan, vanilla_hnsw_acc, color=color_set[3], marker=mks.MarkerStyle('D'), s=50)

ax1.axhline(y=0.373, color='#E69F00', linestyle='dashed', linewidth=2, alpha=1, zorder=1)
ax1.axhline(y=0.112, color='#56B4E9', linestyle='dotted', linewidth=2, alpha=1, zorder=1)
ax1.axhline(y=0.187, color='#56B4E9', linestyle='dashed', linewidth=2, alpha=1, zorder=1)

ax1.scatter(compass_trip_latency_lan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*', fillstyle='none'), s=100, zorder=2)
ax1.scatter(compass_trip_latency_wan, compass_trip_acc, color=color_set[0], marker=mks.MarkerStyle('*'), s=100, zorder=2)
ax1.scatter(tfidf_trip_latency_lan, tfidf_trip_acc, color=color_set[2], marker=mks.MarkerStyle('s', fillstyle='none'), s=50, zorder=2)
ax1.scatter(tfidf_trip_latency_wan, tfidf_trip_acc, color=color_set[2], marker=mks.MarkerStyle('s'), s=50, zorder=2)

ax1.scatter(clustering_trip_latency_lan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^', fillstyle='none'), s=50)
ax1.scatter(clustering_trip_latency_wan, clustering_trip_acc, color=color_set[1], marker=mks.MarkerStyle('^'), s=50)

ax1_title = ax1.set_title('MS MARCO', fontsize=14, y=1)
# ax1_title.set_position([0.5, -1])
# ax1.set_xlabel('X Coordinate')
# ax1.set_ylabel('Y Coordinate')
ax1.set_xscale('log')
ax1.set_xlabel('Latency (s)', fontsize=14)
ax1.xaxis.set_label_coords(0.5, -0.15)
ax1.set_xticks([ 0.01, 0.1, 1, 10, 100, 1000, 10000])
ax1.set_xlim(0.01, 10000)
ax4.set_ylabel('MRR@10', fontsize=14)
ax1.set_yticks([ 0.1, 0.2, 0.3, 0.4])
ax1.set_ylim(0, 0.4)
# ax1.legend()
ax1.grid(True, zorder=0, linewidth=0.5)
ax1.set_axisbelow(True)

left  = 0.05  # the left side of the subplots of the figure
right = 0.98   # the right side of the subplots of the figure
bottom = 0.15  # the bottom of the subplots of the figure
top = 0.78     # the top of the subplots of the figure
wspace = 0.20  # the amount of width reserved for blank space between subplots
hspace = 0.04   # the amount of height reserved for white space between subplots
plt.subplots_adjust(left=left, bottom=bottom, right=right, top=top, wspace=wspace, hspace=hspace)

fig.legend(loc='upper center', bbox_to_anchor=(0.51, 1.04), ncol=6, frameon=False, handlelength=1.5, columnspacing=0.9)

# Display the plot
plt.savefig('point.pdf') 
# plt.show()
