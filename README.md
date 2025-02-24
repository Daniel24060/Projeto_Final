
# Sistema de Alarme para Pontos de Ônibus  

## Descrição
Este projeto implementa um sistema de alarme para pontos de ônibus utilizando o microcontrolador **RP2040** (Raspberry Pi Pico W). O sistema é capaz de detectar ruídos acima de um limiar pré-definido, ativar uma sirene de dois tons e fornecer feedback visual através de uma matriz de LEDs e um display OLED. O projeto também conta com botões de emergência e reset para controle manual.

O código está localizado no arquivo **neopixel_pio.c**.

---

## Como Rodar o Projeto

### 1. Clone o repositório:
```sh
git clone https://github.com/Daniel24060/Projeto_Final.git
```

### 2. Entre na pasta do projeto:
```sh
cd Projeto_Final
```

---

## Instruções para Configuração e Compilação do Projeto

### Passo 1: Baixar o Ninja
1. Acesse a página oficial de releases do Ninja: [Ninja Releases no GitHub](https://github.com/ninja-build/ninja/releases).
2. Baixe o arquivo binário mais recente para Windows (geralmente um `.zip`).
3. Extraia o conteúdo do arquivo `.zip` (haverá um arquivo `ninja.exe`).

### Passo 2: Adicionar o Ninja ao Path do sistema
1. Clique com o botão direito no botão **Iniciar** e selecione **Configurações**.
2. Vá em **Sistema → Sobre → Configurações avançadas do sistema** (no lado direito).
3. Na aba **Avançado**, clique em **Variáveis de Ambiente**.
4. Na seção **Variáveis do Sistema**, localize a variável **Path** e clique em **Editar**.
5. Clique em **Novo** e adicione o caminho completo para o diretório onde você extraiu o `ninja.exe`, por exemplo:
   ```
   C:\Users\SeuUsuario\Downloads\ninja-win
   ```
6. Clique em **OK** em todas as janelas.

### Passo 3: Verificar se o Ninja está funcionando
Abra um terminal (PowerShell ou Prompt de Comando) e digite o comando abaixo para verificar a instalação:
```sh
ninja --version
```

### Passo 4: Configurar e Compilar o Projeto
1. Volte ao diretório do projeto e abra um terminal.
2. Crie ou limpe a pasta `build`:
   ```sh
   rmdir /s /q build
   mkdir build
   cd build
   ```
3. Configure o projeto com o CMake:
   ```sh
   cmake -G Ninja ..
   ```
4. Compile o projeto:
   ```sh
   ninja
   ```
5. Após isso, o arquivo **main.elf** será gerado na pasta `build`.

---

## Dispositivos do projeto:

### 1. Configuração do PWM
- **Buzzer Alto**: Frequência de 2000 Hz (GPIO 21).
- **Buzzer Baixo**: Frequência de 500 Hz (GPIO 10).

### 2. Matriz de LEDs WS2812B
- Controlada por **PIO0 SM3** na GPIO 7.
- Padrão "X" intermitente em vermelho durante o alarme.

### 3. Display OLED
- Conexão **I2C1** com frequência de 400kHz (GPIO 14 e 15).
- Exibe mensagens de status: "Sistema Pronto" ou "ALARME ATIVADO!".

### 4. Botões de Controle
- **Botão de Emergência**: GPIO 5 (pull-up).
- **Botão de Reset**: GPIO 6 (pull-up).

---

## Como Funciona o Sistema

### 1. Modo de Vigília
- O display OLED exibe "Sistema Pronto".
- O sistema monitora continuamente o nível de ruído através do microfone (GPIO 28).

### 2. Modo de Alarme
- **Ativação Automática**: Quando o ruído excede o limiar pré-definido (3000 unidades ADC).
- **Ativação Manual**: Pressionando o botão de emergência (GPIO 5).
- **Ações**:
  - A sirene alterna entre tons alto e baixo.
  - A matriz de LEDs pisca em vermelho.
  - O display OLED mostra "ALARME ATIVADO!".

### 3. Reset do Sistema
- Pressionando o botão de reset (GPIO 6), o sistema retorna ao modo de vigília.

---

## Autor
**Daniel24060**

---

## Licença
Este projeto está licenciado sob a licença **MIT**. Consulte o arquivo [LICENSE](LICENSE) para mais detalhes.

---
