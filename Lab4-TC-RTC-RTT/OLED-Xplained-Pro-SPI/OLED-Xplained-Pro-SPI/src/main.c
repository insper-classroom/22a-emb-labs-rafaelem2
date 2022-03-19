#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"


//LED 1
#define LED_PIO			  PIOA 
#define LED_PIO_ID		  ID_PIOA
#define LED_PIO_IDX		  0
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX)   // Mascara para CONTROLARMOS o LED

//LED 2
#define LED2_PIO			  PIOC
#define LED2_PIO_ID		  ID_PIOC
#define LED2_PIO_IDX		  30
#define LED2_PIO_IDX_MASK  (1 << LED2_PIO_IDX)   // Mascara para CONTROLARMOS o LED

//LED 3
#define LED3_PIO			  PIOB
#define LED3_PIO_ID		  ID_PIOB
#define LED3_PIO_IDX		  2
#define LED3_PIO_IDX_MASK  (1 << LED3_PIO_IDX)   // Mascara para CONTROLARMOS o LED

// Configuracoes do BOTAO 1
#define BUT1_PIO			  PIOD
#define BUT1_PIO_ID        ID_PIOD
#define BUT1_PIO_IDX	      28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX) 

//Variaveis globais
volatile char alarm = 0;

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t seccond;
} calendar;

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		
	}
	
	
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		
		alarm = 1;
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.seccond);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

void pisca_led (int n, int t) {
	for (int i=0;i<n;i++){
		pio_clear(LED3_PIO, LED3_PIO_IDX_MASK);
		delay_ms(t);
		pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
		delay_ms(t);
	}
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrup�c�o no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrup��o foi satisfeita.
	* Isso � realizado pela leitura do status do perif�rico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_PIO_IDX_MASK);  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		pin_toggle(LED2_PIO, LED2_PIO_IDX_MASK);    // BLINK Led
	}
}

void init(void) {
	//Initialize the board clock
	sysclk_init();
	
	// Desativa WatchDog Timer
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	//INICIANDO O PROCESSO COM A PLACA OLED
	pmc_enable_periph_clk(LED_PIO_ID);
	//Inicializa LED1 como sa�da
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 1, 0, 0);
	
	pmc_enable_periph_clk(LED2_PIO_ID);
	//Inicializa LED2 como sa�da
	pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);
	
	pmc_enable_periph_clk(LED3_PIO_ID);
	//Inicializa LED3 como sa�da
	pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, 1, 0, 0);
	
	// Inicializa PIO do BOTAO 1
	pmc_enable_periph_clk(BUT1_PIO_ID);
	// configura pino ligado ao bot�o como entrada com um pull-up.
	pio_set_input(BUT1_PIO,BUT1_PIO_IDX_MASK,PIO_DEFAULT);
	pio_pull_up(BUT1_PIO,BUT1_PIO_IDX_MASK,1);
	
	TC_init(TC0, ID_TC1, 1, 4);
	tc_start(TC0, 1);
	
	RTT_init(0.25, 0, RTT_MR_RTTINCIEN); 
}

int main (void)
{
	board_init();
	sysclk_init();
	init();
	delay_init();

  gfx_mono_ssd1306_init();
  
  gfx_mono_draw_string("Conceito C", 10, 18, &sysfont);
  
  

  /* Insert application code here, after the board has been initialized. */
	while(1) {
			
			if (!pio_get(BUT1_PIO,PIO_INPUT, BUT1_PIO_IDX_MASK)) {
					
					rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
					
					rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
					
					calendar rtc_initial = {current_year, current_month, current_day, current_week, current_hour, current_min ,current_sec};
						
					RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN); 
						
					
					rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 1);
					pio_set(LED3_PIO, LED3_PIO_IDX_MASK);      
					delay_ms(18000);                       
					pio_clear(LED3_PIO, LED3_PIO_IDX_MASK);    
					delay_ms(500);                       
				}
				
			if(alarm){
				pisca_led(5, 200);
				alarm = 0;
				
			}
			
			pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}