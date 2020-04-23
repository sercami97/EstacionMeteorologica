# Estacion Meteorológica Inteligente de Bajo Costo

El objetivo del proyecto es diseñar una estación meteorológica inteligente. Esto es, que sea capaz de usar protocolos de IoT para el almacenamiento de los datos en un servidor remoto y ser autosostenible (Energía solar). Además, se espera que los diseños sean fácilmente replicables y tengan un bajo costo. Esto con el objetivo de ser más acesible para las comunidades campesinas y lograr un mayor impacto.

## Comenzando
Descarga o clona el repositorio a tu maquina local. En la capeta ../CodigoArduino/DATA_SERVIDOR_NSM encontrarán el codigo fuente del proyecto (Arduino). Por otro lado, en la carpeta ../PCB/LATEST/PCB_final se encuentran los archivos pertinentes al diseño de la PCB del proyecto. Dentro de esta misma carpeta, en ../GerberSleep se encuentran los archivos gerber listos para mandar a producción. Finalmente, en ../Diseños3D se encuentran los archivos de los diseños 3D desarrollados para la estructura de la estación. 

### Pre-requisitos
Es necesario tener instalados los siguientes sowftware para acceder a los archivos del proyecto:
- Arduino
- KiCad EDA
- Autodesk Inventor
- Libería de Arduino "DHT.h" https://github.com/adafruit/DHT-sensor-library/archive/master.zip
- Libería de Arduino "Arduino.h"
- Libería de Arduino "Wire.h"
- Libería de "ArduinoSoftwareSerial.h"
- Libería de Arduino "DS3232RTC.h" https://github.com/JChristensen/DS3232RTC


### Instalación
Para contar con un prototipo del proyecto debes:
- Imprimir, soldar y armar PCB (En la wiki se describe con mejor detalle como hacer esto junto con los componentes necesarios).
- Imprimir y replicar piezas 3D (En la wiki se describe con mejor detalle como hacer esto junto con los materiales necesarios).
- Armar estación (En la wiki se describe con mejor detalle como hacer esto junto con las piezas necesarias).

### Ejecutando pruebas

#### 1. Codigo Arduino
Para probar la funcionalidad del codigo de arduino, ubíquese en la carpeta ../CodigoArduino/DATA_SERVIDOR_NSM y ejecute el archivo DATA_SERVIDOR_NSM.ino con Arduino. Compila el programa en 'programa' -> 'Verificar/Compilar'. Verificar que no aparezca ningun error en la consola de salida.

#### 2. PCB
Para probar la funcionalidad de la PCB es necesario hacer pruebas de continuidad entre los nodos de la misma. Para esto, puedes usar un multimetro. Si todo está correcto, se puede pasar conectar los componentes y  alimentar la estación con su respectiva batería de litio. 

#### 4. Arduino + PCB
Una vez alimentada la PCB, puede conectar el arduino pro-mini a su ordenador haciendo uso de un cable y módulo FTDI. Ejecute el programa en Arduino y verifique que la placa en 'Herramientas' -> 'Placa' sea Arduino pro mini. Luego, suba el programa en 'Programa' -> 'Subir' y abra el serial en 'Herramientas' -> 'Monitor Serie'. Deberian comenzar a aparecer mensajes que indican la correcta ejecución del código.

#### 5. Funcionalidad
Configure los intervalos de medición de la estación y verifique la funcionalidad de los mensajes en el servidor: http://datos-env.iafjn3xg9q.us-east-1.elasticbeanstalk.com/datos/new/innovandes

#### 6. Estación Completa
Una vez verificados los pasos anteriores, se puede proceder a armar la estación completa con los diseños 3D. Armela y haga nuevamente la prueba del punto 5.

### Construido con
- Arduino
- KiCad EDA
- Autodesk Inventor

### Wiki 
Visita nuestra Wiki https://github.com/sercami97/EstacionMeteorologica/wiki para obtener mas información sobre el dearrollo y cómo utilizar este proyecto.

### Autores
- Iguarán Jorge - Diseños electrónicos y PCB 
- Rodriguez David - Código Arduino y Diseños electrónicos
- Sanabria Sergio - Diseños 3D y replicabilidad 






