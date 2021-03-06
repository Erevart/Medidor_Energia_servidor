/*

 conmtcp.c - Funciones de comunicacion para la comunicación TCP entre dispositivos ESP8266 - ESP8266.

 */


/******************************************************************************
 * Función : tcp_sent
 * @brief  : Envia al cliente conectado por comunicación TCP los datos indicados.
 * @param  : pdata - Puntero al array de datos que se desea enviar
 * @param  : length - Longitud del array de datos.
 * @return : true - El envio de los datos se ha realizado correctamente.
 * @return : false - El envio de los datos no se ha podido realizar.
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCP_ST".
 *******************************************************************************/
bool tcp_sent(uint8_t *pdata, uint16_t length){

  unsigned long time0;
  int8_t info_tcp = -1;

  transmision_finalizada = false;
  time0 = millis();

  while (!transmision_finalizada){

    yield();

    if (info_tcp != ESPCONN_OK)
       info_tcp = espconn_send(esp_conn, pdata , length);

    #ifdef _DEBUG_COMUNICACION
      debug.print("[TCP_ST] Codigo de envio: ");
      debug.println(info_tcp);
    #endif

    if ((millis()-time0)>MAX_ESPWIFI){
      /* AÚN POR COMPROBAR */
      espconn_disconnect(esp_conn);
      tcp_desconectado = true;
      /* AÚN POR COMPROBAR */
      return false;
    }
  }

  return true;
}


 /******************************************************************************
  * Función : comunicacion_cliente
  * @brief  : Se envia la información solicitada por el cliente a través de la
              comunicación TCP establecida previamente.
  * @param  : none
  * @return : none
  * Etiqueta debug : Todos los comentarios para depuración de esta función
                    estarán asociados a la etiqueta: "COMCL".
  * ------------------------ *
   Protocolo de comunicación
  * ------------------------ *

 |------------|--------------|---------------|---------|--------|----------|
 | -- Start --|-- tcpcount --|-- ident_var --|-- Var --|- \...\-|-- Stop --|
 |------------|--------------|---------------|---------|--------|----------|

 Start (start) - uint8_t = ¿  || Byte de inicio de comunicación.
 tcpcount      - uint8_t =    || Número de variables que serán recibidas.
 ident_var     - *uint8_t =   || Identificador de la variable recibida.
 Var           - *double      || Variable
 Stop          - uint8_t = #  || Byte de fin de comunicación.
  *******************************************************************************/
void comunicacion_cliente(){

  uint8_t *psent;
  // MAX = 1275 para tcpcount = 255
  if ( ( psent = (uint8_t *) os_malloc( 12 * sizeof(psent) ) ) == NULL){
    CMD = '$';
    return;
  }

  union {
  float float_value;
  uint64_t  long_value;
  uint8_t byte[8];
} data;

  // Se comprueba si las operaciones solicitadas ya han sido realizadas.
  if (CMD == '$' || !transmision_finalizada)
    return;

  switch (CMD)
  {
    // Se realiza el proceso de registro en supuesto de ser solicitado.
    case USUARIO_REGISTRADO:
      #ifdef _DEBUG_COMUNICACION
      debug.print("[COMCL] Confirmacion de registro.");
      unsigned long time0;
      time0 = millis();
      #endif

      psent[0] = TCP_START;
      psent[1] = 1;
      psent[2] = WACK;
      psent[3] = TCP_STOP;

      if (tcp_sent(psent,4))
        registrado  = true;
      else{
        registrado = false;
        return;
      }

      // Se actualiza el contador de tiempo.
      update_rtc_time(true);

      #ifdef _DEBUG_COMUNICACION
       debug.println("-----------------------------------------------");
       debug.print("[COMCL] REGISTRADO. Tiempo requerido: ");
       debug.println(millis()-time0);
       debug.println("-----------------------------------------------");
      #endif
     break;

     default:
       #ifdef _DEBUG_COMUNICACION
         debug.print("[COMCL] Operacion no identificada. CMD: ");
         debug.println(CMD);
       #endif
     break;

     // Operación de depuración
       case '!':
        #ifdef _DEBUG_COMUNICACION
          debug.print("[COMCL] !Soy el servidor: ");
          debug.println(ESP.getChipId());
        #endif


         data.long_value = get_rtc_time();
         // Ver Protocolo de comunicación
         psent[0] = TCP_START;  // Byte start
         psent[1] = 1;    // Número de datos a enviar
         psent[2] = 0x21; // Identificador: caracter '!'
         for (uint8_t i = 0; i < 8; i++)
            psent[i+3] = data.byte[i];  // Data long - 8 byte
         psent[11] = TCP_STOP;  // Byte stop

        //sprintf(reinterpret_cast<char*>(psent),"!Soy el servidor: %d",ESP.getChipId());
        tcp_sent(psent, 12);


       break;
   }

   os_free(psent);

   // Se indica que la operación solicitada ya ha sido realizada.
   CMD = '$';

}

/******************************************************************************
 * Función : tcp_server_sent_cb
 * @brief  : Callback cuando la transmisión es finalizada. Determina que los
             ha sido enviados correctamente.
 * @param  : arg - puntero a la variable tipo espconn que determina la comunicación.
 * @return : none
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCP_ST_CB".
 *******************************************************************************/
void tcp_server_sent_cb(void *arg){
 //Datos enviados correctamente
 transmision_finalizada = true;

 #ifdef _DEBUG_COMUNICACION
   debug.println("[TCP_ST_CB] TRANSMISION REALIZADA CORRECTAMENTE");
 #endif
}

/******************************************************************************
 * Función : tcp_server_discon_cb
 * @brief  : Callback cuando la comunicación tcp se finaliza. Indica que la
             comunicación tcp ha finalizado.
 * @param  : arg - puntero a la variable tipo espconn que determina la comunicación.
 * @return : none
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCP_DC_CB".
 *******************************************************************************/
void tcp_server_discon_cb(void *arg){
 //tcp disconnect successfully
 tcp_desconectado = true;
 tcp_establecido = false;
 transmision_finalizada = true;


 #ifdef _DEBUG_COMUNICACION
   debug.println("[TCP_DC_CB] DESCONEXION REALIZADA CORRECTAMENTE");
 #endif
}


/******************************************************************************
 * Función : tcp_server_recon_cb
 * @brief  : Callback cuando la comunicación tcp es interrumpida. Indica que la
             comunicación tcp ha sido forzada a cerrarse.
 * @param  : arg - puntero a la variable tipo espconn que determina la comunicación.
 * @return : none
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCP_RC_CB".
 *******************************************************************************/
void tcp_server_recon_cb(void *arg, sint8 err){
   // Se ha producido una fallo, y la conexión ha sido cerrada.
   tcp_desconectado = true;
   tcp_establecido = false;
   transmision_finalizada = true;

   #ifdef _DEBUG_COMUNICACION
       debug.print("[TCP_RC_CB] CONEXION INTERRUMPIDA. CODIGO DE ERROR: ");
       debug.println(err);
   #endif
}


/******************************************************************************
 * Función : tcp_server_recv_cb
 * @brief  : Callback cuando se recibe información del cliente. Permite leer la
             la trama de datos recibida e identificar la operación solicitada.
 * @param  : arg - puntero a la variable tipo espconn que determina la comunicación.
 * @param  : tcp_data - puntero al array de datos recibidos.
 * @param  : length - longitud del array de datos.
 * @return : none
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCP_RV_CB".
 *******************************************************************************/
void tcp_server_recv_cb(void *arg, char *tcp_data, unsigned short length){
   // Indicador de recepción de datos.
   digitalWrite(2, !digitalRead(2));

   #ifdef _DEBUG_COMUNICACION
       debug.print("[TCP_RV_CB] Recepcion de datos. Numero de datos recibidos: ");
       debug.println(length);
       debug.print("[TCP_RV_CB] Información recibida: ");
       debug.println(tcp_data);
   #endif

   /* PROCESAMIENTO DE LA INFORMACIÓN RECIBIDA */
   switch (tcp_data[0]) {

       case USUARIO_REGISTRADO:
         #ifdef _DEBUG_COMUNICACION
           debug.println("[TCP_RV_CB] Proceso de registro");
         #endif
         CMD = tcp_data[0];
       break;


       default:
         #ifdef _DEBUG_COMUNICACION
           debug.println("[TCP_RV_CB]: Operacion no identificado.");
         #endif
       break;

       // Operación de debug.
         case '!':
             #ifdef _DEBUG_COMUNICACION
              debug.println("TCP: Comunicacion !");
             #endif
             CMD = tcp_data[0];
         break;
   }

    #ifdef _DEBUG_COMUNICACION
        debug.println("[TCP_RV_CB] Fin recepción de datos");
    #endif

}

/******************************************************************************
 * Función : tcp_listen
 * @brief  : Callback cuando se establece la comunicación TCP. Permite identificar
             cuando se ha iniciado a la comunicación y establecer las funciones
             de Callback para los distintos eventos de la comunicación TCP.
 * @param  : arg - puntero a la variable tipo espconn que determina la comunicación.
 * @return : none
 * Etiqueta debug : Todos los comentarios para depuración de esta función
                   estarán asociados a la etiqueta: "TCPL".
 *******************************************************************************/
void tcp_listen(void *arg){
  #ifdef _DEBUG_COMUNICACION
   debug.println("[TCPL] Comunicacion iniciada");
  #endif

  struct espconn *pesp_conn = static_cast<struct espconn *> (arg);

  /* Función llamada cuando se reciben datos */
  espconn_regist_recvcb(pesp_conn, tcp_server_recv_cb);
  /* Función llamada cuando la conexión es interrumpida */
  espconn_regist_reconcb(pesp_conn, tcp_server_recon_cb);
  /* Función llamada cuando se finaliza la conexión */
  espconn_regist_disconcb(pesp_conn, tcp_server_discon_cb);
  /* Función llamada cuando los datos se han enviado correctamente */
  espconn_regist_sentcb(pesp_conn, tcp_server_sent_cb);

  tcp_establecido = true;
  tcp_desconectado = false;
  transmision_finalizada = true;

}

/******************************************************************************
* Función : servidor_tcp
* @brief  : Callback cuando se establece la comunicación TCP. Permite identificar
            cuando se ha iniciado a la comunicación y establecer las funciones
            de Callback para los distintos eventos de la comunicación TCP.
* @param  : none
* @return : none
* Etiqueta debug : Todos los comentarios para depuración de esta función
                  estarán asociados a la etiqueta: "CTCP".
*******************************************************************************/
void servidor_tcp(){

 int8_t info_tcp;
 unsigned long time0;

 #ifdef _DEBUG_COMUNICACION
   debug.println("[CTCP] Arranque del servidor tcp.");
 #endif

 // Configuración de los parámetros de la comunicación TCP
 esp_conn = (struct espconn *)os_malloc((uint32)sizeof(struct espconn));
 esp_conn->type = ESPCONN_TCP;
 esp_conn->state = ESPCONN_NONE;
 esp_conn->proto.tcp = (esp_tcp *)os_malloc((uint32)sizeof(esp_tcp));
 esp_conn->proto.tcp->local_port = MCPESP_SERVER_PORT;

 time0 = millis();
 #ifdef _DEBUG_COMUNICACION
   info_tcp = espconn_accept(esp_conn);
   debug.print("[CTCP] Codigo de arranque: ");
   debug.println(info_tcp);

   while(info_tcp != ESPCONN_OK){
     yield();
     debug.print("[CTCP] Estableciendo servidor TCP. Tiempo requerido: ");
     debug.println(millis()-time0);
     info_tcp = espconn_accept(esp_conn);
     if ((millis()-time0)>MAX_ESPWIFI){
       return;
     }
   }
 #else
   while(espconn_accept(esp_conn) != ESPCONN_OK ){
     yield();
     if ((millis()-time0)>MAX_ESPWIFI)
       return;
     }
 #endif

 // Tiempo de inactividad que debe transcurrir para finalizar la conexión.
 espconn_regist_time(esp_conn, TCP_TIEMPO_CONEXION, 1);

 // Se establace la función que será invocada cuando se inicie la comunicación.
 espconn_regist_connectcb(esp_conn, tcp_listen);

 #ifdef _DEBUG_COMUNICACION
   debug.println("[CTCP] SERVICIO TCP: Establecido.");
 #endif

}
