import matplotlib.pyplot as plt
import numpy as np

# each one include
# [fast_perceived, fast_full, slow_perceived, slow_full]
def draw_latency_breakdown(
    laion,
    sift,
    trip,
    msmarco,
    laion_mal,
    sift_mal,
    trip_mal,
    msmarco_mal
):
    plt.rcParams.update({'font.size': 14})
    plt.rcParams.update({'font.family': "Times New Roman"})

    color1 = "#e6a002"

    color2 = "#019e73"

    # Data
    categories1 = ['LAION', 'SIFT1M']

    perceived_fast_sh = [laion[0].mean(), sift[0].mean(), trip[0].mean(), msmarco[0].mean()]
    full_fast_sh = [laion[1].mean(), sift[1].mean(), trip[1].mean(), msmarco[1].mean()]
    # evict_fast_sh = [0.028, 0.028, 0.29, 0.511]

    perceived_fast_mal = [laion_mal[0].mean(), sift_mal[0].mean(), trip_mal[0].mean(), msmarco_mal[0].mean()]
    full_fast_mal = [laion_mal[1].mean(), sift_mal[1].mean(), trip_mal[1].mean(), msmarco_mal[1].mean()]

    perceived_slow_sh = [laion[2].mean(), sift[2].mean(), trip[2].mean(), msmarco[2].mean()]
    full_slow_sh = [laion[3].mean(), sift[3].mean(), trip[3].mean(), msmarco[3].mean()]

    perceived_slow_mal = [laion_mal[2].mean(), sift_mal[2].mean(), trip_mal[2].mean(), msmarco_mal[2].mean()]
    full_slow_mal = [laion_mal[3].mean(), sift_mal[3].mean(), trip_mal[3].mean(), msmarco_mal[3].mean()]

    # todo: compute diff and error bar


    # perceived_fast_sh = [0.024, 0.027, 0.139, 0.197]
    # evict_fast_sh = [0.028, 0.028, 0.29, 0.511]

    # perceived_fast_mal = [0.027, 0.033, 0.164, 0.233]
    # evict_fast_mal = [0.06, 0.076, 0.545, 0.964]

    # perceived_slow_sh = [0.581, 0.585, 0.985, 1.236]
    # evict_slow_sh = [0.277, 0.275, 3.19, 5.32]

    # perceived_slow_mal = [0.622, 0.595, 1.172, 1.251]
    # evict_slow_mal = [0.4, 0.414, 3.466, 5.752]


    bar1_subcat1 = [3, 7]  # First bar for Category 1 and Category 2
    bar2_subcat1 = [4, 6]  # Second bar for Category 1 and Category 2

    categories2 = ['TripClick', 'MS MARCO']
    bar1_subcat2 = [2, 3]  # First bar for Category 3 and Category 4
    bar2_subcat2 = [5, 6]  # Second bar for Category 3 and Category 4

    # X-axis positions
    x1 = np.arange(len(categories1))
    x2 = np.arange(len(categories2))

    # Width of the bars
    bar_width = 0.35

    # Create the figure and two subplots
    fig, axs = plt.subplots(2, 2, figsize=(6, 3.8))

    ax1 = axs[0][0]
    ax2 = axs[0][1]
    ax3 = axs[1][0]
    ax4 = axs[1][1]

    # First subplot
    ax1.bar(x1 - bar_width/2, perceived_fast_sh[0:2], bar_width, color=color1, label='Perceived Latency - SH.')
    ax1.bar(x1 - bar_width/2, evict_fast_sh[0:2], bar_width, bottom=perceived_fast_sh[0:2], color=color1, alpha=0.5, label='Full Latency - SH.')

    ax1.bar(x1 + bar_width/2, perceived_fast_mal[0:2], bar_width, color=color2, label='Perceived Latency - Mal.')
    ax1.bar(x1 + bar_width/2, evict_fast_mal[0:2], bar_width, bottom=perceived_fast_mal[0:2], color=color2, alpha=0.5, label='Full Latency - Mal.')

    perceived_fast = [perceived_fast_sh, perceived_fast_mal]
    evict_fast = [evict_fast_sh, evict_fast_mal]
    xs = [- bar_width/2, + bar_width/2]
    for x in x1:
        for i in range(2):
            ax1.text(x + xs[i], perceived_fast[i][x], f'{perceived_fast[i][x]:.3f}', ha='center', va='bottom', fontsize=11)
            ax1.text(x + xs[i], perceived_fast[i][x] + evict_fast[i][x], f'{perceived_fast[i][x] + evict_fast[i][x]:.3f}', ha='center', va='bottom', fontsize=11)

    ax1.set_ylabel('fast - Latency (s)')
    ax1.set_xticks(x1)
    # ax1.set_xticklabels(categories1)
    ax1.set_xticklabels([])
    ax1.set_ylim(0, 0.14)
    ax1.set_yticks([0, 0.04, 0.08, 0.12 ])
    # ax1.legend()

    # Second subplot
    ax2.bar(x1 - bar_width/2, perceived_fast_sh[2:4], bar_width, color=color1)
    ax2.bar(x1 - bar_width/2, evict_fast_sh[2:4], bar_width, bottom=perceived_fast_sh[2:4], color=color1, alpha=0.5)

    ax2.bar(x1 + bar_width/2, perceived_fast_mal[2:4], bar_width, color=color2)
    ax2.bar(x1 + bar_width/2, evict_fast_mal[2:4], bar_width, bottom=perceived_fast_mal[2:4], color=color2, alpha=0.5)

    for x in x1:
        for i in range(2):
            ax2.text(x + xs[i], perceived_fast[i][x + 2], f'{perceived_fast[i][x + 2]:.3f}', ha='center', va='bottom', fontsize=11)
            ax2.text(x + xs[i], perceived_fast[i][x + 2] + evict_fast[i][x + 2], f'{perceived_fast[i][x + 2] + evict_fast[i][x + 2]:.3f}', ha='center', va='bottom', fontsize=11)


    ax2.set_xticks(x2)
    # ax2.set_xticklabels(categories2)
    ax2.set_xticklabels([])
    ax2.set_ylim(0, 1.4)
    ax2.set_yticks([0, 0.4, 0.8, 1.2])
    # ax2.legend()

    ax3.bar(x1 - bar_width/2, perceived_slow_sh[0:2], bar_width, color=color1)
    ax3.bar(x1 - bar_width/2, evict_slow_sh[0:2], bar_width, bottom=perceived_slow_sh[0:2], color=color1, alpha=0.5)

    ax3.bar(x1 + bar_width/2, perceived_slow_mal[0:2], bar_width, color=color2)
    ax3.bar(x1 + bar_width/2, evict_slow_mal[0:2], bar_width, bottom=perceived_slow_mal[0:2], color=color2, alpha=0.5)

    perceived_slow = [perceived_slow_sh, perceived_slow_mal]
    evict_slow = [evict_slow_sh, evict_slow_mal]
    for x in x1:
        for i in range(2):
            ax3.text(x + xs[i], perceived_slow[i][x], f'{perceived_slow[i][x]:.3f}', ha='center', va='bottom', fontsize=11)
            ax3.text(x + xs[i], perceived_slow[i][x] + evict_slow[i][x], f'{perceived_slow[i][x] + evict_slow[i][x]:.3f}', ha='center', va='bottom', fontsize=11)



    ax3.set_xticks(x1)
    # ax3.set_xlabel('Fast Network')
    ax3.set_ylabel('slow - Latency (s)', labelpad = 10)
    ax3.set_xticklabels(categories1)
    ax3.set_ylim(0, 1.6)
    ax3.set_yticks([0, 0.4, 0.8, 1.2])
    #

    ax4.bar(x1 - bar_width/2, perceived_slow_sh[2:4], bar_width, color=color1)
    ax4.bar(x1 - bar_width/2, evict_slow_sh[2:4], bar_width, bottom=perceived_slow_sh[2:4], color=color1, alpha=0.5)

    ax4.bar(x1 + bar_width/2, perceived_slow_mal[2:4], bar_width, color=color2)
    ax4.bar(x1 + bar_width/2, evict_slow_mal[2:4], bar_width, bottom=perceived_slow_mal[2:4], color=color2, alpha=0.5)

    for x in x1:
        for i in range(2):
            ax4.text(x + xs[i], perceived_slow[i][x + 2], f'{perceived_slow[i][x + 2]:.3f}', ha='center', va='bottom', fontsize=11)
            ax4.text(x + xs[i], perceived_slow[i][x + 2] + evict_slow[i][x + 2], f'{perceived_slow[i][x + 2] + evict_slow[i][x + 2]:.3f}', ha='center', va='bottom', fontsize=11)


    # ax4.set_xlabel('Slow Network')
    ax4.set_xticks(x2)
    ax4.set_xticklabels(categories2)
    ax4.set_ylim(0, 8)
    ax4.set_yticks([0, 2, 4, 6])

    left  = 0.12  # the left side of the subplots of the figure
    right = 0.99   # the right side of the subplots of the figure
    bottom = 0.08  # the bottom of the subplots of the figure
    top = 0.85   # the top of the subplots of the figure
    wspace = 0.20  # the amount of width reserved for blank space between subplots
    hspace = 0.10   # the amount of height reserved for white space between subplots
    plt.subplots_adjust(left=left, bottom=bottom, right=right, top=top, wspace=wspace, hspace=hspace)

    fig.legend(loc='upper center', bbox_to_anchor=(0.56, 1.03), ncol=2, frameon=False)

    plt.savefig('latency_breakdown.pdf') 
    # Adjust layout and show the plot
    # plt.tight_layout()
    # plt.show()
