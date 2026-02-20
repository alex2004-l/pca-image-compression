import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

DIR_PATH = '../../results/'
NOF_COMPONENTS = 20

def plot_seq_pixels_dep(df):
    df_25 = df[df['nof_components'] == 20].copy()

    df_25['total_pixels'] = df_25['width'] * df_25['height']
    df_25['pixel_desc'] = df_25['width'].astype(str) + " x " + df_25['height'].astype(str)

    df_25 = df_25.sort_values(by='total_pixels')

    plt.figure(figsize=(12, 6))

    sns.pointplot(data=df_25, x='pixel_desc', y='time_comp', color='teal', markers="D")

    plt.title('Execution Time vs Image Resolution (nof_components = 25)', fontsize=14)
    plt.xlabel('Resolution (Width x Height)', fontsize=12)
    plt.ylabel('Execution Time (seconds)', fontsize=12)
    plt.xticks(rotation=45)
    plt.grid(True, axis='y', linestyle='--', alpha=0.6)

    plt.tight_layout()
    plt.savefig(DIR_PATH + 'images/pixels_vs_time.png')
    plt.show()

def plot_seq_components_dep(df, height=2048):
    df_fixed_dim = df[df['height'] == height].copy()

    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_fixed_dim, x='nof_components', y='time_comp', marker='s', linewidth=2, color='darkorange')

    plt.title('Execution Time vs Number of Components (Fixed Height = 2048)', fontsize=14)
    plt.xlabel('Number of Components')
    plt.ylabel('Execution Time (seconds)')
    plt.grid(True, linestyle='--', alpha=0.7)

    plt.savefig(DIR_PATH + f'images/components_scaling_{height}.png')
    plt.show()

def plot_seq_lanczos_spectra_comparison(df_lanczos, df_spectra, nof_components=100):
    plt.figure(figsize=(10, 6))

    df_lanczos = df_lanczos[df_lanczos['nof_components'] == nof_components]
    df_spectra = df_spectra[df_spectra['nof_components'] == nof_components]

    sns.lineplot(data=df_lanczos, x='height', y='time_comp', marker='s', linewidth=2, color='darkorange', label='Lanczos')
    sns.lineplot(data=df_spectra, x='height', y='time_comp', marker='o', linewidth=2, color='blue', label='Spectra')

    plt.title(f'Execution Time Comparison: Lanczos vs Spectra({nof_components} components)', fontsize=14)
    plt.xlabel('Image dimensions (pixels)')
    plt.ylabel('Execution Time (seconds)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()

    plt.savefig(DIR_PATH + 'images/lanczos_vs_spectra.png')
    plt.show()

if __name__ == "__main__":
    df_lanczos = pd.read_csv(DIR_PATH + 'seq.csv')
    df_spectra = pd.read_csv(DIR_PATH + 'seq_spectra.csv')

    plot_seq_pixels_dep(df_lanczos)
    plot_seq_components_dep(df_lanczos, height=2048)
    plot_seq_lanczos_spectra_comparison(df_lanczos, df_spectra)
