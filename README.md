# SEL0433_Projeto2_checkpointFINAL
Alexsandra Pavani Xavier NUSP 14681372   

Antonio Rosa Galindo de Andrade NUSP 15440868

# Introducao
  O projeto teve como objetivo desenvolver um firmware em C para o microcontrolador PIC18F4550, capaz de realizar contagem regressiva com duas durações distintas, leitura de temperatura via ADC e exibição das informações em um display LCD. A simulação foi realizada no Simulide e o código desenvolvido no MikroC.

# 2. Desenvolvimento

# 2.1 Estrutura de Flags

A lógica principal do programa foi organizada em torno de flags de sincronismo — variáveis do tipo unsigned char declaradas como volatile, que funcionam como sinalizadores entre as rotinas de interrupção e o loop principal.

As flags utilizadas foram:
 - flagAtualizaSelecao — sinaliza que o menu de seleção deve ser redesenhado
 - flagIniciaContagem — sinaliza o início de uma contagem, disparando a primeira leitura do ADC
 - flagAtualizaValor — levantada a cada segundo pelos timers, indica que o display deve ser atualizado
 - flagFimContagem — indica que o tempo esgotou e a tela final deve ser exibida

Esse modelo evita o uso de delays no loop principal e mantém o programa responsivo às interrupções.

## 2.2 Temporizadores

Foram utilizados dois temporizadores com interrupção para gerar as bases de tempo:
  - Timer 0 — utilizado no modo longo (60 segundos). Configurado com prescaler 1:8 e clock de 8 MHz, gera estouro a cada 200 ms. Um subcontador subTick200ms acumula 5 estouros para completar 1 segundo, quando então a contagem regressiva é decrementada.
  - Timer 1 — utilizado no modo curto (10 segundos). Configurado com prescaler 1:8, gera estouro a cada 250 ms. O subcontador subTick250ms acumula 4 estouros para completar 1 segundo.
Ambos os timers são recarregados manualmente com presets TMR0H/TMR0L e TMR1H/TMR1L a cada interrupção para manter a precisão do período.

## 2.3 Pinos e Direção (I/O)

A configuração dos pinos seguiu o padrão do kit EasyPIC v7:

<img width="642" height="343" alt="{33802313-621F-4D86-B4D8-A0F808FEFB58}" src="https://github.com/user-attachments/assets/6c7947ea-058d-4d31-8fc8-24c240d7b544" />

Os registradores TRIS foram usados para definir direção: 1 para entrada e 0 para saída. O registrador LAT foi usado para escrita segura nos pinos de saída.

## 2.4 Botões e Interrupções Externas

Os botões foram configurados como interrupções externas INT0 (RB0) e INT1 (RB1), com disparo na borda de subida (INTEDG = 1). Um debounce por software de 20 ms foi aplicado antes de confirmar o estado do pino, evitando leituras múltiplas por um único acionamento.

O Botão 1 alterna entre os modos longo e curto no menu. O Botão 2 inicia a contagem no modo selecionado.

## 2.5 Leitura ADC e Cálculo de Temperatura

O módulo ADC do PIC18F4550 possui resolução de 10 bits, produzindo valores entre 0 e 1023. Com Vref+ de 1V (conforme especificado), a relação com o LM35 é direta: 1V na entrada corresponde a 100°C.

O cálculo foi realizado em inteiros para evitar o uso de float, que consome memória excessiva no PIC:

cvalor_multiplicado = (unsigned long)valorADC * 1000;
temperatura = (unsigned int)(valor_multiplicado / 1023);

O resultado representa a temperatura em décimos de grau (ex: 253 = 25,3°C), permitindo exibição no formato XX.X°C sem ponto flutuante. O tipo unsigned long foi necessário porque 1023 × 1000 = 1.023.000, valor que ultrapassa o limite de 16 bits (65535).

## 2.6 Display LCD

O LCD HD44780 em modo 4 bits foi controlado pela biblioteca nativa do MikroC, com os pinos mapeados em RB2–RB7. A exibição foi dividida em duas linhas:


Linha 1: temperatura no formato Temp: XX.X°C
Linha 2: contagem regressiva no formato Tempo: XXs


O caractere de grau (°) foi gerado com o código ASCII estendido 223, compatível com o HD44780.

## 2.7 Simulação no Simulide

O circuito foi montado no Simulide com os seguintes componentes: PIC18F4550, display HD44780, dois push buttons com resistores de pull-down de 10 kΩ, LED com resistor de 220 Ω, fonte de tensão variável em RA0 simulando o LM35, e fonte fixa de 1V em RA3 como Vref externo.

A contagem regressiva e os botões funcionaram corretamente em simulação. A parte do ADC e exibição de temperatura apresentou dificuldades descritas na conclusão.

<img width="879" height="606" alt="{1CB3C630-06A8-4DE1-BAB5-2D18A30B0A1C}" src="https://github.com/user-attachments/assets/294aa7cb-959e-4823-905a-1ae48f38e90d" />

<img width="873" height="616" alt="{D253D456-ED7B-4772-BABE-F840900FCEF5}" src="https://github.com/user-attachments/assets/b1bc4c11-c8a5-498c-8575-6ca8974b8f47" />


# 3. Conclusão

O firmware desenvolvido implementou com sucesso a lógica de contagem regressiva com dois modos de duração, controle por interrupções externas, debounce por software e exibição no LCD. A estrutura de flags mostrou-se eficiente para sincronizar interrupções com o loop principal sem uso de polling contínuo.

A parte de leitura ADC e exibição de temperatura não pôde ser verificada em simulação. As principais hipóteses para o problema são:


Conflito entre ADC e LCD: a configuração do registrador ADCON1 pode estar alterando o comportamento de pinos compartilhados, impedindo a inicialização correta do LCD quando o ADC é configurado simultaneamente.
Carregamento incorreto do hex: verificou-se que diferentes arquivos .hex carregados no Simulide produziram comportamentos distintos, sugerindo que o arquivo gerado pela compilação do novo código pode não estar sendo carregado corretamente no PIC dentro do simulador.
Vref externo no Simulide: o Simulide pode não simular corretamente a referência externa de tensão em RA3, fazendo com que o ADC retorne valores incorretos ou trave o microcontrolador.


Para corrigir e melhorar o projeto, recomenda-se:


Testar na placa física EasyPIC v7, onde o hardware real do PIC18F4550 responde confiavelmente à configuração de Vref externo e ADC, eliminando limitações do simulador.
Verificar o processo de compilação e carregamento do hex no MikroC, garantindo que o arquivo gerado corresponde ao código mais recente antes de carregar no Simulide.
Isolar o teste do ADC com um código mínimo que apenas leia o ADC e exiba o valor bruto no LCD, sem timers ou interrupções, para confirmar o funcionamento do módulo isoladamente antes de integrar ao projeto completo.

## Nota:
Nao se percebeu o detalhe sobre os pinos A/D da PIC elucidados no final do documento do projeto. O erro foi dos alunos que nao perceberam  que haveria mais para se ler depois de todo o projeto, mas segue que a ignorancia desse detalhe pode(provavelmente) ter sido crucial no projeto nao simular corretamente no SIMULIDE.

"

Com relação às tensões de referência externas para o conversor A/D do microcontrolador
PIC18F4550, seguem algumas considerações importantes:
● Ao utilizar a biblioteca ADC no software MikroC PRO For PIC, ocorre o seguinte
erro (do próprio compilador): a função ADC_Init coloca os bits 4 e 5 do registrador
ADCON1 em 0 (por padrão). No entanto, estes dois bits devem estar configurados de
forma específica para permitir ajustar Vref do conversor A/D como sendo a
alimentação externa conectada aos pinos A2 e A3 do PIC. Essa configuração estava
causando erros ao testar o circuito no SimulIDE, pois os bits 4 e 5 sempre estavam em
0 (neste caso, o microcontrolador utiliza as próprias tensões de alimentação VDD e
VSS como referência Vref do A/D algo que não queremos no caso deste projeto). Da
mesma forma, a função ADC_Read por vezes pode não funcionar corretamente para
tensões de referência externas. Se for o caso, deve-se usar a função
ADC_Get_Sample.
● De qualquer forma, para resolver o problema, o módulo ADC deve ser inicializado
primeiro e, somente em linhas posteriores, definir a configuração do registrador
ADCON1, pois notamos que o problema persiste se ADCON1 já tiver sido
configurado em linha de código anterior a inicialização da biblioteca.
● Para testar, durante a simulação no SimulIDE, basta clicar com o botão direito do
mouse sobre o microcontrolador e escolher a opção “Open MCU Monitor” e escolher
o registrador ADCON1 para verificar se a configuração dos bits está correta.

"
