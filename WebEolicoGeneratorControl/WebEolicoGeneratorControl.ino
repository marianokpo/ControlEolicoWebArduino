#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#define ANALOGCARGA 4
#define ANALOGINTENCIDAD 3
#define ANALOGVOLTAGE 2

//Variables WEB ----------------------------------------------------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Dirección MAC de la etiqueta de Sheld Ethernet debajo de la Placa
IPAddress ip(192, 168, 0, 200); // Dirección IP, puede necesitar cambiar dependiendo de la red
EthernetServer server(80);  // crear un servidor en el puerto 80

File webFile;

// Variables---------------------------------------------------------
int analogValor = 0;
float voltaje = 0;

// Umbrales
float maximo = 12.5;
float medio = 11.0;
float minimo = 10.0;

// Sensibilidad del sensor en V/A-------------------------------------
//float SENSIBILITY = 0.185;   // Modelo 5A
//float SENSIBILITY = 0.100; // Modelo 20A
float SENSIBILITY = 0.066; // Modelo 30A

int SAMPLESNUMBER = 100;

float AmperGen = 0.0;


// Variables del sensor en Voltaje-------------------------------------
int sensorVolt;         // variable que almacena el valor raw (0 a 1023)
float VoltGen;            // variable que almacena el voltaje (0.0 a 25.0)


//********************************************************************
void setup()
{
  Ethernet.begin(mac, ip);  // inicializar dispositivo Ethernet
  server.begin();           // empieza a escuchar a los clientes
  
  // Iniciamos el monitor serie
  Serial.begin(9600); 
  
  // initialize SD card
  Serial.println("Inicializando SD...");
  if (!SD.begin(4)) 
  {
      Serial.println("ERROR - Fallo Inicializacion de tarjeta SD!");
      return;    // init failed
  }
  Serial.println("ÉXITO - Tarjeta SD inicializada.");
  // check for index.htm file
  if (!SD.exists("index.htm")) 
  {
      Serial.println("ERROR - No se puede encontrar el archivo index.htm!");
      return;  // can't find index file
  }
  Serial.println("SUCCESS - Archivo index.htm encontrado.");
  
}

void loop() 
{
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Leemos valor de la entrada analógica
  analogValor = analogRead(ANALOGCARGA);
 
  // Obtenemos el voltaje
  voltaje = 0.0048 * analogValor;
  
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  sensorVolt = analogRead(ANALOGVOLTAGE);          // realizar la lectura
  VoltGen = fmap(sensorVolt, 0, 1023, 0.0, 25.0);   // cambiar escala a 0.0 - 25.0
  
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
  AmperGen = getCorriente(SAMPLESNUMBER);
  //float currentRMS = 0.707 * current;
  //WattsGen = 230.0 * currentRMS;
  
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  EthernetClient client = server.available();  // tratar de conseguir cliente

  if (client) 
  {  // tiene cliente?
      boolean currentLineIsBlank = true;
      while (client.connected()) 
      {
          if (client.available()) 
          {   // datos de clientes disponibles para leer
              char c = client.read(); // leer 1 byte (carácter) del cliente
              // la última línea de solicitud del cliente está en blanco y termina con \ n
              // responder al cliente solo después de la última línea recibida
              if (c == '\n' && currentLineIsBlank) 
              {
                  // enviar un encabezado de respuesta http estándar
                  client.println("HTTP/1.1 200 OK");
                  client.println("Content-Type: text/html");
                  client.println("Connection: close");
                  client.println();
                  // enviar página web
                  webFile = SD.open("index.htm");        // abrir el archivo de la página web
                  if (webFile) 
                  {
                      while(webFile.available()) 
                      {
                          client.write(webFile.read()); // enviar página web al cliente
                      }
                      webFile.close();
                  }
                  break;
              }
              // cada línea de texto recibida del cliente termina con \ r \ n
              if (c == '\n') 
              {
                  // último carácter en la línea de texto recibido
                  // comenzando una nueva linea con el siguiente caracter leído
                  currentLineIsBlank = true;
              } 
              else if (c != '\r') 
              {
                  // se recibió un carácter de texto del cliente
                  currentLineIsBlank = false;
              }
          } // end if (client.available())
      } // end while (client.connected())
      delay(1);      // darle tiempo al navegador web para recibir los datos
      client.stop(); // cierra la conexión
  } // end if (client)
}

float getCorriente(int samplesNumber)
{
   float volta;
   float corrienteSum = 0;
   for (int i = 0; i < samplesNumber; i++)
   {
      volta = analogRead(ANALOGINTENCIDAD) * 5.0 / 1023.0;
      corrienteSum += (volta - 2.5) / SENSIBILITY;
   }
   return(corrienteSum / samplesNumber);
}

// cambio de escala entre floats
float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
   return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
