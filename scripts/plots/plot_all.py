import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

DIR_PATH = '../../results/'
SAVE_PATH = "../../plots/"
NOF_COMPONENTS = 25

def plot_hybrid_speedup_heatmap(df, nof_components=NOF_COMPONENTS, image_dim=2048):
    df_filtered = df[(df['nof_components'] == nof_components) & 
                    (df['width'] == image_dim)].copy()

    threads_range = [1, 2, 4, 8, 16, 32]
    procs_range = [1, 4, 9, 16, 25]

    df_filtered = df_filtered[df_filtered['num_threads'].isin(threads_range) & 
                              df_filtered['num_procs'].isin(procs_range)]

    best_configs = df_filtered.groupby(['num_procs', 'num_threads'])['time_comp'].min().reset_index()
    baseline = best_configs[(best_configs['num_procs'] == 1) & (best_configs['num_threads'] == 1)]
    
    if baseline.empty:
        print("Eroare: Configurația 1P x 1T lipsește. Nu se poate calcula Speedup-ul.")
        return

    t_serial = baseline['time_comp'].iloc[0]

    best_configs['Speedup'] = t_serial / best_configs['time_comp']

    heatmap_data = best_configs.pivot(index='num_procs', columns='num_threads', values='Speedup')
    heatmap_data = heatmap_data.reindex(index=procs_range, columns=threads_range)

    plt.figure(figsize=(12, 8))
    sns.set_context("talk")
    
    ax = sns.heatmap(heatmap_data, 
                     annot=True, 
                     fmt=".2f", 
                     cmap="YlGnBu", 
                     linewidths=.5,
                     cbar_kws={'label': 'Speedup ($S_p$)'})

    plt.title(f'Heatmap Speedup {image_dim}px, k={nof_components}', pad=20)
    plt.xlabel('Thread-uri per Proces', fontsize=12)
    plt.ylabel('Procese MPI', fontsize=12)

    plt.tight_layout()
    plt.savefig(SAVE_PATH + f"speedup_heatmap_{image_dim}.png")
    plt.show()

def plot_hybrid_time_heatmap(df, nof_components=NOF_COMPONENTS, image_dim=2048):
    df_filtered = df[(df['nof_components'] == nof_components) & 
                    (df['width'] == image_dim)].copy()

    threads_range = [1, 2, 4, 8, 16, 32]
    procs_range = [1, 4, 9, 16, 25]

    df_filtered = df_filtered[df_filtered['num_threads'].isin(threads_range) & 
                              df_filtered['num_procs'].isin(procs_range)]

    best_configs = df_filtered.groupby(['num_procs', 'num_threads'])['time_comp'].min().reset_index()

    heatmap_data = best_configs.pivot(index='num_procs', columns='num_threads', values='time_comp')
    heatmap_data = heatmap_data.reindex(index=procs_range, columns=threads_range)

    plt.figure(figsize=(12, 8))
    sns.set_context("talk")
    
    ax = sns.heatmap(heatmap_data, 
                     annot=True, 
                     fmt=".2f", 
                     cmap="YlGnBu", 
                     linewidths=.5,
                     cbar_kws={'label': 'Time (s)'})

    plt.title(f'Heatmap Time Comparison {image_dim}px, k={nof_components}', pad=20)
    plt.xlabel('Thread-uri per Proces', fontsize=12)
    plt.ylabel('Procese MPI', fontsize=12)

    plt.tight_layout()
    plt.savefig(SAVE_PATH + f"time_heatmap_{image_dim}.png")
    plt.show()


if __name__ == "__main__":
    df_seq = pd.read_csv(DIR_PATH + 'seq.csv')
    df_mpi = pd.read_csv(DIR_PATH + 'mpi.csv')
    df_pthreads = pd.read_csv(DIR_PATH + 'pthreads.csv')
    df_omp = pd.read_csv(DIR_PATH + 'omp.csv')
    df_hybrid_omp = pd.read_csv(DIR_PATH + 'hybrid_omp.csv')
    df_hybrid_pthreads = pd.read_csv(DIR_PATH + 'hybrid_pthr.csv')

    df_total = pd.concat([df_seq, df_mpi, df_pthreads, df_omp, df_hybrid_omp, df_hybrid_pthreads], ignore_index=True)
    plot_hybrid_speedup_heatmap(df_total, nof_components=NOF_COMPONENTS, image_dim=2048)
    plot_hybrid_speedup_heatmap(df_total, nof_components=NOF_COMPONENTS, image_dim=1024)
    plot_hybrid_time_heatmap(df_total, nof_components=NOF_COMPONENTS, image_dim=2048)
    plot_hybrid_time_heatmap(df_total, nof_components=NOF_COMPONENTS, image_dim=1024)

