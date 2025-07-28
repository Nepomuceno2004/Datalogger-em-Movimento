import pandas as pd
import matplotlib.pyplot as plt

nome_arquivo = 'Arquivos/dados.csv'

# Lê o CSV com separador por ponto e vírgula
df = pd.read_csv(nome_arquivo, encoding='utf-8-sig', sep=';')

# Gráfico de Aceleração
plt.figure(figsize=(10, 6))
plt.plot(df['numero_amostra'], df['accel_x'], label='accel_x')
plt.plot(df['numero_amostra'], df['accel_y'], label='accel_y')
plt.plot(df['numero_amostra'], df['accel_z'], label='accel_z')
plt.xlabel('Número da Amostra')
plt.ylabel('Aceleração (g)')
plt.title('Dados de Aceleração')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# Gráfico de Giroscópio
plt.figure(figsize=(10, 6))
plt.plot(df['numero_amostra'], df['giro_x'], label='giro_x')
plt.plot(df['numero_amostra'], df['giro_y'], label='giro_y')
plt.plot(df['numero_amostra'], df['giro_z'], label='giro_z')
plt.xlabel('Número da Amostra')
plt.ylabel('Velocidade Angular (°/s)')
plt.title('Dados do Giroscópio')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
