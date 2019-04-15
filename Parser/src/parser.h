#ifndef PARSER_H_
#define PARSER_H_

#define MAX_TOKENS_LENGTH    100

enum errores{ VALUE_INVALIDO = -6,
			  CONSISTENCIA_INVALIDA,
	          ERROR_TAMANIO_BUFFER,
			  IDENTIFICADOR_INVALIDO,
			  CONSTANTE_INVALIDA,
			  COMANDOS_INVALIDOS,
			  ERROR,
			  OK,
			  EXIT,
			  METRICS
};

int parser(char*,char*,int);
int isIdentifier(char* );
int isConstant(char* );
int isConsistency(char* );
int obtenerProximaPalabra(char* , char* ,char ,int );
int ejecutarSelect(char*,char*);
int ejecutarInsert(char*,char*);
int ejecutarCreate(char*,char*);
int ejecutarDescribe(char*,char*);
int ejecutarDrop(char*,char*);
int ejecutarJournal(char*,char*);
int ejecutarAdd(char*,char*);
void manejarError(int, char*);
//-----------PARA PRUEBAS----------
/*void casosDeUso(void);

#endif /* PARSER_H_ */
