#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"


#define BUT_PIO1				PIOD
#define BUT_PIO1_ID				ID_PIOD
#define BUT_PIO1_IDX			28
#define BUT_PIO_1_IDX_MASK		(1u << BUT_PIO1_IDX)

#define TRIG_PIO				PIOA
#define TRIG_PIO_ID				ID_PIOA
#define TRIG_PIO_IDX			6
#define TRIG_PIO_IDX_MASK		(1u << TRIG_PIO_IDX)

#define ECHO_PIO				PIOD
#define ECHO_PIO_ID				ID_PIOD
#define ECHO_PIO_IDX			30
#define ECHO_PIO_IDX_MASK		(1u << ECHO_PIO_IDX)

volatile char but_1 = 0;
volatile char but_2 = 0;
volatile char but_3 = 0;

int freq = (float)1/(0.000058*2);
double tempo;

void init(void);
void limpa_oled();

void RTT_Handler(void);

void limpa_oled() {
	gfx_mono_draw_string("             ", 0, 5, &sysfont);
	gfx_mono_draw_string("             ", 0, 16, &sysfont);
}


void call_callback(void){
	but_1 = 1;
}

void RTT_Handler(void) {
	uint32_t ul_status;

	ul_status = rtt_get_status(RTT);
	
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		but_3 = 1;
	}
	
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
	}
	
}

void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN)){
		rtt_enable_interrupt(RTT, rttIRQSource);
	}
	
	else{
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	}
	
	
}


void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask)) {
		pio_clear(pio, mask);
	}
	
	else{
		pio_set(pio,mask);
	}
	
}

void echo_callback(void){
	if (pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_IDX_MASK)) {

		if (but_1){
			RTT_init(freq, 0 , 0);
			but_2 = 1;
		}
		
		else {
			but_3 = 1;
		}
	}
	
	else {
		but_2 = 0;
		tempo = rtt_read_timer_value(RTT);
	}
}



float distancia(){
	float dist = (float)(100.0*tempo*340)/(freq*2);
	
	if(dist >= 400){
		
		return -1;
		
	}
	else{
		
		return dist;
		
	}
	
}

void init(void){

	sysclk_init();
	WDT->WDT_MR = WDT_MR_WDDIS;
	pmc_enable_periph_clk(TRIG_PIO_ID);
	pio_set_output(TRIG_PIO, TRIG_PIO_IDX_MASK, 0, 0, 1);

	pmc_enable_periph_clk(BUT_PIO1_ID);
	pio_configure(BUT_PIO1, PIO_INPUT, BUT_PIO_1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO1, BUT_PIO_1_IDX_MASK, 60);
	pio_handler_set(BUT_PIO1,BUT_PIO1_ID,BUT_PIO_1_IDX_MASK,PIO_IT_FALL_EDGE,call_callback);
	pio_enable_interrupt(BUT_PIO1, BUT_PIO_1_IDX_MASK);
	pio_get_interrupt_status(BUT_PIO1);
	NVIC_EnableIRQ(BUT_PIO1_ID);
	NVIC_SetPriority(BUT_PIO1_ID, 4);

	pmc_enable_periph_clk(ECHO_PIO_ID);
	pio_configure(ECHO_PIO, PIO_INPUT,ECHO_PIO_IDX_MASK, PIO_DEFAULT);
	pio_set_debounce_filter(ECHO_PIO, ECHO_PIO_IDX_MASK, 60);
	pio_handler_set(ECHO_PIO,ECHO_PIO_ID,ECHO_PIO_IDX_MASK,PIO_IT_EDGE,echo_callback);
	pio_enable_interrupt(ECHO_PIO, ECHO_PIO_IDX_MASK);
	pio_get_interrupt_status(ECHO_PIO);
	NVIC_EnableIRQ(ECHO_PIO_ID);
	NVIC_SetPriority(ECHO_PIO_ID, 5);
}


int main (void) {
	board_init();
	delay_init();
	init();
	gfx_mono_ssd1306_init();

	while(1) {
		if (but_3){
			limpa_oled();
			gfx_mono_draw_string("sem leitura    ", 5, 10, &sysfont);
			but_3 = 0;
		}
		
		else if(but_2){
			int d = distancia();
			if(d == -1){
				limpa_oled();
				gfx_mono_draw_string("distante    ", 5, 10, &sysfont);
				but_3 = 0;
			}
			
			else {
				limpa_oled();
				char str[10];
				sprintf(str, "%d", d);
				gfx_mono_draw_string(str, 5, 10, &sysfont);
			}
			
			but_1 = 0;
		}

		else if(but_1){
			pio_clear(TRIG_PIO, TRIG_PIO_IDX_MASK);
			delay_us(10);
			pio_set(TRIG_PIO, TRIG_PIO_IDX_MASK);
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}