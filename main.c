// Primeiro Trabalho de FPPD - Covid-19
// Autor: Gabriel N. dos Passos - 20182BSI0450
// Turma: 2018/2

/*
Explicação dos dados:
	- Threads pares remetem a Laboratorios e impares para Infectados.
	- Cada laboratorio produz um item igual seu identificador e (identificador + 1 ).
	Ex:	Lab 0 produz 0 e 1
		Lab 2 produz 2 e 3
		Lab 4 produz 4 e 5
	- A lista de produtos é estruturada como o número de threads * 2.
	- Cada infectado tem um suprimento infinito do item relacionado ao seu identificador e apenas
	  consume os quatros próximos itens da lista de produtos relativos a seu identificador + 2.
	Ex: produtos [0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5].
		O Infectado 1 apenas consome os itens 3, 4, 5, 0.
		O Infectado 3 apenas consome os itens 5, 0, 1, 2.
		O Infectado 5 apenas consome os itens 1, 2, 3, 4.
	- A identificação dos itens é pensada na lista de Threads de trás pra frente.
	Ex: threads [0, 1, 2, 3, 4, 5]
		Virus é 0, 5
		Insumo é 4, 3
		Segredo é 2, 1
*/ 


#define _CRT_SECURE_NO_WARNINGS 1
#define	_WINSOCK_DEPRECATED_NO_WARNINGS	1

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>


#define BufferSize 4
#define NumThreads 6


sem_t semVazio;
sem_t semCheio;

int bufferProdutos[BufferSize];
int bufferLoop[NumThreads];

pthread_mutex_t mutex;


struct Lab {
	int id;
	int loop;
	int prod[2];
};

struct Infec {
	int id;
	int loop;
	int cons[4];
};


int find(int a) {
	for (int i = 0; i < BufferSize; i++) {
		if (a == bufferProdutos[i]) { // Existe medicamento no bufferProdutos
			return i; // Retorna localizacao
		}
	}
	return -1; // Não existe medicamento no bufferProdutos
}


void* laboratorio(void *lab) {
	struct Lab *new_lab = (struct Lab*)lab;
	int produzido[2] = { -1,-1 }; // Lista para checagem se produziu todos produtos
	bool skip = false; // flag para checagem se todos threads trabalharam 

	while (!skip) { // Rodar a thread até o objetivo ser cumprido para todos
		sem_wait(&semVazio);
		pthread_mutex_lock(&mutex);

		for (int j = 0; j < 2; j++) {
			int flag = find((*new_lab).prod[j]); // flag para criação de produto

			if (flag < 0) {
				for (int ins = 0; ins < BufferSize; ins++) { // buscar qual posicao acrescentar novo produto
					if (bufferProdutos[ins] == -1) {
						if ((*new_lab).prod[j] == (*new_lab).id) {
							produzido[0] = (*new_lab).prod[j];
						}
						else {
							produzido[1] = (*new_lab).prod[j];
						}

						bufferProdutos[ins] = (*new_lab).prod[j]; // acrescenta produto no buffer

						if (produzido[0] != -1 && produzido[1] != -1) { // caso cumpriu uma parte do objetivo
							bufferLoop[(*new_lab).id]++;
							int produzido[2] = { -1, -1 }; 
						}
						break;
					}
				}
			}
		}
	
		for (int i = 0; i < NumThreads; i++){ 
			skip = true;

			if (bufferLoop[i] < (*new_lab).loop){
				skip = false;
				break;
			}
		}

		pthread_mutex_unlock(&mutex);
		sem_post(&semCheio);
		
	}
	printf("Laboratorio %d: %d\n", (*new_lab).id, bufferLoop[(*new_lab).id]);
	free(lab);
	return NULL;
}

void *infectado(void *infec) {
	struct Infec *new_infec = (struct Infec*)infec;
	int consumido[2] = { -1, -1 };
	bool skip = false;
	
	while (!skip) { // Rodar a thread até o objetivo ser cumprido para todos

		sem_wait(&semCheio);
		pthread_mutex_lock(&mutex);

		for (int j = 0; j < 4; j++) {
			int flag = find((*new_infec).cons[j]); // flag para consumir produto na posicao especifica

			if (flag >= 0) {
				if (((*new_infec).cons[j] == (*new_infec).cons[0]) || ((*new_infec).cons[j] == (*new_infec).cons[1])) { // apenas consumir caso não haja produto do mesmo tipo
					if (consumido[0] == -1) {
						consumido[0] = (*new_infec).cons[j];
					}
				}
				else {
					if (consumido[1] == -1) {
						consumido[1] = (*new_infec).cons[j];
					}
				}
				bufferProdutos[flag] = -1;
				if (consumido[0] != -1 && consumido[1] != -1) {
					bufferLoop[(*new_infec).id]++;
					int consumido[2] = { -1,-1 };
				}
			}
		}
		
		for (int i = 0; i < NumThreads; i++){
			skip = true;

			if (bufferLoop[i] < (*new_infec).loop){
				skip = false;
				break;
			}
		}

		pthread_mutex_unlock(&mutex);
		sem_post(&semVazio);
		
	}
	printf("Infectado %d: %d\n", (*new_infec).id, bufferLoop[(*new_infec).id]);
	free(infec);
	return NULL;
}

int main(int argc, char *argv[]) {
	int lst_prod[(NumThreads * 2)];
	int x = 0;

	pthread_t thread[NumThreads];
	pthread_mutex_init(&mutex, NULL);
	sem_init(&semVazio, 0, BufferSize);
	sem_init(&semCheio, 0, 0);

	struct Lab *lab;
	struct Infec *infec;

	
	if (argc < 2){
		printf("Insira o parametro.\n");
		return 1;
	}

	char* p;
	errno = 0;
	long aux = strtol(argv[1], &p, 10);
	if (*p != '\0' || errno != 0) {
		printf("Falha ao passar argumento - Insira apenas inteiros.\n");
		return 0;
	}

	int loop = aux;

	for (int i = 0; i < BufferSize; i++) { // Popular bufferProdutos com valores negativos
		bufferProdutos[i] = -1;
	}

	for (int i = 0; i < NumThreads; i++) { // Popular array de produtos
		lst_prod[i] = i;
		lst_prod[NumThreads + i] = i;
	}

	for (int i = 0; i < NumThreads; i++) {
		if (i % 2 == 0) { // Threads pares destinadas aos laboratorios
			lab = malloc(sizeof(struct Lab));

			(*lab).loop = loop;
			(*lab).prod[0] = lst_prod[i];
			(*lab).id = i;

			if (i - 1 < 0) {
				(*lab).prod[1] = lst_prod[NumThreads - 1];
			}
			else {
				(*lab).prod[1] = lst_prod[i - 1];
			}
			if (pthread_create(&thread[i], NULL, &laboratorio, (void*)lab) != 0) {
				perror("Falha ao criar thread para laboratorio.");
			}
		}
		else { // Threads impares aos infectados
			infec = malloc(sizeof(struct Infec));

			(*infec).loop = loop;
			(*infec).id = i;

			for (int j = i + 2; j <= i + (NumThreads - 1); j++) {
				(*infec).cons[x] = lst_prod[j];
				x++;
			}
			x = 0;
			if (pthread_create(&thread[i], NULL, infectado, (void*)infec) != 0) {
				perror("Falha ao criar thread para infectado.");
			}
		}
	}

	for (int i = 0; i < NumThreads; i++) {
		if (pthread_join(thread[i], NULL) != 0) {
			perror("Falha no join da thread.");
		}
	}

	pthread_mutex_destroy(&mutex);
	sem_destroy(&semVazio);
	sem_destroy(&semCheio);

	return 0;

}