# EmonTx01
Monitor de energia

Medicion de parametros electricos del tablero de alimentacion mediante la tarjeta de hardware open source EmonTx Arduino Shield SMT, para realizar la medicion de la corriente
se utilizan 4 transformadores de corrientes (total,tomacorrientes,luces y terma) y para medir el voltaje de red se utiliza un transformador de 220/9VAC
Los datos medidos son enviados al Raspberry Pi (EmonPi) mediante la tarjeta de RF RM69CW 433Mhz utilizando el protocolo RF12 de jeelib.
