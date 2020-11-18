#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>
#include "tinyxml.h"

using namespace std;

class Program {
public:
	int load;				//	Создаваемая программой нагрузка на процессор
	int proc;				//	На каком процессоре находится программа
};

class Processor {
public:
	int limit;				//	Верхняя граница нагрузки на процессор
};

class DataExchange {
public:
	int rate;				//	Интенсивность обмена между программами
	int prog1, prog2;		//	Пара программ, между которыми происходит обмен 
	bool dif_proc;			//	Находятся ли программы, на разных процессорах
};

int NetworkLoad(DataExchange* de, int N) {

	/*
	 *	ARGUMENTS
	 *		de	- вектор пар программ, которые обмениваются данными
	 *		N	- количество пар программ
	 *
	 *	RETURN
	 *		функция возвращает нагрузку на сеть
	 *
	 *	ALGORITHM
	 *		Если в обмене данными участвуют разные процессоры
	 *		то к общему числу прибавляем нагрузку на сеть для конкретной пары.
	 */

	int ret = 0;
	for (int i = 0; i < N; i++) {
		if (de[i].dif_proc)
			ret += de[i].rate;
	}
	return ret;
}

bool isCorrect(Program* prog, Processor* proc, int NumProg, int NumProc) {

	/*
	 *	ARGUMENTS
	 *		prog	- вектор программ
	 *		proc	- вектор процессоров
	 *		NumProg	- количество программ
	 *		NumProc	- количество процессоров
	 *
	 *	RETURN
	 *		функция возвращает является ли данное распределения программ по процессорам корректно
	 *		(не превышает ли сумма нагрузок программ на каждом процессоре верхнюю границу)
	 *
	 *	ALGORITHM
	 *		В цикле перебираем каждый процессор. Для процессора перебираем все программы.
	 *		Если программа находится на процессоре (prog[j].proc == i), прибавляем нагрузку.
	 *		Если сумма нагрузок больше, чем лимит процессора, то вектор распределения программ
	 *		по процессорам не корректен.
	 */

	int sum;
	for (int i = 0; i < NumProc; i++) {
		sum = 0;
		for (int j = 0; j < NumProg; j++) {
			if (prog[j].proc == i)
				sum += prog[j].load;
		}
		if (sum > proc[i].limit)
			return false;
	}
	return true;
}

mutex mtx;

int main(int argc, char **argv) {						//	В аргументах к программе передается имя файла в формате xml,
	if (argc != 2) {									//	в котором находятся входные данные
		cerr << "Error! Wrong arguments" << endl;		//	Можем передать только один файл
		exit(0);
	}
	
	int T;							//	T - количество потоков
	cin >> T;
	cout << endl;

	auto start = chrono::high_resolution_clock::now();	//	start - начало выполнения программы

	int NumProc, NumProg, NumDE;
	Processor* Proc;
	Program* Prog;
	DataExchange* DE;

	/*
	 *	VARIABLES
	 *		NumProc	- количество процессоров
	 *		NumProg	- количество программ
	 *		NumDE	- количество пар программ, между которыми происходит обмен данными
	 *		Proc	- массив процессоров
	 *		Prog	- массив программ
	 *		DE		- массив пар программ
	 */

	 /************************XML READ**************************/
	 /* Для чтения xml-файла использовалась библиотека tinyxml */

	TiXmlDocument doc(argv[1]);							//	Открываем файл
	if (!doc.LoadFile()) {								//	Если файл открыть не получилось, 
		cerr << "Error! Cannot use file" << endl;		//	печатаем диагностику в поток ошибок
		exit(0);										//	и завершаем выполнение программы.
	}

	TiXmlElement* note = doc.FirstChildElement("root");						//	Ставим указатель на тег <root>
	note = note->FirstChildElement("Processor");							//	Переходим к массиву процессоров, который заключен в тег <Processor>
	if (note->QueryIntAttribute("N", &NumProc) == TIXML_SUCCESS) {			//	Считываем атрибут N - количество процессоров
		if (NumProc <= 0) {													//	Диагностика на некорректное значение
			cerr << "Error! Uncorrect number of processors" << endl;
			exit(0);
		}

		Proc = new Processor[NumProc];
		TiXmlElement* elem = note->FirstChildElement("limit");							//	Каждый элемент массива заключен в тег <limit>
		int c = 0;
		for (int i = 0; i < NumProc && elem != NULL; i++, c++) {
			if (elem->QueryIntAttribute("value", &Proc[i].limit) == TIXML_SUCCESS) {		//	Считываем атрибут
				if (Proc[i].limit != 60 && Proc[i].limit != 80 && Proc[i].limit != 100) {
					cerr << "Error! Uncorrect limit" << endl;
					delete[] Proc;
					exit(0);
				}
			}
			else {
				cerr << "Error! Cannot read value" << endl;
				delete[] Proc;
				exit(0);
			}

			elem = elem->NextSiblingElement("limit");			//	Переходим к следующему элементу
		}
		if (c < NumProc || elem != NULL) {								//	Если количество элементов в файле не равно заявленному количеству
			cerr << "Error! Uncorrect number of processors" << endl;	//	элементов массива
			exit(0);
		}
	}
	else {
		cerr << "Error! Cannot read value" << endl;
		exit(0);
	}

	note = note->NextSiblingElement("Program");							//	Переходим к массиву программ. Аналогично как и с процессорами
	if (note->QueryIntAttribute("N", &NumProg) == TIXML_SUCCESS) {
		if (NumProg < 0) {
			cerr << "Error! Uncorrect number of program" << endl;
			exit(0);
		}

		Prog = new Program[NumProg];
		TiXmlElement* elem = note->FirstChildElement("load");
		int c = 0;
		for (int i = 0; i < NumProg && elem != NULL; i++, c++) {
			if (elem->QueryIntAttribute("value", &Prog[i].load) == TIXML_SUCCESS) {
				if (Prog[i].load != 5 && Prog[i].load != 10 && Prog[i].load != 20) {
					cerr << "Error! Uncorrect load" << endl;
					delete[] Proc;
					delete[] Prog;
					exit(0);
				}
				Prog[i].proc = -1;			//	По умолчанию инициализируем все процессоры -1
			}
			else {
				cerr << "Error! Cannot read value" << endl;
				delete[] Proc;
				delete[] Prog;
				exit(0);
			}
			elem = elem->NextSiblingElement("load");
		}
		if (c < NumProg || elem != NULL) {
			delete[] Proc;
			delete[] Prog;
			cerr << "Error! Uncorrect number of program" << endl;
			exit(0);
		}
	}
	else {
		cerr << "Error! Cannot read value" << endl;
		delete[] Proc;
		exit(0);
	}

	note = note->NextSiblingElement("DE");									//	Аналогично как и с процессорами. Отличие - у элементов массива
	if (note->QueryIntAttribute("N", &NumDE) == TIXML_SUCCESS) {			//	3 атрибута
		if (NumDE > ((NumProg * NumProg - 1) / 2) || NumDE < 0) {
			cerr << "Error! Uncorrect number of program pairs" << endl;
			exit(0);
		}

		DE = new DataExchange[NumDE];
		TiXmlElement* elem = note->FirstChildElement("pair");
		int c = 0;
		for (int i = 0; i < NumDE && elem != NULL; i++, c++) {
			if (elem->QueryIntAttribute("prog1", &DE[i].prog1) == TIXML_SUCCESS &&
				elem->QueryIntAttribute("prog2", &DE[i].prog2) == TIXML_SUCCESS &&
				elem->QueryIntAttribute("rate", &DE[i].rate) == TIXML_SUCCESS) {

				DE[i].dif_proc = true;
				if (DE[i].prog1 >= NumProg || DE[i].prog1 < 0 || DE[i].prog2 >= NumProg || DE[i].prog2 < 0 ||
					(DE[i].rate && DE[i].rate != 10 && DE[i].rate != 50 && DE[i].rate != 100)) {
					cerr << "Error! Uncorrect pair of program" << endl;
					delete[] Proc;
					delete[] Prog;
					delete[] DE;
					exit(0);
				}
			}
			else {
				cerr << "Error! Cannot read value" << endl;
				delete[] Proc;
				delete[] Prog;
				delete[] DE;
				exit(0);
			}
			elem = elem->NextSiblingElement("pair");
		}
		if (c < NumDE || elem != NULL) {
			delete[] Proc;
			delete[] Prog;
			delete[] DE;
			cerr << "Error! Uncorrect number of program pairs" << endl;
			exit(0);
		}
	}
	else {
		cerr << "Error! Cannot read value" << endl;
		delete[] Proc;
		delete[] Prog;
		exit(0);
	}

	/******************  ALGORITHM  ************************/

	thread* thr = new thread[T];
	int NL_best, NL = 0, count = 0, * Pr_best, l = 0;
	bool flag_success = false;
	Pr_best = new int[NumProg];

	/*
	 *	VARIABLES
	 *		NL_best			- наименьшая нагрузка на сеть
	 *		NL				- нагрузка на сеть
	 *		count			- счетчик итераций
	 *		flag_success	- нашелся ли хотя бы один корректный вектор программ
	 *		Pr_best			- наилучшее распределение программ по процессорам
	 *		l				- количество итераций между подходящими решениями
	 *		thr				- массив потоков
	 */

	for (int j = 0; j < NumProg; j++) {
		Pr_best[j] = Prog[j].proc;				//	Инициализируем массив -1
	}

	if (!(NL_best = NetworkLoad(DE, NumDE))) {				//	Рассчитываем теоретическую максимальную нагрузку на сеть. Если 0, ищем первый корректный вектор
		for (int w = 0; w < T; w++) {
			thr[w] = thread([&] {									//	Создаем потоки
				srand(w + time(NULL));								//	Для каждого потока генерируем уникальный seed для генерации случайных чисел

				Program* loc_Prog = new Program[NumProg];			//	Записываем значения в локальные переменные
				for (int i = 0; i < NumProg; i++) {
					loc_Prog[i] = Prog[i];
				}

				Processor* loc_Proc = new Processor[NumProc];
				for (int i = 0; i < NumProc; i++) {
					loc_Proc[i] = Proc[i];
				}

				DataExchange* loc_DE = new DataExchange[NumDE];
				for (int i = 0; i < NumDE; i++) {
					loc_DE[i] = DE[i];
				}

				for (int i = 0; !flag_success && i < 1000 && l < 1000; i++, count++, l++) {
					for (int j = 0; j < NumProg; j++) {								//	Генерируем случайный вектор программ.
						loc_Prog[j].proc = rand() % NumProc;						//	Индекс - номер программы. Значение - номер процессора.
					}

					for (int j = 0; j < NumDE; j++) {												//	Перебираем все пары обмена. 
						if (loc_Prog[loc_DE[j].prog1].proc == loc_Prog[loc_DE[j].prog2].proc)		//	Меняем значение нахождения программ на разных процессорах.
							loc_DE[j].dif_proc = false;
						else loc_DE[j].dif_proc = true;
					}

					if (isCorrect(loc_Prog, loc_Proc, NumProg, NumProc)) {			//	Распределение корректно
						mtx.lock();													//	Доступ в критическую секцию
						flag_success = true;
						l = 0;
						for (int j = 0; j < NumProg; j++) {							//	Обновляем науилучшее решение
							Pr_best[j] = loc_Prog[j].proc;
						}
						mtx.unlock();
					}
				}

				delete[] loc_Proc;
				delete[] loc_Prog;
				delete[] loc_DE;

			});
		}

	}
	else {															//  Изначально нагрузка на сеть не 0.
		for (int w = 0; w < T; w++) {								//  Аналогично
			thr[w] = thread([&] {
				srand(w + time(NULL));
				Program* loc_Prog = new Program[NumProg];
				for (int i = 0; i < NumProg; i++) {
					loc_Prog[i] = Prog[i];
				}

				Processor* loc_Proc = new Processor[NumProc];
				for (int i = 0; i < NumProc; i++) {
					loc_Proc[i] = Proc[i];
				}

				DataExchange* loc_DE = new DataExchange[NumDE];
				for (int i = 0; i < NumDE; i++) {
					loc_DE[i] = DE[i];
				}

				for (int i = 0; i < 1000 && NL_best && l < 1000; i++, count++, l++) {

					for (int j = 0; j < NumProg; j++) {
						loc_Prog[j].proc = rand() % NumProc;
					}

					for (int j = 0; j < NumDE; j++) {
						if (loc_Prog[loc_DE[j].prog1].proc == loc_Prog[loc_DE[j].prog2].proc)
							loc_DE[j].dif_proc = false;
						else loc_DE[j].dif_proc = true;
					}

					if (isCorrect(loc_Prog, loc_Proc, NumProg, NumProc)) {		// Если распределение корректно, входим в критическую секцию
						mtx.lock();
						if ((NL = NetworkLoad(loc_DE, NumDE)) < NL_best) {		// Сравниваем текущую нагрузку на сеть с наилучшей
							NL_best = NL;
							flag_success = true;
							i = 0;
							l = 0;
							for (int j = 0; j < NumProg; j++) {
								Pr_best[j] = loc_Prog[j].proc;
							}
						}
						mtx.unlock();
					}
				}

				delete[] loc_Proc;
				delete[] loc_Prog;
				delete[] loc_DE;
			});
		}
	}

	for (int w = 0; w < T; w++) {
		thr[w].join();
	}

	/********************  OUTPUT  ***********************/
	if (flag_success) {
		cout << "success" << endl;
		cout << count << endl;
		for (int i = 0; i < NumProg; i++) {
			cout << Pr_best[i] << ' ';
		}
		cout << endl;
		cout << NL_best << endl;
	}
	else {
		cout << "failure" << endl;
		cout << count << endl;
	}

	delete[] Proc;
	delete[] Prog;
	delete[] DE;
	delete[] Pr_best;

	auto end = chrono::high_resolution_clock::now();		// конец выполнения программы
	chrono::duration<float> duration = end - start;
	cout << duration.count() << endl;

	return 0;
}