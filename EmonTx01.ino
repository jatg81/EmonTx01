/********************************************************************************************************************************************************************************************/
/* Sketch para el Emontx01                                                                                                                                                                  */
/* Version: 2.1                                                                                                                                                                             */
/* Date   : 02/05/2017                                                                                                                                                                      */
/* Author : Juan Taba Gasha                                                                                                                                                                 */
/* Last Rev: 09/04/2018                                                                                                                                                                     */
/********************************************************************************************************************************************************************************************/

/*Versión modificada de la libreria EmonLib que habilita la detención inmediata de los bucles en el método de cálculo de potencias, corrientes,voltajes,etc,mediante el seteo de la variable 
timeout a 0. De esta forma se ejecuta en forma  inmediata el envio del estado del pulsador (tiempo presionado) y se deshabilita la medición de parametros electricos.

El pulsador es normalmente abierto y esta cableado a la entrada digital 4, su función es encender y apagar la barra led. La señal se envia por RF al emonPi el cual la procesa y envia a traves
de la red Wifi utilizando el protocolo TCP al controlador de led UFO. 
*/

#include <EmonLibMod.h>                     
#include <PinChangeInt.h>
#include <RF69Mod.h> 
                                                                           
#define RF_freq            RF69_433MHZ                                                                      //Frecuencia de transmision 433Mhz
#define ledTxPin           9
#define ledStripBtnPin     4                                                                                //Entrada digital para el pulsador que controla la barra de led 

volatile bool disableCalcVI;
byte txErr;
const int nodeId = 11;                                              
const int netGroup = 210;                                                                                       
unsigned long currentTime,cloopTime1,timeOn;
long long ackRev_buf;
volatile unsigned long lastChangeTime;

EnergyMonitor ct1,ct2,ct3,ct4;                                         
volatile typedef struct { int dat0,dat1,dat2,dat3,dat4,dat5,dat6,dat7,dat8,dat9,dat10,dat11; byte dat12;} PayloadTX;  
PayloadTX emontx;                                                       

void sendData(){
  if(!rf69_sendWithRetry(nodeId, &emontx, sizeof emontx)){                                                                                            
    ackRev_buf|=1;                                                                                          //Modifica el contador de errores de Tx cuando no se recibe ACK
    txErr++;
  }
  else digitalWrite (ledTxPin,!digitalRead(ledTxPin));
  if ((ackRev_buf&1LL<<59)) txErr--;                                        
  ackRev_buf=ackRev_buf<<1;
}

void ledStripBtnOn()
{                                                                                                           //Función que guarda el momento en que fue presionado el pulsador
  if (millis() - lastChangeTime > 100){                                                                     //Se filtra las falsas señales debido a rebotes del pulsador por 100ms
       disableCalcVI=true;                                                                                  //Detiene el método de calculo de potencias,voltajes y corrientes
       lastChangeTime=millis();
  }
}

void setup() 
{                                
  pinMode(ledTxPin,OUTPUT);                                                                                  //Configura la salida para el led indicador de transmisión RF
  pinMode(ledStripBtnPin,INPUT);   
  digitalWrite(ledStripBtnPin,LOW);                                                                          //Desactiva resistencia pull up
  attachPinChangeInterrupt(ledStripBtnPin,ledStripBtnOn,FALLING);                                            //Detecta instante cuando es activado el pulsador
  
  for (uint8_t i=0; i < 8; i++) {                                                                            //Espera 3s para estabilizar los filtros y empezar a enviar datos
    digitalWrite (ledTxPin,!digitalRead(ledTxPin));
    delay(375);
  }
  
  rf69_initialize(nodeId, RF_freq, netGroup);
  ct1.current(1, 60.606);                                                 
  ct2.current(2, 60.606);                                                                                    // Factor de calibración = CT ratio / burden resistencia
  ct3.current(3, 60.606); 
  ct4.current(4, 60.606); 
  ct1.voltage(0, 292, 1.7);                                                                                  // Entrada de voltaje (0), factor para el adaptador AC-AC (292) (cada punto equivale a 0.5V aprox)          
  ct2.voltage(0, 292, 1.7);                                                 
  ct3.voltage(0, 292, 1.7);
  ct4.voltage(0, 292, 1.7);                                                 
}

void loop() 
{ 
  currentTime = millis();
  
  if(disableCalcVI){
    if (!digitalRead(ledStripBtnPin)){                                                                        // Verifica que el pulsador esta presionado, para eliminar falsa señales de flicker que afectan la entrada digital
        emontx.dat11=1;
        sendData();
        int i=1;
        do{
            timeOn=millis()-lastChangeTime;                                                                   // Envia el número de segundos que se encuentra presionado el pulsador, para realizar el cambio de color de la barra led                                                                
            if(timeOn>(1000*i+1000) ){
                i++;
                emontx.dat11=i;
                sendData();
             }   
         }while(!digitalRead(ledStripBtnPin));
         delay(100);
         emontx.dat11=0;
         sendData();
    }
    disableCalcVI=false;
  } 
  else if (currentTime-cloopTime1 >= 2000){                                                                    // Cada 2 segundos se envian datos de medición de parametros electricos si y solo si el pulsador no esta activado
    cloopTime1 = currentTime;
    
    ct1.calcVI(20,500);                                                                                        // Ejecuta método de cálculo de potencia, corriente, etc para el CT1, analizando 20 medias ondas (aprox 167ms)                                                           
    ct2.calcVI(20,500);                                                                                        // Ejecuta método de cálculo de potencia, corriente, etc para el CT2, analizando 20 medias ondas (aprox 167ms) 
    ct3.calcVI(20,500);                                                                                        // Ejecuta método de cálculo de potencia, corriente, etc para el CT3, analizando 20 medias ondas (aprox 167ms) 
    ct4.calcVI(20,500);                                                                                        // Ejecuta método de cálculo de potencia, corriente, etc para el CT4, analizando 20 medias ondas (aprox 167ms) 
 
    if(!disableCalcVI){                                                                                        // Si se presiona el boton cuando se esta ejecutando alguna función CalcVI no se envian los datos
      emontx.dat0=max(0,ct1.realPower);                                                                        // 1) Potencia real total
      emontx.dat1=max(0,ct2.realPower);                                                                        // 2) Potencia real tomacorrientes
      emontx.dat2=max(0,ct3.realPower);                                                                        // 3) Potencia real luces  
      emontx.dat3=max(0,ct4.realPower);                                                                        // 4) Potencia real terma
      emontx.dat4=ct1.Irms*100;                                                                                // 5) Corriente total
      emontx.dat5=ct2.Irms*100;                                                                                // 6) Corriente tomacorrientes
      emontx.dat6=ct3.Irms*100;                                                                                // 7) Corriente luces
      emontx.dat7=ct4.Irms*100;                                                                                // 8) Corriente terma
      emontx.dat8=ct1.Vrms;                                                                                    // 9) Voltaje
      emontx.dat9=ct1.apparentPower;                                                                           // 10) Potencia aparente total
      emontx.dat10=ct1.powerFactor*100;                                                                        // 11) Factor de potencia total
      emontx.dat11=0;                                                                                          // 12) Tiempo transcurrido desde que el pulsador se mantiene presionado 
      emontx.dat12=txErr;                                                                                      // 13) Cantidad de envios de datos sin recepción de ACK
      sendData();
    }         
  }
}
