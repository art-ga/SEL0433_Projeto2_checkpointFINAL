// PROJETO 2 - ENTREGA FINAL
// Alexsandra Pavani Xavier NUSP 14681372
// Antonio Rosa Galindo de Andrade NUSP 15440868

// Mapeamento das conexoes do LCD
sbit LCD_RS at RB2_bit;
sbit LCD_EN at RB3_bit;
sbit LCD_D4 at RB4_bit;
sbit LCD_D5 at RB5_bit;
sbit LCD_D6 at RB6_bit;
sbit LCD_D7 at RB7_bit;

sbit LCD_RS_Direction at TRISB2_bit;
sbit LCD_EN_Direction at TRISB3_bit;
sbit LCD_D4_Direction at TRISB4_bit;
sbit LCD_D5_Direction at TRISB5_bit;
sbit LCD_D6_Direction at TRISB6_bit;
sbit LCD_D7_Direction at TRISB7_bit;

// Estados
#define ESTADO_PARADO  0
#define ESTADO_LONGA   1
#define ESTADO_CURTA   2

#define MODO_LONGO     0
#define MODO_CURTO     1

// Presets dos timers para 8MHz
// TMR0: prescaler 1:4, estouro a cada 200ms
#define TMR0_PRESET_H  0x3C
#define TMR0_PRESET_L  0xB0

// TMR1: prescaler 1:8, estouro a cada 250ms
#define TMR1_PRESET_H  0x0B
#define TMR1_PRESET_L  0xDC

// Variaveis globais
volatile unsigned char estado          = ESTADO_PARADO;
volatile unsigned char modoSelecionado = MODO_LONGO;

volatile unsigned int contagemLonga;
volatile unsigned int contagemCurta;

unsigned char subTick200ms;
unsigned char subTick250ms;

volatile unsigned char flagAtualizaSelecao = 1;
volatile unsigned char flagIniciaContagem  = 0;
volatile unsigned char flagAtualizaValor   = 0;
volatile unsigned char flagFimContagem     = 0;

unsigned int valorADC    = 0;
unsigned int temperatura = 0;


void Exibe_Temperatura(unsigned int temp)
{
    unsigned int parte_inteira;
    char dezena, unidade, decimal;

    parte_inteira = temp / 10;
    dezena        = (parte_inteira / 10) + '0';
    unidade       = (parte_inteira % 10) + '0';
    decimal       = (temp % 10) + '0';

    Lcd_Out(1, 1, "Temp: ");
    Lcd_Chr(1, 7, dezena);
    Lcd_Chr(1, 8, unidade);
    Lcd_Chr(1, 9, '.');
    Lcd_Chr(1, 10, decimal);
    Lcd_Chr(1, 11, 223); // simbolo de grau
    Lcd_Chr(1, 12, 'C');
}

void Exibe_Tempo(unsigned int tempoRestante)
{
    char dezena, unidade;

    dezena  = (tempoRestante / 10) + '0';
    unidade = (tempoRestante % 10) + '0';

    Lcd_Out(2, 1, "Tempo: ");
    Lcd_Chr(2, 8, dezena);
    Lcd_Chr(2, 9, unidade);
    Lcd_Out(2, 10, "s  ");
}

void Mostra_Tela_Selecao()
{
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Out(1, 1, "Selec.Tempo (B1)");

    if (modoSelecionado == MODO_LONGO) {
        Lcd_Out(2, 1, "> Longo (60s)   ");
    } else {
        Lcd_Out(2, 1, "> Curto (10s)   ");
    }
}

void interrupt()
{
    // Botao 1: alterna modo no menu
    if (INT0IF_bit == 1)
    {
        Delay_ms(20);
        if (PORTB.B0 == 1)
        {
            if (estado == ESTADO_PARADO)
            {
                if (modoSelecionado == MODO_LONGO)
                    modoSelecionado = MODO_CURTO;
                else
                    modoSelecionado = MODO_LONGO;

                flagAtualizaSelecao = 1;
            }
        }
        INT0IF_bit = 0;
    }

    // Botao 2: inicia contagem
    if (INT1IF_bit == 1)
    {
        Delay_ms(20);
        if (PORTB.B1 == 1)
        {
            if (estado == ESTADO_PARADO)
            {
                if (modoSelecionado == MODO_LONGO)
                {
                    estado        = ESTADO_LONGA;
                    contagemLonga = 60;
                    subTick200ms  = 0;

                    T0CON      = 0x82;       // Timer0 16bit, prescaler 1:8
                    TMR0H      = TMR0_PRESET_H;
                    TMR0L      = TMR0_PRESET_L;
                    TMR0IF_bit = 0;
                    TMR0IE_bit = 1;
                }
                else
                {
                    estado        = ESTADO_CURTA;
                    contagemCurta = 10;
                    subTick250ms  = 0;

                    T1CON      = 0x31;       // Timer1, prescaler 1:8
                    TMR1H      = TMR1_PRESET_H;
                    TMR1L      = TMR1_PRESET_L;
                    TMR1IF_bit = 0;
                    TMR1IE_bit = 1;
                }
                flagIniciaContagem = 1;
            }
        }
        INT1IF_bit = 0;
    }

    // Timer0 — modo longo (5 x 200ms = 1s)
    if (TMR0IF_bit == 1)
    {
        TMR0H      = TMR0_PRESET_H;
        TMR0L      = TMR0_PRESET_L;
        TMR0IF_bit = 0;

        if (estado == ESTADO_LONGA)
        {
            subTick200ms++;
            if (subTick200ms >= 5)
            {
                subTick200ms = 0;
                if (contagemLonga > 0)
                {
                    contagemLonga--;
                    flagAtualizaValor = 1;
                }
                if (contagemLonga == 0)
                {
                    TMR0ON_bit = 0;
                    TMR0IE_bit = 0;
                    estado         = ESTADO_PARADO;
                    flagFimContagem = 1;
                }
            }
        }
    }

    // Timer1 — modo curto (4 x 250ms = 1s)
    if (TMR1IF_bit == 1)
    {
        TMR1H      = TMR1_PRESET_H;
        TMR1L      = TMR1_PRESET_L;
        TMR1IF_bit = 0;

        if (estado == ESTADO_CURTA)
        {
            subTick250ms++;
            if (subTick250ms >= 4)
            {
                subTick250ms = 0;
                if (contagemCurta > 0)
                {
                    contagemCurta--;
                    flagAtualizaValor = 1;
                }
                if (contagemCurta == 0)
                {
                    TMR1ON_bit = 0;
                    TMR1IE_bit = 0;
                    estado         = ESTADO_PARADO;
                    flagFimContagem = 1;
                }
            }
        }
    }
}


void Le_Temperatura()
{
    unsigned long valor_multiplicado;

    // Com Vref+ = 1V em RA3:
    // ADC maximo (1023) = 1V = 100°C no LM35
    // temperatura = (valorADC * 1000) / 1023
    // resultado em decimos de grau: 1000 = 100.0°C
    valorADC             = ADC_Get_Sample(0);
    valor_multiplicado   = (unsigned long)valorADC * 1000;
    temperatura          = (unsigned int)(valor_multiplicado / 1023);
}

void main()
{
    // 1. Primeiro desliga tudo que pode interferir
    CMCON  = 0x07;
    CVRCON = 0x00;

    // 2. Inicializa o LCD antes de qualquer config de ADC
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);
    Lcd_Out(1, 1, "FUNCIONANDO");
    // 3. So agora configura o ADC
    ADCON1 = 0x34;
    ADCON2 = 0xAA;
    ADC_Init();

    // 4. Direcao dos pinos
    TRISA.B0 = 1;
    TRISA.B3 = 1;
    TRISB.B0 = 1;
    TRISB.B1 = 1;
    TRISD.B0 = 0;
    LATD.B0  = 0;

    // 5. Interrupcoes
    INTCON2.INTEDG0 = 1;
    INTCON2.INTEDG1 = 1;
    INT0IE_bit = 1;
    INT1IE_bit = 1;
    GIE_bit    = 1;
    PEIE_bit   = 1;

    // resto igual...
    while (1)
    {
        // Menu de selecao
        if (flagAtualizaSelecao == 1)
        {
            flagAtualizaSelecao = 0;
            Mostra_Tela_Selecao();
        }

        // Primeira leitura ao iniciar
        if (flagIniciaContagem == 1)
        {
            flagIniciaContagem = 0;
            Lcd_Cmd(_LCD_CLEAR);

            Le_Temperatura();
            Exibe_Temperatura(temperatura);

            if (estado == ESTADO_LONGA)
                Exibe_Tempo(contagemLonga);
            else
                Exibe_Tempo(contagemCurta);

            // LED acima de 50.0°C (500 em decimos de grau)
            LATD.B0 = (temperatura > 500) ? 1 : 0;
        }

        // Atualizacao a cada segundo
        if (flagAtualizaValor == 1)
        {
            flagAtualizaValor = 0;

            Le_Temperatura();
            Exibe_Temperatura(temperatura);

            if (estado == ESTADO_LONGA)
                Exibe_Tempo(contagemLonga);
            else if (estado == ESTADO_CURTA)
                Exibe_Tempo(contagemCurta);

            LATD.B0 = (temperatura > 500) ? 1 : 0;
        }

        // Fim da contagem
        if (flagFimContagem == 1)
        {
            flagFimContagem = 0;
            LATD.B0 = 0;

            Lcd_Cmd(_LCD_CLEAR);
            Lcd_Out(1, 1, "    Medicao     ");
            Lcd_Out(2, 1, "   Finalizada   ");

            Delay_ms(2500);
            flagAtualizaSelecao = 1;
        }
    }
}
