void I2C_Init_Hw(void)
{
/* Clock */
RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Âêëþ÷åíèå òàêòèðîâàíèÿ PortB
RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // Âêëþ÷åíèå òàêòèðîâàíèÿ ìîäóëÿ I2C1
RCC->AHBENR |= RCC_AHBENR_DMA1EN;   // Âêëþ÷åíèå òàêòèðîâàíèÿ DMA1
/* IO    */
//GPIOB->CRL |= 0xEE000000;   // PB6,PB7 - Alt.Func. Open Drain 2MHz
/* I2C1  */
I2C1->CR1 |= I2C_CR1_SWRST;  
delay(10);
I2C1->CR1 &= ~I2C_CR1_SWRST;  
//  
I2C1->CR2 = 0;             // Âñå ñáðîñèì, âäðóã ýòî ïîâòîðíàÿ èíèöèàëèçàöèÿ
I2C1->CR2 = 36; //8             // Òàêòîâàÿ 8ÌÃö
I2C1->CCR = 180; //40           // CCR=Period(I2C)/(2xTmaster) (äëÿ StandartMode)
I2C1->TRISE = 37; //9          // TRISER=(Trise/Tmaster)+1=(1/0.125)+1=9
I2C1->CR2 |= I2C_CR2_LAST; // Ãåíåðàöèÿ NACK â ðåæèìå DMA
I2C1->CR1 |= I2C_CR1_PE;   // Âêëþ÷åíèå ìîäóëÿ I2C

DMA1_Channel7->CCR |= DMA_CCR7_MINC;   // Àäðåñ ïàìÿòè èíêðåìåíòèðóåòñÿ
DMA1_Channel7->CCR &= ~DMA_CCR7_DIR ;  // Ïåðèôåðèÿ => Ïàìÿòü
DMA1_Channel7->CPAR = (*((volatile uint32_t *)I2C1->DR)); // Àäðåñ I2C1_DR

DMA1_Channel6->CCR |= DMA_CCR6_MINC;   // Àäðåñ ïàìÿòè èíêðåìåíòèðóåòñÿ
DMA1_Channel6->CCR |= DMA_CCR6_DIR ;   // Ïåðèôåðèÿ => Ïàìÿòü
DMA1_Channel6->CPAR = (*((volatile uint32_t *)I2C1->DR)); // Àäðåñ I2C1_DR
}

//============================================================================================================================
void I2C_DMA_Rx(uint8_t I2C_Addr, uint8_t *ptr, uint8_t I2C_Num_Byte, uint8_t I2C_StopFlag)
{
  // Ïðèåì
  I2C_Error = 0x00;             // Ñáðîñèòü îøèáêè
  I2C1->CR2 |= I2C_CR2_ITERREN; // Âêë. ïðåðûâàíèÿ ïðè îøèáêàõ I2C
  I2C_Start();
  if(!I2C_Error) I2C_Address(I2C_Addr|0x01);
  if(!I2C_Error)
  {
    // Ïðèåì äàííûõ
    I2C1->CR1 |= I2C_CR1_ACK;   // Ïîäòâåðæäåíèå ïðèåìà
    I2C1->CR2 |= I2C_CR2_DMAEN; // Ðàçðåøèòü çàïðîñû I2C=>DMA
    // Êàíàë 7 DMA
    DMA1_Channel7->CCR &= ~DMA_CCR1_EN;     // Îòêë. êàíàë 7 DMA
    DMA1_Channel7->CMAR = (uint32_t)(ptr);  // Àäðåñ ìàññèâà ïðèåìà
    DMA1_Channel7->CNDTR = I2C_Num_Byte;    // Êîëè÷åñòâî ïðèíèìàåìûõ áàéò
    DMA1_Channel7->CCR |= DMA_CCR1_EN;      // Âêë. êàíàë 7 DMA
        while(!(DMA1->ISR&DMA_ISR_TCIF7)) continue;
    DMA1->IFCR = DMA_IFCR_CTCIF7;           // Ñáðîñ ôëàãà ïðåðûâàíèÿ EOT
        while(!(I2C1->SR1&I2C_SR1_RXNE)) continue;
    I2C1->CR2 &= ~I2C_CR2_DMAEN;            // Çàïðåòèòü çàïðîñû I2C=>DMA   
    DMA1_Channel7->CCR &= ~DMA_CCR1_EN;     // Îòêë. êàíàë 7 DMA
  }
  if(I2C_StopFlag) I2C_Stop();
  I2C1->CR2 &= ~I2C_CR2_ITERREN; // Îòêë. ïðåðûâàíèÿ ïðè îøèáêàõ I2C
}

//============================================================================================================================
void I2C_DMA_Tx(uint8_t I2C_Addr, uint8_t *ptr, uint8_t I2C_Num_Byte, uint8_t I2C_StopFlag)
{
  // Ïåðåäà÷à
  I2C_Error = 0x00;         // Ñáðîñèòü îøèáêè
  I2C1->CR2 |= I2C_CR2_ITERREN; // Âêë. ïðåðûâàíèÿ ïðè îøèáêàõ I2C
  I2C_Start();
  if(!I2C_Error) I2C_Address(I2C_Addr);
  if(!I2C_Error)
  {
    I2C1->CR2 |= I2C_CR2_DMAEN; // Ðàçðåøèòü çàïðîñû I2C=>DMA
    // Êàíàë 6 DMA
    DMA1_Channel6->CCR &= ~DMA_CCR1_EN;     // Îòêë. êàíàë 6 DMA
    DMA1_Channel6->CMAR = (uint32_t)(ptr);   // Àäðåñ ìàññèâà ïåðåäà÷è
    DMA1_Channel6->CNDTR = I2C_Num_Byte;     // Êîëè÷åñòâî ïåðåäàâàåìûõ áàéò
    DMA1_Channel6->CCR |= DMA_CCR1_EN;       // Âêë. êàíàë 3 DMA
    while(!(DMA1->ISR&DMA_ISR_TCIF6)) continue;
    DMA1->IFCR = DMA_IFCR_CTCIF6;            // Ñáðîñ ôëàãà ïðåðûâàíèÿ EOT
    while(I2C1->SR1&I2C_SR1_BTF) continue;
    I2C1->CR2 &= ~I2C_CR2_DMAEN;            // Çàïðåòèòü çàïðîñû I2C=>DMA   
    DMA1_Channel6->CCR &= ~DMA_CCR1_EN;     // Îòêë. êàíàë 6 DMA
  }
  if(I2C_StopFlag) I2C_Stop();
  I2C1->CR2 &= ~I2C_CR2_ITERREN; // Îòêë. ïðåðûâàíèÿ ïðè îøèáêàõ I2C
}
