// PROJETO 2 - ENTREGA FINAL
// Alexsandra Pavani Xavier NUSP 14681372
// Antonio Rosa Galindo de Andrade NUSP 15440868


// Mapeamento das conex?es do LCD
sbit LCD_RS at RB2_bit;
sbit LCD_EN at RB3_bit;
sbit LCD_D4 at RB4_bit;
sbit LCD_D5 at RB5_bit;
sbit LCD_D6 at RB6_bit;
sbit LCD_D7 at RB7_bit;

// Direcionamento dos pinos do LCD
sbit LCD_RS_Direction at TRISB2_bit;
sbit LCD_EN_Direction at TRISB3_bit;
sbit LCD_D4_Direction at TRISB4_bit;
sbit LCD_D5_Direction at TRISB5_bit;
sbit LCD_D6_Direction at TRISB6_bit;
sbit LCD_D7_Direction at TRISB7_bit;

// Estados da m?quina de contagem
#define ESTADO_PARADO  0
#define ESTADO_LONGA   1
#define ESTADO_CURTA   2

// Modos de dura??o
#define MODO_LONGO     0  // 60 segundos
#define MODO_CURTO     1  // 10 segundos

// Presets dos timers
#define TMR0_PRESET_H  0x3C
#define TMR0_PRESET_L  0xB0
#define TMR1_PRESET_H  0x0B
#define TMR1_PRESET_L  0xDC

// Vari?veis globais
volatile unsigned char estado = ESTADO_PARADO;
volatile unsigned char modoSelecionado = MODO_LONGO;

volatile unsigned int contagemLonga;
volatile unsigned int contagemCurta;

unsigned char subTick200ms;
unsigned char subTick250ms;

// Flags de sincronismo
volatile unsigned char flagAtualizaSelecao = 1;
volatile unsigned char flagIniciaContagem  = 0;
volatile unsigned char flagAtualizaValor   = 0;
volatile unsigned char flagFimContagem     = 0;

unsigned int valorADC = 0;
unsigned int temperatura = 0;


// FUun??es de Tela

// Formata e exibe a temperatura na primeira linha
void Exibe_Temperatura(unsigned int temp)
{
    unsigned int parte_inteira;
    char dezena, unidade, decimal;

    //Extrai apenas a parte inteira
    parte_inteira = temp / 10;

    //Separa os d?gitos da parte inteira e converter para texto com o '+ '0''
    dezena  = (parte_inteira / 10) + '0';
    unidade = (parte_inteira % 10) + '0';

    //Pega no ?ltimo d?gito ap?s a v?rgula e converter para texto
    decimal = (temp % 10) + '0';

    //Escreve tudo no display
    Lcd_Out(1, 1, "Temp: ");
    Lcd_Chr(1, 7, dezena);
    Lcd_Chr(1, 8, unidade);
    Lcd_Chr(1, 9, '.');
    Lcd_Chr(1, 10, decimal);
    Lcd_Chr(1, 11, 223);
    Lcd_Chr(1, 12, 'C');
}

// Exibe a contagem regressiva restante na segunda linha do display
void Exibe_Tempo(unsigned int tempoRestante)
{
    char dezena, unidade;

    // Pega a dezena, Divide por 10 e soma '0'
    dezena  = (tempoRestante / 10) + '0';

    //Pega a unidade usando o resto da divis?o (%) e somar '0'
    unidade = (tempoRestante % 10) + '0';

    // Escreve no Display
    Lcd_Out(2, 1, "Tempo: ");
    Lcd_Chr(2, 8, dezena);
    Lcd_Chr(2, 9, unidade);
    Lcd_Out(2, 11, "s");
}

// Mostra o menu de sele??o em Standby - controlado pelo Bot?o 1
void Mostra_Tela_Selecao()
{
    Lcd_Cmd(_LCD_CLEAR); // Limpa a tela
    Lcd_Out(1, 1, "Selec.Tempo (B1)");

    // Se a vari?vel estiver guardando o valor do modo longo
    if (modoSelecionado == MODO_LONGO) {
        Lcd_Out(2, 1, "> Longo (60s)");
    }
    // Se n?o, s? pode ser o modo curto
    else {
        Lcd_Out(2, 1, "> Curto (10s)");
    }
}

// Bloco de Interrup??es

void interrupt()
{
    // Botao 1: Muda o tempo (longo ou curto)
    if (INT0IF_bit == 1) // se a flag de interrup??o do bot?o 1 subiu
    {
        Delay_ms(20); // espera 20ms(Debounce)

        if (PORTB.B0 == 1) // confirma se o bot?o realmente foi apertado
        {
            // s? deixa mudar o menu se o forno estiver parado
            if (estado == ESTADO_PARADO)
            {
                if (modoSelecionado == MODO_LONGO) {
                    modoSelecionado = MODO_CURTO; // troca para curto
                } else {
                    modoSelecionado = MODO_LONGO; // troca para longo
                }
                flagAtualizaSelecao = 1; // avisa o main para atualizar a tela
            }
        }
        INT0IF_bit = 0; // abaixa a flag da interrup??o
    }

    // Botao 2: Bot?o de START
    if (INT1IF_bit == 1)
    {
        Delay_ms(20); // Debounce

        if (PORTB.B1 == 1)
        {
            if (estado == ESTADO_PARADO) // S? d? o start se estiver parado
            {
                if (modoSelecionado == MODO_LONGO)
                {
                    estado = ESTADO_LONGA; // muda o estado geral
                    contagemLonga = 60;    // prepara a contagem
                    subTick200ms  = 0;     // zera o sub-contador

                    // Liga o Timer 0
                    T0CON = 0x82;
                    TMR0H = TMR0_PRESET_H;
                    TMR0L = TMR0_PRESET_L;
                    TMR0IF_bit = 0;
                    TMR0IE_bit = 1;
                }
                else
                {
                    estado = ESTADO_CURTA;
                    contagemCurta = 10;
                    subTick250ms  = 0;

                    // Liga o Timer 1
                    T1CON = 0x31;
                    TMR1H = TMR1_PRESET_H;
                    TMR1L = TMR1_PRESET_L;
                    TMR1IF_bit = 0;
                    TMR1IE_bit = 1;
                }
                flagIniciaContagem = 1; // avisa o main para dar o primeiro passo na tela
            }
        }
        INT1IF_bit = 0;
    }

    // TIMER 0 (Conta 1 segundo para o modo longo)
    if (TMR0IF_bit == 1)
    {
        TMR0H = TMR0_PRESET_H; // recarrega o timer
        TMR0L = TMR0_PRESET_L;
        TMR0IF_bit = 0;

        if (estado == ESTADO_LONGA)
        {
            subTick200ms = subTick200ms + 1; // Soma +1 estouro

            if (subTick200ms >= 5) // Se deu 5 estouros (5 x 200ms = 1s)
            {
                subTick200ms = 0;  // Zera para o pr?ximo segundo

                if (contagemLonga > 0) {
                    contagemLonga = contagemLonga - 1; // Tira 1 segundo do rel?gio
                    flagAtualizaValor = 1;             // Avisa o main para mudar a tela
                }

                if (contagemLonga == 0) { // Se o tempo acabou
                    TMR0ON_bit = 0;       // Desliga o Timer
                    TMR0IE_bit = 0;
                    estado = ESTADO_PARADO;
                    flagFimContagem = 1;  // Avisa o main para mostrar tela final
                }
            }
        }
    }

    //TIMER 1 (Conta 1 segundo para o modo curto)
    if (TMR1IF_bit == 1)
    {
        TMR1H = TMR1_PRESET_H;
        TMR1L = TMR1_PRESET_L;
        TMR1IF_bit = 0;

        if (estado == ESTADO_CURTA)
        {
            subTick250ms = subTick250ms + 1;

            if (subTick250ms >= 4) // Se deu 4 estouros (4 x 250ms = 1s)
            {
                subTick250ms = 0;

                if (contagemCurta > 0) {
                    contagemCurta = contagemCurta - 1;
                    flagAtualizaValor = 1;
                }

                if (contagemCurta == 0) {
                    TMR1ON_bit = 0;
                    TMR1IE_bit = 0;
                    estado = ESTADO_PARADO;
                    flagFimContagem = 1;
                }
            }
        }
    }
}


// Programa principal
void main()
{
    //vari?vel de apio para o calculo
    unsigned long valor_multiplicado;

    //inicializa??o
    ADC_Init(); // Inicia o m?dulo anal?gico

    // configura??o dos registradores
    ADCON1 = 0x3B;     // configura os pinos anal?gicos e ativa a refer?ncia de 1V
    CMCON  = 0x07;     // desliga coisas anal?gicas antigas do PIC

    // defini??o entrada e sa?da
    TRISA.B0 = 1;      // sensor de temperatura
    TRISA.B2 = 1;      // fio do Ground do Sensor
    TRISA.B3 = 1;      // fio de 1V do Sensor
    TRISB.B0 = 1;      // bot?o 1
    TRISB.B1 = 1;      // bot?o 2
    TRISD.B0 = 0;      // LED (Forno)

    LATD.B0  = 0;      // garante que o LED comece desligado

    // liga display e interrup??es
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);

    INTCON2.INTEDG0 = 1; // bot?o 1 dispara quando solta
    INTCON2.INTEDG1 = 1; // bot?o 2 dispara quando solta

    INT0IE_bit = 1; // liga interrup??o do bot?o 1
    INT1IE_bit = 1; // liga interrup??o do bot?o 2
    GIE_bit    = 1; // chave geral de interrup??es ligada
    PEIE_bit   = 1; // chave secund?ria ligada

    //Loop Infinito
    while (1)
    {
        // tela de menu
        if (flagAtualizaSelecao == 1)
        {
            flagAtualizaSelecao = 0; // abaixa a flag
            Mostra_Tela_Selecao();   // chama a fun??o que desenha a tela
        }

        // Start
        // L? pela primeira vez o ADC e desenha a tela para n?o come?ar vazia
        if (flagIniciaContagem == 1)
        {
            flagIniciaContagem = 0;
            Lcd_Cmd(_LCD_CLEAR);

            valorADC = ADC_Get_Sample(0); // L? o sensor (pino 0)

            // Calculo da temperatura
            // Usamos "unsigned long" porque 1023 x 1000 d? mais de 1 milh?o.
            // Uma vari?vel normal do PIC s? aguenta at? 65535 (16 bits) e iria travar.
            valor_multiplicado = (unsigned long)valorADC * 1000;
            temperatura = valor_multiplicado / 1023;

            Exibe_Temperatura(temperatura);

            // Tempo mostrado na tela
            if (estado == ESTADO_LONGA) {
                Exibe_Tempo(contagemLonga);
            } else {
                Exibe_Tempo(contagemCurta);
            }

            // Liga ou desliga o forno (LED)
            if (temperatura > 500) {  // 500 equivale a 50.0 graus
                LATD.B0 = 1;          // Liga o LED
            } else {
                LATD.B0 = 0;          // Desliga o LED
            }
        }

        // Aop?s 1 segundo
        // Essa flag sobe a cada 1 segundo redondo contado pelos Timers
        if (flagAtualizaValor == 1)
        {
            flagAtualizaValor = 0; // Abaixa a flag

            valorADC = ADC_Get_Sample(0);

            // Faz o c?lculo passo a passo novamente
            valor_multiplicado = (unsigned long)valorADC * 1000;
            temperatura = valor_multiplicado / 1023;

            Exibe_Temperatura(temperatura);

            if (estado == ESTADO_LONGA) {
                Exibe_Tempo(contagemLonga);
            } else if (estado == ESTADO_CURTA) {
                Exibe_Tempo(contagemCurta);
            }

            // Sistema de seguran?a do LED
            if (temperatura > 500) {
                LATD.B0 = 1;
            } else {
                LATD.B0 = 0;
            }
        }

        // Fim
        if (flagFimContagem == 1)
        {
            flagFimContagem = 0; // Abaixa a flag

            LATD.B0 = 0; // Desliga forno

            Lcd_Cmd(_LCD_CLEAR); // Limpa tela
            Lcd_Out(1, 1, "    Medicao     ");
            Lcd_Out(2, 1, "   Finalizada   ");

            Delay_ms(2500); // Fica congelado por 2,5 segundos mostrando a mensagem

            flagAtualizaSelecao = 1; // Levanta a flag do menu inicial de novo
        }
    }
}
