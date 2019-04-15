#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include "parser.h"
#include "protocol.h"

int parser(char* msg,char* buffer, int max){
	if(strlen(msg) > max){
		return ERROR_TAMANIO_BUFFER;
	}
	if(!memcmp(msg,"SELECT ",7      )){ return ejecutarSelect(msg,buffer);}
	if(!memcmp(msg,"INSERT ",7      )){ return ejecutarInsert(msg,buffer);}
	if(!memcmp(msg,"CREATE ",7      )){ return ejecutarCreate(msg,buffer);}
	if(!memcmp(msg,"DESCRIBE",8     )){ return ejecutarDescribe(msg,buffer);}
	if(!memcmp(msg,"DROP ",5        )){ return ejecutarDrop(msg,buffer);}
	if(!strcmp(msg,"JOURNAL"        )){ return ejecutarJournal(msg,buffer);}
	if(!memcmp(msg,"ADD MEMORY ",11 )){ return ejecutarAdd(msg,buffer);}
	if(!strcmp(msg,"METRICS"        )){ return METRICS;}
	if(!strcmp(msg,"EXIT"           )){ return EXIT;}
	return ERROR;
}

int obtenerProximaPalabra(char* msg, char* buff,char centinela,int inicio){
	int contador=inicio;
	while(msg[contador] != centinela && msg[contador] != '\0'){
		contador++;
	}
	if(msg[contador] == '\0' && centinela != '\0') {return -1;}
	int letra;
	for(letra=inicio; letra<contador; letra++){
		buff[letra-inicio] = msg[letra];
	}
	buff[letra-inicio] = '\0';
	return contador-inicio;
}

int ejecutarSelect(char* msg,char* buffer){
	//SELECT + NOMBRE_TABLA + KEY
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char key[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	tamanioPalabra = obtenerProximaPalabra(msg,key, '\0', inicio+tamanioPalabra+1);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(key)){ return CONSTANTE_INVALIDA; }
	uint16_t nuevaKey = strtoumax(key,NULL,10);

	struct select_request mensaje;
	init_select_request(nombreTabla,nuevaKey,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct select_request));
	return OK;
}

int ejecutarInsert(char* msg,char* buffer){
	//INSERT + NOMBRE_TABLA + KEY + "VALUE" + TIMESTAMP
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char key[MAX_TOKENS_LENGTH];
	char value[MAX_TOKENS_LENGTH];
	char espacio[MAX_TOKENS_LENGTH];
	char timestamp[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,key, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(key)){ return CONSTANTE_INVALIDA; }
	uint16_t nuevaKey = strtoumax(key,NULL,10);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,espacio, '"', inicio);
	if(tamanioPalabra!= 0){ return VALUE_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg, value, '"', inicio);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,espacio, ' ', inicio);
	if(tamanioPalabra!= 0){ return VALUE_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,timestamp, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(timestamp)){ return CONSTANTE_INVALIDA; }
	uint32_t nuevoTimestamp = strtoumax(timestamp,NULL,10);

	struct insert_request mensaje;
	init_insert_request(nombreTabla,nuevaKey,value,nuevoTimestamp,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct insert_request));
	return OK;
}
int ejecutarCreate(char* msg,char* buffer){
	//CREATE + NOMBRE_TABLA + TIPO CONSISTENCIA + NUMERO PARTICIONES + TIEMPO DE COMPACTACION
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char consistencia[MAX_TOKENS_LENGTH];
	char particiones[MAX_TOKENS_LENGTH];
	char compactacion[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,consistencia, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	int valorConsistencia = isConsistency(consistencia);
	if(valorConsistencia < 0){ return CONSISTENCIA_INVALIDA; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,particiones, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(particiones)){ return CONSTANTE_INVALIDA; };
	uint8_t nuevaParticion = strtoumax(particiones,NULL,10);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,compactacion, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(compactacion)){ return CONSTANTE_INVALIDA; }
	uint32_t nuevaCompactacion = strtoumax(compactacion,NULL,10);

	struct create_request mensaje;
	init_create_request(nombreTabla,valorConsistencia,nuevaParticion,nuevaCompactacion,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct create_request));
	return OK;
}

int ejecutarDescribe(char* msg,char* buffer){
	//DESCRIBE + NOMBRE_TABLA o DESCRIBE
	int inicio = 8;
	char nombreTabla[MAX_TOKENS_LENGTH];
	if(msg[inicio]=='\0'){
		 struct describe_request mensaje;
		 init_describe_request(0,NULL,&mensaje);
		 memcpy(buffer, &mensaje, sizeof(struct describe_request));
		 return OK;
	}
	inicio += 1;
	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	struct describe_request mensaje;
	init_describe_request(1,nombreTabla,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct describe_request));
	return OK;
}

int ejecutarDrop(char* msg,char* buffer){
	//DROP + NOMBRE_TABLA
	int inicio = 6;
	char nombreTabla[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	struct drop_request mensaje;
	init_drop_request(nombreTabla,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct drop_request));
	return OK;
}

int ejecutarJournal(char* msg,char* buffer){
	//JOURNAL
	struct journal_request mensaje;
	init_journal_request(&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct journal_request));
	return OK;
}

int ejecutarAdd(char* msg,char* buffer){
	//ADD MEMORY + NUMERO + TO + CRITERIO
	int inicio = 11;
	char numero[MAX_TOKENS_LENGTH];
	char to[MAX_TOKENS_LENGTH];
	char criterio[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, numero, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(numero)){ return IDENTIFICADOR_INVALIDO; }
	uint8_t nuevoNumero = strtoumax(numero,NULL,10);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg, to, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(strcmp(to,"TO")){ return IDENTIFICADOR_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,criterio, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	int valorConsistencia = isConsistency(criterio);
	if(valorConsistencia < 0){ return CONSISTENCIA_INVALIDA; }

	struct add_request mensaje;
	init_add_request(nuevoNumero,valorConsistencia,&mensaje);
	memcpy(buffer, &mensaje, sizeof(struct add_request));
	return OK;
}

int isIdentifier(char* id){ //Controla que id este conformado solo por letras , '_' o '-'
	for(int i=0 ; i < strlen(id); i++){
		if(!( isalnum(id[i]) || id[i]=='_' || id[i]=='-')){
			return ERROR;
		}
	}
	return OK;
}
int isConstant(char* id){ //Controla que id este conformado solo por numeros
	for(int i=0 ; i < strlen(id); i++){
		if(!isdigit(id[i])){
			return ERROR;
		}
	}
	return OK;
}
int isConsistency(char* id){ //Controla que id haga match con alguna de las 3 consistencias validas
	if(!strcmp(id,"SC")){ // Consistencias validas
		return SC;
	} else if(!strcmp(id,"SHC")){ // Consistencias validas
		return SHC;
	} else if(!strcmp(id,"EC")){ // Consistencias validas
		return EC;
	}
	return -1;
}
void manejarError(int error, char* buff){
	char mensaje[100] = "Estimado colega, ha cometido un error ";
	switch(error){
		case VALUE_INVALIDO:
			strcat(mensaje,"al ingresar un valor invalido");
			break;
		case CONSISTENCIA_INVALIDA:
			strcat(mensaje,"al ingresar una consistencia invalida");
			break;
		case ERROR_TAMANIO_BUFFER:
			strcat(mensaje,"dado que el comando ingresado supera el buffer");
			break;
		case IDENTIFICADOR_INVALIDO:
			strcat(mensaje,"al ingresar un identificador invalido");
			break;
		case CONSTANTE_INVALIDA:
			strcat(mensaje,"al ingresar una constante invalida");
			break;
		case COMANDOS_INVALIDOS:
		case ERROR:
			strcat(mensaje,"al ingresar los comandos");
			break;
	}
	memcpy(buff, &mensaje, sizeof(mensaje));
}
// ----------PARA PRUEBAS------------
/*
int main(void) {
	casosDeUso();
	return 0;
}

void casosDeUso(){
	char buffer[get_max_msg_size()];
	int a= get_max_msg_size();
	char* msg = "SELECT acb 123"; // 1
	int i = parser(msg, &buffer , a );
	printf("SELECT acb 123:   %d\n",i);

	msg = "SELECT"; // 0 pues ERROR FATAL
	i = parser(msg, &buffer , a );
	printf("SELECT: %d\n",i);

	msg = "SELECT acb"; // -1
	i = parser(msg, &buffer , a );
	printf("SELECT acb: %d\n",i);

	msg = "SELECT ab!c"; // -1
	i = parser(msg, &buffer , a );
	printf("SELECT ab!c: %d\n",i);

	msg = "SELECT "; // -1
	i = parser(msg, &buffer , a );
	printf("SELECT : %d\n",i);

	msg = "SELECT abc 123 asdf"; // -2
	i = parser(msg, &buffer , a );
	printf("SELECT abc 123 asdf: %d\n",i);

	msg = "SELECT abc 12c3"; // -2
	i = parser(msg, &buffer , a );
	printf("SELECT abc 12c3: %d\n",i);

	msg = "SELECT ab!c 123"; // -3 pues NOMBRE_TABLA_INVALIDO
	i = parser(msg, &buffer , a );
	printf("SELECT ab!c 123: %d\n",i);

	msg = "INSERT TABLA1 3 \"Mi nombre es Lissandra\" 1548421507";
	i = parser(msg, &buffer , 200);
	printf("\nINSERT TABLA1 3 \"Mi nombre es Lissandra\" 1548421507: %d\n",i);

	msg = "INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507";
	i = parser(msg, &buffer , 200);
	printf("INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507: %d\n",i);

	msg = "INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507";
	i = parser(msg, &buffer , 200);
	printf("INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507: %d\n",i);

	msg = "CREATE TABLA1 SC 4 60000";
	i = parser(msg, &buffer , a );
	printf("\nCREATE TABLA1 SC 4 60000: %d\n",i);

	msg = "CREATE TABLA1 EC 4 60000";
	i = parser(msg, &buffer , a );
	printf("CREATE TABLA1 EC 4 60000: %d\n",i);

	msg = "CREATE TABLA1 acereje 4 60000";
	i = parser(msg, &buffer , a );
	printf("CREATE TABLA1 acereje 4 60000: %d\n",i);

	msg = "CREATE TABLA1 SC B4 60000";
	i = parser(msg, &buffer , a );
	printf("CREATE TABLA1 SC B4 60000: %d\n",i);

	msg = "CREATE TABLA1 SpC 4";
	i = parser(msg, &buffer , a );
	printf("CREATE TABLA1 SpC 4: %d\n",i);

	msg = "CREATE ";
	i = parser(msg, &buffer , a );
	printf("CREATE : %d\n",i);

	msg = "DESCRIBE TABLA1";
	i = parser(msg, &buffer , a );
	printf("\nDESCRIBE TABLA1: %d\n",i);

	msg = "DESCRIBE";
	i = parser(msg, &buffer , a );
	printf("DESCRIBE: %d\n",i);

	msg = "DESCRIBE TABLA1 acereje";
	i = parser(msg, &buffer , a );
	printf("DESCRIBE TABLA1 acereje: %d\n",i);

	msg = "DESCRIBE TABLA#$";
	i = parser(msg, &buffer , a );
	printf("DESCRIBE TABLA#$: %d\n",i);

	msg = "DROP TABLA1";
	i = parser(msg, &buffer , a );
	printf("\nDROP TABLA1: %d\n",i);

	msg = "DROP";
	i = parser(msg, &buffer , a );
	printf("DROP: %d\n",i);

	msg = "DROP TABLA1 acereje";
	i = parser(msg, &buffer , a );
	printf("DROP TABLA1 acereje: %d\n",i);

	msg = "DROP TABLA#$";
	i = parser(msg, &buffer , a );
	printf("DROP TABLA#$: %d\n",i);

	msg = "JOURNAL";
	i = parser(msg, &buffer , a );
	printf("\nJOURNAL: %d\n",i);

	msg = "JOURNAL TABLA1";
	i = parser(msg, &buffer , a );
	printf("JOURNAL TABLA1: %d\n",i);

	msg = "JOURNAL TABLA1 acereje";
	i = parser(msg, &buffer , a );
	printf("JOURNAL TABLA1 acereje: %d\n",i);

	msg = "JOURNAL TABLA#$";
	i = parser(msg, &buffer , a );
	printf("JOURNAL TABLA#$: %d\n",i);

	msg = "ADD MEMORY 4 TO SC";
	i = parser(msg, &buffer , a );
	printf("\nADD MEMORY 4 TO SC: %d\n",i);

	msg = "ADD MEMORY 400 TO EC";
	i = parser(msg, &buffer , a );
	printf("ADD MEMORY 400 TO EC: %d\n",i);

	msg = "ADD MEMORY acereje TO EC";
	i = parser(msg, &buffer , a );
	printf("ADD MEMORY acereje TO EC: %d\n",i);

	msg = "ADD MEMORY 400 TO 22";
	i = parser(msg, &buffer , a );
	printf("ADD MEMORY 400 TO 22: %d\n",i);

	msg = "ADD MEMORY";
	i = parser(msg, &buffer , a );
	printf("ADD MEMORY: %d\n",i);

	msg = "ADD MEMORY 400 acereje SC ";
	i = parser(msg, &buffer , a );
	printf("ADD MEMORY 400 acereje SC: %d\n",i);

	msg = "METRICS";
	i = parser(msg, &buffer , a );
	printf("\nMETRICS: %d\n",i);

	msg = "EXIT";
	i = parser(msg, &buffer , a );
	printf("\nEXIT: %d\n",i);

	int j=0;
	char msg2[100];
	manejarError(-6, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(-5, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(-4, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(-3, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(-2, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(-1, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");

	j=0;
	manejarError(0, &msg2);
	while(msg2[j]!='\0'){
		printf("%c",msg2[j]);
		j++;
	}
	printf("\n");
}
*/
