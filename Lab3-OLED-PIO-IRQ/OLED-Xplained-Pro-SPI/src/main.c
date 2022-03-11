#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

// LED
#define LED_PIO      PIOA
#define LED_PIO_ID   ID_PIOA
#define LED_IDX      0
#define LED_IDX_MASK (1 << LED_IDX)

// Botão
#define BUT_PIO      PIOD
#define BUT_PIO_ID   ID_PIOD
#define BUT_IDX  28
#define BUT_IDX_MASK (1 << BUT_IDX)

#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX) // esse já está pronto.

#define BUT3_PIO PIOA
#define BUT3_PIO_ID ID_PIOA
#define BUT3_PIO_IDX 19
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX) // esse já está pronto.


volatile char flag;
volatile char flag2;
volatile char flag3;


void io_init(void);
void pisca_led(int n, int t);

	

void but_callback(void)
{
	if(pio_get(BUT_PIO, PIO_INPUT, BUT_IDX_MASK)){
		flag = 1;
	}
	else{
		flag = 0;	
	}
	
}

void but2_callback(void)
{
	if(pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK)){
		flag2 = 1;
	}
	else{
		flag2 = 0;
	} 
}

void but3_callback(void)
{
	if(pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK)){
		flag3 = 1;
	}
	else{
		flag3 = 0;
	}
}
// pisca led N vez no periodo T
void pisca_led(int n, int t){
	for (int i=0;i<n;i++){
		if (flag2){
			flag=0;
			flag3=0;
			flag2=0;
			break;
		}
		else{
			pio_clear(LED_PIO, LED_IDX_MASK);
			delay_ms(t);
			pio_set(LED_PIO, LED_IDX_MASK);
			delay_ms(t);
		}
	}
}

// Inicializa botao SW0 do kit com interrupcao
void io_init(void)
{

	// Configura led
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

	// Inicializa clock do periférico PIO responsavel pelo botao
	pmc_enable_periph_clk(BUT_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);

	// Configura PIO para lidar com o pino do botão como entrada
	// com pull-up
	pio_configure(BUT_PIO, PIO_INPUT, BUT_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO, BUT_IDX_MASK, 60);
	
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT2_PIO, BUT2_PIO_IDX_MASK, 60);
	
	pio_configure(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT3_PIO, BUT3_PIO_IDX_MASK, 60);

	// Configura interrupção no pino referente ao botao e associa
	// função de callback caso uma interrupção for gerada
	// a função de callback é a: but_callback()
	pio_handler_set(BUT_PIO,
	BUT_PIO_ID,
	BUT_IDX_MASK,
	PIO_IT_EDGE,
	but_callback);
	
	pio_handler_set(BUT2_PIO,
	BUT2_PIO_ID,
	BUT2_PIO_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but2_callback);
	
	pio_handler_set(BUT3_PIO,
	BUT3_PIO_ID,
	BUT3_PIO_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but3_callback);

	// Ativa interrupção e limpa primeira IRQ gerada na ativacao
	pio_enable_interrupt(BUT_PIO, BUT_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO);
	
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT2_PIO);
	
	pio_enable_interrupt(BUT3_PIO, BUT3_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT3_PIO);
	
	// Configura NVIC para receber interrupcoes do PIO do botao
	// com prioridade 4 (quanto mais próximo de 0 maior)
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 4); // Prioridade 4
	
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 4); // Prioridade 4
}

int main (void)
{
	// Inicializa clock
	sysclk_init();
	int t = 500;
	board_init();
	sysclk_init();
	delay_init();
	io_init();
	
	// Init OLED
	gfx_mono_ssd1306_init();
	
	gfx_mono_draw_string("Delay", 10, 18, &sysfont);
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if (flag){
			t+=100;
			char str[128];
			sprintf(str, "%d", t);
			
			gfx_mono_draw_string(str, 70, 18, &sysfont);
			pisca_led(30,t);
			

			flag = 0;
			}
		
		else if (flag3){
			t = t-100;
			char str[128];
			sprintf(str, "%d", t);
			
			gfx_mono_draw_string(str, 70, 18, &sysfont);
			pisca_led(30,t);
			
			flag3 = 0;
		
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}		
	
}
