import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

DIR_PATH = '../../results/'
SAVE_PATH = "../../plots/"
NOF_COMPONENTS = 20

def plot_hybrid_clean_lines(df, variant_label):
    if df.empty:
        print(f"DataFrame-ul pentru {variant_label} este gol.")
        return

    plot_df = df.copy()
    plot_df['Hybrid Config'] = (plot_df['num_procs'].astype(int).astype(str) + "P x " + 
                                plot_df['num_threads'].astype(int).astype(str) + "T")
    plot_df = plot_df.sort_values(by='width')

    plt.figure(figsize=(10, 6))
    sns.set_style("whitegrid")

    ax = sns.lineplot(
        data=plot_df, 
        x='width', 
        y='time_comp', 
        hue='Hybrid Config', 
        marker=6,
        linewidth=3,  
        palette='tab10',
        legend='full',
        errorbar=None
    )

    plt.xscale('log', base=2)
    
    res_values = sorted(plot_df['width'].unique())
    plt.xticks(res_values, [str(r) for r in res_values])

    plt.title(f'Scalabilitate {variant_label}\n(Timp de calcul vs. Rezoluție)', fontsize=14, pad=15)
    plt.xlabel('Rezoluție (Lățime)', fontsize=12)
    plt.ylabel('Timp (s)', fontsize=12)

    plt.legend(title='Configurație Hibridă', title_fontsize='11', loc='best', frameon=True)

    plt.tight_layout()

    file_name = f"plot_lines_{variant_label.lower().replace(' ', '_')}.png"
    plt.savefig(SAVE_PATH + file_name)
    plt.show()

if __name__ == "__main__":
    df_hybrid_omp = pd.read_csv(DIR_PATH + 'hybrid_omp.csv')
    df_hybrid_pthreads = pd.read_csv(DIR_PATH + 'hybrid_pthr.csv')

    plot_hybrid_clean_lines(df_hybrid_omp, variant_label='Hybrid OpenMP')
    plot_hybrid_clean_lines(df_hybrid_pthreads, variant_label='Hybrid Pthreads')