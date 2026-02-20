import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

DIR_PATH = '../../results/'
SAVE_PATH = "../../plots/"
NOF_COMPONENTS = 100

def get_filtered_data(df, nof_components, image_dim):
    return df[(df['nof_components'] == nof_components) & (df['width'] == image_dim)].copy()

def plot_all_metrics(df_seq, df_omp, df_pthreads, nof_components=NOF_COMPONENTS, image_dim=2048):
    s_data = get_filtered_data(df_seq, nof_components, image_dim)
    o_data = get_filtered_data(df_omp, nof_components, image_dim)
    p_data = get_filtered_data(df_pthreads, nof_components, image_dim)

    if s_data.empty or o_data.empty or p_data.empty:
        return

    t_serial = s_data['time_comp'].iloc[0]

    plt.figure(figsize=(10, 6))
    plt.plot(o_data['num_threads'], o_data['time_comp'], marker='s', label='OpenMP', color='darkorange', linewidth=2)
    plt.plot(p_data['num_threads'], p_data['time_comp'], marker='o', label='Pthreads', color='blue', linewidth=2)
    plt.axhline(y=t_serial, color='red', linestyle='--', label='Sequential Baseline')
    plt.title(f'Timp de Execuție (Imagine {image_dim}px, k={nof_components})')
    plt.xlabel('Număr Thread-uri')
    plt.ylabel('Timp (secunde)')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.savefig(DIR_PATH + f'images/parallel_metrics_{image_dim}_{nof_components}.png')
    plt.show()

def plot_components_comparison(df, variant_name='OpenMP', image_dim=2048):
    df = df[df['width'] == image_dim]

    plt.figure(figsize=(12, 7))
    sns.set_style("whitegrid")

    ax = sns.barplot(
        data=df, 
        x='num_threads', 
        y='time_comp', 
        hue='nof_components',
        palette='viridis'
    )

    plt.title(f'Timp de Execuție {variant_name} funcție de Nr. Componente (Imagine {image_dim}px)', fontsize=14)
    plt.xlabel('Număr de Thread-uri', fontsize=12)
    plt.ylabel('Timp de Execuție (secunde)', fontsize=12)
    plt.legend(title='Nr. Componente (k)')
    
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(format(p.get_height(), '.2f'), 
                        (p.get_x() + p.get_width() / 2., p.get_height()), 
                        ha = 'center', va = 'center', 
                        xytext = (0, 9), 
                        textcoords = 'offset points',
                        fontsize=9)

    plt.tight_layout()
    plt.savefig(DIR_PATH + f'images/{variant_name.lower()}_components_comparison_{image_dim}.png')
    plt.show()

def generate_comparison_table(df_seq, df_omp, df_pthreads, nof_components=50, image_dim=2048):
    s_data = df_seq[(df_seq['nof_components'] == nof_components) & (df_seq['width'] == image_dim)]
    o_data = df_omp[(df_omp['nof_components'] == nof_components) & (df_omp['width'] == image_dim)]
    p_data = df_pthreads[(df_pthreads['nof_components'] == nof_components) & (df_pthreads['width'] == image_dim)]

    if s_data.empty:
        return

    t_serial = s_data['time_comp'].iloc[0]

    omp_results = o_data[['num_threads', 'time_comp']].copy()
    omp_results['Implementation'] = 'OpenMP'
    omp_results['Speedup'] = t_serial / omp_results['time_comp']
    omp_results['Efficiency (%)'] = (omp_results['Speedup'] / omp_results['num_threads']) * 100

    pth_results = p_data[['num_threads', 'time_comp']].copy()
    pth_results['Implementation'] = 'Pthreads'
    pth_results['Speedup'] = t_serial / pth_results['time_comp']
    pth_results['Efficiency (%)'] = (pth_results['Speedup'] / pth_results['num_threads']) * 100

    final_table = pd.concat([omp_results, pth_results])
    
    final_table = final_table.round({'time_comp': 3, 'Speedup': 2, 'Efficiency (%)': 1})
    
    final_table = final_table.rename(columns={
        'num_threads': 'Threads',
        'time_comp': 'Time (s)'
    })

    return final_table[['Implementation', 'Threads', 'Time (s)', 'Speedup', 'Efficiency (%)']]

def render_mpl_table(data, col_width=3.0, row_height=0.625, font_size=14,
                     header_color="#00574B", row_colors=['#f1f1f2', 'w'], edge_color='w',
                     bbox=[0, 0, 1, 1], header_columns=0,
                     ax=None, name='default', **kwargs):
    if ax is None:
        size = (np.array(data.shape[::-1]) + np.array([0, 1])) * np.array([col_width, row_height])
        fig, ax = plt.subplots(figsize=size)
        ax.axis('off')

    mpl_table = ax.table(cellText=data.values, bbox=bbox, colLabels=data.columns, **kwargs)

    mpl_table.auto_set_font_size(False)
    mpl_table.set_fontsize(font_size)

    for k, cell in mpl_table._cells.items():
        cell.set_edgecolor(edge_color)
        if k[0] == 0 or k[1] < header_columns:
            cell.set_text_props(weight='bold', color='w')
            cell.set_facecolor(header_color)
        else:
            cell.set_facecolor(row_colors[k[0]%len(row_colors)])
    
    plt.savefig(DIR_PATH + f'images/metrics_table_{name}.png')
    plt.show()

def plot_mpi_scaling_by_resolution(df, nof_components=25, variant_name='OpenMP'):
    plt.figure(figsize=(10, 6))
    sns.set_style("whitegrid")

    target_dims = [256, 512, 1024, 2048]
    data_filtered = df[(df['nof_components'] == nof_components) & 
                       (df['width'].isin(target_dims))].copy()
    
    data_filtered = data_filtered.sort_values(by='width')

    ax = sns.lineplot(
        data=data_filtered, 
        x='width', 
        y='time_comp', 
        hue='num_threads', 
        palette='tab10',
        linewidth=3,
        marker=6,        
        errorbar=None
    )

    plt.xscale('log', base=2)

    plt.xticks(target_dims, [str(d) for d in target_dims])

    plt.title(f'Scalabilitate {variant_name}: Timp vs. Rezoluție\n(Componente k={nof_components})', fontsize=14, pad=15)
    plt.xlabel('Rezoluție Imagine (Width px)', fontsize=12)
    plt.ylabel('Timp Compresie (s)', fontsize=12)

    plt.legend(title="Nr. Threaduri", loc='best', frameon=True)
    
    plt.grid(True, which="both", ls="--", alpha=0.5)
    
    plt.tight_layout()
    plt.savefig(SAVE_PATH + f'{variant_name.lower()}_scaling_by_res_k{nof_components}.png')
    plt.show()



if __name__ == "__main__":
    df_lanczos  = pd.read_csv(DIR_PATH + 'seq.csv')
    df_omp      = pd.read_csv(DIR_PATH + 'omp.csv')
    df_pthreads = pd.read_csv(DIR_PATH + 'pthreads.csv')

    # plot_all_metrics(df_lanczos, df_omp, df_pthreads, NOF_COMPONENTS, 2048)
    # plot_all_metrics(df_lanczos, df_omp, df_pthreads, NOF_COMPONENTS, 1024)
    # plot_all_metrics(df_lanczos, df_omp, df_pthreads, 50, 2048)
    # plot_all_metrics(df_lanczos, df_omp, df_pthreads, 50, 1024)

    # plot_components_comparison(df_omp, 'OpenMP', 2048)
    # plot_components_comparison(df_pthreads, 'Pthreads', 2048)

    plot_mpi_scaling_by_resolution(df_omp, nof_components=NOF_COMPONENTS, variant_name='OpenMP')
    plot_mpi_scaling_by_resolution(df_pthreads, nof_components=NOF_COMPONENTS, variant_name='Pthreads')

    results_table = generate_comparison_table(df_lanczos, df_omp, df_pthreads, NOF_COMPONENTS, 2048)
    results_table_omp = results_table[results_table['Implementation'] == 'OpenMP']
    results_table_pth = results_table[results_table['Implementation'] == 'Pthreads']
    
    print(f"\nRezultate Compresie PCA (Imagine {2048}px, k={NOF_COMPONENTS})")
    print("="*65)
    print(results_table.to_markdown(index=False))

    render_mpl_table(results_table_omp, name='omp')
    render_mpl_table(results_table_pth, name='pthreads')
    render_mpl_table(results_table, name='combined_parallel')
