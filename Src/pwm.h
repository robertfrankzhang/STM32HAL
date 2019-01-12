
#ifndef __pwm_h
#define __pwm_h
void disablePWMClocks(TIM_HandleTypeDef* htim_pwm);
void configChannel(TIM_HandleTypeDef* htim_pwm,uint32_t pulse,uint32_t channel);
void pwm_Init(uint32_t pin, TIM_TypeDef* timer, GPIO_TypeDef* port, uint32_t prescaler, uint32_t period, uint32_t pulse, uint32_t channel, TIM_HandleTypeDef *_htim);
void TIM_Init(uint32_t prescaler, uint32_t period, uint32_t pulse, uint32_t channel,TIM_HandleTypeDef* htim_pwm, uint32_t pin, TIM_TypeDef* timer,GPIO_TypeDef* port);
void HAL_TIM_PWM_MspInitGeneric(TIM_HandleTypeDef* htim_pwm, uint32_t pin, TIM_TypeDef* timer,GPIO_TypeDef* port);
#endif  //__pwm_h
