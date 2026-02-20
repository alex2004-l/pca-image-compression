import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

DIR_PATH = '../../results/'
SAVE_PATH = "../../plots/"
NOF_COMPONENTS = 25


def plot_components_comparison(df, variant_name='MPI', image_dim=2048, nof_components=NOF_COMPONENTS):
    df_e = df[df['width'] == image_dim]

    plt.figure(figsize=(12, 7))
    sns.set_style("whitegrid")

    ax = sns.barplot(
        data=df_e, 
        x='num_procs', 
        y='time_comp', 
        hue='nof_components',
        palette='viridis'
    )

    plt.title(f'Timp de Execuție {variant_name} funcție de Nr. Componente (Imagine {image_dim}px)', fontsize=14)
    plt.xlabel('Număr Procese', fontsize=12)
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
    plt.savefig(SAVE_PATH + f'{variant_name.lower()}_components_comparison_{image_dim}.png')
    plt.show()

def plot_mpi_scaling_multiple_dimensions(df, nof_components=NOF_COMPONENTS):
    plt.figure(figsize=(10, 6))
    sns.set_style("whitegrid")

    target_dims = [256, 512, 1024, 2048]
    data_filtered = df[(df['nof_components'] == 25) & 
                       (df['width'].isin(target_dims))].copy()

    sns.lineplot(
        data=data_filtered, 
        x='num_procs', 
        y='time_comp', 
        hue='width', 
        style='width',
        marker='o', 
        markersize=8,
        linewidth=2, 
        palette='viridis'
    )

    plt.title(f'Analiză MPI: Scalabilitate pe Dimensiuni Multiple (k={nof_components})', fontsize=14)
    plt.xlabel('Număr Procese (MPI Ranks)', fontsize=12)
    plt.ylabel('Timp Compresie (s)', fontsize=12)

    plt.legend(title="Rezoluție Imagine (px)", loc='best')
    
    plt.grid(True, which="both", ls="--", alpha=0.5)
    
    plt.tight_layout()
    plt.savefig(SAVE_PATH + f'mpi_multidim_scaling_k{nof_components}.png')
    plt.show()

def plot_mpi_scaling_by_resolution(df, nof_components=25):
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
        hue='num_procs', 
        palette='tab10',
        linewidth=3,
        marker=6,        
        errorbar=None       # Șterge zonele transparente (shadows)
    )

    plt.xscale('log', base=2)

    plt.xticks(target_dims, [str(d) for d in target_dims])

    plt.title(f'Scalabilitate MPI: Timp vs. Rezoluție\n(Componente k={nof_components})', fontsize=14, pad=15)
    plt.xlabel('Rezoluție Imagine (Width px)', fontsize=12)
    plt.ylabel('Timp Compresie (s)', fontsize=12)

    plt.legend(title="Nr. Procese MPI", loc='best', frameon=True)
    
    plt.grid(True, which="both", ls="--", alpha=0.5)
    
    plt.tight_layout()
    plt.savefig(SAVE_PATH + f'mpi_scaling_by_res_k{nof_components}.png')
    plt.show()

def generate_comparison_table(df_seq, df_mpi, nof_components=25, image_dim=2048):
    s_data = df_seq[(df_seq['nof_components'] == nof_components) & (df_seq['width'] == image_dim)]
    m_data = df_mpi[(df_mpi['nof_components'] == nof_components) & (df_mpi['width'] == image_dim)].copy()

    if s_data.empty or m_data.empty:
        print(f"Atenție: Lipsesc date pentru k={nof_components}, dim={image_dim}")
        return None

    t_serial = s_data['time_comp'].iloc[0]

    # 2. Calcule
    m_data['Implementation'] = 'MPI'
    m_data['Speedup'] = t_serial / m_data['time_comp']
    m_data['Efficiency (%)'] = (m_data['Speedup'] / m_data['num_procs']) * 100

    final_table = m_data.copy()
    
    final_table = m_data.rename(columns={
        'num_procs': 'Processes',
        'time_comp': 'Time (s)'
    })
    
    # Selectăm doar coloanele relevante în ordinea dorită pentru slide
    columns_order = ['Implementation', 'Processes', 'Time (s)', 'Speedup', 'Efficiency (%)']
    final_table = final_table[columns_order]

    return final_table

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
    
    plt.savefig(SAVE_PATH + f'metrics_table_{name}.png')
    plt.show()


if __name__ == "__main__":
    df = pd.read_csv(DIR_PATH + 'mpi.csv')
    df_seq = pd.read_csv(DIR_PATH + 'seq.csv')

    plot_components_comparison(df, variant_name='MPI', image_dim=2048)
    plot_mpi_scaling_multiple_dimensions(df, nof_components=NOF_COMPONENTS)

    plot_mpi_scaling_by_resolution(df, nof_components=NOF_COMPONENTS)

    mpi_table = generate_comparison_table(df_seq=df_seq, df_mpi=df)

    print(mpi_table.to_markdown(index=False))
    
    # render_mpl_table(mpi_table, name='mpi_2048_k25')
